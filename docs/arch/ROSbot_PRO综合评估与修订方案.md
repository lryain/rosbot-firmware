# ROSbot PRO 综合评估与修订方案

> **版本**: V3.1  
> **日期**: 2026-04-13  
> **适用MCU**: STM32F407**Z**ET6 (LQFP-144, 512KB Flash, 192KB SRAM)  
> **引脚验证依据**: `PeripheralPins.c` — `framework-arduinoststm32/variants/STM32F4xx/F407Z(E-G)T_F417Z(E-G)T/`

---

## 目录

1. [通信方案分析：I2C vs UART](#1-通信方案分析i2c-vs-uart)
2. [Witty Pi 5 固件改动评估](#2-witty-pi-5-固件改动评估)
3. [Witty Pi 5 额外功能与使用方法](#3-witty-pi-5-额外功能与使用方法)
4. [DRV8874 功能增强分析](#4-drv8874-功能增强分析)
5. [DRV8874 ROS2 集成方案](#5-drv8874-ros2-集成方案)
6. [综合建议](#6-综合建议)
7. [STM32F407ZET6 引脚分配](#7-stm32f407zet6-引脚分配)
8. [ROSbot XL 兼容性：Extra GPIO 扩展连接器](#8-rosbot-xl-兼容性extra-gpio-扩展连接器)

---

## 1. 通信方案分析：I2C vs UART

### 1.1 对比总结

| 特性 | I2C（推荐✅） | UART（备选） |
|------|:---:|:---:|
| WP5 原生支持 | ✅ 内置 I2C Slave (0x51) | ❌ 仅有调试串口，需重写固件 |
| 引脚数量 | 2 (SCL + SDA) | 2 (TX + RX) |
| 多设备共享 | ✅ 地址寻址，可共享总线 | ❌ 点对点，每设备需独立串口 |
| 协议复杂度 | 低（寄存器读写） | 中（需自定义帧格式+校验） |
| 数据方向 | 双向（主机发起） | 双向（异步） |
| 实时性 | 良好（轮询模式，可配合 GPIO 中断） | 更好（中断驱动，适合事件通知） |
| 吞吐量 | 100-400 kbps | 最高 921600 bps |
| 可靠性 | ACK/NACK 硬件确认 | 需软件校验和 |
| CPU 开销 | 低（硬件 I2C 外设） | 中（中断 + DMA） |
| 总线仲裁 | 硬件支持 | 无 |

### 1.2 结论

**推荐使用 I2C 通信**，理由如下：

1. **WP5 原生支持**：RP2350 固件已实现 I2C Slave 模式（地址 0x51），寄存器映射完整（0x00-0x67），无需任何固件改动
2. **寄存器式访问**：读电压只需 `I2C_Read(0x51, REG_VIN_MV_MSB, 2)`，协议简洁无歧义
3. **总线独立分离**：ZET6 144引脚方案中 I2C2(PF0/PF1) 给 IMU，I2C1(PB8/PB9) 给 WP5，两条总线完全独立，无需 FreeRTOS mutex
4. **已验证的可靠性**：Witty Pi 5 在树莓派生态中已大量验证 I2C 通信稳定性

**UART 的唯一优势**是更高的异步事件吞吐（如实时电流波形流式传输），但电源板通信场景不需要高带宽。

---

## 2. Witty Pi 5 固件改动评估

### 2.1 基本集成：无需改动 ✅

| 功能 | WP5 固件状态 | 改动需求 |
|------|------------|---------|
| I2C Slave (0x51) | 已实现 | 无 |
| 电压监测 (VUSB/VIN/VOUT) | 已实现 | 无 |
| 电流监测 (IOUT) | 已实现 | 无 |
| 心跳看门狗 | 已实现 (REG 0x46) | 无 |
| 低压保护 | 已实现 (REG 0x16) | 无 |
| 恢复电压 | 已实现 (REG 0x17) | 无 |
| RTC 虚拟寄存器 | 已实现 (0x50-0x5F) | 无 |
| 温度虚拟寄存器 | 已实现 (0x60-0x61) | 无 |
| 电源优先级 | 已实现 (REG 0x18) | 无 |
| 开机延迟/断电延迟 | 已实现 (REG 0x11/0x12) | 无 |

### 2.2 可选优化（生产级别）

| 优化项 | 说明 | 改动量 |
|--------|------|--------|
| GPIO16 中断输出 | 将 GPIO16 从输入改为输出，主动通知 STM32 关机事件 | 小（改一行方向寄存器） |
| USB MSC 禁用 | 生产环境禁用 U 盘功能，节省 RP2350 资源 | 小（编译开关） |
| RP2350-E9 勘误规避 | GPIO11 默认输入已禁用（3.3V 检测受影响） | 已规避，无需改动 |
| 自定义关机确认 | 增加 STM32→WP5 确认寄存器，替代 GPIO 检测 | 中（新增 I2C 寄存器） |

### 2.3 结论

**基本集成无需修改 WP5 固件**。所有核心功能（电压/电流监测、心跳看门狗、低压保护、RTC、温度）均通过标准 I2C 寄存器访问实现。生产阶段可考虑 GPIO16 中断输出优化。

---

## 3. Witty Pi 5 额外功能与使用方法

### 3.1 超越原电源板的功能

| 功能 | 原 PowerBoard (UART) | WP5 | 使用方法 |
|------|:---:|:---:|----------|
| 电压监测 | ✅ VIN + VOUT | ✅ VUSB + VIN + VOUT | 读 REG 0x03-0x08 |
| 电流监测 | ✅ IOUT | ✅ IOUT | 读 REG 0x09-0x0A |
| **RTC 定时开关机** | ❌ | ✅ | 写 RX8025 虚拟寄存器 0x50-0x5F |
| **温度监测** | ❌ | ✅ TMP112 (±0.5°C) | 读 REG 0x60-0x61 |
| **低压自动保护** | ❌ 需主控实现 | ✅ 硬件级 | 写 REG 0x16 (阈值 ×100mV) |
| **恢复电压自启** | ❌ | ✅ | 写 REG 0x17 |
| **看门狗重启** | ❌ | ✅ 心跳超时自动断电重启 | 写 REG 0x1D (次数), REG 0x46 (心跳) |
| **双电源管理** | ❌ | ✅ USB + VIN 自动切换 | 写 REG 0x18 (优先级) |
| **按键控制** | ❌ | ✅ 按键触发开关机 | 硬件按键，无需代码 |
| **温度触发动作** | ❌ | ✅ 过热自动关机 | WP5 配置脚本 |

### 3.2 RTC 定时开关机使用示例

```
场景：每天 08:00 开机，22:00 关机（节能模式）

写 RX8025 闹钟寄存器：
  REG 0x57 (Alarm Min)  = 0x00
  REG 0x58 (Alarm Hour) = 0x08
  REG 0x59 (Alarm Day)  = 0x80 (每天)

通过 STM32 I2C 发送：
  i2c.write(0x51, 0x57, 0x00);  // 分钟
  i2c.write(0x51, 0x58, 0x08);  // 小时
  i2c.write(0x51, 0x59, 0x80);  // 每天重复
```

### 3.3 看门狗使用方法

```cpp
// 配置：3 次心跳超时后自动重启
wp5_power.writeReg(REG_CONF_WATCHDOG, 3);

// 固件主循环中定期发送心跳（30 秒间隔）
// PowerBoardWP5::update() 已自动处理
```

---

## 4. DRV8874 功能增强分析

### 4.1 DRV8874 vs DRV8848 对比

| 特性 | DRV8848 (ROSbot) | DRV8874 (ROSbot PRO) | 提升 |
|------|:---:|:---:|------|
| 通道数 | 双通道 (2×H桥) | 单通道 (1×H桥) | 每电机独立驱动，隔离性更好 |
| 峰值电流 | 2A/通道 | 6A | **3× 提升** |
| 持续电流 | 1A/通道 | 3.5A | **3.5× 提升** |
| 工作电压 | 4-18V | 4.5-37V | **2× 提升**，支持更高电压电机 |
| 控制模式 | nSLEEP+IN_A/IN_B | IN/IN 或 PH/EN | 更灵活 |
| **电流检测 (IPROPI)** | ❌ 无 | ✅ 模拟电流输出 | **核心新功能** |
| PWM 输入 | 1 PWM + 2 GPIO/电机 | 2 PWM/电机 | 更精确的制动控制 |
| 诊断输出 | nFAULT | nFAULT + IPROPI | 故障+电流双重诊断 |
| 再生制动 | 基本 | 完整（IN/IN 模式） | 更好的能量回收 |
| 封装 | TSSOP-16 (双通道) | HTSSOP-16 (单通道) | 散热更优 |

### 4.2 IPROPI 电流检测

DRV8874 最重要的新特性是 **IPROPI 引脚**（比例电流输出）：

```
I_IPROPI = I_LOAD / 1200

配合外部电阻 R_IPROPI = 1kΩ：
V_IPROPI = I_LOAD × (1/1200) × 1000 = I_LOAD × 0.833V/A

示例：
  - 1A 负载 → 0.833V
  - 3A 负载 → 2.5V  (ADC 满量程约 3.96A)
  - 堵转检测：V > 2.7V → 电流 > 3.24A → 触发保护
```

> **注意**：在 LQFP-100 封装上，可用 ADC 通道有限。当前仅 PC0 (ADC1_CH10) 空闲可用于 IPROPI。如需 4 个电机全部电流检测，需外部模拟多路选择器 (如 CD74HC4052) 或在 PCB 预留但暂不焊接。

### 4.3 DRV8874 IN/IN 模式控制真值表

| IN1 | IN2 | 输出状态 | 说明 |
|-----|-----|---------|------|
| PWM | LOW | 正转 | IN1 占空比控制速度 |
| LOW | PWM | 反转 | IN2 占空比控制速度 |
| LOW | LOW | 制动（低侧） | 两个低侧 FET 导通 |
| HIGH | HIGH | 制动（高侧） | 两个高侧 FET 导通 |
| Hi-Z | Hi-Z | 滑行 | nSLEEP=LOW 时 |

---

## 5. DRV8874 ROS2 集成方案

### 5.1 当前 ROS2 话题结构

```
已实现：
  /_motors/feedback  (JointState)   ← 编码器 + PID 输出
  /_motors/cmd       (Float32MultiArray) ← 速度指令
  /battery           (BatteryState) ← 电池状态

待新增（DRV8874 增强）：
  /_motors/current   (Float32MultiArray) ← IPROPI 电流反馈
  /_motors/status    (DiagnosticArray)   ← 故障 + 温度 + 电流状态
```

### 5.2 IPROPI 电流反馈实现方案

```cpp
// 新增 Publisher: MotorCurrentPublisher
// 发布话题: _motors/current
// 消息类型: std_msgs/Float32MultiArray
// 数据: [fl_current, fr_current, rl_current, rr_current] (Amps)

// 在 MotorDrv8874Config 中新增:
struct MotorDrv8874Config {
    // ... 现有字段 ...
    uint8_t ipropi_pin;      // IPROPI ADC 引脚 (0xFF = 未连接)
    float ipropi_scale;      // ADC 到安培的换算系数
};

// 在 MotorDrv8874::getData() 中返回电流:
MotorData MotorDrv8874::getData() const {
    MotorData d;
    // ... 现有代码 ...
    if (cfg_.ipropi_pin != 0xFF) {
        uint16_t raw = analogRead(cfg_.ipropi_pin);
        d.current = raw * cfg_.ipropi_scale;
    }
    return d;
}
```

### 5.3 ROS2 参数配置

```yaml
# ros2 param set /rosbot_pro_mcu motor_max_current 3.0
# ros2 param set /rosbot_pro_mcu stall_current_threshold 2.5
# ros2 param set /rosbot_pro_mcu stall_timeout_ms 500
```

### 5.4 堵转检测与保护

```cpp
// motorControlTask 中添加:
if (motor.getCurrent() > STALL_THRESHOLD && duration > STALL_TIMEOUT) {
    motor.brake();
    publishDiagnostic("STALL_DETECTED", motor.name());
}
```

> **实施状态**：IPROPI 功能已在 V1 版本实现。使用 STM32F407ZET6 的 ADC3 专属引脚 (PF3/PF4/PF5/PF10)，与其他外设无冲突。固件中 `ipropi_pin != 0xFF` 时自动读取 ADC 并换算为安培值，ROS 话题 `_motors/current` 发布 `Float32MultiArray` 格式的 4 路电流数据。

---

## 6. 综合建议

### 6.1 关键建议清单

| 优先级 | 建议 | 理由 |
|--------|------|------|
| 🔴 关键 | **STM32F407ZET6 (144引脚)** | LQFP-144 封装，完整 Port A-G，所有设计引脚均可用 |
| � 已实现 | **TIM8 共享风扇/RR** | RR 使用 TIM8_CH1/2 (PC6/PC7)，释放 PC9 给 I2C3_SDA；SPI2 迁至标准引脚 PB12/13/14/15（ETH TX 迁移至 PG11/13/14 后释放），PC2/PC3 回归纯 ADC，与 EXT2 ADC 不再冲突 |
| 🟢 已实现 | **IPROPI 全部 4 路电流监测** | PF3/PF4/PF5/PF10 (ADC3 专属)，无冲突，V1 正式启用 |
| 🟢 建议 | **ESD 防护** | I2C 总线、电机驱动使用 TVS 二极管 |
| 🟢 建议 | **电源去耦** | 每个 DRV8874 使用 0.1µF + 10µF 陶瓷电容 |
| 🟢 建议 | **看门狗分层** | STM32 IWDG + WP5 心跳看门狗双重保护 |

### 6.2 电源时序与启动建议

```
上电时序：
  1. WP5 上电 → RP2350 启动 (< 100ms)
  2. WP5 使能 VOUT → STM32 + 外设上电
  3. STM32 启动 → boardPheripheralsInit()
  4. STM32 → WP5 I2C 初始化 + 发送首次心跳
  5. STM32 → IMU/EEPROM 初始化
  6. FreeRTOS 启动 → 定期心跳任务运行

关机时序：
  1. WP5 检测按键/低压/定时 → 设置 RPI_STATE = STOPPING
  2. STM32 shutdownTask 检测到 STOPPING 状态
  3. STM32 → HTTP GET /shutdown 通知 SBC
  4. 等待 5 秒 → STM32 暂停任务
  5. WP5 在 POWER_CUT_DELAY 后切断 VOUT
```

---

## 7. STM32F407ZET6 引脚分配

### 7.1 MCU 确认

**目标芯片：STM32F407ZET6（LQFP-144，144引脚）**

- 包含完整 Port A-G，所有之前 ROSbot XL 使用的 PF/PG 引脚均可用
- Flash: 512KB（固件当前 ~220KB，充裕）
- SRAM: 192KB（当前使用 50.7%，充裕）
- 验证来源：`framework-arduinoststm32/variants/STM32F4xx/F407Z(E-G)T_F417Z(E-G)T/PeripheralPins.c`

### 7.2 所有设计引脚的 AF 验证结果

| 引脚 | 功能 | AF 验证 | 来源 |
|------|------|:---:|------|
| **PF0** | I2C2_SDA | ✅ AF4 | `{PF_0, I2C2, GPIO_AF4_I2C2}` |
| **PF1** | I2C2_SCL | ✅ AF4 | `{PF_1, I2C2, GPIO_AF4_I2C2}` |
| **PF2** | IMU INT (EXTI) | ✅ GPIO | PF2 = GPIO，无重要AF冲突 |
| **PF6** | Motor FL IN2 (TIM10_CH1) | ✅ AF3 | `{PF_6, TIM10, GPIO_AF3}` |
| **PF7** | Motor FR IN2 (TIM11_CH1) | ✅ AF3 | `{PF_7, TIM11, GPIO_AF3}` |
| **PF8** | Motor FR IN1 (TIM13_CH1) | ✅ AF9 | `{PF_8, TIM13, GPIO_AF9}` |
| **PF9** | Motor FL IN1 (TIM14_CH1) | ✅ AF9 | `{PF_9, TIM14, GPIO_AF9}` |
| **PF11** | PUSH_BUTTON1 (GPIO) | ✅ GPIO | 无重要AP冲突 |
| **PF12** | PUSH_BUTTON2 (GPIO) | ✅ GPIO | 无重要AP冲突 |
| **PF13** | EN_LOC_5V (GPIO) | ✅ GPIO | 无重要AF冲突 |
| **PG9** | Right nSLEEP (GPIO OUT) | ✅ GPIO | USART6_RX(AF8)已知，但用作GPIO输出无冲突 |
| **PG10** | Left nSLEEP (GPIO OUT) | ✅ GPIO | 纯GPIO，无AF冲突 |
| **PB8** | I2C1_SCL (WP5) | ✅ AF4 | `{PB_8, I2C1, GPIO_AF4_I2C1}` |
| **PB9** | I2C1_SDA (WP5) | ✅ AF4 | `{PB_9, I2C1, GPIO_AF4_I2C1}` |
| **PE5** | Motor RL IN1 (TIM9_CH1) | ✅ AF3 | `{PE_5, TIM9, GPIO_AF3}` |
| **PE6** | Motor RL IN2 (TIM9_CH2) | ✅ AF3 | `{PE_6, TIM9, GPIO_AF3}` |
| **PC6** | Motor RR IN1 (TIM8_CH1) | ✅ AF3 | `{PC_6_ALT1, TIM8, GPIO_AF3_TIM8}` |
| **PC7** | Motor RR IN2 (TIM8_CH2) | ✅ AF3 | `{PC_7_ALT1, TIM8, GPIO_AF3_TIM8}` |
| **PC8** | EXT_PWM1 (TIM8_CH3)，Motor RR 迁移后空闲 | ✅ AF3 | `{PC_8, TIM8, GPIO_AF3}` |
| **PC9** | I2C3_SDA (EXT I2C3) | ✅ AF4 | `{PC_9, I2C3, GPIO_AF4_I2C3}` |
| **PA8** | I2C3_SCL (EXT I2C3) | ✅ AF4 | `{PA_8, I2C3, GPIO_AF4_I2C3}` |
| **PB0** | Fan PWM (TIM8_CH2N) | ✅ AF3 | `{PB_0_ALT2, TIM8, GPIO_AF3}` |
| **PC12** | SPI3_MOSI (LED Strip) | ✅ AF6 | `{PC_12, SPI3, GPIO_AF6_SPI3}` |
| **PC11** | SPI3_MISO (LED Strip) | ✅ AF6 | `{PC_11, SPI3, GPIO_AF6_SPI3}` |
| **PC10** | SPI3_SCK (LED Strip) | ✅ AF6 | `{PC_10, SPI3, GPIO_AF6_SPI3}` |
| **PD12** | TIM4_CH1 (Encoder FL A) | ✅ AF2 | ROSbot XL已验证 |
| **PD13** | TIM4_CH2 (Encoder FL B) | ✅ AF2 | ROSbot XL已验证 |
| **PB4/PB5** | TIM3_CH1/2 (Encoder FR A/B) | ✅ AF2 | PB4: NJTRST / SPI3_MISO / TIM3_CH1; PB5: I2C1_SMBA / CAN2_RX / TIM3_CH2 |
| **PA15** | TIM2_CH1 (Encoder RL A) | ✅ AF1 | ROSbot XL已验证 |
| **PB3** | TIM2_CH2 (Encoder RL B) | ✅ AF1 | ROSbot XL已验证 |
| **PE9** | TIM1_CH1 (Encoder RR A) | ✅ AF1 | ROSbot XL已验证 |
| **PE11** | TIM1_CH2 (Encoder RR B) | ✅ AF1 | ROSbot XL已验证 |
| **TIM5** | Runtime Stats (32-bit) | ✅ | 32位通用定时器，APB1 84MHz |

### 7.3 关键发现：TIM8 频率冲突修复与引脚決策

**结论**：Motor RR 使用 TIM8\_CH1/2（PC6/PC7），Fan 使用 TIM8\_CH2N（PB0），二者由不同通道独立工作无冲突；PC8（TIM8\_CH3）空闲可作 EXT\_PWM1；PC9 因 Motor RR 迁至 PC6/PC7 而释放，给到 I2C3\_SDA（EXT2 扩展 I2C3）。

**ETH TX 保留在 PB（选定方案）**：ETH RMII TX 信号保留在 PB11（ETH\_TX\_EN AF11）/ PB12（ETH\_TXD0 AF11）/ PB13（ETH\_TXD1 AF11）。虽然 PG11/PG13/PG14 同样有 ETH TX AF11 能力，但保留在 PB 可使 PG11 空闲为 Right nSLEEP GPIO，需要的 SPI2 采用非标准组：PB10（SCK AF5）+ PB14（MISO AF5）+ PB15（MOSI AF5）+ PE15（软件 GPIO CS）

### 7.4 完整引脚分配表（ZET6 最终方案）

| 功能 | 引脚 | 定时器/总线 | AF | 备注 |
|------|------|------------|:---:|------|
| I2C2_SDA (IMU+EEPROM) | PF0 | I2C2 | AF4 | OK | |
| I2C2_SCL (IMU+EEPROM) | PF1 | I2C2 | AF4 | OK | |
| I2C1_SCL (WP5) | PB8 | I2C1 | AF4 | OK | |
| I2C1_SDA (WP5) | PB9 | I2C1 | AF4 | OK | |
| I2C3_SCL (EXT I2C3) | PA8 | I2C3 | AF4 | OK EXT2 扩展 |
| I2C3_SDA (EXT I2C3) | PC9 | I2C3 | AF4 | OK EXT2 扩展；Motor RR 迁移后释放 |
| Motor FL IN1 | PF8 | TIM14_CH1 | AF9 | OK | |
| Motor FL IN2 | PF9 | TIM10_CH1 | AF3 | OK | |
| Motor FR IN1 | PF6 | TIM13_CH1 | AF9 | OK | |
| Motor FR IN2 | PF7 | TIM11_CH1 | AF3 | OK | |
| Motor RL IN1 | PE5 | TIM9_CH1 | AF3 | OK | |
| Motor RL IN2 | PE6 | TIM9_CH2 | AF3 | OK | |
| Motor RR IN1 | PC6 | TIM8_CH1 | AF3 | OK | |
| Motor RR IN2 | PC7 | TIM8_CH2 | AF3 | OK | |
| Encoder FL A/B | PD12/PD13 | TIM4_CH1/2 | AF2 | OK | |
| Encoder FR A/B | PB4/PB5 | TIM3_CH1/2 | AF2 | OK | |
| Encoder RL A/B | PA15/PB3 | TIM2_CH1/2 | AF1 | OK | |
| Encoder RR A/B | PE9/PE11 | TIM1_CH1/2 | AF1 | OK | |
| SPI3 MOSI | PC12 | SPI3 | AF6 | LED Strip | OK | |
| SPI3 MISO | PC11 | SPI3 | AF6 | LED Strip | OK | |
| SPI3 SCK | PC10 | SPI3 | AF6 | LED Strip | OK | |
| SPI2 MOSI | PB15 | SPI2 | AF5 | EXT1 扩展 | OK | |
| SPI2 SCK | PB10 | SPI2 | AF5 | EXT1 扩展 | OK | |
| SPI2 MISO | PB14 | SPI2 | AF5 | EXT1 扩展 | OK | |
| SPI2 NSS (GPIO CS) | PE15 | GPIO | — | EXT1 扩展；软件片选，PE15 无 SPI2_NSS 硬件 AF，不能用硬件 NSS | OK | |
| USART1 TX/RX | PA9/PA10 | USART1 | AF7 | 下载 | OK | |
| USART2 TX/RX | PD5/PD6 | USART2 | AF7 | EXT | OK | |
| USART3 TX/RX | PD8/PD9 | USART3 | AF7 | SBC | OK | |
| USART6 TX/RX | PG14/PG9 | USART6 | AF7 | 舵机 | OK | |
| ETH RMII | PA1/PA2/PA7, **PB11/PB12/PB13**, PC1/PC4/PC5 | ETH | AF11 | LAN9303；ETH TX 保留在 PB，PG11 附用为 Right nSLEEP | OK | |
| Right nSLEEP | PG11 | GPIO | — | OK 释放 PG9 USART6 |
| Left nSLEEP | PG10 | GPIO | — | OK | |
| Right nFAULT | PE1 | GPIO | — | OK | |
| Left nFAULT | PE0 | GPIO | — | OK | |
| PB_SHD_DETECT | PD4 | GPIO | — | WP5关机检测 | OK | |
| PB_SHD_CONFIRM | PD3 | GPIO | — | 关机确认 | OK | |
| PUSH_BUTTON1 | PF11 | GPIO | — | OK | |
| PUSH_BUTTON2 | PF12 | GPIO | — |  OK | |
| EN_LOC_5V | PF13 | GPIO | — | OK | |
| EN_LOC2_5V | PF14 | GPIO | — | OK | |
| EN_LOC3_5V | PF15 | GPIO | — | OK | |
| IMU INT | PF2 | EXTI | — | OK | |
| Fan PWM | PB0 | TIM8_CH2N | AF3 |20kHz complementary | OK | |
| NTC ADC | PB1 | ADC | Analog | OK | |
| Audio SHDN | PD2 | GPIO | — | OK | |
| Battery ADC | PA5 | ADC1_CH5 | Analog | OK | |
| Audio DAC | PA4 | DAC | Analog | OK | |
| RED LED | PE4 | GPIO | — | OK | |
| GRN LED | PE3 | GPIO | — | OK | |
| Runtime Stats | TIM5 | 32-bit timer | — | 84MHz/840=100kHz |
| SWD Debug | PA13/PA14 | Debug | — | |

### 7.2 引脚资源统计与扩展预留

### 7.2 引脚资源统计与扩展预留

| 项目 | 说明 |
|------|------|
| 总引脚数 | 114 (去除 VSS/VDD 等电源) |
| 已用引脚 | ~62 个 |
| 空闲引脚 | ~52 个，可用于未来扩展 |
| I2C 总线 | **3 条独立总线**：I2C1(WP5/PB8-9)、I2C2(IMU/PF0-1)、I2C3(EXT2/PA8-PC9) |
| SPI 总线 | SPI3 (LED Strip/PC10-12) + **SPI2 (EXT1 扩展/PB10-SCK, PB14-MISO, PB15-MOSI, PE15-GPIO CS)** |
| UART 总线 | **3 路**：USART1(SBC/PA9-10) + USART2(PD5/PD6) + USART3(PD8/PD9) |
| ADC 预留 | PC0/PC2/PC3 (EXT_ADC3/2/1)，无 SPI2 冲突 |

### 7.3 PCB 布线建议

1. **I2C 分离布局**
   - I2C1 (PB6/PB7 IMU+EEPROM) 与 I2C2 (PF0/PF1 WP5) 走线分开
   - 4.7kΩ 上拉电阻靠近芯片放置
   
2. **电机驱动分组**
   - FL: PF9/PF6 相邻
   - FR: PF8/PF7 相邻
   - RL: PE5/PE6 相邻
   - RR: PC6/PC7 相邻
   
3. **SPI3 LED Strip**
  - PC12/PC11/PC10 紧靠连接器，减少走线长度

4. **电源去耦**
   - 每个 DRV8874 旁边放置 0.1µF + 10µF 陶瓷电容

-------

## 8. ROSbot XL 兼容性：Extra GPIO 扩展连接器

### 8.1 硬件兼容性声明

ROSbot PRO 设计完全兼容 ROSbot XL 的扩展接口标准，包括：
- **EXT1 connector**（Extra GPIO - Bank 1）
- **EXT2 connector**（Extra GPIO - Bank 2）

所有扩展引脚采用 **3.3V 逻辑**，均接入 **220Ω 保护电阻**。ADC 引脚靠近 MCU 处额外配置 **100nF 退耦电容**。

连接器规格：Harting 09 18 514 5813 58U（IDC 排线连接器，2.54mm 间距）

### 8.2 EXT1 连接器引脚定义（Extra GPIO - Bank 1）

所有信号采用 **3.3V 逻辑**，每条信号线均接 **220Ω 保护电阻**。

| Pin | 信号名 | 功能说明 | STM32 引脚 | 备注 |
|-----|--------|---------|----------|------|
| 1 | EXT_GPIO2 | 通用 GPIO | PE8 | GPIO 输入/输出 |
| 2 | EXT_GPIO1 | 通用 GPIO | PE2 | GPIO 输入/输出 |
| 3 | EXT_PWM2 | 扩展 PWM 2 | PA6 (TIM13_CH1) | PWM 输出 |
| 4 | EXT_PWM1 | 扩展 PWM 1 | PC8 (TIM8_CH3, AF3) | PWM 输出；Motor RR 迁移后空闲 |
| 5 | EXT_GPIO3 | 通用 GPIO | PG0 | GPIO 输入/输出 |
| 6 | EXT_GPIO4 | 通用 GPIO | PG1 | GPIO 输入/输出 |
| 7 | I2C1_SCL | I2C1 时钟 | PB6 (I2C1_SCL) | **需外部上拉** 4.7kΩ |
| 8 | I2C1_SDA | I2C1 数据 | PB7 (I2C1_SDA) | **需外部上拉** 4.7kΩ |
| 9 | SPI2_MOSI | SPI2_MOSI | PB15 (AF5) | SPI2_MOSI 标准引脚 |
| 10 | SPI2_SCK | SPI2_SCK | PB10 (AF5) | SPI2_SCK；PB13 被 ETH_TXD1 占用，改用 PB10 |
| 11 | SPI2_MISO | SPI2_MISO | PB14 (AF5) | SPI2_MISO 标准引脚 |
| 12 | +3V3_PROT | 3.3V 保护输出 | +3V3 | 500mA 过流保护 |
| 13 | GND | 地 | GND | — |
| 14 | +5V_PROT | 5V 保护输出 | +5V | 500mA 过流保护 |

**说明**：
- **SPI2 NSS**：PB12 被 ETH_TXD0 占用，改用 PE15 作为 GPIO 软件片选（SPI\_NSS\_SOFT 模式），SPI2 SCK 改用 PB10（AF5）
- **USART6 在 PRO 中不可用**：PC6/PC7 = Motor RR，PG9 = nSLEEP，PG14 = ETH TXD1；Encoder FR 已迁至 PB4/PB5 (TIM3)
- **EXT1 默认配置**：GPIO / PWM (PA6 + PC8) / I2C1 (PB6/PB7) / **SPI2 标准引脚（无需犊牲任何 ADC）**

### 8.3 EXT2 连接器引脚定义（Extra GPIO - Bank 2）

所有信号采用 **3.3V 逻辑**，每条信号线均接 **220Ω 保护电阻**。ADC 引脚靠近 MCU 处额外配置 **100nF 退耦电容**。

| Pin | 信号名 | 功能说明 | STM32 引脚 | 备注 |
|-----|--------|---------|----------|------|
| 1 | GND | 地 | GND | — |
| 2 | BOOT0 | 启动模式选择 | BOOT0 | 板上已下拉 |
| 3 | EXT_UART2_TX | USART2 TX | PD5 (AF7) | USART2_TX 调试/扩展串口；PA11 已保留给 CAN1 |
| 4 | NRST | 系统复位 | NRST | 板上已上拉 |
| 5 | EXT_ADC1 | 扩展 ADC 1 | PC3 (ADC123_CH13) | 100nF 退耦；PC1=ETH_MDC 不可用作 ADC |
| 6 | EXT_ADC2 | 扩展 ADC 2 | PC2 (ADC123_CH12) | 100nF 退耦 |
| 7 | EXT_ADC3 | 扩展 ADC 3 | PC0 (ADC123_CH10) | 100nF 退耦 |
| 8 | EXT_GPIO3 | 通用 GPIO | PB9 (GPIO) | GPIO 输入/输出 |
| 9 | I2C3_SDA | I2C3 数据线 | PC9 (I2C3_SDA) | **需外部上拉** 4.7kΩ；（已保留给 EXT I2C3，因 Encoder/ RR 重新分配而可用） |
| 10 | I2C3_SCL | I2C3 时钟线 | PA8 (I2C3_SCL) | **需外部上拉** 4.7kΩ；（已保留给 EXT I2C3，因 Encoder/ RR 重新分配而可用） |
| 11 | CANH | CAN 总线高线 | CANH | **需主板板载收发器**（如 SN65HVD230） |
| 12 | CANL | CAN 总线低线 | CANL | **需主板板载收发器**（如 SN65HVD230） |
| 13 | GND | 地 | GND | — |
| 14 | +5V_PROT | 5V 保护输出 | +5V | 500mA 过流保护 |

**⚠️ 引脚复用说明**：
- **CANH/CANL**: EXT2 只暴露 CAN 总线层信号，不直接暴露 MCU 侧 PA11/PA12
  - 板载 SN65HVD230（或同类 3.3V CAN 收发器）负责 STM32 CAN1 与外部总线之间的电平转换
  - 这样 EXT2 可以直接接标准 CAN 设备，不需要在扩展板上再加一颗收发器
- **I2C3** (Pin 9/10): EXT2 独立扩展 I2C3 总线；WP5 使用 I2C1(PB8/PB9)，IMU 使用 I2C2(PF0/PF1)，EXT2 使用 I2C3(PA8/PC9)，三路完全独立
- **PC3/PC2/PC0** (EXT_ADC1~3): 真实 ADC 三合一通道 (ADC1/2/3)，无冲突；PC1=ETH_MDC 不可用作 ADC；SPI2 已迁移至 PB，PC2/PC3 回归纯 ADC；IPROPI 已改用 PF3/PF4/PF5/PF10

**说明**：
- EXT1 和 EXT2 是两个独立的扩展连接器，可同时使用
- 所有信号线均通过 220Ω 保护电阻，增强 ESD 防护
- I2C1 (EXT1 PIN 7/8) 和 I2C3 (EXT2 PIN 9/10) 需在 PCB 上提供 4.7kΩ 外部上拉电阻至 3.3V
- CAN (EXT2 PIN 11/12) 为总线层 CANH/CANL，主板上需板载 CAN 收发器（如 SN65HVD230 或 TJA1050）
- ADC 引脚 (EXT2 PIN 5/6/7) 在 MCU 侧配置 100nF 退耦电容
- USART6 在 PRO 设计中**不可用**（PC6/PC7 被 Motor RR 占用，PG9 被 nSLEEP 占用，PG14 被 ETH TXD1 占用）

### 8.4 与 ROSbot XL 的完全兼容性说明

#### 连接器物理规格
- **型号**: Harting 09 18 514 5813 58U
- **连接方式**: IDC 排线连接器
- **引脚间距**: 0.1" (2.54mm)
- **排线间距**: 0.05" (1.27mm)

#### 功能兼容性对比

| 特性 | ROSbot XL | ROSbot PRO | 状态 |
|------|----------|----------|------|
| EXT1 GPIO 数量 | 2 | 2 (PE8, PE2) | ✅ 相同 |
| EXT1 PWM 数量 | 2 | 2 | ✅ 相同 |
| EXT1 UART6 | PC6/PC7 | 不提供 | ❌ PC6/PC7=Motor RR，USART6 在 PRO 中全部冲突 |
| EXT1 I2C1 | PB6/PB7 | PB6/PB7 | ✅ 相同 |
| EXT1 SPI1 | PA5/PA6/PA7 | 不提供 | ❌ 与 Ethernet / Battery ADC 冲突 |
| **EXT1 SPI2** | — | PB10(SCK)/PB14(MISO)/PB15(MOSI)/PE15(GPIO CS) | ✅ ETH TX 保留在 PB，SPI2 调整 NSS和SCK |
| EXT2 ADC 数量 | 3 | 3 | ✅ 相同数量 |
| EXT2 ADC 引脚 | PB4/PB5/PC0 (XL) | PC3/PC2/PC0 (PRO) | ✅ PC1=ETH_MDC 改用 PC3；SPI2 迁至 PB 后 PC2/PC3 无冲突 |
| EXT2 I2C3 | PA8/PC9 | PA8/PC9 | ✅ 相同 |
| EXT2 CAN 接口 | 支持 | 支持（CANH/CANL） | ✅ 相同 |
| 逻辑电压 | 3.3V | 3.3V | ✅ 相同 |
| 保护电阻 | 220Ω | 220Ω | ✅ 相同 |
| ADC 退耦 | 100nF | 100nF | ✅ 相同 |

#### 使用建议

**EXT1 配置**:
1. ✅ **GPIO 用途**: 通用输入/输出扩展，适合传感器 or 指示灯
2. ✅ **PWM 用途**: 额外电机驱动或 LED 亮度控制
3. ✅ **GPIO 扩展**: 可使用 PG0/PG1/PG2/PG3/PE7 等空闲引脚补足扩展能力
4. ✅ **I2C1**: 可作为传感器总线扩展
5. ✅ **SPI2 (PB12/13/14/15)**：EXT1 提供硬件 SPI2 标准引脚，无需犊牲任何 ADC 引脚

**EXT2 配置**:
1. ✅ **ADC**: 适合模拟传感器输入（温度、光照等）
2. ✅ **I2C3**: Witty Pi 5 默认通信总线（无冲突）
3. ✅ **CAN**: EXT2 提供 CANH/CANL，总线收发器放在主板上，扩展板可直接接入标准 CAN 节点
4. ✅ **SPI2**：已在 EXT1 引出，使用标准引脚 PB13/14/15（NSS: PB12），EXT2 所有 ADC 引脚均不受影响

**推荐扩展方案**：
- SPI2 已迁移至标准引脚 PB12/13/14/15，EXT1 SPI2 与 EXT2 ADC1/2/3 可同时使用，无需任何迁移
- 若使用 UART6，调整编码器到其他定时器通道
- CAN 功能依赖主板板载收发器，且总线末端需要 120Ω 终端电阻

---

**结论**: ROSbot PRO 的 EXT1/EXT2 连接器在物理规格上与 ROSbot XL 兼容，但 EXT1 的 UART6 与编码器 FR 共享 PC6/PC7，功能上不能同时启用；现有 XL 扩展板是否可直接使用取决于是否占用这组引脚。

-------

## 附录：EXT 连接器完整引脚映射表

### 连接器物理信息
- **标准**: IDC 排线连接器（2.54mm 间距）
- **模型**: Harting 09 18 514 5813 58U
- **信号完整性**: 所有引脚通过 220Ω 保护电阻（增强 ESD 防护）
- **电源保护**: 500mA 过流保护在 +3V3_PROT 和 +5V_PROT

### EXT1 (Extra GPIO - Bank 1) 完整映射

```
EXT1 连接器布局（从左到右）：
Pin  1: EXT_GPIO2  (PE8)      |  Pin  2: EXT_GPIO1  (PE2)
Pin  3: EXT_PWM2   (PA6)      |  Pin  4: EXT_PWM1   (PA11)
Pin  5: EXT_GPIO3  (PG0)      |  Pin  6: EXT_GPIO4  (PG1)
Pin  7: I2C1_SCL   (PB6)      |  Pin  8: I2C1_SDA   (PB7)
Pin  9: SPI2_MOSI  (PC3)      |  Pin 10: SPI2_SCK  (PB10)
Pin 11: SPI2_MISO  (PC2)      |  Pin 12: +3V3_PROT
Pin 13: GND                   |  Pin 14: +5V_PROT
```

### EXT2 (Extra GPIO - Bank 2) 完整映射

```
EXT2 连接器布局（从左到右）：
Pin  1: GND                   |  Pin  2: BOOT0 (下拉)
Pin  3: EXT_PWM3  (PA11 已保留给 CAN，需重新分配)      |  Pin  4: NRST (上拉)
Pin  5: EXT_ADC1  (PC1)       |  Pin  6: EXT_ADC2  (PC2)
Pin  7: EXT_ADC3  (PC0)       |  Pin  8: EXT_GPIO3 (PB9)
Pin  9: I2C3_SDA  (PC9)       |  Pin 10: I2C3_SCL  (PA8)
Pin 11: CANH      (CAN bus H)  |  Pin 12: CANL      (CAN bus L)
Pin 13: GND                   |  Pin 14: +5V_PROT
```

### 关键引脚冲突分析

| 引脚 | EXT1 功能 | 主板用途 | 解决方案 |
|------|---------|---------|---------|
| PG0 | EXT_GPIO3 | Free GPIO | EXT1 通用扩展 |
| PG1 | EXT_GPIO4 | Free GPIO | EXT1 通用扩展 |
| PG2 | EXT_GPIO5 | Free GPIO | EXT1 通用扩展 |
| PG3 | EXT_GPIO6 | Free GPIO | EXT1 通用扩展 |
| PE7 | EXT_GPIO7 | Free GPIO | EXT1 通用扩展 |
| CANH/CANL | CAN 总线 | 板载 SN65HVD230 | EXT2 总线接口 |

### PCB 设计建议

**EXT1 配置**:
1. BOOT0: 已板上下拉到 GND，正常情况下无需连接
2. NRST: 已板上上拉到 3.3V
3. I2C1_SCL/SDA: **必须** 在 PCB 上提供 4.7kΩ 外部上拉（至 3.3V）
4. SPI1: 设计 SPI 器件连接时需注意与以太网/电池 ADC 的冲突
5. USART6: 若使用编码器反馈，不建议启用 USART6

**EXT2 配置**:
1. BOOT0/NRST: 保留用于系统维护（烧录/复位）
2. ADC 引脚: **建议** 在 PC0 处靠近 MCU 配置 100nF 退耦电容
3. I2C3: **必须** 提供 4.7kΩ 外部上拉（至 3.3V）
4. CAN: 主板板载 CAN 收发器（推荐 SN65HVD230）负责 MCU 与总线转换，EXT2 只暴露 CANH/CANL
  - 收发器供电: 3.3V (与 STM32 CAN1 逻辑侧一致)
  - 终端电阻: 120Ω (总线末端，可选)

**电源连接**:
- +3V3_PROT: 3.3V 电源输出，单个扩展板最多 500mA（内部集成 OC 保护）
- +5V_PROT: 5V 电源输出，单个扩展板最多 500mA（内部集成 OC 保护）

---

## 附录：扩展资源预留引脚

| 引脚 | 备选功能 | 建议用途 |
|------|---------|--------|
| PA11 | CAN1_RX (保留给 CAN) | AF9 / 保留 |
| PA12 | CAN1_TX (保留给 CAN) | AF9 / 保留 |
| PB4 | SPI1_MISO, TIM3_CH1 | 扩展 SPI（无 ADC 功能）|
| PB5 | SPI1_MOSI, TIM3_CH2 | 扩展 SPI（无 ADC 功能）|
| PB9 | TIM11_CH1 | 备用 PWM |
| PC0 | ADC123_CH10 | EXT_ADC3（通用模拟输入）|
| PC1 | ADC123_CH11 | EXT_ADC1（通用模拟输入）|
| PC2 | ADC123_CH12 | EXT_ADC2（通用模拟输入）|
| PG0 | GPIO | EXT1 预留 / 通用扩展 |
| PG1 | GPIO | EXT1 预留 / 通用扩展 |
| PG2 | GPIO | EXT1 预留 / 通用扩展 |
| PG3 | GPIO | EXT1 预留 / 通用扩展 |
| PE7 | GPIO | EXT1 预留 / 通用扩展 |
| PC8 | TIM8_CH3 | RR 电机 IN1 / 已占用 |
| PC10-PC15 | USART3/4, SPI3 | 串口 / SPI 扩展 |
| PD9-PD11 | USART3, FSMC | 扩展串口 |
| PD14-PD15 | TIM4_CH3/CH4 | 扩展定时器 |
| PE2,PE7-PE8 | 多用途 | 通用 GPIO |

---

**项目决策**：STM32F407ZET6 (LQFP-144) 最终选型已确认，所有引脚分配基于该器件。
