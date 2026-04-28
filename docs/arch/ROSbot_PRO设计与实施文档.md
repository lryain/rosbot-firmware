# ROSbot PRO 设计与实施文档

> ROSbot PRO 是基于 Husarion ROSbot XL 设计的自研 ROS 开发板，采用 STM32F407ZET6 MCU（LQFP-144）、DRV8874 电机驱动、ADC 电池监测、4 路 IPROPI 电流检测和以太网通信。

---

## 1. 设计概述

### 1.1 基本信息

| 参数 | ROSbot XL (参考) | ROSbot PRO |
|------|------------------|------------|
| MCU | STM32F407LGT6 (LQFP-144) | **STM32F407ZET6 (LQFP-144)** |
| 电机驱动 | MAX22205 × 4 | **DRV8874 × 4** |
| 电池监测 | PowerBoard UART | **BatteryAdc (ADC 直测)** |
| IMU | BNO055 | BNO055 |
| 通信 | Ethernet + UART | Ethernet + UART |
| LED Strip | SPI (18 LEDs) | SPI3 (18 LEDs) |
| 传感器 | 无测距 | 无测距 |
| 风扇 | PWM (TIM8) + NTC | PWM (TIM8) + NTC |
| 电源板 | 有（UART 通信） | **无（V1），预留接口（V2）** |

### 1.2 与 ROSbot XL 的差异汇总

| 变更项 | 说明 | 影响范围 |
|--------|------|---------|
| 电机驱动更换 | MAX22205 → DRV8874 (IN/IN 模式) | 新驱动类 + 引脚重映射 |
| 电池监测方式 | PowerBoard → BatteryAdc | config + main.cpp |
| 电源板通信切换 | UART → I2C | 仅在 `USE_WITTYPI5_POWER` 下切换通信方式 |
| 保留关机协调 | PD4/PD7 保留 | `shutdownTask` 与 ROSbot XL 保持一致 |
| ILIM 引脚释放 | PE10/PG15/PG7/PD14 释放 | DRV8874 不需要 ILIM |
| 增加电池 ADC | 占用 1 路 ADC 引脚 | 新增分压电路 |
| 增加 IPROPI ADC | 4 路 ADC3 专用引脚 (PF3/4/5/10)，V1 已实现 | 电流监测 + ROS 话题 |
| PC13 解耦 | 风扇和 nSLEEP 分离 | 更清晰的硬件设计 |
| RR 电机迁移 | TIM8 → TIM12，PB14/PB15 独占 | 解除与风扇的频率耦合 |
| LED Strip 迁移 | SPI2 → SPI3，PC10/PC11/PC12 | 解除与 RR 电机的引脚冲突 |

---

## 2. ROSbot PRO 引脚分配

### 2.1 编码器 — 与 ROSbot XL 完全一致

| 功能 | 引脚 | 定时器 | 说明 |
|------|------|--------|------|
| ENC_FL_A/B | PD12/PD13 | TIM4 | 前左 |
| ENC_FR_A/B | PC6/PC7 | TIM3 | 前右 |
| ENC_RL_A/B | PA15/PB3 | TIM2 | 后左 |
| ENC_RR_A/B | PE9/PE11 | TIM1 | 后右 |

### 2.2 电机驱动 — DRV8874 IN/IN 模式 (最终方案)

| 电机 | IN1 (PWM) | IN2 (PWM) | 定时器 | 方向反转 |
|------|-----------|-----------|--------|---------|
| FL | PF9 | PF6 | TIM14_CH1 + TIM10_CH1 | ✅ |
| FR | PF8 | PF7 | TIM13_CH1 + TIM11_CH1 | ❌ |
| RL | PE5 | PE6 | TIM9_CH1 + TIM9_CH2 | ✅ |
| RR | PB14 | PB15 | TIM12_CH1 + TIM12_CH2 | ❌ |

> **最终方案**：RR 电机独占 TIM12，PB14/PB15 不再与风扇共享 TIM8。

### 2.3 电机驱动控制信号

| 功能 | 引脚 | 模式 | 说明 |
|------|------|------|------|
| RIGHT_nSLEEP | PG9 | OUTPUT | 右侧驱动使能（新分配，释放 PC13） |
| RIGHT_nFAULT | PE0 | INPUT_PULLUP | 右侧故障检测 |
| LEFT_nSLEEP | PG10 | OUTPUT | 左侧驱动使能（新分配，释放 PC14） |
| LEFT_nFAULT | PE1 | INPUT_PULLUP | 左侧故障检测 |

