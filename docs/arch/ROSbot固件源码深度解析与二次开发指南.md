# Husarion ROSbot 固件源码深度解析与二次开发指南

> 基于 STM32F407 的 ROS 机器人开发板固件分析  
> 分析日期：2026-04-09  
> 固件版本：1.0.0 (micro-ROS Jazzy)

---

## 目录

1. [项目总览与评价](#1-项目总览与评价)
2. [架构设计解析](#2-架构设计解析)
3. [开发环境与工具链](#3-开发环境与工具链)
4. [RTOS 实时操作系统层](#4-rtos-实时操作系统层)
5. [通信管理模块](#5-通信管理模块)
6. [ROS 节点模块](#6-ros-节点模块)
7. [电机驱动模块](#7-电机驱动模块)
8. [编码器模块](#8-编码器模块)
9. [PID 控制器模块](#9-pid-控制器模块)
10. [IMU 惯性测量模块](#10-imu-惯性测量模块)
11. [测距传感器模块](#11-测距传感器模块)
12. [电池管理模块](#12-电池管理模块)
13. [LED 指示与灯带模块](#13-led-指示与灯带模块)
14. [风扇温控模块](#14-风扇温控模块)
15. [电源板通信模块](#15-电源板通信模块)
16. [ROSbot vs ROSbot XL 差异对比](#16-rosbot-vs-rosbot-xl-差异对比)
17. [二次开发建议与路线图](#17-二次开发建议与路线图)

---

## 1. 项目总览与评价

### 1.1 项目概述

这是 Husarion 公司为其 ROSbot 和 ROSbot XL 机器人产品开发的 STM32F407 固件。它通过 **micro-ROS** 中间件与 ROS 2 主机通信，支持 **Serial（串口）** 和 **Ethernet（以太网）** 两种传输方式。

### 1.2 总体评价

| 维度 | 评分 | 说明 |
|------|------|------|
| 代码质量 | ⭐⭐⭐⭐ | C++17 现代风格，constexpr 配置，接口抽象良好 |
| 架构设计 | ⭐⭐⭐⭐⭐ | FreeRTOS 多任务分层，优先级设计合理 |
| 可扩展性 | ⭐⭐⭐⭐ | 接口驱动分离，新硬件只需实现接口 |
| 可维护性 | ⭐⭐⭐⭐ | 配置集中，但两个板型代码有一定冗余 |
| 文档质量 | ⭐⭐ | 代码注释较少，缺少架构文档 |

**优点：**
- 使用 `constexpr` 编译期配置，零运行时开销
- 接口/实现分离的面向对象设计（`BatteryInterface`、`EncoderInterface`、`MotorInterface` 等）
- FreeRTOS 任务优先级分层合理（安全 > 控制 > 通信 > 传感器 > 观察 > 阻塞）
- 通信管理器支持运行时传输切换（Serial/Ethernet/诊断串口）
- PID 控制器带惯性补偿和加速度限制

**不足：**
- 两个板型（ROSbot/ROSbot XL）之间存在大量重复代码（`rtos.cpp`、`ros.cpp`）
- 缺少 DMA 传输（代码中有 TODO 标注）
- 硬件编码器仅支持16位定时器（TIM2/TIM5 的32位模式未使用）
- 缺少看门狗（Watchdog）保护
- LED 灯带动画逻辑与硬件驱动耦合

### 1.3 技术栈

| 组件 | 技术选择 |
|------|---------|
| MCU | STM32F407LGT6 (Cortex-M4, 168MHz, FPU) |
| 框架 | Arduino STM32 (Husarion 定制版) |
| RTOS | STM32 FreeRTOS v10.3.3 |
| ROS 通信 | micro-ROS Arduino v2.0.8 (Jazzy) |
| 以太网 | STM32Ethernet + LwIP (LAN9303 交换机) |
| IMU | Adafruit BNO055 库 |
| 测距 | Pololu VL53L0X 库 |
| 编译 | C++17, PlatformIO |

---

## 2. 架构设计解析

### 2.1 整体架构图

```
┌─────────────────────────────────────────────────────┐
│                    ROS 2 主机                        │
│               (micro-ROS Agent)                      │
└──────────────┬────────────────┬─────────────────────┘
               │ Serial(UART)  │ Ethernet(UDP)
               │ 921600baud    │ 192.168.77.x:8888
┌──────────────┴────────────────┴─────────────────────┐
│              通信管理器 (CommunicationManager)        │
│     ┌─────────────┬──────────────┬───────────┐      │
│     │  主串口      │  诊断串口    │  以太网    │      │
│     │  (UART1)    │  (UART3)     │ (LAN9303) │      │
│     └─────────────┴──────────────┴───────────┘      │
├─────────────────────────────────────────────────────┤
│              ROS 节点 (RosNode)                      │
│  ┌──────────┬──────────┬──────────┬──────────┐      │
│  │Publishers│Subscribers│ Services │ Clients  │      │
│  └──────────┴──────────┴──────────┴──────────┘      │
├─────────────────────────────────────────────────────┤
│              FreeRTOS 任务调度器                      │
│  ┌─────────┬─────────┬──────────┬──────────────┐    │
│  │ SAFETY  │ CONTROL │  COMMS   │   SENSORS    │    │
│  │  (P6)   │  (P5)   │  (P4)   │    (P3)      │    │
│  │Shutdown │Motor PID│ uROS    │Battery/IMU   │    │
│  │         │Encoder  │ Spin    │Range         │    │
│  └─────────┴─────────┴──────────┴──────────────┘    │
├─────────────────────────────────────────────────────┤
│              硬件抽象层 (HAL)                         │
│  ┌──────┬──────┬─────┬──────┬──────┬──────┐         │
│  │Motor │Enc   │IMU  │Range │LED   │Fan   │         │
│  │DRV8848│HW-TIM│BNO055│VL53L0X│Strip │PWM  │       │
│  └──────┴──────┴─────┴──────┴──────┴──────┘         │
└─────────────────────────────────────────────────────┘
```

### 2.2 源码目录结构说明

```
rosbot-firmware/
├── platformio.ini          # PlatformIO 构建配置（环境、依赖、编译选项）
├── include/
│   ├── config.hpp          # 条件编译入口，根据板型选择配置
│   ├── rtos.hpp            # RTOS 任务定义、优先级、栈大小
│   ├── STM32FreeRTOSConfig.h  # FreeRTOS 内核配置
│   ├── rosbot/config.hpp   # ROSbot 所有硬件引脚和参数配置
│   └── rosbot_xl/config.hpp # ROSbot XL 所有硬件引脚和参数配置
├── lib/                    # 功能模块库（每个文件夹是一个独立模块）
│   ├── battery/           # 电池 ADC 采集
│   ├── comm_manager/      # 通信管理器（Serial/Ethernet 选择）
│   ├── eeprom/            # EEPROM 读写（板型版本检测）
│   ├── encoder/           # 硬件编码器（定时器编码模式）
│   ├── fan/               # 风扇 PWM + NTC 温度采集
│   ├── imu/               # BNO055 九轴 IMU
│   ├── indicator/         # LED 状态指示器
│   ├── led_strip/         # APA102 LED 灯带
│   ├── motor/             # DRV8848 电机驱动
│   ├── pid/               # PID 控制器
│   ├── power_board/       # 电源板串口通信
│   ├── range/             # VL53L0X 激光测距
│   └── ros/               # micro-ROS 节点、发布者、订阅者
└── src/                    # 板型特定的入口代码
    ├── rosbot/            # ROSbot 的 main/rtos/ros
    └── rosbot_xl/         # ROSbot XL 的 main/rtos/ros
```

### 2.3 设计模式解析

**接口抽象模式**：每个硬件模块都有一个 `*Interface` 基类：
```cpp
// 电池接口 - 不关心是 ADC 还是电源板
class BatteryInterface {
  virtual void init() = 0;
  virtual void update() = 0;
  virtual BatteryData getData() const = 0;
};

// ROSbot 使用 ADC：   BatteryInterface* g_battery = &battery_adc;
// ROSbot XL 使用电源板：BatteryInterface* g_battery = &power_board;
```

**配置驱动模式**：所有硬件参数通过 `constexpr` 结构体在编译期确定：
```cpp
inline constexpr MotorDrv8848Config motor_fl_config = {
    .pwm_pin = PF9,
    .in_a_pin = PE5,
    .in_b_pin = PE6,
    .inv_dir = false,
    .max_velocity = 25.0f,
    ...
};
```

**FreeRTOS 队列通信**：传感器任务通过队列向 ROS 发布者传递数据：
```
[传感器任务] → xQueueOverwrite → [队列] → xQueueReceive → [ROS 发布者]
```

---

## 3. 开发环境与工具链

### 3.1 PlatformIO 配置

**编译环境**：

| 环境名 | 用途 | 特殊编译标志 |
|--------|------|-------------|
| `rosbot` | ROSbot Debug | `ENABLE_HWSERIAL1`, `ENABLE_HWSERIAL3`, `ROSBOT` |
| `rosbot_release` | ROSbot Release | 额外 `-O2` 优化 |
| `rosbot_xl` | ROSbot XL Debug | `ENABLE_HWSERIAL1`, `ETHERNET_USE_FREERTOS`, `LAN9303`, `ROSBOT_XL` |
| `rosbot_xl_release` | ROSbot XL Release | 额外 `-O2` 优化 |

**依赖库**：

| 库 | 版本 | 用途 |
|----|------|------|
| STM32Ethernet | 1.0.0 | 以太网驱动（Husarion 定制） |
| LwIP | 2.2.1 | TCP/IP 协议栈 |
| micro_ros_arduino | v2.0.8-jazzy | micro-ROS 通信 |
| STM32FreeRTOS | 10.3.3 | 实时操作系统 |
| Adafruit_BNO055 | 1.6.4 | IMU 传感器驱动 |
| Adafruit_BusIO | 1.14.0 | I2C/SPI 通用接口 |
| vl53l0x-arduino | 1.3.1 | 激光测距传感器 |

### 3.2 快速上手

```bash
# 安装 PlatformIO
pip install platformio

# 编译 ROSbot 固件
platformio run -e rosbot

# 编译并烧录（需要 ST-Link）
platformio run -e rosbot --target upload

# 编译 Release 版本（优化后体积更小）
platformio run -e rosbot_release
```

### 3.3 调试方法

- **ST-Link**：通过 SWD 接口调试，速度 4000kHz
- **串口监视**：波特率 921600，通过诊断串口查看运行状态
- **Monitor 任务**：Debug 模式下会输出各任务的堆栈使用和运行时间统计

---

## 4. RTOS 实时操作系统层

### 4.1 为什么需要 RTOS？

机器人控制系统需要同时处理多个实时任务：
- 电机 PID 控制需要精确的 200Hz 更新频率
- 编码器需要 500Hz 高频采样避免丢步
- 传感器读取、ROS 通信、LED 显示等需要并行运行

FreeRTOS 提供了抢占式多任务调度，确保高优先级任务（如电机控制）不会被低优先级任务（如 LED 动画）阻塞。

### 4.2 任务优先级设计

```
优先级 6 (SAFETY)    : 关机监控 — 3Hz   — 最高优先级，确保安全
优先级 5 (CONTROL)   : 电机控制 — 200Hz — 需要实时性保证
                     : 编码器   — 500Hz — 高频采样保证精度
优先级 4 (COMMS)     : uROS     — 1000Hz — ROS 通信轮询
优先级 3 (SENSORS)   : 电池     — 10Hz  — 低频采样足够
                     : IMU      — 50Hz  — 匹配 BNO055 输出速率
                     : 测距     — 10Hz  — VL53L0X 连续测量周期
优先级 2 (OBSERVING) : LED 指示 — 20Hz  — 人眼感知频率
                     : 硬件监控 — 10Hz  — 温度/风扇
                     : LED 灯带 — 25Hz  — 动画帧率
优先级 1 (BLOCKING)  : 监控     — 1Hz   — 调试信息输出
                     : uROS Ping— 2Hz   — Agent 连接检测
```

### 4.3 重要概念解释

**TickType_t 和频率转换**：
```cpp
// FreeRTOS 的 Tick 频率为 1000Hz（每 1ms 一个 Tick）
// 频率转 Tick 周期：例如 200Hz → 1000/200 = 5 Ticks = 5ms
inline TickType_t frequencyToTicks(float freq) {
  return (TickType_t)(configTICK_RATE_HZ / freq);
}
```

**队列通信**（xQueueOverwrite）：
```cpp
// 使用 Overwrite 而非 Send，这样只保留最新数据
// 如果消费者来不及读取，旧数据会被覆盖（适合传感器数据）
xQueueOverwrite(battery_queue, &data);
```

**任务栈大小**：
```cpp
// 栈大小单位是 StackType_t（4 字节），加上最小栈基础值
// 例如 Stack::M = 128 → 实际栈 = configMINIMAL_STACK_SIZE + 128 words
```

### 4.4 二次开发：如何添加新任务

```cpp
// 1. 在 rtos.cpp 中声明新任务函数
void myNewTask(void* p);

// 2. 在 tasks[] 数组中添加配置
TaskConfig tasks[] = {
    // ... 既有任务 ...
    {"MyTask", Priority::SENSORS, Stack::S, 50, myNewTask},
    // 名称, 优先级, 栈大小, 频率(Hz), 函数指针
};

// 3. 实现任务函数
void myNewTask(void* p) {
  TickType_t period = taskGetPeriod(p);     // 从参数获取周期
  TickType_t wake_time = xTaskGetTickCount();
  
  while (true) {
    // 你的代码...
    vTaskDelayUntil(&wake_time, period);    // 精确周期等待
  }
}
```

---

## 5. 通信管理模块

### 5.1 模块概述

`CommunicationManager` 是系统启动时的"交通指挥官"，负责决定固件通过哪个通道与 ROS 主机通信。

### 5.2 传输选择流程

```
开机 → 初始化所有串口
  │
  ├── 等待 2000ms 检测按钮状态
  │     │
  │     ├── 按钮被按下 → 使用诊断串口作为 ROS 通信
  │     │                  （Debug 输出禁用）
  │     │
  │     └── 超时无操作 → 使用主传输通道
  │           │
  │           ├── ROSbot → UART1 (PA9/PA10, 921600baud)
  │           │
  │           └── ROSbot XL → Ethernet (192.168.77.3)
  │                            Debug 通过 UART1
  │
  └── 命名空间协商 (FW/NS/ACK 握手)
        │
        ├── 固件发送: "FW: 1.0.0\r\n" (每 250ms 重发)
        ├── 主机回复: "NS:robot_name\r\n"
        └── 固件确认: "ACK\r\n"
```

### 5.3 关键代码解析

```cpp
// 传输选择的核心逻辑
const SerialConfig* selectTransport(uint32_t timeout_ms = 2000) {
  // 在 timeout_ms 内轮询检查条件
  while ((millis() - start) < timeout_ms) {
    if (cfg_.useDiagnosticCondition()) {
      // 条件满足 → 使用诊断串口作为 ROS 传输
      selected_type_ = TransportType::kSerial;
      selected_serial_ = &cfg_.diagnostic_serial;
      debug_available_ = false;  // 诊断口被占用，无法 Debug
      return selected_serial_;
    }
  }
  // 超时 → 使用默认主传输
  debug_available_ = true;  // 诊断口空闲可用于 Debug
  return (primary_type == kSerial) ? &primary_serial : nullptr;
}
```

### 5.4 二次开发建议

如果你只需要 Serial 通信（不需要 Ethernet），可以简化为：
```cpp
CommunicationManagerConfig communication_config = {
    .primary_type = TransportType::kSerial,
    .primary_serial = {
        .serial = &Serial1,
        .baudrate = 921600,
        .rxPin = PA10,
        .txPin = PA9,
        .timeout_ms = 1,
        .name = "SBC_SERIAL"
    },
    // 可选：保留诊断串口用于调试
    .diagnostic_serial = { ... },
};
```

---

## 6. ROS 节点模块

### 6.1 模块概述

`RosNode` 类封装了 micro-ROS 的完整生命周期管理，包括 Agent 发现、实体创建/销毁、消息发布/订阅。

### 6.2 状态机

```
  ┌──────────┐    pingAgent()成功    ┌─────────────────┐
  │ WAITING  │ ──────────────────→  │ AGENT_AVAILABLE  │
  │ (等待)   │                      │ (Agent已发现)     │
  └──────────┘                      └────────┬─────────┘
       ▲                                     │ createEntities()
       │                                     ▼
  ┌──────────────┐  pingAgent()失败  ┌──────────────┐
  │ DISCONNECTED │ ←───────────────  │  CONNECTED   │
  │ (已断开)      │                   │  (已连接)     │
  └──────────────┘                   └──────────────┘
       │                                     │
       └─── destroyEntities() ──────────────→│
             重新进入 WAITING
```

### 6.3 ROS 实体一览

**发布者（Publishers）**：

| 话题名 | 消息类型 | 频率 | 说明 |
|--------|---------|------|------|
| `battery` | `sensor_msgs/BatteryState` | 10Hz/1Hz | 电池状态 |
| `buttons` | `std_msgs/UInt8` | 定时器触发 | 按钮状态位掩码 |
| `_imu/data` | `sensor_msgs/Imu` | 50/100Hz | 加速度/角速度/姿态 |
| `_motors/feedback` | `sensor_msgs/JointState` | 500Hz | 编码器位置/速度 |
| `ranges` | `sensor_msgs/Range` (x4) | 10Hz | 四路测距（仅ROSbot） |

**订阅者（Subscribers）**：

| 话题名 | 消息类型 | QoS | 说明 |
|--------|---------|-----|------|
| `_motors/cmd` | `std_msgs/Float32MultiArray` | BestEffort | 电机速度指令 [rad/s] |
| `leds` | `std_msgs/UInt8` | Reliable | LED 位掩码控制 |
| `led_strip` | `sensor_msgs/Image` | BestEffort | LED 灯带 RGB 数据（仅XL） |

**服务（Services）**：

| 服务名 | 类型 | 说明 |
|--------|------|------|
| `_mcu_id` | `std_srvs/Trigger` | 返回 MCU 唯一 ID（JSON格式） |

### 6.4 消息流转过程

以 IMU 数据为例：
```
1. imuTask (50Hz) → g_imu->update() → 读取 I2C 数据
2. 获取时间戳 → rtos_get_timestamp_ns()（与 ROS Agent 时间同步）
3. 写入队列 → xQueueOverwrite(imu_queue, &data)
4. timerCallback (10ms) → ImuPublisher::publish()
5. 从队列读取 → xQueueReceive(imu_queue, &data)
6. 填充 ROS 消息 → rcl_publish()
```

### 6.5 二次开发：添加新的发布者

```cpp
// 1. 创建发布者类（继承 PublisherInterface）
class MyPublisher : public PublisherInterface {
  rcl_ret_t init(rcl_node_t& node, rcl_allocator_t& alloc) override {
    return rclc_publisher_init_default(
        &publisher_, &node,
        ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Float32),
        "my_topic");
  }
  void publish() override {
    // 从队列读取数据，填充消息，发布
    rcl_publish(&publisher_, &msg_, nullptr);
  }
};

// 2. 在 ros.cpp 中添加到 publishers 数组
static MyPublisher s_my_pub(my_config);
static std::vector<PublisherInterface*> publishers = {
    &s_battery_pub, &s_imu_pub, &s_my_pub,  // ← 添加
};
```

---

## 7. 电机驱动模块

### 7.1 模块概述

使用 **DRV8848** 双路H桥电机驱动芯片，4轮驱动采用 2 片 DRV8848（每片驱动同侧两个电机）。ROSbot XL 另外使用了 **MAX22205** 电流限制芯片。

### 7.2 控制原理

**Hi-Z PWM 控制方案**：

DRV8848 的控制不是传统的 PWM + 方向引脚方式，而是使用 Hi-Z（高阻态）方案：

```
前进：IN_A = Hi-Z(输入模式), IN_B = LOW, PWM → EN引脚
后退：IN_A = LOW, IN_B = Hi-Z(输入模式), PWM → EN引脚
制动：IN_A = HIGH, IN_B = HIGH, PWM = 100%
空档：IN_A = LOW, IN_B = LOW, PWM = 0%
```

> 💡 **为什么使用 Hi-Z？** DRV8848 内部逻辑是：当 IN 引脚为 Hi-Z 时，
> 该半桥跟随 EN/PWM 信号；当 IN = LOW 时，该半桥关断。
> 这比简单的 PWM+DIR 控制提供了更好的再生制动效果。

### 7.3 电机组管理

```cpp
// 两组驱动器，每组有使能引脚和故障检测引脚
struct DriverGroupConfig {
  uint8_t enable_pin;  // nSLEEP 引脚（LOW 使驱动器休眠）
  uint8_t fault_pin;   // nFAULT 引脚（LOW 表示过流/过温故障）
};

// 左侧电机组：FL + RL（共享 PC14 使能，PE1 故障）
// 右侧电机组：FR + RR（共享 PC13 使能，PE0 故障）
```

### 7.4 电机控制任务流程

```
motorControlTask (200Hz)
    ↓
g_motors.update()
    ↓ （对每个电机）
motor.update(dt, move=true)
    ↓
① 读取目标速度 (target_velocity_)
② 读取编码器当前速度 (encoder_->getData().velocity)
③ PID 计算 → output = pid_.compute(target, current, dt)
④ 应用 PWM → applyPWM(output)
    ↓
设置旋转方向 → setMode(FORWARD/REVERSE/BRAKE)
设置 PWM 占空比 → pwm_timer_->setCaptureCompare(...)
```

### 7.5 二次开发注意事项

1. **PWM 频率**：20kHz，超出人耳范围，避免电机啸叫
2. **电机方向**：通过 `inv_dir` 标志翻转，FR 和 RR 因为安装方向相反需要反转
3. **安全超时**：如果没有收到新的速度指令，电机会自动刹车（在 MotorArray 中实现）
4. **MAX22205 电流限制**（仅 XL）：v1.1 通过 GPIO HIGH 设置最大电流，v1.2 通过外部电阻设置

---

## 8. 编码器模块

### 8.1 模块概述

使用 STM32 定时器的硬件编码器模式，直接在硬件层面对正交编码器信号进行四倍频计数，无需软件中断处理。

### 8.2 工作原理

```
编码器输出两路正交信号 A 和 B：
       ┌───┐   ┌───┐
  A ───┘   └───┘   └───   → TIM_CH1
     ┌───┐   ┌───┐
  B ─┘   └───┘   └───     → TIM_CH2
  
四倍频模式 (TIM_ENCODERMODE_TI12)：
  A 和 B 的每个上升/下降沿都计数 → 1个机械周期 = 4个电气脉冲
  
ROSbot: 48 CPR × 4 = 192 计数/转, × 34齿轮比 = 6528 计数/输出轴转
ROSbot XL: 64 CPR × 4 = 256 计数/转, × 50齿轮比 = 12800 计数/输出轴转
```

### 8.3 速度计算

```cpp
void HardwareEncoder::update() {
  // 1. 读取定时器计数值（硬件自动累加）
  const uint32_t cnt = getTicks();
  
  // 2. 计算计数差（处理 16位溢出回绕）
  int32_t delta = cnt - last_cnt;
  if (delta > 0x8000)  delta -= 0x10000;  // 正向溢出
  if (delta < -0x8000) delta += 0x10000;  // 反向溢出
  
  // 3. 转换为弧度
  float delta_pos = delta * rad_per_tick;  // 每个 tick 对应的弧度
  position += delta_pos;
  
  // 4. 计算速度（低通滤波平滑）
  float vel = delta_pos / dt;
  velocity = alpha * vel + (1-alpha) * last_velocity;
  // ROSbot: alpha=0.1（更灵敏）, ROSbot XL: alpha=0.05（更平滑）
}
```

### 8.4 关键参数

| 参数 | ROSbot | ROSbot XL |
|------|--------|-----------|
| 编码器 CPR | 48 | 64 |
| 齿轮比 | 34:1 | 50:1 |
| 每转 Ticks | 1632 | 3200 |
| rad/tick | 0.00385 | 0.00196 |
| 低通系数 α | 0.1 | 0.05 |
| 采样频率 | 500Hz | 500Hz |

---

## 9. PID 控制器模块

### 9.1 功能概述

PID 控制器是电机闭环控制的核心，将期望速度转换为 PWM 占空比输出。本实现包含以下增强功能：

- **积分抗饱和（Anti-windup）**：限制积分项大小
- **加速度限制**：平滑加速/减速
- **惯性补偿**：克服电机静摩擦力

### 9.2 算法详解

```
        ┌───────────────────────────────────────────┐
        │              PID 控制器                    │
        │                                           │
设定值 ──┤  error = setpoint - measurement           │── 输出
        │  P = Kp × error                          │  (PWM)
        │  I = Ki × clamp(∫error·dt)               │
        │  D = Kd × (error - prev_error) / dt      │
        │  output = clamp(P + I + D + 惯性补偿)     │
        └───────────────────────────────────────────┘
                          ▲
                          │
                    编码器速度反馈
```

**惯性补偿**（解决电机"死区"问题）：
```cpp
// 当目标速度不为零，但 PID 输出小于电机启动阈值时
// 添加额外的驱动力，帮助电机克服静摩擦
float inertiaCompensation(float measurement) {
  // 低速时补偿大，高速时补偿小
  float scale = max(0, 1.0 - |measurement| / compensation_up_to_speed);
  return min_power_to_move * scale;
}
// ROSbot: min_power=0.4, up_to_speed=4.0 rad/s
// 含义：速度低于 4 rad/s 时逐渐增加补偿，最大 40% PWM
```

### 9.3 PID 参数

| 参数 | ROSbot | ROSbot XL | 说明 |
|------|--------|-----------|------|
| Kp | 0.07 | 0.15 | 比例增益（越大响应越快） |
| Ki | 0.4 | 0.55 | 积分增益（消除稳态误差） |
| Kd | 0.002 | 0.007 | 微分增益（抑制振荡） |
| 输出范围 | ±1.0 | ±1.0 | 对应 ±100% PWM |
| 最小启动力 | 0.4 | 0.0 | 惯性补偿阈值 |
| 补偿速度上限 | 4.0 | 0.0 | rad/s |

> 💡 **调参提示**：ROSbot XL 的 PID 参数更大是因为其电机齿轮比更高（50:1 vs 34:1），
> 需要更强的控制力来克服更大的等效惯量。

---

## 10. IMU 惯性测量模块

### 10.1 模块概述

使用 Bosch BNO055 九轴 IMU 传感器，内置传感器融合算法，直接输出姿态四元数，无需 MCU 端进行卡尔曼滤波。

### 10.2 传感器特性

| 特性 | 值 |
|------|-----|
| 接口 | I2C, 400kHz |
| 加速度计 | ±2g/4g/8g/16g |
| 陀螺仪 | ±125/250/500/1000/2000 °/s |
| 磁力计 | ±1300/2500 µT |
| 融合模式 | NDOF (所有9轴) |
| 输出 | 加速度、角速度、四元数 |

### 10.3 初始化流程

```cpp
void ImuBno055::init() {
  // 1. 上电（ROSbot 需要 GPIO 控制电源）
  // 2. I2C 初始化并设置频率为 400kHz
  // 3. 验证传感器 ID
  // 4. 设置轴映射（根据安装方向）
  //    ROSbot:   REMAP_CONFIG_P0, REMAP_SIGN_P3
  //    ROSbot XL: REMAP_CONFIG_P1, REMAP_SIGN_P2
  // 5. 启用外部晶振（更高精度）
  // 6. 设置 NDOF 模式
}
```

### 10.4 数据输出

```cpp
ImuData ImuBno055::getData() {
  return {
    .acceleration = {ax, ay, az},      // m/s² 线性加速度 
    .angular_velocity = {gx, gy, gz},  // rad/s（原始是 deg/s × π/180）
    .orientation = {qx, qy, qz, qw},  // 四元数姿态
  };
}
```

### 10.5 二次开发建议

- **更换为 MPU6050/ICM-20948**：需要实现 `ImuInterface`，并在主机端做传感器融合
- **轴映射**：BNO055 有 8 种预设映射（P0-P7），根据传感器安装朝向选择
- **校准**：BNO055 需要磁力计校准，可以保存校准数据到 EEPROM

---

## 11. 测距传感器模块

### 11.1 模块概述（仅 ROSbot）

使用 4 个 ST VL53L0X ToF（飞行时间）激光测距传感器，分布在机器人四角，实现近距离避障。

### 11.2 I2C 地址分配

4 个 VL53L0X 共享同一条 I2C 总线，通过 XSHUT（关断）引脚逐个初始化并分配不同地址：

```
初始化流程：
1. 所有 XSHUT = LOW（全部关闭）
2. FL: XSHUT = HIGH → 上电 → 设置地址 0x30
3. FR: XSHUT = HIGH → 上电 → 设置地址 0x31
4. RL: XSHUT = HIGH → 上电 → 设置地址 0x32
5. RR: XSHUT = HIGH → 上电 → 设置地址 0x33
```

### 11.3 传感器配置

| 参数 | 值 | 说明 |
|------|-----|------|
| 信号速率限制 | 0.1 MCPS | 最低有效信号阈值 |
| VCSEL 预测距 | 16 | 激光脉冲周期 |
| VCSEL 最终测距 | 12 | 激光脉冲周期 |
| 时序预算 | 50ms | 单次测量时间 |
| 连续测量周期 | 100ms | 自动测量间隔 |
| 有效范围 | 10-900mm | 超出范围返回无效 |

### 11.4 二次开发建议

- **ROSbot XL 没有测距传感器**，如需添加可参考 ROSbot 的实现
- **VL53L1X**：升级版传感器，距离更远（4m），API 类似
- **超声波替代**：如用 HC-SR04，需实现 `RangeInterface` 接口

---

## 12. 电池管理模块

### 12.1 两种实现

**ROSbot — ADC 直接采集**：
```
电池电压 → 56kΩ+10kΩ 分压 → PA5(ADC) → 10位 ADC → 换算
  
公式：V_bat = ADC_raw × (3.3V × 0.986 × 6.6) / 1023
  
其中：
  3.3V = 参考电压
  0.986 = 校正系数（补偿分压电阻偏差）
  6.6 = 分压比 (56k+10k)/10k
  1023 = 10位 ADC 最大值
```

**ROSbot XL — 电源板串口通信**：
```
STM32 ←── UART (38400baud) ──→ 电源板
         帧格式: <CC SS AA...AA CS>
         CC=命令, SS=参数数, AA=参数, CS=XOR校验
```

### 12.2 电池参数

| 参数 | 值 |
|------|-----|
| 电池类型 | 3S LiPo |
| 额定电压 | 11.1V |
| 满电电压 | 12.6V |
| 低电压告警 | 9.6V |
| 设计容量 | 7.8 Ah (3×2.6Ah) |

### 12.3 电量百分比计算

```cpp
// 简单的线性插值（不够精确，但胜在简单）
percentage = (voltage - v_min) / (v_max - v_min)
           = (voltage - 9.6) / (12.6 - 9.6)
           = (voltage - 9.6) / 3.0
```

> ⚠️ **精度问题**：线性插值对锂电池不够准确（锂电池放电曲线非线性）。
> 建议二次开发时使用查找表或库仑计方案。

---

## 13. LED 指示与灯带模块

### 13.1 GPIO LED 指示器

`LedIndicator` 类根据系统状态控制 LED：

| 状态 | LED 行为 |
|------|---------|
| 正常运行 | 常亮 |
| 电池低电 | 快闪（500ms 周期） |
| ROS 断连 | 慢闪 |
| 错误 | 双闪 |

### 13.2 APA102 LED 灯带（仅 ROSbot XL）

| 特性 | 值 |
|------|-----|
| 类型 | APA102/SK9822 |
| 数量 | 18 颗 |
| 接口 | SPI (PB15/PB14/PB10, 4MHz) |
| SPI 模式 | Mode 3, MSB First |

**特殊功能**：
- LED 序号交换：物理布线顺序与逻辑顺序不同，通过 SwapPair 映射
- 空闲动画：从中间向两端扩展的呼吸效果
- ROS 控制：通过 `sensor_msgs/Image` 消息设置任意 RGB 颜色

---

## 14. 风扇温控模块

### 14.1 概述（仅 ROSbot XL）

NTC 热敏电阻测温 + PWM 风扇控制，板型版本决定控制方式。

### 14.2 温度测量

```
NTC 热敏电阻 → 分压电路 → PB1(ADC)
  
Steinhart-Hart 方程计算温度：
1/T = c1 + c2×ln(R) + c3×(ln(R))³

其中：
  c1 = 1.112613927×10⁻³
  c2 = 2.37277392×10⁻⁴
  c3 = 7.167×10⁻⁸
  上拉电阻 = 5.23kΩ
```

### 14.3 风扇控制策略

| 板型 | 控制方式 | 引脚 |
|------|---------|------|
| v1.1 | AlwaysOn（常开GPIO） | PC13 |
| v1.2 | 比例PWM（TIM8 CH2N） | PB0 |

**v1.2 PWM 温度-占空比映射**：
```
温度 ≤ 30°C → 20% 占空比（最低转速）
温度 = 42°C → 60% 占空比（线性插值）
温度 ≥ 55°C → 100% 占空比（全速）
```

---

## 15. 电源板通信模块

### 15.1 概述（仅 ROSbot XL）

电源板通过 UART 与 MCU 通信，提供电池状态和板卡信息。同时负责关机流程管理。

### 15.2 通信协议

```
帧格式：< CC SS AA ... AA CS >

  <  = 帧起始标记 (ASCII '<')
  CC = 命令字 (2个十六进制字符，如 "01")
  SS = 参数字节数 (2个十六进制字符)
  AA = 参数数据 (每字节2个十六进制字符)
  CS = XOR 校验 (cmd ^ arg_size ^ arg[0] ^ ... ^ arg[n-1])
  >  = 帧结束标记 (ASCII '>')
```

### 15.3 关机流程

```
电源按钮按下 → PB_SHD_DETECT = HIGH
    ↓
STM32 检测到 → 通知 SBC 关机 (HTTP GET /shutdown)
    ↓
等待 5 秒（让 SBC 保存数据）
    ↓
PB_SHD_CONFIRM = HIGH → 电源板切断电源
```

---

## 16. ROSbot vs ROSbot XL 差异对比

| 特性 | ROSbot | ROSbot XL |
|------|--------|-----------|
| **通信方式** | Serial (UART1) | Ethernet + Serial 备用 |
| **电池检测** | ADC 直接采集 | 电源板串口通信 |
| **编码器 CPR** | 48 | 64 |
| **齿轮比** | 34:1 | 50:1 |
| **每转 Ticks** | 1632 | 3200 |
| **最大速度** | 25 rad/s | 22 rad/s |
| **PID Kp/Ki/Kd** | 0.07/0.4/0.002 | 0.15/0.55/0.007 |
| **惯性补偿** | 有 (0.4, 4.0) | 无 |
| **IMU I2C** | PC9/PA8 (独立) | PF0/PF1 (共享) |
| **测距传感器** | 4× VL53L0X | 无 |
| **LED** | 3×GPIO (R+G×2) | 1×GPIO + 18×APA102灯带 |
| **风扇** | 无 | PWM/AlwaysOn |
| **电源板** | 无 | UART 通信 |
| **关机管理** | 无 | 按钮检测+SBC通知 |
| **EEPROM** | 无 | 板型版本检测 |
| **电流限制** | 无 | MAX22205 (ILIM) |
| **串口数** | 2 (主+诊断) | 2 (诊断+电源板) |
| **按钮** | 2 (PG12/PG13) | 1+1 (PF11+PF12复位) |

---

## 17. 二次开发建议与路线图

### 17.1 最小可行产品（MVP）推荐方案

如果你是第一次设计 ROS 开发板，建议以 ROSbot 的架构为基础：

```
必需模块：
✅ STM32F407 + FreeRTOS
✅ UART × 1（与 ROS 主机通信，921600baud）
✅ 4 × 电机驱动（DRV8848 × 2）
✅ 4 × 编码器（Timer 编码模式）
✅ PID 闭环控制
✅ IMU（BNO055 或 ICM-20948）
✅ 电池电压 ADC 采集

推荐但可选：
☑️ UART × 1（诊断串口）
☑️ LED 状态指示
☑️ 按钮输入

高级功能：
☐ 测距传感器（VL53L0X / 超声波）
☐ LED 灯带
☐ Ethernet 扩展板
☐ 风扇温控
☐ 电源板通信
```

### 17.2 代码移植要点

1. **修改 config.hpp**：在 `include/your_board/config.hpp` 中定义你的引脚映射
2. **修改 platformio.ini**：添加新的编译环境
3. **保留 lib/ 目录**：所有模块都是可复用的，只需更改配置
4. **简化 CommunicationManager**：如果只用 Serial，可以去掉 Ethernet 相关代码

### 17.3 常见"坑"列表

| 问题 | 原因 | 解决方案 |
|------|------|---------|
| 编码器计数不稳定 | GPIO 复用功能冲突 | 确认引脚的 AF 映射正确 |
| 电机不转 | DRV8848 nSLEEP 未拉高 | 检查 enable_pin 初始化 |
| I2C 通信失败 | 上拉电阻缺失 | 加 4.7kΩ 上拉到 3.3V |
| PID 振荡 | 参数不匹配 | 先只调 Kp，再加 Ki，最后加 Kd |
| micro-ROS 连接不上 | Agent 未启动 | 确认 `ros2 run micro_ros_agent` |
| FreeRTOS 栈溢出 | 栈分配不够 | 增大 Stack 枚举值 |
| 串口数据乱码 | 波特率不匹配 | 两端都设为 921600 |

### 17.4 进阶优化建议

1. **DMA 传输**：I2C/ADC 使用 DMA 减少 CPU 占用（源码中有 TODO 标注）
2. **看门狗（IWDG）**：添加独立看门狗防止系统死机
3. **CAN 总线**：如需连接更多外设，可添加 CAN 通信
4. **OTA 升级**：通过 ROS 服务实现固件在线升级
5. **参数服务**：通过 ROS 参数动态调整 PID 参数

---

> 📝 **本文档基于源码分析自动生成，如有疑问请结合源码阅读理解。**
