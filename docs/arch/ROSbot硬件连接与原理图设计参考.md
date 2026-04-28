# ROSbot 硬件连接与原理图设计参考文档

> 基于源码逆向分析的完整硬件引脚映射  
> 目标：为设计新的 ROS 开发板提供硬件连接参考  
> 分析日期：2026-04-09  
> ⚠️ Ethernet 部分设计为可选扩展板，默认使用 Serial 通信

---

## 目录

1. [MCU 选型与核心资源](#1-mcu-选型与核心资源)
2. [电源系统设计](#2-电源系统设计)
3. [串口通信接口](#3-串口通信接口)
4. [电机驱动电路](#4-电机驱动电路)
5. [编码器接口](#5-编码器接口)
6. [IMU 惯性测量单元](#6-imu-惯性测量单元)
7. [测距传感器接口](#7-测距传感器接口)
8. [电池电压采集电路](#8-电池电压采集电路)
9. [LED 指示灯与灯带](#9-led-指示灯与灯带)
10. [按钮输入](#10-按钮输入)
11. [风扇温控电路](#11-风扇温控电路)
12. [EEPROM 存储](#12-eeprom-存储)
13. [电源板接口](#13-电源板接口)
14. [Ethernet 扩展板设计](#14-ethernet-扩展板设计可选)
15. [调试接口](#15-调试接口)
16. [完整引脚分配表](#16-完整引脚分配表)
17. [定时器资源分配](#17-定时器资源分配)
18. [I2C 总线分配](#18-i2c-总线分配)
19. [新板设计建议](#19-新板设计建议)

---

## 1. MCU 选型与核心资源

### 1.1 原始 MCU

| 参数 | 值 |
|------|-----|
| 型号 | STM32F407LGT6 |
| 内核 | ARM Cortex-M4F, 168MHz |
| Flash | 1MB |
| SRAM | 192KB + 64KB CCM |
| 封装 | LQFP-144 |
| FPU | 单精度浮点 |
| 工作电压 | 1.8V - 3.6V |

### 1.2 外设资源使用情况

| 资源类型 | 原始使用数量 | 总可用数 | 说明 |
|----------|------------|---------|------|
| 通用定时器(16bit) | 4 (编码器) | 8 | TIM1/2/3/4/8 被编码器占用 |
| 通用定时器(32bit) | 1 (运行统计) | 2 | TIM5 用于 RTOS 统计 |
| PWM 输出 | 4 (电机) + 1 (风扇) | 多通道 | PF6-PF9 用于电机 |
| UART | 2-3 | 6 | UART1/3 或 UART1 + PB_Serial |
| I2C | 1-2 | 3 | IMU + 测距/EEPROM |
| SPI | 1 (LED灯带) | 3 | 仅 XL 使用 |
| ADC | 1-2 | 3x16ch | 电池+NTC |
| GPIO (输入) | 2 (按钮) | 大量 | Pull-up 输入 |
| GPIO (输出) | 3-5 (LED+使能) | 大量 | Push-Pull 输出 |
| Ethernet MAC | 1 (仅 XL) | 1 | 需外部 PHY |

### 1.3 新板 MCU 选择建议

如果只需要基本功能（Serial 通信，无 Ethernet），可以考虑降级到：
- **STM32F407VET6** (LQFP-100)：引脚更少但足够，成本更低
- **STM32F407ZET6** (LQFP-144)：与原版引脚兼容
- **STM32F446RET6** (LQFP-64)：最小化方案，180MHz

> ⚠️ 必须确保有足够的定时器通道支持 4 路编码器 + 4 路 PWM

---

## 2. 电源系统设计

### 2.1 供电架构

```
电池 (3S LiPo, 9.6V~12.6V)
    │
    ├── 5V 电源轨 ──→ 传感器、编码器、LED 灯带
    │   (建议 LDO 或 DC-DC Buck: MP1584/TPS54331)
    │
    ├── 3.3V 电源轨 ──→ MCU、IMU、EEPROM、通信接口
    │   (建议 LDO: AMS1117-3.3 或 LDO 从 5V 降)
    │
    └── 电机电源 ──→ DRV8848 VM (直接电池电压)
```

### 2.2 ROSbot XL 特有的电源控制

| 信号 | 引脚 | 方向 | 功能 |
|------|------|------|------|
| EN_LOC_5V | PF13 | OUTPUT | 本地 5V 使能（HIGH=开启） |
| AUDIO_SHDN | PB2 | OUTPUT | 音频放大器使能（HIGH=关闭） |

### 2.3 电流预估

| 部件 | 典型电流 | 峰值电流 |
|------|---------|---------|
| STM32F407 | 60mA | 100mA |
| BNO055 IMU | 12mA | 18mA |
| VL53L0X × 4 | 40mA | 80mA |
| DRV8848 × 2（逻辑） | 10mA | 20mA |
| 电机 × 4 | 2A | 4A(堵转) |
| LED 指示灯 | 20mA | 40mA |
| APA102 × 18 | 180mA | 1A |
| **总计（不含电机）** | **~300mA** | **~1.3A** |

---

## 3. 串口通信接口

### 3.1 ROSbot 串口配置

#### 主通信串口（连接 SBC/ROS 主机）

| 参数 | 值 | 说明 |
|------|-----|------|
| 外设 | UART1 (Serial1) | 使用 APB2 时钟 |
| TX | **PA9** (UART1_TX) | 连接到 SBC 的 RX |
| RX | **PA10** (UART1_RX) | 连接到 SBC 的 TX |
| 波特率 | 921600 | micro-ROS XRCE 协议 |
| 电平 | 3.3V TTL | 如连接 USB 需加转换芯片 |

```
新板设计电路建议：

STM32          CH340G/CP2102        USB
PA9(TX) ───→  RXD        ┌──→  USB-B/Type-C
PA10(RX) ←──  TXD        │     (连接ROS主机)
GND ─────────  GND  ──────┘
3.3V ────────  VCC
```

#### 诊断串口（调试/备用通信）

| 参数 | 值 | 说明 |
|------|-----|------|
| 外设 | UART3 (Serial3) | 使用 APB1 时钟 |
| TX | **PB10** (UART3_TX) | 后面板 USB 接口 |
| RX | **PB11** (UART3_RX) | FTDI/CH340 转换 |
| 波特率 | 921600 | Debug 输出或备用 ROS 通道 |

```
                  ┌─────────────┐
PB10(TX) ────────→│ CH340G      │──→ USB
PB11(RX) ←────────│ (USB转UART) │
                  └─────────────┘
```

### 3.2 ROSbot XL 串口配置

| 串口 | TX | RX | 波特率 | 功能 |
|------|----|----|--------|------|
| UART1 | **PA9** | **PA10** | 921600 | 诊断串口 / 备用 ROS 通信 |
| PB_Serial | **PD5** | **PD6** | 38400 | 电源板通信 |

> ⚠️ ROSbot XL 主要通过 Ethernet 通信，UART1 作为备用/调试口

### 3.3 SBC 接口辅助信号（ROSbot）

| 信号 | 引脚 | 方向 | 功能 |
|------|------|------|------|
| SBC_STATUS | PG6 | INPUT (Pullup) | 检测树莓派是否连接 |
| RPI_CONSOLE | PG5 | - | 树莓派控制台 |
| RPI_BTN | PG7 | - | 树莓派按钮 |

---

## 4. 电机驱动电路

### 4.1 驱动芯片选型

| 参数 | DRV8848 | MAX22205 (XL电流限制) |
|------|---------|----------------------|
| 工作电压 | 4V ~ 18V | 4.5V ~ 36V |
| 每通道电流 | 1A (持续) | 可配置限流 |
| 控制方式 | PWM + IN_A/IN_B | H桥 + ILIM引脚 |
| 保护 | 过流/过温/欠压 | 过流保护 |

### 4.2 ROSbot 电机引脚映射

```
┌──────────────────── DRV8848 #1 (右侧电机) ────────────────────┐
│                                                                │
│  使能: PC13 (nSLEEP) ──→ 高电平使能                            │
│  故障: PE0  (nFAULT) ←── 低电平表示故障                        │
│                                                                │
│  ┌── 前右(FR)电机 ──┐    ┌── 后右(RR)电机 ──┐                  │
│  │ PWM:  PF6        │    │ PWM:  PF7        │                  │
│  │ IN_A: PG10       │    │ IN_A: PD3        │                  │
│  │ IN_B: PG11       │    │ IN_B: PD4        │                  │
│  │ 方向: 反转        │    │ 方向: 反转        │                  │
│  └──────────────────┘    └──────────────────┘                  │
└────────────────────────────────────────────────────────────────┘

┌──────────────────── DRV8848 #2 (左侧电机) ────────────────────┐
│                                                                │
│  使能: PC14 (nSLEEP) ──→ 高电平使能                            │
│  故障: PE1  (nFAULT) ←── 低电平表示故障                        │
│                                                                │
│  ┌── 前左(FL)电机 ──┐    ┌── 后左(RL)电机 ──┐                  │
│  │ PWM:  PF9        │    │ PWM:  PF8        │                  │
│  │ IN_A: PE5        │    │ IN_A: PC15       │                  │
│  │ IN_B: PE6        │    │ IN_B: PF2        │                  │
│  │ 方向: 正常        │    │ 方向: 正常        │                  │
│  └──────────────────┘    └──────────────────┘                  │
└────────────────────────────────────────────────────────────────┘
```

### 4.3 ROSbot XL 电机引脚映射

```
┌──── DRV8848-兼容驱动 + MAX22205 电流限制 ────────────────────┐
│                                                              │
│  使能(右): PC13  故障(右): PE0                                │
│  使能(左): PC14  故障(左): PE1                                │
│                                                              │
│  ┌── FL 电机 ──┐  ┌── FR 电机 ──┐                            │
│  │ PWM:  PF9   │  │ PWM:  PF8   │                            │
│  │ IN_A: PD10  │  │ IN_A: PG5   │                            │
│  │ IN_B: PD11  │  │ IN_B: PG6   │                            │
│  │ 方向: 反转   │  │ 方向: 正常   │                            │
│  │ ILIM: PE10  │  │ ILIM: PG15  │                            │
│  └─────────────┘  └─────────────┘                            │
│                                                              │
│  ┌── RL 电机 ──┐  ┌── RR 电机 ──┐                            │
│  │ PWM:  PF7   │  │ PWM:  PF6   │                            │
│  │ IN_A: PG11  │  │ IN_A: PE12  │                            │
│  │ IN_B: PG12  │  │ IN_B: PE13  │                            │
│  │ 方向: 反转   │  │ 方向: 正常   │                            │
│  │ ILIM: PG7   │  │ ILIM: PD14  │                            │
│  └─────────────┘  └─────────────┘                            │
│                                                              │
│  ILIM 控制:                                                  │
│    v1.1: OUTPUT HIGH = 最大电流                               │
│    v1.2: INPUT 浮空 = 由外部电阻决定                          │
└──────────────────────────────────────────────────────────────┘
```

### 4.4 电机驱动电路设计参考

```
                    DRV8848 (每芯片2个H桥)
                    ┌───────────────────┐
电池 V+ ──────────→ │ VM              │
3.3V ─────────────→ │ VCC             │
GND ──────────────→ │ GND             │
                    │                   │
PC13/PC14 ────────→ │ nSLEEP (使能)    │
PE0/PE1   ←────────│ nFAULT (故障)    │
                    │                   │
PF9 (PWM 20kHz)──→ │ EN_A  ──→ │OUTA1│──→ 电机A+
PE5 ──────────────→ │ AIN1     │OUTA2│──→ 电机A-
PE6 ──────────────→ │ AIN2             │
                    │                   │
PF8 (PWM 20kHz)──→ │ EN_B  ──→ │OUTB1│──→ 电机B+
PC15 ─────────────→ │ BIN1     │OUTB2│──→ 电机B-
PF2 ──────────────→ │ BIN2             │
                    └───────────────────┘

注意：
- VM 需加 100nF + 10µF 去耦电容
- nSLEEP 需 100kΩ 下拉到 GND（默认休眠）
- nFAULT 开漏输出，需 10kΩ 上拉到 VCC
- 电机端加 100nF 陶瓷电容抑制 EMI
```

### 4.5 PWM 配置

| 参数 | 值 | 说明 |
|------|-----|------|
| 频率 | 20kHz | 超出人耳范围（避免啸叫） |
| 分辨率 | 16-bit ARR | 由定时器自动配置 |
| 占空比范围 | 0-100% | 通过 CaptureCompare 设置 |

---

## 5. 编码器接口

### 5.1 ROSbot 编码器引脚映射

| 电机位置 | 通道A (CH1) | 通道B (CH2) | 定时器 | 方向反转 |
|---------|------------|------------|--------|---------|
| FL (前左) | **PB6** | **PB7** | TIM4 | 否 |
| FR (前右) | **PA0** | **PA1** | TIM2 | 是 |
| RL (后左) | **PB4** | **PA7** | TIM3 | 否 |
| RR (后右) | **PC6** | **PC7** | TIM8 | 是 |

### 5.2 ROSbot XL 编码器引脚映射

| 电机位置 | 通道A (CH1) | 通道B (CH2) | 定时器 | 方向反转 |
|---------|------------|------------|--------|---------|
| FL (前左) | **PD12** | **PD13** | TIM4 | 是 |
| FR (前右) | **PC6** | **PC7** | TIM3 | 否 |
| RL (后左) | **PA15** | **PB3** | TIM2 | 是 |
| RR (后右) | **PE9** | **PE11** | TIM1 | 否 |

### 5.3 编码器电路设计

```
编码器 (增量式, 正交输出)
┌──────────────┐
│ VCC (5V)     │←── 5V 供电
│ GND          │←── GND
│ CH_A         │───→ PB6 (TIM4_CH1) ──┐
│ CH_B         │───→ PB7 (TIM4_CH2) ──┤
└──────────────┘                       │
                                       ▼
                               STM32 定时器编码模式
                               (硬件自动四倍频计数)

GPIO 配置：
  Mode: GPIO_MODE_AF_PP (推挽复用)
  Pull: GPIO_PULLUP     (内部上拉)
  Speed: GPIO_SPEED_FREQ_HIGH
  Alternate: 对应定时器的 AF 映射
    TIM1: AF1
    TIM2: AF1
    TIM3: AF2
    TIM4: AF2
    TIM8: AF3
```

### 5.4 编码器参数

| 参数 | ROSbot | ROSbot XL |
|------|--------|-----------|
| 编码器类型 | 增量式正交 | 增量式正交 |
| CPR (每转脉冲数) | 48 | 64 |
| 齿轮比 | 34:1 | 50:1 |
| 有效分辨率 | 48×4×34 = 6528 计数/转 | 64×4×50 = 12800 计数/转 |
| rad/tick | 0.000963 | 0.000491 |

> ⚠️ **定时器选择限制**：编码器模式要求 CH1 和 CH2 在同一个定时器上。
> STM32F407 的 TIM1/2/3/4/5/8 支持编码器模式。
> 注意本固件只使用了 16 位计数（0~65535），即使 TIM2/TIM5 是 32 位定时器。

---

## 6. IMU 惯性测量单元

### 6.1 ROSbot IMU 连接

| 信号 | 引脚 | 说明 |
|------|------|------|
| I2C_SDA | **PC9** | BNO055 数据线 |
| I2C_SCL | **PA8** | BNO055 时钟线 |
| INT | **PA6** | 中断输出（可选） |
| POWER_ON | **PG4** | 电源使能（HIGH=上电） |

```
BNO055 连接电路：

                  ┌───────────────┐
3.3V ──→ VCC ───→ │ BNO055        │
GND ───→ GND ───→ │               │
                   │               │
PC9 ←──── SDA ←──→│ SDA           │ ← 4.7kΩ 上拉至 3.3V
PA8 ←──── SCL ←──→│ SCL           │ ← 4.7kΩ 上拉至 3.3V
PA6 ←──── INT ←───│ INT           │ (可选，需上拉)
                   │               │
GND ───→ ADR ───→ │ ADR (LOW→0x28)│ → I2C 地址 = 0x29
                   │     (HIGH→0x29)│   (ADR=HIGH 在源码中)
                   │               │
         晶振 ──→ │ XIN/XOUT      │ → 32.768kHz 外部晶振
                   └───────────────┘

PG4 控制电源（通过 MOSFET 或 Load Switch）：
PG4 ──→ [MOSFET Gate] ──→ BNO055 VCC
         (例如 SI2301 P-MOSFET)
```

### 6.2 ROSbot XL IMU 连接

| 信号 | 引脚 | 说明 |
|------|------|------|
| I2C_SDA | **PF0** | 共享 I2C 总线 |
| I2C_SCL | **PF1** | 与 EEPROM 共享 |
| INT | **PF2** | 中断输出 |

### 6.3 IMU 配置参数

| 参数 | ROSbot | ROSbot XL |
|------|--------|-----------|
| I2C 地址 | 0x29 | 0x29 |
| Sensor ID | 0xA0 | 0x37 |
| I2C 频率 | 400kHz | 400kHz |
| 轴映射 | P0 (Config), P3 (Sign) | P1 (Config), P2 (Sign) |
| 融合模式 | NDOF | NDOF |

> 💡 **轴映射说明**：BNO055 的安装方向不同需要不同的轴映射。
> P0-P7 是预设的轴交换和符号翻转组合。
> 设计新板时需根据 BNO055 的实际安装朝向选择正确的映射。

---

## 7. 测距传感器接口

### 7.1 VL53L0X 连接（仅 ROSbot）

共 4 个 VL53L0X 传感器共享一条 I2C 总线，通过 XSHUT 引脚控制上电顺序。

| 传感器 | XSHUT 引脚 | I2C 地址 | frame_id |
|--------|-----------|----------|----------|
| FL (前左) | **PD8** | 0x30 | fl_range |
| FR (前右) | **PB1** | 0x31 | fr_range |
| RL (后左) | **PD10** | 0x32 | rl_range |
| RR (后右) | **PD9** | 0x33 | rr_range |

### 7.2 I2C 总线连接

| 信号 | 引脚 | 说明 |
|------|------|------|
| SDA | **PB9** | 测距专用 I2C 数据 |
| SCL | **PB8** | 测距专用 I2C 时钟 |

```
VL53L0X 连接电路 (×4)：

              ┌─── 4.7kΩ ──→ 3.3V (共用上拉)
              │
PB9 ←←←←←←←←─┼──→ SDA ──→ [VL53L0X #1].SDA
              │            [VL53L0X #2].SDA
              │            [VL53L0X #3].SDA
              │            [VL53L0X #4].SDA

PB8 ←←←←←←←←─┼──→ SCL ──→ [VL53L0X #1].SCL
              │            [VL53L0X #2].SCL
              │            [VL53L0X #3].SCL
              │            [VL53L0X #4].SCL

PD8  ──→ XSHUT ──→ [VL53L0X #1].XSHUT (FL)
PB1  ──→ XSHUT ──→ [VL53L0X #2].XSHUT (FR)
PD10 ──→ XSHUT ──→ [VL53L0X #3].XSHUT (RL)
PD9  ──→ XSHUT ──→ [VL53L0X #4].XSHUT (RR)

每个 VL53L0X：
  VCC → 3.3V (或 2.8V, 视模块而定)
  GND → GND
  XSHUT 需 10kΩ 下拉（默认关闭状态）
```

---

## 8. 电池电压采集电路

### 8.1 ROSbot ADC 采集电路

```
电池电压 (9.6V~12.6V)
    │
    ├── R1 = 56kΩ ──┐
    │                ├──→ PA5 (ADC) ──→ STM32 10-bit ADC
    └── R2 = 10kΩ ──┤
                     │
                    GND

分压比：V_adc = V_bat × R2/(R1+R2) = V_bat × 10/(56+10) = V_bat/6.6
最大 ADC 输入：12.6V / 6.6 = 1.91V (< 3.3V, 安全)

建议额外加 100nF 电容在 PA5 和 GND 之间滤波
建议加 3.3V TVS 二极管保护 ADC 输入
```

### 8.2 ADC 配置

| 参数 | 值 |
|------|-----|
| ADC 引脚 | PA5 |
| 分辨率 | 10-bit (0~1023) |
| 参考电压 | 3.3V |
| 分压电阻 | R1=56kΩ, R2=10kΩ |
| 校正系数 | 0.986 |

### 8.3 电压换算公式

```
V_battery = ADC_raw × (V_ref × correction × divider_ratio) / ADC_max
          = ADC_raw × (3.3 × 0.986 × 6.6) / 1023
          = ADC_raw × 0.02098
```

---

## 9. LED 指示灯与灯带

### 9.1 ROSbot GPIO LED

| LED | 引脚 | 颜色 | 功能 |
|-----|------|------|------|
| RED_LED | **PE2** | 红色 | 状态指示（开机亮） |
| GRN_LED | **PE3** | 绿色 | 后部指示灯 |
| GRN_LED2 | **PE4** | 绿色 | 后部指示灯 2 |

```
LED 连接电路：

PE2 ──→ [330Ω] ──→ [红色 LED] ──→ GND
PE3 ──→ [330Ω] ──→ [绿色 LED] ──→ GND
PE4 ──→ [330Ω] ──→ [绿色 LED] ──→ GND

GPIO 模式：OUTPUT, Push-Pull
初始状态：RED_LED = HIGH (点亮)
```

### 9.2 ROSbot XL GPIO LED

| LED | 引脚 | 颜色 | 功能 |
|-----|------|------|------|
| RED_LED | **PE4** | 红色 | 状态指示 |
| GRN_LED | **PE3** | 绿色 | 后部指示灯 |

### 9.3 ROSbot XL APA102 LED 灯带

| 参数 | 值 |
|------|-----|
| 类型 | APA102 / SK9822 (兼容) |
| 数量 | 18 颗 |
| MOSI | **PB15** |
| MISO | **PB14** (未使用，但SPI需要) |
| SCK | **PB10** |
| SPI 速率 | 4 MHz |
| SPI 模式 | Mode 3 (CPOL=1, CPHA=1) |
| 字节序 | MSB First |

```
APA102 LED 灯带连接：

STM32 SPI          APA102 灯带
PB10 (SCK) ────→   CKI (时钟)
PB15 (MOSI) ───→   SDI (数据)
5V ────────────→   VCC
GND ───────────→   GND

数据格式（每颗LED 4字节）：
┌───────────┬──────┬──────┬──────┐
│ 0xE0+亮度 │ Blue │ Green│ Red  │
│ (5-bit)   │(8bit)│(8bit)│(8bit)│
└───────────┴──────┴──────┴──────┘

帧格式：
[起始帧 4×0x00] [LED1][LED2]...[LED18] [结束帧 4×0xFF]
```

---

## 10. 按钮输入

### 10.1 ROSbot 按钮

| 按钮 | 引脚 | GPIO 模式 | ROS 话题位 |
|------|------|----------|-----------|
| PUSH_BUTTON1 | **PG12** | INPUT_PULLUP | bit 1 |
| PUSH_BUTTON2 | **PG13** | INPUT_PULLUP | bit 0 |

### 10.2 ROSbot XL 按钮

| 按钮 | 引脚 | GPIO 模式 | 功能 |
|------|------|----------|------|
| PUSH_BUTTON1 | **PF11** | INPUT_PULLUP | 用户按钮 |
| PUSH_BUTTON2 | **PF12** | INPUT_PULLUP | MCU 复位 |

```
按钮电路：
                    ┌───── STM32 内部上拉
                    │
引脚 (PG12等) ──────┤
                    │
                [ 按钮 ]
                    │
                   GND

按下 = LOW, 释放 = HIGH (内部上拉)
建议加 100nF 去抖电容
```

---

## 11. 风扇温控电路

### 11.1 NTC 温度传感器（ROSbot XL）

| 参数 | 值 |
|------|-----|
| ADC 引脚 | **PB1** |
| 上拉电阻 | 5.23kΩ (到 3.3V 或 5V) |
| NTC 类型 | 10kΩ @ 25°C (推测) |

```
NTC 温度采集电路：

3.3V
 │
 ├── R_pullup = 5.23kΩ
 │
 ├──→ PB1 (ADC 输入)
 │
 └── NTC 热敏电阻
 │
GND

Steinhart-Hart 系数：
  c1 = 1.112613927 × 10⁻³
  c2 = 2.37277392 × 10⁻⁴
  c3 = 7.167 × 10⁻⁸
```

### 11.2 风扇 PWM 控制

**v1.1 版本（常开）**：

| 参数 | 值 |
|------|-----|
| 引脚 | **PC13** |
| 模式 | GPIO OUTPUT, Push-Pull |
| 控制 | 始终 HIGH |

**v1.2 版本（PWM 调速）**：

| 参数 | 值 |
|------|-----|
| 引脚 | **PB0** |
| 定时器 | TIM8, CH2N (互补输出) |
| GPIO AF | GPIO_AF3_TIM8 |
| PWM 频率 | 25kHz (标准 PC 风扇频率) |
| 最低占空比 | 20% (30°C 以下) |
| 最高占空比 | 100% (55°C 以上) |

```
风扇驱动电路 (v1.2)：

PB0 (TIM8_CH2N) ──→ [MOSFET Gate] ──→ 风扇负极
                          │
                     风扇正极 ──→ 12V/5V
                     
注意：CH2N 是互补输出，logic LOW = 风扇转
建议加续流二极管保护 MOSFET
```

---

## 12. EEPROM 存储

### 12.1 ROSbot XL EEPROM

| 参数 | 值 |
|------|-----|
| I2C 总线 | PF0 (SDA) / PF1 (SCL) |
| 设备地址 | 0x50 |
| 页大小 | 16 bytes |
| 写延迟 | 5ms |
| 用途 | 存储板型版本号 |

```
EEPROM 连接电路 (例如 AT24C02)：

              ┌───────────────┐
3.3V ──→ VCC │ AT24C02       │
GND ───→ GND │               │
PF0 ←──→ SDA │ SDA           │ ← 4.7kΩ 上拉
PF1 ←──→ SCL │ SCL           │ ← 4.7kΩ 上拉
GND ───→ A0  │               │ → 地址 = 0x50
GND ───→ A1  │               │
GND ───→ A2  │               │
              └───────────────┘
```

---

## 13. 电源板接口

### 13.1 ROSbot XL 电源板通信

| 信号 | 引脚 | 方向 | 功能 |
|------|------|------|------|
| PB_SERIAL_TX | **PD5** | OUTPUT | 发送命令到电源板 |
| PB_SERIAL_RX | **PD6** | INPUT | 接收电源板响应 |
| PB_SHD_DETECT | **PD4** | INPUT (Pullup) | 检测关机按钮 (HIGH=按下) |
| PB_SHD_CONFIRM | **PD7** | OUTPUT | 确认关机 (HIGH=断电) |

```
STM32               电源板
PD5(TX) ──────────→ RX (38400 baud)
PD6(RX) ←──────────TX
PD4 ←──────────────SHUTDOWN_DETECT
PD7 ──────────────→ SHUTDOWN_CONFIRM

通信协议：< CC SS AA...AA CS >
  CC = 命令 (01=板信息, 02=电池状态)
  SS = 参数字节数
  AA = 参数（十六进制编码）
  CS = XOR 校验和
```

---

## 14. Ethernet 扩展板设计（可选）

### 14.1 设计理念

根据你的需求，Ethernet 做成**可选扩展板**形式，默认使用 Serial 通信。扩展板通过排针/排母与主板连接。

### 14.2 STM32F407 Ethernet MAC 引脚

STM32F407 内置 Ethernet MAC，需要外部 PHY 芯片（如 LAN8720A）或交换机芯片（原版使用 LAN9303）。

**RMII 接口引脚（最少引脚方案）**：

| 信号 | STM32 引脚 | 方向 | 说明 |
|------|-----------|------|------|
| ETH_RMII_REF_CLK | PA1 | INPUT | 50MHz 参考时钟 |
| ETH_RMII_MDIO | PA2 | I/O | 管理数据 |
| ETH_RMII_MDC | PC1 | OUTPUT | 管理时钟 |
| ETH_RMII_CRS_DV | PA7 | INPUT | 载波检测/数据有效 |
| ETH_RMII_RXD0 | PC4 | INPUT | 接收数据 0 |
| ETH_RMII_RXD1 | PC5 | INPUT | 接收数据 1 |
| ETH_RMII_TX_EN | PB11 | OUTPUT | 发送使能 |
| ETH_RMII_TXD0 | PB12 | OUTPUT | 发送数据 0 |
| ETH_RMII_TXD1 | PB13 | OUTPUT | 发送数据 1 |

> ⚠️ **引脚冲突警告**：
> - PA1 在 ROSbot 中被编码器 FR(TIM2_CH2) 使用
> - PA7 在 ROSbot 中被编码器 RL(TIM3_CH2) 使用
> - PB11 在 ROSbot 中被诊断串口(UART3_RX) 使用
> 
> **设计新板时需要重新规划引脚，避免 Ethernet 与编码器/串口冲突！**

### 14.3 扩展板连接器设计建议

```
主板侧排针（2×10 或 2×12）：

Pin  信号             说明
1    3.3V            电源
2    GND             地
3    PA1/ETH_CLK     RMII 参考时钟 (⚠️需避免编码器冲突)
4    PA2/ETH_MDIO    管理数据
5    PC1/ETH_MDC     管理时钟
6    PA7/ETH_CRS_DV  载波检测 (⚠️需避免编码器冲突)
7    PC4/ETH_RXD0    接收数据
8    PC5/ETH_RXD1    接收数据
9    PB11/ETH_TX_EN  发送使能 (⚠️需避免UART冲突)
10   PB12/ETH_TXD0   发送数据
11   PB13/ETH_TXD1   发送数据
12   nRESET/ETH_RST  PHY 复位

扩展板上：
┌──────────────────────────────────┐
│  LAN8720A (RMII PHY)            │
│  + RJ45 带变压器网口             │
│  + 50MHz 晶振                   │
│  + 指示 LED                     │
└──────────────────────────────────┘
```

### 14.4 ROSbot XL Ethernet 配置参考

| 参数 | 值 |
|------|-----|
| PHY/Switch | LAN9303 (三口交换机) |
| MAC 地址 | 02:47:00:00:00:01 |
| 板卡 IP | 192.168.77.3 |
| Agent IP | 192.168.77.2 |
| Agent 端口 | 8888 (UDP) |
| 协议 | micro-ROS over UDP |

### 14.5 软件支持

Ethernet 相关的编译标志：
```ini
# platformio.ini 中添加
build_flags =
    -D ETHERNET_USE_FREERTOS
    -D LAN9303  # 或 -D LAN8720 根据你选的 PHY
```

固件库依赖：
```ini
lib_deps =
    https://github.com/husarion/STM32Ethernet#1.0.0
    https://github.com/stm32duino/LwIP#2.2.1
```

---

## 15. 调试接口

### 15.1 SWD 调试

| 信号 | STM32 引脚 | 说明 |
|------|-----------|------|
| SWDIO | PA13 | 调试数据 |
| SWCLK | PA14 | 调试时钟 |
| SWO | PB3 | 可选追踪输出 |
| nRESET | NRST | 复位 |

```
SWD 调试接口（建议留出排针）：

┌──────┐
│ 3.3V │ ← 仅检测用，不从调试器供电
│ SWDIO│ ← PA13
│ SWCLK│ ← PA14
│ SWO  │ ← PB3 (可选)
│ nRST │ ← NRST
│ GND  │
└──────┘

调试工具：ST-Link V2
调试速度：4000 kHz
```

### 15.2 运行时统计定时器

| 参数 | 值 |
|------|-----|
| 定时器 | TIM5 (32-bit) |
| 预分频 | 1680 (168MHz/1680 = 100kHz) |
| 精度 | 10µs |
| 用途 | FreeRTOS 任务运行时间统计 |

---

## 16. 完整引脚分配表

### 16.1 ROSbot 引脚分配

| 引脚 | 功能 | 外设/模式 | 说明 |
|------|------|----------|------|
| **PA0** | ENC_FR_A | TIM2_CH1, AF1 | 前右编码器 A |
| **PA1** | ENC_FR_B | TIM2_CH2, AF1 | 前右编码器 B |
| **PA5** | BAT_ADC | ADC (10bit) | 电池电压采集 |
| **PA6** | IMU_INT | INPUT | BNO055 中断 |
| **PA7** | ENC_RL_B | TIM3_CH2, AF2 | 后左编码器 B |
| **PA8** | IMU_SCL | I2C (AF) | IMU I2C 时钟 |
| **PA9** | UART1_TX | UART1, AF7 | SBC 通信发送 |
| **PA10** | UART1_RX | UART1, AF7 | SBC 通信接收 |
| **PA13** | SWDIO | SWD | 调试 |
| **PA14** | SWCLK | SWD | 调试 |
| **PB1** | RANGE_FR_XSHUT | OUTPUT | 前右测距使能 |
| **PB4** | ENC_RL_A | TIM3_CH1, AF2 | 后左编码器 A |
| **PB6** | ENC_FL_A | TIM4_CH1, AF2 | 前左编码器 A |
| **PB7** | ENC_FL_B | TIM4_CH2, AF2 | 前左编码器 B |
| **PB8** | RANGE_SCL | I2C | 测距 I2C 时钟 |
| **PB9** | RANGE_SDA | I2C | 测距 I2C 数据 |
| **PB10** | UART3_TX | UART3, AF7 | 诊断串口发送 |
| **PB11** | UART3_RX | UART3, AF7 | 诊断串口接收 |
| **PC6** | ENC_RR_A | TIM8_CH1, AF3 | 后右编码器 A |
| **PC7** | ENC_RR_B | TIM8_CH2, AF3 | 后右编码器 B |
| **PC9** | IMU_SDA | I2C (AF) | IMU I2C 数据 |
| **PC13** | MOT_EN_R | OUTPUT | 右侧电机使能 |
| **PC14** | MOT_EN_L | OUTPUT | 左侧电机使能 |
| **PC15** | MOT_RL_INA | OUTPUT/INPUT | 后左电机方向 A |
| **PD3** | MOT_RR_INA | OUTPUT/INPUT | 后右电机方向 A |
| **PD4** | MOT_RR_INB | OUTPUT/INPUT | 后右电机方向 B |
| **PD8** | RANGE_FL_XSHUT | OUTPUT | 前左测距使能 |
| **PD9** | RANGE_RR_XSHUT | OUTPUT | 后右测距使能 |
| **PD10** | RANGE_RL_XSHUT | OUTPUT | 后左测距使能 |
| **PE0** | MOT_FAULT_R | INPUT | 右侧电机故障 |
| **PE1** | MOT_FAULT_L | INPUT | 左侧电机故障 |
| **PE2** | RED_LED | OUTPUT | 红色状态 LED |
| **PE3** | GRN_LED | OUTPUT | 绿色 LED |
| **PE4** | GRN_LED2 | OUTPUT | 绿色 LED 2 |
| **PE5** | MOT_FL_INA | OUTPUT/INPUT | 前左电机方向 A |
| **PE6** | MOT_FL_INB | OUTPUT/INPUT | 前左电机方向 B |
| **PF2** | MOT_RL_INB | OUTPUT/INPUT | 后左电机方向 B |
| **PF6** | MOT_FR_PWM | TIM PWM | 前右电机 PWM |
| **PF7** | MOT_RR_PWM | TIM PWM | 后右电机 PWM |
| **PF8** | MOT_RL_PWM | TIM PWM | 后左电机 PWM |
| **PF9** | MOT_FL_PWM | TIM PWM | 前左电机 PWM |
| **PG4** | IMU_POWER | OUTPUT | IMU 电源使能 |
| **PG5** | RPI_CONSOLE | - | 树莓派控制台 |
| **PG6** | SBC_STATUS | INPUT | SBC 连接检测 |
| **PG7** | RPI_BTN | - | 树莓派按钮 |
| **PG10** | MOT_FR_INA | OUTPUT/INPUT | 前右电机方向 A |
| **PG11** | MOT_FR_INB | OUTPUT/INPUT | 前右电机方向 B |
| **PG12** | PUSH_BUTTON1 | INPUT_PULLUP | 用户按钮 1 |
| **PG13** | PUSH_BUTTON2 | INPUT_PULLUP | 用户按钮 2 |

### 16.2 ROSbot XL 引脚分配

| 引脚 | 功能 | 外设/模式 | 说明 |
|------|------|----------|------|
| **PA4** | AUDIO_DAC | DAC Output | 音频 DAC 输出 |
| **PA9** | UART1_TX | UART1 | 诊断/备用串口 TX |
| **PA10** | UART1_RX | UART1 | 诊断/备用串口 RX |
| **PA13** | SWDIO | SWD | 调试 |
| **PA14** | SWCLK | SWD | 调试 |
| **PA15** | ENC_RL_A | TIM2_CH1, AF1 | 后左编码器 A |
| **PB0** | FAN_PWM | TIM8_CH2N, AF3 | 风扇 PWM (v1.2) |
| **PB1** | NTC_ADC | ADC | NTC 温度采集 |
| **PB2** | AUDIO_SHDN | OUTPUT | 音频使能 |
| **PB3** | ENC_RL_B | TIM2_CH2, AF1 | 后左编码器 B |
| **PB10** | LED_SCK | SPI SCK | LED 灯带时钟 |
| **PB14** | LED_MISO | SPI MISO | LED 灯带 (未用) |
| **PB15** | LED_MOSI | SPI MOSI | LED 灯带数据 |
| **PC6** | ENC_FR_A | TIM3_CH1, AF2 | 前右编码器 A |
| **PC7** | ENC_FR_B | TIM3_CH2, AF2 | 前右编码器 B |
| **PC13** | FAN_PP / MOT_EN_R | OUTPUT | 风扇常开(v1.1) / 右侧电机使能 |
| **PC14** | MOT_EN_L | OUTPUT | 左侧电机使能 |
| **PD4** | PB_SHD_DETECT | INPUT_PULLUP | 电源板关机检测 |
| **PD5** | PB_SERIAL_TX | UART TX | 电源板串口发送 |
| **PD6** | PB_SERIAL_RX | UART RX | 电源板串口接收 |
| **PD7** | PB_SHD_CONFIRM | OUTPUT | 电源板关机确认 |
| **PD10** | MOT_FL_INA | OUTPUT/INPUT | 前左电机方向 A |
| **PD11** | MOT_FL_INB | OUTPUT/INPUT | 前左电机方向 B |
| **PD12** | ENC_FL_A | TIM4_CH1, AF2 | 前左编码器 A |
| **PD13** | ENC_FL_B | TIM4_CH2, AF2 | 前左编码器 B |
| **PD14** | ILIM4 | OUTPUT/INPUT | RR 电机电流限制 |
| **PE0** | MOT_FAULT_R | INPUT | 右侧电机故障 |
| **PE1** | MOT_FAULT_L | INPUT | 左侧电机故障 |
| **PE3** | GRN_LED | OUTPUT | 绿色 LED |
| **PE4** | RED_LED | OUTPUT | 红色状态 LED |
| **PE9** | ENC_RR_A | TIM1_CH1, AF1 | 后右编码器 A |
| **PE10** | ILIM1 | OUTPUT/INPUT | FL 电机电流限制 |
| **PE11** | ENC_RR_B | TIM1_CH2, AF1 | 后右编码器 B |
| **PE12** | MOT_RR_INA | OUTPUT/INPUT | 后右电机方向 A |
| **PE13** | MOT_RR_INB | OUTPUT/INPUT | 后右电机方向 B |
| **PF0** | I2C_SDA | I2C | 共享 I2C 数据 (IMU + EEPROM) |
| **PF1** | I2C_SCL | I2C | 共享 I2C 时钟 |
| **PF2** | IMU_INT | INPUT | BNO055 中断 |
| **PF6** | MOT_RR_PWM | TIM PWM | 后右电机 PWM |
| **PF7** | MOT_RL_PWM | TIM PWM | 后左电机 PWM |
| **PF8** | MOT_FR_PWM | TIM PWM | 前右电机 PWM |
| **PF9** | MOT_FL_PWM | TIM PWM | 前左电机 PWM |
| **PF11** | PUSH_BUTTON1 | INPUT_PULLUP | 用户按钮 |
| **PF12** | PUSH_BUTTON2 | INPUT_PULLUP | MCU 复位按钮 |
| **PF13** | EN_LOC_5V | OUTPUT | 本地 5V 使能 |
| **PG5** | MOT_FR_INA | OUTPUT/INPUT | 前右电机方向 A |
| **PG6** | MOT_FR_INB | OUTPUT/INPUT | 前右电机方向 B |
| **PG7** | ILIM3 | OUTPUT/INPUT | RL 电机电流限制 |
| **PG11** | MOT_RL_INA | OUTPUT/INPUT | 后左电机方向 A |
| **PG12** | MOT_RL_INB | OUTPUT/INPUT | 后左电机方向 B |
| **PG15** | ILIM2 | OUTPUT/INPUT | FR 电机电流限制 |

---

## 17. 定时器资源分配

### 17.1 ROSbot

| 定时器 | 用途 | 通道 | 引脚 |
|--------|------|------|------|
| TIM2 | 编码器 FR | CH1+CH2 | PA0, PA1 |
| TIM3 | 编码器 RL | CH1+CH2 | PB4, PA7 |
| TIM4 | 编码器 FL | CH1+CH2 | PB6, PB7 |
| TIM5 | RTOS 统计 | - | (内部) |
| TIM8 | 编码器 RR | CH1+CH2 | PC6, PC7 |
| TIMx* | 电机 PWM | 各1通道 | PF6-PF9 |

> *电机 PWM 的定时器由 Arduino HAL 自动根据引脚分配

### 17.2 ROSbot XL

| 定时器 | 用途 | 通道 | 引脚 |
|--------|------|------|------|
| TIM1 | 编码器 RR | CH1+CH2 | PE9, PE11 |
| TIM2 | 编码器 RL | CH1+CH2 | PA15, PB3 |
| TIM3 | 编码器 FR | CH1+CH2 | PC6, PC7 |
| TIM4 | 编码器 FL | CH1+CH2 | PD12, PD13 |
| TIM5 | RTOS 统计 | - | (内部) |
| TIM8 | 风扇 PWM | CH2N | PB0 |
| TIMx* | 电机 PWM | 各1通道 | PF6-PF9 |

---

## 18. I2C 总线分配

### 18.1 ROSbot (两条独立 I2C 总线)

| 总线 | SDA | SCL | 频率 | 设备 |
|------|-----|-----|------|------|
| IMU I2C | PC9 | PA8 | 400kHz | BNO055 (0x29) |
| Range I2C | PB9 | PB8 | 400kHz | VL53L0X ×4 (0x30-0x33) |

### 18.2 ROSbot XL (共享一条 I2C 总线)

| 总线 | SDA | SCL | 频率 | 设备 |
|------|-----|-----|------|------|
| 共享 I2C | PF0 | PF1 | 400kHz | BNO055 (0x29), EEPROM (0x50) |

---

## 19. 新板设计建议

### 19.1 推荐的最小系统方案

以下是为你的新 ROS 板推荐的最小化设计：

```
┌─────────────────────── 新 ROS 板 设计 ──────────────────────┐
│                                                              │
│  MCU: STM32F407VET6 (LQFP-100, 够用且更便宜)                │
│                                                              │
│  核心功能：                                                   │
│  ├── UART1 (TX/RX) → USB 转串口 → ROS 主机                  │
│  ├── UART2/3 → 调试串口 (可选)                               │
│  ├── DRV8848 ×2 → 4路电机驱动                               │
│  ├── TIM1/2/3/4(编码器模式) → 4路正交编码器                   │
│  ├── BNO055/ICM-20948 (I2C) → IMU                           │
│  ├── ADC → 电池电压采集                                      │
│  ├── GPIO → LED + 按钮                                      │
│  └── SWD → 调试接口                                         │
│                                                              │
│  扩展接口（排针留出）：                                       │
│  ├── Ethernet 扩展板接口 (RMII 信号)                         │
│  ├── I2C 扩展 (测距传感器/其他)                              │
│  ├── SPI 扩展 (LED 灯带/其他)                               │
│  └── 额外 UART (电源板/其他)                                │
└──────────────────────────────────────────────────────────────┘
```

### 19.2 引脚规划建议（新板）

**关键约束**：
1. 编码器需要 4 个定时器，每个用 CH1+CH2
2. 电机 PWM 需要 4 个 PWM 通道（不能与编码器定时器冲突）
3. 如果预留 Ethernet，需避免 RMII 引脚与编码器冲突
4. I2C 上拉电阻必须外置（4.7kΩ 到 3.3V）

**建议的引脚分配**（如果用 STM32F407VET6 LQFP-100）：

| 功能 | 建议引脚 | 说明 |
|------|---------|------|
| 编码器 FL | PD12/PD13 (TIM4) | 避开 Ethernet 引脚 |
| 编码器 FR | PA0/PA1 (TIM5 或 TIM2) | TIM5建议用32位 |
| 编码器 RL | PB4/PB5 (TIM3) | |
| 编码器 RR | PE9/PE11 (TIM1) | |
| 电机 PWM ×4 | PB6/PB7/PB8/PB9 | 确认定时器通道可用 |
| UART ROS | PA9/PA10 (UART1) | 921600 baud |
| UART Debug | PB10/PB11 (UART3) | 可选 |
| I2C IMU | PB6/PB7 或 PF0/PF1 | 与编码器不冲突 |
| ADC 电池 | PA5 | |
| LED | PE2/PE3 | |
| 按钮 | PE4/PE5 | INPUT_PULLUP |
| SWD | PA13/PA14 | 保留不动 |

> ⚠️ 以上仅为建议，实际设计时需查阅 STM32F407 数据手册的 Alternate Function 映射表，
> 确认每个引脚的 AF 功能是否匹配你选择的外设。

### 19.3 PCB 布局建议

1. **电源部分**与数字/模拟电路隔离，电机驱动部分走粗线（2A+ 电流）
2. **去耦电容**：每个 VCC 引脚 100nF，电源入口 10µF+100nF
3. **I2C 路由**：尽量短，远离电机驱动走线（EMI 干扰）
4. **晶振布局**：靠近 MCU，周围铺地
5. **Ethernet 扩展板接口**：放在板边缘，排针间距建议 2.54mm
6. **USB 转串口芯片**：放在板边缘靠近 USB 接口

### 19.4 BOM 参考清单（最小系统）

| 器件 | 型号推荐 | 数量 | 说明 |
|------|---------|------|------|
| MCU | STM32F407VET6 | 1 | LQFP-100 |
| 电机驱动 | DRV8848PWPR | 2 | 双路H桥 |
| IMU | BNO055 或 ICM-20948 | 1 | 九轴 |
| USB转串口 | CH340G 或 CP2102 | 1 | USB-C |
| 晶振 | 8MHz + 32.768kHz | 2 | 主晶振+RTC |
| LDO | AMS1117-3.3 | 1 | 或 DC-DC |
| EEPROM | AT24C02 | 1 | 可选 |
| 按钮 | 6×6mm 轻触 | 2 | |
| LED | 0805 红+绿 | 3 | |
| 电阻 | 0805 混合 | 若干 | |
| 电容 | 0805 混合 | 若干 | |
| SWD 排针 | 2×3 1.27mm | 1 | |
| 排针 | 2.54mm | 若干 | 扩展接口 |

---

> 📝 **本文档基于源码逆向分析生成，未经过实物验证。**
> **设计 PCB 前请务必查阅 STM32F407 数据手册和各芯片的应用手册确认引脚功能。**
> **电路设计需经过专业人员审核后再投板。**
