# Witty Pi 5 电源板整合 ROSbot PRO 设计与实施文档

> 本文档详细描述如何将 Witty Pi 5 (RP2350) 电源管理方案整合到 ROSbot PRO 固件，完全替代 ROSbot XL 原来的PowerBoard。

---

## 1. 概述

### 1.1 设计目标

| 目标 | 说明 |
|------|------|
| 替代 ROSbot XL PowerBoard | 使用 Witty Pi 5 方案提供完整的电池管理和电源控制功能 |
| I2C 通信接口 | 替代原 UART 协议，使用标准 I2C 寄存器读写 |
| 保留关机协调能力 | 按钮关机 → 通知 SBC → 确认断电的完整流程 |
| 增值功能 | RTC 定时开关机、看门狗心跳、过温保护、过欠压保护 |
| 编译兼容 | 通过编译开关切换 ADC 电池方案和 WP5 电源板方案 |

### 1.2 方案对比

| 特性 | ROSbot XL PowerBoard | BatteryAdc (V1) | Witty Pi 5 (V2) |
|------|------|------|------|
| MCU | 未知 | 无（STM32 ADC） | RP2350 Cortex-M33 |
| 通信方式 | UART 38400 | GPIO ADC | **I2C Slave 0x51** |
| 电压监测 | ✅ (精确) | ✅ (分压估算) | **✅ (ADC 4通道)** |
| 电流监测 | ✅ 充放电 | ❌ | **✅ 输出电流** |
| 温度监测 | ✅ 电池温度 | ❌ | **✅ TMP112 环境温度** |
| 电源控制 | GPIO关机 | ❌ | **✅ DCDC开关 + 电源切断** |
| RTC定时 | ❌ | ❌ | **✅ RX8025 定时开关机** |
| 看门狗 | ❌ | ❌ | **✅ 心跳超时重启** |
| 低压保护 | ❌ | ❌ | **✅ 可配置阈值** |
| 恢复电压 | ❌ | ❌ | **✅ 充电恢复自动开机** |
| 定时脚本 | ❌ | ❌ | **✅ .wpi 调度脚本** |
| 双电源源 | ❌ | ❌ | **✅ VUSB + VIN 优先级** |
| 固件格式 | 闭源 | N/A | **开源 UF2** |

---

## 2. Witty Pi 5 架构分析

### 2.1 硬件规格

- **MCU**: RP2350 ARM Cortex-M33, 150MHz, 520KB SRAM, 4MB Flash
- **RTC**: RX8025 (I2C 0x32)，精准实时时钟
- **温度传感器**: TMP112 (I2C 0x48)
- **ADC**: 12-bit, 4通道 — VUSB, VIN, VOUT, IOUT
- **电源控制**: DC/DC变换器使能 (GPIO 12) + Pi电源控制 (GPIO 13)
- **存储**: FAT32 文件系统，USB MSC（优盘模式）配置文件管理
- **逻辑电平**: 3.3V（与STM32F407直接兼容）

### 2.2 I2C 从设备寄存器映射

Witty Pi 5 I2C从设备地址: **0x51**

#### 只读寄存器 (0x00 - 0x0F)

| 地址 | 名称 | 说明 |
|------|------|------|
| 0x00 | FW_ID | 固件ID = 0x51 |
| 0x01 | FW_VERSION_MAJOR | 主版本号 |
| 0x02 | FW_VERSION_MINOR | 次版本号 (×100) |
| 0x03-0x04 | VUSB_MV | USB电压 (mV, MSB先) |
| 0x05-0x06 | VIN_MV | 输入电压 (mV, MSB先) |
| 0x07-0x08 | VOUT_MV | 输出电压 (mV, MSB先) |
| 0x09-0x0A | IOUT_MA | 输出电流 (mA, MSB先) |
| 0x0B | POWER_MODE | 0=VUSB供电, 1=VIN供电 |
| 0x0C | MISSED_HEARTBEAT | 丢失心跳次数 |
| 0x0D | RPI_STATE | 状态: 0=OFF, 1=STARTING, 2=ON, 3=STOPPING |
| 0x0E | ACTION_REASON | 动作原因 (高4位=开机, 低4位=关机) |

