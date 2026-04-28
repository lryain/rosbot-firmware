# ROSbot XL 引脚分配精确文档

> 基于 `include/rosbot_xl/config.hpp` 源码和 STM32F407LGT6 (LQFP-144) 数据手册的精确引脚映射。

---

## 1. MCU 信息

| 参数 | 值 |
|------|------|
| 芯片 | STM32F407LGT6 |
| 封装 | LQFP-144 |
| 内核 | ARM Cortex-M4F @ 168 MHz |
| Flash | 1 MB |
| SRAM | 192 KB |
| GPIO 端口 | PA-PG (7 端口, 最多 114 GPIO) |

---

## 2. 完整引脚分配表

### 2.1 编码器 — 硬件定时器编码器模式

| 功能 | 引脚 | AF | 定时器 | 电机 | 方向反转 |
|------|------|-----|--------|------|---------|
| ENC_FL_A | PD12 | AF2 | TIM4_CH1 | 前左 | ✅ |
| ENC_FL_B | PD13 | AF2 | TIM4_CH2 | 前左 | |
| ENC_FR_A | PC6 | AF2 | TIM3_CH1 | 前右 | ❌ |
| ENC_FR_B | PC7 | AF2 | TIM3_CH2 | 前右 | |
| ENC_RL_A | PA15 | AF1 | TIM2_CH1 | 后左 | ✅ |
| ENC_RL_B | PB3 | AF1 | TIM2_CH2 | 后左 | |
| ENC_RR_A | PE9 | AF1 | TIM1_CH1 | 后右 | ❌ |
| ENC_RR_B | PE11 | AF1 | TIM1_CH2 | 后右 | |

**编码器参数**：
- CPR: 64 (每转计数)
- 齿轮比: 50:1
- 每转总脉冲: 3200
- 低通滤波 α: 0.05

### 2.2 电机驱动 — PWM + 方向控制

| 功能 | 引脚 | AF/模式 | 定时器 | 电机 |
|------|------|---------|--------|------|
| MOT_FL_PWM | PF9 | AF9 | TIM14_CH1 | 前左 |
| MOT_FL_IN_A | PD10 | GPIO | — | 前左 |
| MOT_FL_IN_B | PD11 | GPIO | — | 前左 |
| MOT_FR_PWM | PF8 | AF9 | TIM13_CH1 | 前右 |
| MOT_FR_IN_A | PG5 | GPIO | — | 前右 |
| MOT_FR_IN_B | PG6 | GPIO | — | 前右 |
| MOT_RL_PWM | PF7 | AF3 | TIM11_CH1 | 后左 |
| MOT_RL_IN_A | PG11 | GPIO | — | 后左 |
| MOT_RL_IN_B | PG12 | GPIO | — | 后左 |
| MOT_RR_PWM | PF6 | AF3 | TIM10_CH1 | 后右 |
| MOT_RR_IN_A | PE12 | GPIO | — | 后右 |
| MOT_RR_IN_B | PE13 | GPIO | — | 后右 |

**电机驱动参数**：
- PWM 频率: 20 kHz
- 最大速度: 22.0 rad/s
- 最小速度(死区): 0.5 rad/s

### 2.3 电机驱动控制信号

| 功能 | 引脚 | 模式 | 说明 |
|------|------|------|------|
| RIGHT_nSLEEP | PC13 | OUTPUT | 右侧驱动组使能（也是 FAN_PP） |
| RIGHT_nFAULT | PE0 | INPUT_PULLUP | 右侧驱动组故障 |
| LEFT_nSLEEP | PC14 | OUTPUT | 左侧驱动组使能 |
| LEFT_nFAULT | PE1 | INPUT_PULLUP | 左侧驱动组故障 |

### 2.4 电流限制 (ILIM) — MAX22205