> PC13/PC14 释放给其他功能（如 32.768kHz 晶振 OSC32 或 RTC 使用）。

### 2.4 电池 ADC

| 功能 | 引脚 | ADC 通道 | 说明 |
|------|------|---------|------|
| BATT_ADC | PA5 | ADC1_CH5 / ADC2_CH5 | 电池电压分压后读取 |

分压器：R1=56kΩ, R2=10kΩ → 分压比 6.6:1

### 2.5 IPROPI 电流监测（V1 已实现）

| 功能 | 引脚 | ADC 通道 | 说明 |
|------|------|---------|------|
| IPROPI_FL | PF3 | ADC3_CH9 | FL 电机电流 |
| IPROPI_FR | PF4 | ADC3_CH14 | FR 电机电流 |
| IPROPI_RL | PF5 | ADC3_CH15 | RL 电机电流 |
| IPROPI_RR | PF10 | ADC3_CH8 | RR 电机电流 |

**硬件电路**：每路 IPROPI 引脚接 2kΩ 电阻至 GND，DRV8874 KIPROPI ≈ 2000。
换算公式：`I_MOTOR = ADC_count × (3.3V / 1023) / (2000Ω × 2000)`

**固件实现**：
- `MotorDrv8874::getData()` 自动调用 `analogRead(ipropi_pin)` 并乘以 `ipropi_scale`
- `MotorArray::update()` 收集各电机 `current` 字段到 `MotorsData`
- `motorControlTask` 每 200Hz 将电流写入 `motor_current_queue`
- ROS 话题 `_motors/current` 发布 `std_msgs/Float32MultiArray`（顺序：FL/FR/RL/RR）

### 2.6 IMU — 与 ROSbot XL 完全一致

| 功能 | 引脚 | 说明 |
|------|------|------|
| I2C_SDA | PF0 | 共享 I2C 总线 |
| I2C_SCL | PF1 | 共享 I2C 总线 |
| IMU_INT | PF2 | 中断 |

### 2.7 LED / LED Strip / 风扇

| 功能 | 引脚 | 说明 |
|------|------|------|
| LED Strip | PC12/PC11/PC10 (SPI3) | 18 颗 APA102，独立于电机 PWM |
| 风扇 | PB0 (TIM8_CH2N) | 25kHz，TIM8 独占 |

风扇和 LED Strip 都保留，但已经从电机资源中解耦。

### 2.8 通信

| 功能 | 引脚 | 说明 |
|------|------|------|
| FTDI_TX | PA9 (USART1) | 诊断串口 |
| FTDI_RX | PA10 (USART1) | 诊断串口 |
| Ethernet | 标准 RMII 引脚组 | 保持 LAN9303 方案 |

### 2.9 预留电源板接口

| 功能 | 引脚 | 说明 |
|------|------|------|
| PB_TX | PD5 (USART2) | 预留给 V2 电源板 |
| PB_RX | PD6 (USART2) | 预留给 V2 电源板 |
| PB_SHD_DETECT | PD4 | 关机检测，默认保留 |
| PB_SHD_CONFIRM | PD7 | 关机确认，默认保留 |

### 2.10 扩展接口 EXT1 (兼容 ROSbot XL)

| 引脚序号 | 功能 | 引脚 | 说明 |
|---------|------|------|------|
| 1 | EXT_GPIO1 | PG0 | 通用 GPIO |
| 2 | EXT_GPIO2 | PG1 | 通用 GPIO |
| 3 | EXT_PWM1 | PG2 | PWM 输出 |
| 4 | EXT_PWM2 | PG3 | PWM 输出 |
| 5 | USART6_TX | PC6 | ⚠️ 与编码器冲突，建议换 |
| 6 | USART6_RX | PC7 | ⚠️ 与编码器冲突，建议换 |
| 7 | I2C1_SCL | PB6 | 扩展 I2C |
| 8 | I2C1_SDA | PB7 | 扩展 I2C |
| 9 | SPI1_MOSI | PA7 | ⚠️ 与 ETH 冲突 |
| 10 | SPI1_SCK | PA5 | ⚠️ 与电池 ADC 冲突 |
| 11 | SPI1_MISO | PA6 | 可用 |
| 12 | +3V3 | — | 电源 |
| 13 | GND | — | 地 |
| 14 | +5V | — | 电源 |

> 建议重新分配 EXT1 避免冲突。

---

## 3. 定时器分配总览（ROSbot PRO）