#### 可读写配置寄存器 (0x10 - 0x3F)

| 地址 | 名称 | 默认值 | 说明 |
|------|------|--------|------|
| 0x10 | I2C地址 | 0x51 | 可修改从设备地址 |
| 0x11 | DEFAULT_ON_DELAY | 255(禁用) | 上电自动开机延迟(秒) |
| 0x12 | POWER_CUT_DELAY | 15 | 关机后断电延迟(秒) |
| 0x16 | LOW_VOLTAGE | 0(禁用) | 低压阈值 (×100mV) |
| 0x17 | RECOVERY_VOLTAGE | 0(禁用) | 恢复电压阈值 (×100mV) |
| 0x18 | PS_PRIORITY | 0 | 0=VUSB优先, 1=VIN优先 |
| 0x1D | WATCHDOG | 0(禁用) | 允许丢失心跳次数 |
| 0x20-0x27 | ALARMx | - | RTC定时开关机 (BCD) |
| 0x28-0x2B | TEMP_ACTION | - | 温度触发动作 |

#### 管理寄存器 (0x40 - 0x4F)

| 地址 | 名称 | 说明 |
|------|------|------|
| 0x46 | HEARTBEAT | 写入任意值重置心跳计时器 |
| 0x47 | SHUTDOWN | 非零=请求关机, 零=正常 |

#### 虚拟寄存器 (0x50 - 0x67) — 映射到RTC和温度传感器

| 地址 | 名称 | 说明 |
|------|------|------|
| 0x50-0x5F | RX8025 | RTC时间/闹钟/控制 |
| 0x60-0x67 | TMP112 | 温度/配置/阈值 |

### 2.3 状态机

```
                 ┌──────────┐
    上电/按钮 ──►│ STARTING │
                 │  (1)     │
                 └────┬─────┘
                      │ 系统启动完成 / 30s超时
                      ▼
                 ┌──────────┐
                 │   ON     │◄── 心跳正常
                 │  (2)     │
                 └────┬─────┘
                      │ 关机请求/低压/温度/看门狗
                      ▼
                 ┌──────────┐
                 │ STOPPING │
                 │  (3)     │
                 └────┬─────┘
                      │ POWER_CUT_DELAY 超时
                      ▼
                 ┌──────────┐
    定时唤醒 ◄───│   OFF    │
    电压恢复     │  (0)     │
                 └──────────┘
```

### 2.4 电源管理功能

| 功能 | 机制 | 说明 |
|------|------|------|
| **双源切换** | VUSB/VIN优先级 | 自动在USB和电池之间切换 |
| **低压保护** | ADC轮询 (1Hz) | VIN低于阈值 → 触发关机 |
| **恢复开机** | ADC轮询 | VIN恢复到恢复电压 → 自动开机 |
| **温度保护** | TMP112 | 过温/低温触发开/关机动作 |
| **看门狗** | 心跳寄存器 | 丢失N次心跳 → 电源重启 |
| **定时调度** | RTC闹钟 | 定时开关机，支持脚本 |
| **按钮控制** | GPIO中断 | 单击开/关，长按工厂复位 |

---

## 3. 系统集成架构

### 3.1 整体架构