| 功能 | 引脚 | 模式 | 说明 |
|------|------|------|------|
| ILIM1 | PE10 | OUTPUT/INPUT | FL 电流限制 |
| ILIM2 | PG15 | OUTPUT/INPUT | FR 电流限制 |
| ILIM3 | PG7 | OUTPUT/INPUT | RL 电流限制 |
| ILIM4 | PD14 | OUTPUT/INPUT | RR 电流限制 |

- V1.1 板：设为 OUTPUT HIGH（最大电流）
- V1.2 板：设为 INPUT（使用外部电阻设定）

### 2.5 IMU — BNO055

| 功能 | 引脚 | AF | 说明 |
|------|------|-----|------|
| IMU_SDA | PF0 | I2C2_SDA | I2C 数据线（共享 I2C 总线） |
| IMU_SCL | PF1 | I2C2_SCL | I2C 时钟线（共享 I2C 总线） |
| IMU_INT | PF2 | GPIO/EXTI | 中断输入（代码中未实际使用中断） |

**IMU 参数**：
- I2C 地址: 0x29
- I2C 速率: 400 kHz
- 传感器 ID: 0x37
- 轴映射: REMAP_CONFIG_P1
- 轴符号: REMAP_SIGN_P2

### 2.6 EEPROM — I2C

| 功能 | 引脚 | AF | 说明 |
|------|------|-----|------|
| EEPROM_SDA | PF0 | I2C2_SDA | 与 IMU 共享总线 |
| EEPROM_SCL | PF1 | I2C2_SCL | 与 IMU 共享总线 |

**EEPROM 参数**：
- I2C 地址: 0x50
- 页面大小: 16 字节
- 写入延迟: 5 ms

### 2.7 LED 指示灯

| 功能 | 引脚 | 模式 | 说明 |
|------|------|------|------|
| RED_LED | PE4 | OUTPUT | 状态指示（初始高电平） |
| GRN_LED | PE3 | OUTPUT | 绿色 LED / ROS subscriber 控制 |

### 2.8 LED Strip — SPI

| 功能 | 引脚 | AF | 说明 |
|------|------|-----|------|
| STRIP_MOSI | PB15 | SPI2 | 数据输出 |
| STRIP_MISO | PB14 | SPI2 | 数据输入（未实际使用） |
| STRIP_SCK | PB10 | SPI2 | 时钟 |

**LED Strip 参数**：
- SPI 速率: 4 MHz
- 位序: MSB First
- SPI 模式: MODE3 (CPOL=1, CPHA=1)
- LED 数量: 18

### 2.9 风扇控制

| 功能 | 引脚 | AF | 说明 |
|------|------|-----|------|
| FAN_PP | PC13 | GPIO | V1.1 板：始终开启（与 RIGHT_nSLEEP 复用） |
| FAN_PWM | PB0 | AF3 | V1.2 板：TIM8_CH2N (互补通道) |

**风扇 PWM 参数**（V1.2）：
- PWM 频率: 25 kHz
- 低温阈值: 30°C（最低占空比 20%）
- 高温阈值: 55°C（最高占空比 100%）

### 2.10 NTC 温度传感器

| 功能 | 引脚 | 模式 | 说明 |
|------|------|------|------|
| NTC_ADC | PB1 | ANALOG | 温度传感器模拟输入 |

**NTC 参数**：
- 上拉电阻: 5230Ω
- Steinhart-Hart 系数：C1=1.112613927e-03, C2=2.37277392e-04, C3=7.1670e-08
- 校准偏移: +3°C

### 2.11 电源板通信

| 功能 | 引脚 | AF | 说明 |
|------|------|-----|------|
| PB_TX | PD5 | USART2_TX | 电源板 UART 发送 |
| PB_RX | PD6 | USART2_RX | 电源板 UART 接收 |
| PB_SHD_DETECT | PD4 | INPUT_PULLUP | 关机检测 |
| PB_SHD_CONFIRM | PD7 | OUTPUT | 关机确认 |

**电源板参数**：
- 波特率: 38400
- 超时: 100 ms

### 2.12 音频