| 定时器 | 功能 | 引脚 | 备注 |
|--------|------|------|------|
| TIM1 | 编码器 RR | PE9/PE11 | |
| TIM2 | 编码器 RL | PA15/PB3 | |
| TIM3 | 编码器 FR | PC6/PC7 | |
| TIM4 | 编码器 FL | PD12/PD13 | |
| TIM5 | FreeRTOS 运行时 | 无 | |
| TIM8 | 风扇(CH2N) | PB0 | |
| TIM9 | RL 电机 IN1/IN2 | PE5/PE6 | |
| TIM10 | FL 电机 IN2 | PF6 | |
| TIM11 | FR 电机 IN2 | PF7 | |
| TIM13 | FR 电机 IN1 | PF8 | |
| TIM14 | FL 电机 IN1 | PF9 | |
| TIM6 | **空闲** | — | |
| TIM7 | **空闲** | — | |
| TIM12 | RR 电机 | PB14/PB15 | 独立于风扇 |

---

## 4. 软件架构

### 4.1 文件结构

```
include/
  config.hpp                 # 修改：添加 ROSBOT_PRO 分支
  rosbot_pro/
    config.hpp               # 新建：ROSbot PRO 配置
lib/
  motor/
    motor_drv8874.hpp        # 新建：DRV8874 驱动头文件
    motor_drv8874.cpp        # 新建：DRV8874 驱动实现
src/
  rosbot_pro/
    main.cpp                 # 新建：入口和初始化
    rtos.cpp                 # 新建：RTOS 任务
    ros.cpp                  # 新建：ROS 配置
platformio.ini               # 修改：添加 [env:rosbot_pro]
```

### 4.2 条件编译

```cpp
// include/config.hpp
#if defined(ROSBOT)
#include "rosbot/config.hpp"
#elif defined(ROSBOT_XL)
#include "rosbot_xl/config.hpp"
#elif defined(ROSBOT_PRO)
#include "rosbot_pro/config.hpp"
#else
#error "No board version defined!"
#endif
```

### 4.3 主要差异代码

**main.cpp 变更**：
- 电源板通信默认保留原逻辑；`USE_WITTYPI5_POWER` 仅切换为 I2C
- 使用 `MotorDrv8874` 替代 `MotorDrv8848`
- 保留电源板关机引脚 `PB_SHD_DETECT/PB_SHD_CONFIRM`
- 保留 Ethernet 初始化

**rtos.cpp 变更**：
- 保留 `shutdownTask`，与 ROSbot XL 的关机时序一致
- `hwMonitorTask` 使用 `BatteryAdc::update()` 替代 `PowerBoard::request/update`

### 4.4 PID 参数

初始继承 ROSbot XL 参数，DRV8874 可能需要微调：

```cpp
inline constexpr PIDConfig pid_config = {
    .kp = 0.15f,
    .ki = 0.55f,
    .kd = 0.007f,
    .min_output = -1.0f,
    .max_output = 1.0f,
};
```

---

## 5. PCB 设计要点

### 5.1 DRV8874 布局

- 每个 DRV8874 芯片下方需要大面积铜皮（散热焊盘）
- VM 引脚旁放 0.1µF + 10µF 去耦电容
- 芯片间距建议 > 5mm 以减少热耦合
- 电机走线采用粗线（≥1mm）承载大电流

### 5.2 模拟信号布局

- 电池 ADC 分压器靠近 MCU ADC 引脚
- IPROPI 引线尽量短，远离数字信号
- ADC VREF 引脚加 100nF 去耦电容

### 5.3 Ethernet 布局

- RMII 信号走差分线，阻抗 50Ω
- 变压器靠近 LAN9303 / PHY
- 数字地和模拟地在 PHY 附近单点连接

### 5.4 电源设计

- 电池输入增加反向保护（P-MOS 或肖特基）
- 3.3V LDO 近 MCU 放置，多点去耦
- 5V 供电使用同步降压，效率 > 90%

---

## 6. 实施路线图

| 阶段 | 任务 | 依赖 |
|------|------|------|
| 1 | 创建 DRV8874 驱动代码 | 无 |
| 2 | 创建 rosbot_pro 配置和代码 | 阶段1 |
| 3 | 添加 platformio.ini 环境 | 阶段2 |
| 4 | 编译验证 | 阶段3 |
| 5 | PCB 原理图绘制 | 引脚分配文档 |
| 6 | PCB Layout | 阶段5 |
| 7 | 打样测试 | 阶段4 + 阶段6 |
| 8 | PID 调参 | 阶段7 |
| 9 | 电源板设计（V2） | 阶段7 验证通过后 |