```
                    ┌─────────────────────────────────────────────────────┐
                    │                    ROSbot PRO                       │
                    │                                                     │
  ┌──────────┐     │  ┌────────────────┐      ┌─────────────────────┐   │
  │ 3S LiPo  │─VIN─┤  │  Witty Pi 5    │      │    STM32F407LGT6    │   │
  │ Battery  │     │  │  (RP2350)      │      │    (Digital Board)  │   │
  │9.6-12.6V │     │  │                │      │                     │   │
  └──────────┘     │  │ ADC0: VUSB     │      │  [I2C1 Master]      │   │
                    │  │ ADC1: VIN ◄────┤      │   PB8 (SCL) ────────┼───┼── I2C @ 100kHz
                    │  │ ADC2: VOUT     │      │   PB9 (SDA) ────────┼───┘
                    │  │ ADC3: IOUT     │      │                     │
                    │  │                │      │  [I2C2 Master]      │
                    │  │ DC/DC 5V OUT ──┼──5V──┤   PF0/PF1: IMU     │   │
                    │  │                │      │                     │   │
                    │  │ GPIO13:POWER ──┼──?───┤   PD4: SHD_DETECT  │   │
                    │  │ GPIO22:LED     │      │   PD7: SHD_CONFIRM │   │
                    │  │ GPIO15:BUTTON  │      │                     │   │
                    │  │                │      │  [Ethernet]         │   │
                    │  │ I2C Slave 0x51 │      │   → SBC (RPi)      │   │
                    │  │ RX8025 (0x32)  │      │                     │   │
                    │  │ TMP112 (0x48)  │      │  [Motors]           │   │
                    │  └────────────────┘      │   DRV8874 × 4      │   │
                    │                          └─────────────────────┘   │
                    └─────────────────────────────────────────────────────┘
```

### 3.2 I2C 总线设计

| 总线 | 引脚 | 速率 | 设备 |
|------|------|------|------|
| **I2C1 (新增)** | PB8 (SCL), PB9 (SDA) | 100kHz | Witty Pi 5 (0x51) |
| I2C2 (已有) | PF0 (SDA), PF1 (SCL) | 400kHz | BNO055 IMU (0x29), EEPROM (0x50) |

**设计决策**: 使用独立 I2C1 总线连接 WP5，避免与 IMU 总线冲突。FreeRTOS 多任务环境下，IMU（imuTask 100Hz）和电源板（hwMonitorTask 1Hz）分别使用独立总线，无需互斥锁。

### 3.3 电源拓扑

```
Battery (3S LiPo 9.6-12.6V)
        │
        ├──── VIN ──── Witty Pi 5 ──── DC/DC ──── 5V Rail
        │                                           │
        │                                    ┌──────┼──────┐
        │                                    │      │      │
        │                               STM32F407  IMU   Sensors
        │
        └──── VBAT (直通) ──── DRV8874 Motor Drivers × 4
                                      │
                                  Motors × 4
```

**关键设计点**:
- 电机驱动直接由电池供电（VIN直通），不经过WP5的DC/DC
- WP5的DC/DC仅为5V逻辑系统供电
- WP5通过ADC1监测VIN（电池）电压
- WP5通过ADC3监测IOUT（5V输出）电流

---

## 4. 关机协调机制

### 4.1 关机流程 (替代ROSbot XL方案)

```
 Witty Pi 5            STM32F407 (ROSbot PRO)         SBC (RPi)
     │                          │                         │
     │ [按钮/低压/温度/调度]       │                         │
     │ state → STOPPING          │                         │
     │                          │                         │
     │  I2C: RPI_STATE=3 ──────▶│                         │
     │  (shutdownTask 轮询检测)   │                         │
     │                          │── HTTP GET /shutdown ──▶│
     │                          │   (192.168.77.2:3000)   │
     │                          │                         │
     │                          │◀─── TCP连接关闭 ────────│
     │                          │                         │
     │                          │── delay 5000ms ─────────│
     │                          │                         │
     │  (POWER_CUT_DELAY=15s)   │                         │
     │  超时，切断电源             │── 停止心跳 ─────────────│
     │                          │                         │
     │  GPIO13=LOW (断电)        │                         │
     │  state → OFF              │                         │
     └──────────────────────────└─────────────────────────┘
```

### 4.2 可选: GPIO中断方案

若需要更快响应，可连接 WP5 的一个 GPIO 到 STM32 的 PD4：

| 连接 | WP5端 | STM32端 | 功能 |
|------|-------|---------|------|
| 关机检测 | GPIO16 (OUTPUT) | PD4 (INPUT_PULLUP) | 高电平=关机请求 |
| 关机确认 | GPIO17 (INPUT) | PD7 (OUTPUT) | 高电平=确认完成 |