| 功能 | 引脚 | 模式 | 说明 |
|------|------|------|------|
| AUDIO_SHDN | PB2 | OUTPUT | 音频放大器使能（高电平开启） |
| AUDIO_DAC_OUT | PA4 | ANALOG | DAC 输出 |

### 2.13 按钮

| 功能 | 引脚 | 模式 | 说明 |
|------|------|------|------|
| PUSH_BUTTON1 | PF11 | INPUT_PULLUP | 用户按钮 1 |
| PUSH_BUTTON2 | PF12 | INPUT_PULLUP | MCU 复位按钮 |

### 2.14 Ethernet

| 功能 | 引脚 | AF | 说明 |
|------|------|-----|------|
| ETH_RMII_REF_CLK | PA1 | AF11 | 参考时钟 50MHz |
| ETH_RMII_MDIO | PA2 | AF11 | 管理数据 I/O |
| ETH_RMII_CRS_DV | PA7 | AF11 | 载波检测/数据有效 |
| ETH_RMII_MDC | PC1 | AF11 | 管理数据时钟 |
| ETH_RMII_RXD0 | PC4 | AF11 | 接收数据 0 |
| ETH_RMII_RXD1 | PC5 | AF11 | 接收数据 1 |
| ETH_RMII_TX_EN | PB11 | AF11 | 发送使能 |
| ETH_RMII_TXD0 | PB12 | AF11 | 发送数据 0 |
| ETH_RMII_TXD1 | PB13 | AF11 | 发送数据 1 |

> 注意：以太网 RMII 引脚在代码中未直接配置，由 STM32Ethernet 库在 `Ethernet.begin()` 时自动初始化。上述为 STM32F407 标准 RMII 引脚映射。

**Ethernet 参数**：
- MAC: 02:47:00:00:00:01
- 客户端 IP: 192.168.77.3
- Agent IP: 192.168.77.2
- Agent 端口: 8888
- PHY: LAN9303（交换机芯片）

### 2.15 SBC 通信串口

| 功能 | 引脚 | AF | 说明 |
|------|------|-----|------|
| FTDI_TX | PA9 | USART1_TX | 诊断/备用串口发送 |
| FTDI_RX | PA10 | USART1_RX | 诊断/备用串口接收 |

**串口参数**：
- 波特率: 921600
- 超时: 1 ms

### 2.16 外设电源控制

| 功能 | 引脚 | 模式 | 说明 |
|------|------|------|------|
| EN_LOC_5V | PF13 | OUTPUT | 本地 5V 使能（高电平开启） |

---

## 3. 定时器分配总览

| 定时器 | 类型 | 功能 | 引脚 | 备注 |
|--------|------|------|------|------|
| TIM1 | 高级 | 编码器 RR | PE9/PE11 | 不可它用 |
| TIM2 | 通用32位 | 编码器 RL | PA15/PB3 | 不可它用 |
| TIM3 | 通用 | 编码器 FR | PC6/PC7 | 不可它用 |
| TIM4 | 通用 | 编码器 FL | PD12/PD13 | 不可它用 |
| TIM5 | 通用32位 | 运行时统计 | 无引脚 | FreeRTOS 内部 |
| TIM8 | 高级 | 风扇 PWM | PB0 (CH2N) | CH1/CH3/CH4 可用 |
| TIM10 | 基本16位 | 电机 RR PWM | PF6 | |
| TIM11 | 基本16位 | 电机 RL PWM | PF7 | |
| TIM13 | 基本16位 | 电机 FR PWM | PF8 | |
| TIM14 | 基本16位 | 电机 FL PWM | PF9 | |
| TIM6 | 基本 | **空闲** | 无 | DAC 触发可用 |
| TIM7 | 基本 | **空闲** | 无 | 内部用途 |
| TIM9 | 通用 | **空闲** | PE5/PE6 (CH1/CH2) | 2 通道可用 |
| TIM12 | 通用 | **空闲** | PB14/PB15 (CH1/CH2) | ⚠️ 与 SPI2 冲突 |