需修改 WP5 固件在 `request_shutdown()` 中拉高 GPIO16。此为生产版本可选优化。

### 4.3 看门狗心跳

```
STM32 ──(每30秒)──▶ I2C Write 0x46=0x01 ──▶ Witty Pi 5
                                               │
                                    重置心跳计时器 (60s)
                                               │
                                    如果连续丢失N次心跳
                                               │
                                    电源重启 (power cycle)
```

**配置建议**:
- `HEARTBEAT_INTERVAL`: 30秒
- `WATCHDOG` (寄存器 0x1D): 设为3（允许丢失3次 → ~180秒超时）

---

## 5. 软件设计

### 5.1 类继承结构

```
BatteryInterface (abstract)
    ├── BatteryAdc         (V1: 简单ADC分压读取)
    ├── PowerBoard         (ROSbot XL: UART协议)
    └── PowerBoardWP5      (V2: I2C Witty Pi 5)
```

### 5.2 PowerBoardWP5 类设计

```cpp
class PowerBoardWP5 : public BatteryInterface {
 public:
  // BatteryInterface 接口
  void init() override;      // I2C初始化 + 读取板卡信息 + 配置WP5
  void update() override;    // 读取电压/电流/状态 + 自动心跳

  // WP5 特有 API
  bool sendHeartbeat();           // 发送心跳
  bool isShutdownRequested();     // 检测关机请求
  void configureWP5();            // 配置低压/看门狗等参数

  // 数据访问
  uint16_t vinMv() const;        // 输入电压 (mV)
  uint16_t voutMv() const;       // 输出电压 (mV)
  uint16_t ioutMa() const;       // 输出电流 (mA)
  uint8_t  powerMode() const;    // 电源模式
  uint8_t  rpiState() const;     // WP5 状态机
};
```

### 5.3 配置结构

```cpp
struct PowerBoardWP5Config {
  TwoWire& i2c_bus;                     // I2C总线引用
  uint8_t i2c_addr;                     // I2C地址 (默认 0x51)
  float v_min;                          // 电池最低电压 (V)
  float v_max;                          // 电池最高电压 (V)
  uint8_t shd_detect_pin;              // 关机检测引脚 (0xFF=I2C轮询)
  uint32_t heartbeat_interval_ms;      // 心跳间隔 (ms)
  uint8_t low_voltage_threshold;       // 低压阈值 (×100mV, 0=禁用)
  uint8_t recovery_voltage_threshold;  // 恢复电压 (×100mV, 0=禁用)
  uint8_t watchdog_count;              // 看门狗允许丢失次数 (0=禁用)
  uint8_t default_on_delay;            // 上电自动开机延迟 (s, 255=禁用)
  uint8_t power_cut_delay;             // 关机断电延迟 (s)
};
```

### 5.4 RTOS 任务分配

| 任务 | 频率 | 功能变化 |
|------|------|----------|
| hwMonitorTask | 10Hz (电池1Hz) | 调用 `g_battery->update()` (WP5 I2C) |
| **shutdownTask (新增)** | **3Hz** | **轮询 WP5 状态 → HTTP 通知 SBC → 确认断电** |
| encoderTask | 500Hz | 无变化 |
| imuTask | 100Hz | 无变化 |
| motorControlTask | 200Hz | 无变化 |

### 5.5 初始化流程

```
setup()
  │
  ├── boardPheripheralsInit()
  │     ├── WP5 I2C 总线初始化 (PB8/PB9, 100kHz)
  │     ├── 关机检测引脚配置 (PD4 INPUT_PULLUP)
  │     └── 关机确认引脚配置 (PD7 OUTPUT LOW)
  │
  ├── g_comm_mgr.init()
  │
  ├── 组件初始化
  │     ├── wp5_power_board.init()    ← 读取板卡信息 + 配置WP5
  │     ├── g_encoders.init()
  │     ├── g_motors.init()
  │     └── ...
  │
  └── RTOS
        ├── createQueues()
        ├── createTasks()            ← 包含 shutdownTask
        └── vTaskStartScheduler()
```