---

## 4. I2C 总线分配

| 总线 | 引脚 | 设备 | 地址 | 速率 |
|------|------|------|------|------|
| I2C2 (PF0/PF1) | PF0=SDA, PF1=SCL | BNO055 IMU | 0x29 | 400kHz |
| I2C2 (PF0/PF1) | PF0=SDA, PF1=SCL | EEPROM | 0x50 | 400kHz |

---

## 5. UART 分配

| 外设 | 引脚 | 波特率 | 用途 |
|------|------|--------|------|
| USART1 | PA9(TX)/PA10(RX) | 921600 | 诊断串口/备用 ROS 通信 |
| USART2 | PD5(TX)/PD6(RX) | 38400 | 电源板通信 |

---

## 6. ADC 分配

| 引脚 | ADC 通道 | 用途 |
|------|---------|------|
| PB1 | ADC1_CH9/ADC2_CH9 | NTC 温度传感器 |
| PA4 | DAC_OUT1 | 音频 DAC 输出 |

---

## 7. 引脚冲突分析与建议

### 7.1 已知冲突

| 冲突 | 引脚 | 说明 |
|------|------|------|
| PC13 双用 | PC13 | RIGHT_nSLEEP 和 FAN_PP 共享。V1.2 风扇用 PB0(TIM8)，PC13 仅作 nSLEEP |
| PB14/PB15 | PB14/PB15 | LED Strip SPI 与 TIM12 通道冲突（如需 TIM12 PWM，需移除 LED Strip SPI） |

### 7.2 优化建议

1. **PC13 分离**：建议风扇和电机使能使用独立引脚，避免功能耦合
2. **I2C 扩展**：所有 I2C 设备共享一条总线，建议预留第二条 I2C 以支持扩展
3. **SBC 串口**：仅有一路诊断串口（USART1），ROSbot 有双串口（USART1 + USART3），建议保留
4. **ADC 预留**：当前仅用 2 路 ADC（NTC + DAC），DRV8874 IPROPI 需要额外 4 路 + 电池 ADC 1 路
5. **编码器定时器**：4 个高级/通用定时器全部用于编码器，这是最优分配（硬件编码器模式最精确）

### 7.3 空闲引脚一览

STM32F407LGT6 (LQFP-144) 有约 114 个 GPIO，以上代码仅使用约 55 个，还有大量引脚可用于扩展：

**空闲且推荐的 PWM 引脚**：
- PE5 (TIM9_CH1), PE6 (TIM9_CH2)
- PA2 (TIM5_CH3 / TIM9_CH1 — 注意 TIM5 已用, 但 AF3=TIM9 可用)
- PA3 (TIM5_CH4 / TIM9_CH2)

**空闲 ADC 引脚**：
- PA5 (ADC1_CH5/ADC2_CH5 / DAC_OUT2)
- PC0-PC3 (ADC1_CH10-CH13)
- PF3-PF5 (ADC3_CH9, CH14, CH15)
- PF10 (ADC3_CH8)

**空闲 UART**：
- USART3: PB10(TX)/PB11(RX) — ⚠️ PB10 用于 SPI SCK, PB11 用于 ETH
- USART6: PC6(TX)/PC7(RX) — ⚠️ 用于编码器
- UART4: PA0(TX)/PA1(RX) — ⚠️ PA1 用于 ETH

---

## 8. 引脚按端口汇总

### PA 端口
| 引脚 | 功能 | 模式 |
|------|------|------|
| PA1 | ETH_RMII_REF_CLK | AF11 |
| PA2 | ETH_RMII_MDIO | AF11 |
| PA4 | AUDIO_DAC_OUT | ANALOG |
| PA7 | ETH_RMII_CRS_DV | AF11 |
| PA9 | FTDI_TX (USART1) | AF7 |
| PA10 | FTDI_RX (USART1) | AF7 |
| PA15 | ENC_RL_A (TIM2_CH1) | AF1 |