---

## 6. 引脚分配

### 6.1 新增引脚 (相比V1)

| STM32引脚 | 功能 | 方向 | 连接WP5端 |
|-----------|------|------|-----------|
| PB8 | I2C1_SCL | OD输出 | GPIO7 (I2C Slave SCL) |
| PB9 | I2C1_SDA | OD输出 | GPIO6 (I2C Slave SDA) |
| PD4 | PB_SHD_DETECT | INPUT_PULLUP | GPIO16 (可选) |
| PD7 | PB_SHD_CONFIRM | OUTPUT | - (预留) |

### 6.2 保留但更改用途的引脚

| STM32引脚 | V1用途 | V2用途 |
|-----------|--------|--------|
| PD5 | PB_SERIAL_TX (预留) | 可释放或保留 |
| PD6 | PB_SERIAL_RX (预留) | 可释放或保留 |
| PA5 | Battery ADC | **移除** (WP5接管电池监测) |

### 6.3 WP5 端连接

| WP5引脚 | 功能 | 连接 |
|---------|------|------|
| GPIO6 | I2C Slave SDA | → STM32 PB9 (需4.7kΩ上拉至3.3V) |
| GPIO7 | I2C Slave SCL | → STM32 PB8 (需4.7kΩ上拉至3.3V) |
| GPIO11 | Pi 3.3V检测 | → STM32的3.3V输出 |
| GPIO12 | DCDC使能 | WP5上板内连接 |
| GPIO13 | Pi电源控制 | WP5上板内连接 |
| GPIO15 | 按钮 | WP5上板按钮 |
| GPIO22 | LED | WP5上板LED |
| GPIO16 | 关机通知 (可选) | → STM32 PD4 |
| ADC0 (GPIO26) | VUSB检测 | WP5上板分压网络 |
| ADC1 (GPIO27) | VIN检测 | WP5上板分压网络 (连接电池VIN) |
| ADC2 (GPIO28) | VOUT检测 | WP5上板分压网络 |
| ADC3 (GPIO29) | IOUT检测 | WP5上板电流传感器 |

---

## 7. WP5 首次配置

STM32 在 `PowerBoardWP5::init()` 中通过 I2C 写入以下配置:

```cpp
// 电源优先级: VIN优先 (电池供电为主)
writeConfigReg(0x18, 1);  // PS_PRIORITY = VIN

// 上电自动开机: 3秒延迟
writeConfigReg(0x11, 3);  // DEFAULT_ON_DELAY = 3s

// 关机断电延迟: 20秒 (给STM32足够时间通知SBC)
writeConfigReg(0x12, 20); // POWER_CUT_DELAY = 20s

// 低压保护: 9.6V (3S LiPo 空电压)
writeConfigReg(0x16, 96); // LOW_VOLTAGE = 96 (×100mV = 9.6V)

// 恢复电压: 10.5V (充电恢复开机)
writeConfigReg(0x17, 105); // RECOVERY_VOLTAGE = 105 (×100mV = 10.5V)

// 看门狗: 允许丢失3次心跳
writeConfigReg(0x1D, 3);  // WATCHDOG = 3
```

---

## 8. WP5 固件适配建议

### 8.1 核心兼容性

Witty Pi 5 固件**无需修改**即可作为 I2C 从设备为 STM32 提供服务。原始固件的 I2C 寄存器接口是通用的，不依赖于主控是 Raspberry Pi 还是 STM32。

### 8.2 推荐优化 (可选)

| 修改项 | 文件 | 说明 |
|--------|------|------|
| 新增关机通知GPIO | power.c | 在 `request_shutdown()` 中拉高 GPIO16 |
| 调整3V3检测 | power.c | 确认 `GPIO_PI_HAS_3V3` 能正确检测STM32的3.3V |
| 禁用USB MSC | main.c | 生产环境可禁用USB优盘功能减少攻击面 |
| CRC8校验 | i2c.c | 已有CRC8计算函数，可启用通信校验 |