### PB 端口
| 引脚 | 功能 | 模式 |
|------|------|------|
| PB0 | FAN_PWM (TIM8_CH2N) | AF3 |
| PB1 | NTC_ADC | ANALOG |
| PB2 | AUDIO_SHDN | OUTPUT |
| PB3 | ENC_RL_B (TIM2_CH2) | AF1 |
| PB10 | STRIP_SCK (SPI2) | AF5 |
| PB11 | ETH_RMII_TX_EN | AF11 |
| PB12 | ETH_RMII_TXD0 | AF11 |
| PB13 | ETH_RMII_TXD1 | AF11 |
| PB14 | STRIP_MISO (SPI2) | AF5 |
| PB15 | STRIP_MOSI (SPI2) | AF5 |

### PC 端口
| 引脚 | 功能 | 模式 |
|------|------|------|
| PC1 | ETH_RMII_MDC | AF11 |
| PC4 | ETH_RMII_RXD0 | AF11 |
| PC5 | ETH_RMII_RXD1 | AF11 |
| PC6 | ENC_FR_A (TIM3_CH1) | AF2 |
| PC7 | ENC_FR_B (TIM3_CH2) | AF2 |
| PC13 | RIGHT_nSLEEP / FAN_PP | OUTPUT |
| PC14 | LEFT_nSLEEP | OUTPUT |

### PD 端口
| 引脚 | 功能 | 模式 |
|------|------|------|
| PD4 | PB_SHD_DETECT | INPUT_PULLUP |
| PD5 | PB_TX (USART2) | AF7 |
| PD6 | PB_RX (USART2) | AF7 |
| PD7 | PB_SHD_CONFIRM | OUTPUT |
| PD10 | MOT_FL_IN_A | GPIO |
| PD11 | MOT_FL_IN_B | GPIO |
| PD12 | ENC_FL_A (TIM4_CH1) | AF2 |
| PD13 | ENC_FL_B (TIM4_CH2) | AF2 |
| PD14 | ILIM4 (RR) | OUTPUT/INPUT |

### PE 端口
| 引脚 | 功能 | 模式 |
|------|------|------|
| PE0 | RIGHT_nFAULT | INPUT_PULLUP |
| PE1 | LEFT_nFAULT | INPUT_PULLUP |
| PE3 | GRN_LED | OUTPUT |
| PE4 | RED_LED | OUTPUT |
| PE9 | ENC_RR_A (TIM1_CH1) | AF1 |
| PE10 | ILIM1 (FL) | OUTPUT/INPUT |
| PE11 | ENC_RR_B (TIM1_CH2) | AF1 |
| PE12 | MOT_RR_IN_A | GPIO |
| PE13 | MOT_RR_IN_B | GPIO |

### PF 端口
| 引脚 | 功能 | 模式 |
|------|------|------|
| PF0 | I2C_SDA (I2C2) | AF4 |
| PF1 | I2C_SCL (I2C2) | AF4 |
| PF2 | IMU_INT | INPUT |
| PF6 | MOT_RR_PWM (TIM10) | AF3 |
| PF7 | MOT_RL_PWM (TIM11) | AF3 |
| PF8 | MOT_FR_PWM (TIM13) | AF9 |
| PF9 | MOT_FL_PWM (TIM14) | AF9 |
| PF11 | PUSH_BUTTON1 | INPUT_PULLUP |
| PF12 | PUSH_BUTTON2 | INPUT_PULLUP |
| PF13 | EN_LOC_5V | OUTPUT |

### PG 端口
| 引脚 | 功能 | 模式 |
|------|------|------|
| PG5 | MOT_FR_IN_A | GPIO |
| PG6 | MOT_FR_IN_B | GPIO |
| PG7 | ILIM3 (RL) | OUTPUT/INPUT |
| PG11 | MOT_RL_IN_A | GPIO |
| PG12 | MOT_RL_IN_B | GPIO |
| PG15 | ILIM2 (FR) | OUTPUT/INPUT |