### 8.3 GPIO11 (Pi 3.3V检测) 注意事项

WP5使用 `GPIO_PI_HAS_3V3` (GPIO 11) 来检测树莓Pi是否已上电。在ROSbot PRO中:
- 将STM32板的3.3V输出连接到WP5的GPIO 11
- 当STM32上电后，WP5可正确检测到 "Pi已启动"
- 注意: RP2350-E9勘误导致此引脚默认禁用输入 (`gpio_set_input_enabled(GPIO_PI_HAS_3V3, false)`)

---

## 9. 编译配置

### 9.1 编译开关

在 `platformio.ini` 中通过 `-D USE_WITTYPI5_POWER` 启用 WP5 电源板:

```ini
[env:rosbot_pro]
build_flags =
    ...
    -D USE_WITTYPI5_POWER     ; 启用 Witty Pi 5 电源板
```

不定义此宏时，自动回退到 BatteryAdc (V1 方案)。

### 9.2 条件编译

```cpp
// main.cpp
#ifdef USE_WITTYPI5_POWER
  static PowerBoardWP5 wp5_power(wp5_config);
  BatteryInterface* g_battery = &wp5_power;
#else
  static BatteryAdc battery_adc(battery_adc_config);
  BatteryInterface* g_battery = &battery_adc;
#endif
```

---

## 10. 测试验证计划

### 10.1 编译验证

- [x] 代码编译通过 (rosbot_pro target)
- [ ] 无编译警告 ([-Wextra])

### 10.2 无硬件测试 (仿真)

| 测试项 | 验证方法 | 预期结果 |
|--------|----------|----------|
| I2C超时处理 | 无WP5板运行 | 电池数据为NAN，系统正常 |
| 心跳停止 | 无WP5板运行 | writeReg返回false，不影响其他功能 |
| shutdownTask | 无WP5板运行 | 轮询I2C失败，不触发关机 |

### 10.3 有硬件测试

| 测试项 | 步骤 | 预期结果 |
|--------|------|----------|
| 板卡识别 | 上电后检查调试串口 | 输出 "WP5 FW: 1.2" |
| 电压读取 | 接12V电池 | VIN ≈ 12000mV |
| 心跳机制 | 监控WP5 LED | 正常呼吸LED |
| 按钮关机 | 按WP5按钮 | SBC收到HTTP /shutdown |
| 低压保护 | 降低电池电压 | 自动触发关机流程 |
| 看门狗重启 | 暂停STM32心跳 | WP5重启电源 |
| RTC定时 | 设置定时开机 | 到时间自动开机 |

---

## 11. 硬件BOM增量

| 器件 | 数量 | 规格 | 说明 |
|------|------|------|------|
| Witty Pi 5 HAT+ | 1 | — | 电源管理模块 |
| 4.7kΩ 电阻 | 2 | 0402 | I2C1 上拉电阻 |
| 排针/排座 | 1 | 2×5 2.54mm | WP5连接器 |
| 4pin连接器 | 1 | JST-PH 2.0mm | I2C + 电源 |

---

## 12. 文件清单

| 文件 | 类型 | 说明 |
|------|------|------|
| `lib/power_board/power_board_wp5.hpp` | 新增 | WP5驱动头文件 |
| `lib/power_board/power_board_wp5.cpp` | 新增 | WP5驱动实现 |
| `include/rosbot_pro/config.hpp` | 修改 | 新增WP5配置 |
| `src/rosbot_pro/main.cpp` | 修改 | 使用WP5替代BatteryAdc |
| `src/rosbot_pro/rtos.cpp` | 修改 | 新增shutdownTask |
| `platformio.ini` | 修改 | 添加编译开关 |
| `docs/WittyPi5电源板整合设计文档.md` | 新增 | 本文档 |

---

**文档版本**: 1.0
**最后更新**: 2026年4月9日
