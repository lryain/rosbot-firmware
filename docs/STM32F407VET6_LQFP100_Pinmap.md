# STM32F407VET6 LQFP-100 完整引脚映射参考手册

> **版本**: V1.0  
> **日期**: 2026-04-13  
> **芯片**: STM32F407VET6 (LQFP-100)  
> **数据来源**: ST 官方数据手册 DS8626 + `framework-arduinoststm32/variants/STM32F4xx/F407V(E-G)T_F417V(E-G)T/PeripheralPins.c`  
> **关键区别（vs STM32F407ZET6 LQFP-144）**: 无 Port F/G，无 Ethernet MAC

---

## 目录

1. [芯片概述与与 ZET6 关键差异](#1-芯片概述与-zet6-关键差异)
2. [LQFP-100 引脚编号速查](#2-lqfp-100-引脚编号速查)
3. [GPIO 端口 AF 功能完整表](#3-gpio-端口-af-功能完整表)
4. [外设引脚汇总（按外设分类）](#4-外设引脚汇总按外设分类)
5. [冲突分析与设计约束](#5-冲突分析与设计约束)
6. [ROSbot Lite 最优引脚分配方案](#6-rosbot-lite-最优引脚分配方案)

---

## 1. 芯片概述与 ZET6 关键差异

| 参数 | STM32F407VET6 (本文) | STM32F407ZET6 (PRO) |
|------|---------------------|---------------------|
| 封装 | **LQFP-100** | LQFP-144 |
| GPIO 端口 | **PA~PE**（无 PF/PG！） | PA~PG |
| GPIO I/O 数量 | **约 82 引脚** | 约 114 引脚 |
| 以太网 MAC | **❌ 无** | ✅ 有 |
| Flash / SRAM | 512KB / 192KB | 512KB / 192KB |
| 内核频率 | 168 MHz | 168 MHz |
| ADC | ADC1/2/3（部分通道受限） | ADC1/2/3（完整） |
| USB OTG FS | ✅ | ✅ |
| CAN | CAN1/CAN2 ✅ | CAN1/CAN2 ✅ |
| SPI | SPI1/2/3 ✅ | SPI1/2/3 ✅ |
| I2C | I2C1/2/3 ✅ | I2C1/2/3 ✅ |
| UART | USART1/2/3, UART4/5, USART6 ✅ | 同 ✅ |
| 定时器 | TIM1-14 ✅ | TIM1-14 ✅ |

### ⚠️ 关键约束（无 Port F/G）

以下在 ZET6 PRO 设计中使用的引脚在 VET6 LQFP-100 中**完全不可用**：
- `PF0/PF1` → I2C2_SDA/SCL（IMU）→ **改用 PB10/PB11**
- `PF2` → IMU INT → **改用其他 GPIOk**
- `PF3/PF4/PF5/PF10` → IPROPI ADC3 → **改用 PA/PB/PC ADC 引脚**
- `PF6/PF7` → TIM10/TIM11 Motor FL → **改用 PB8/PB9**
- `PF8/PF9` → TIM13/TIM14 Motor FR → **改用 PA6/PA7**
- `PF11/PF12` → Buttons → **改用 PC/PD GPIO**
- `PF13` → EN_LOC_5V → **改用空闲 GPIO**
- `PG9/PG10` → nSLEEP → **改用 PC/PD GPIO**
- `PG11/PG13/PG14` → ETH TX → **无需替换（无 ETH）**

---

## 2. LQFP-100 引脚编号速查

LQFP-100：每侧 25 引脚，从封装缺口/圆点逆时针编号。

| 引脚# | GPIO | 引脚# | GPIO | 引脚# | GPIO | 引脚# | GPIO |
|:---:|------|:---:|------|:---:|------|:---:|------|
| 1 | PE2 | 26 | PA5 | 51 | PD14 | 76 | PB7 |
| 2 | PE3 | 27 | PA6 | 52 | PD15 | 77 | BOOT0 |
| 3 | PE4 | 28 | PA7 | 53 | PG2\* | 78 | PB8 |
| 4 | PE5 | 29 | PC4 | 54 | PG3\* | 79 | PB9 |
| 5 | PE6 | 30 | PC5 | 55 | PG4\* | 80 | PE0 |
| 6 | VBAT | 31 | PB0 | 56 | PG5\* | 81 | PE1 |
| 7 | PC13 | 32 | PB1 | 57 | PG6\* | 82 | NRST |
| 8 | PC14 | 33 | PB2 | 58 | PG7\* | 83 | PC0 |
| 9 | PC15 | 34 | PE7 | 59 | PG8\* | 84 | PC1 |
| 10 | PH0 | 35 | PE8 | 60 | PC6 | 85 | PC2 |
| 11 | PH1 | 36 | PE9 | 61 | PC7 | 86 | PC3 |
| 12 | NRST | 37 | PE10 | 62 | PC8 | 87 | PA0 |
| 13 | PC0 | 38 | PE11 | 63 | PC9 | 88 | PA1 |
| 14 | PC1 | 39 | PE12 | 64 | PA8 | 89 | PA2 |
| 15 | PC2 | 40 | PE13 | 65 | PA9 | 90 | PA3 |
| 16 | PC3 | 41 | PE14 | 66 | PA10 | 91 | PA4 |
| 17 | PA0 | 42 | PE15 | 67 | PA11 | 92 | PA5 |
| 18 | PA1 | 43 | PB10 | 68 | PA12 | 93 | PA6 |
| 19 | PA2 | 44 | PB11 | 69 | PA13 | 94 | PA7 |
| 20 | PA3 | 45 | VSS | 70 | PA14 | 95 | PC4 |
| 21 | PA4 | 46 | VDD | 71 | PA15 | 96 | PC5 |
| 22 | PA5 | 47 | PD8 | 72 | PC10 | 97 | PB0 |
| 23 | PA6 | 48 | PD9 | 73 | PC11 | 98 | PB1 |
| 24 | PA7 | 49 | PD10 | 74 | PC12 | 99 | PB2 |
| 25 | PC4 | 50 | PD11 | 75 | PD0 | 100 | PE7 |

> \* PG 引脚**在 LQFP-100 封装中不被引出**，以上编号仅作参考。实际可用 GPIO 为 PA~PE。

**常用引脚 LQFP-100 实际位置（DS8626 权威）**：

| 引脚# | GPIO | 常见用途 |
|:---:|------|---------|
| 36 | PE9 | Encoder RR A (TIM1_CH1) |
| 38 | PE11 | Encoder RR B (TIM1_CH2) |
| 43 | PB10 | I2C2_SCL / USART3_TX |
| 44 | PB11 | I2C2_SDA / USART3_RX |
| 47 | PD8 | USART3_TX 备选 |
| 48 | PD9 | USART3_RX 备选 |
| 60 | PC6 | TIM3_CH1 / TIM8_CH1 |
| 61 | PC7 | TIM3_CH2 / TIM8_CH2 |
| 62 | PC8 | TIM3_CH3 / TIM8_CH3 |
| 63 | PC9 | TIM3_CH4 / TIM8_CH4 / I2C3_SDA |
| 64 | PA8 | I2C3_SCL / TIM1_CH1 |
| 65 | PA9 | USART1_TX |
| 66 | PA10 | USART1_RX |
| 67 | PA11 | CAN1_RX / OTG_FS_DM |
| 68 | PA12 | CAN1_TX / OTG_FS_DP |
| 78 | PB8 | TIM10_CH1 / I2C1_SCL |
| 79 | PB9 | TIM11_CH1 / I2C1_SDA |

---

## 3. GPIO 端口 AF 功能完整表

> 仅列出 LQFP-100 实际引出的端口 (PA~PE)。AF = Alternate Function。

### Port A

| 引脚 | AF1 | AF2 | AF3 | AF4 | AF5 | AF6 | AF7 | AF8 | AF9 | AF10 | 模拟 | 说明 |
|------|-----|-----|-----|-----|-----|-----|-----|-----|-----|------|------|------|
| PA0 | TIM2_CH1 | TIM5_CH1 | — | — | — | — | USART2_CTS | UART4_TX | — | OTG_HS_ULPI | ADC1/2/3_IN0 | WKUP |
| PA1 | TIM2_CH2 | TIM5_CH2 | — | — | — | — | USART2_RTS | UART4_RX | — | OTG_HS_ULPI | ADC1/2/3_IN1 | |
| PA2 | TIM2_CH3 | TIM5_CH3 | TIM9_CH1 | — | — | — | USART2_TX | — | — | OTG_HS_ULPI | ADC1/2/3_IN2 | |
| PA3 | TIM2_CH4 | TIM5_CH4 | TIM9_CH2 | — | — | — | USART2_RX | — | — | OTG_HS_ULPI | ADC1/2/3_IN3 | |
| PA4 | — | — | — | — | SPI1_SSEL / SPI3_SSEL | — | USART2_CK | — | — | — | ADC1/2_IN4; DAC_OUT1 | DAC/Audio |
| PA5 | TIM2_CH1 | — | TIM8_CH1N | — | SPI1_SCK | — | — | — | — | OTG_HS_ULPI_CK | ADC1/2_IN5 | Battery ADC |
| PA6 | TIM1_BKIN | TIM3_CH1 | TIM8_BKIN | — | SPI1_MISO | — | — | — | **TIM13_CH1** | — | ADC1/2_IN6 | **Motor FL IN1** |
| PA7 | TIM1_CH1N | TIM3_CH2 | TIM8_CH1N | — | SPI1_MOSI | — | — | — | **TIM14_CH1** | — | ADC1/2_IN7 | **Motor FL IN2** |
| PA8 | TIM1_CH1 | — | — | **I2C3_SCL** | — | — | USART1_CK | — | — | OTG_FS_SOF | — | **I2C3_SCL** |
| PA9 | TIM1_CH2 | — | — | — | — | — | **USART1_TX** | — | — | OTG_FS_VBUS | — | SBC 通信 |
| PA10 | TIM1_CH3 | — | — | — | — | — | **USART1_RX** | — | — | OTG_FS_ID | — | SBC 通信 |
| PA11 | TIM1_CH4 | — | — | — | — | — | USART1_CTS | — | **CAN1_RX** | OTG_FS_DM | — | CAN1 保留 |
| PA12 | TIM1_ETR | — | — | — | — | — | USART1_RTS | — | **CAN1_TX** | OTG_FS_DP | — | CAN1 保留 |
| PA13 | JTMS/SWDIO | — | — | — | — | — | — | — | — | — | — | SWD |
| PA14 | JTCK/SWCLK | — | — | — | — | — | — | — | — | — | — | SWD |
| PA15 | JTDI / **TIM2_CH1** | — | — | — | SPI1_SSEL/SPI3_SSEL | — | — | — | — | — | — | **Encoder RL A** |

### Port B

| 引脚 | AF1 | AF2 | AF3 | AF4 | AF5 | AF6 | AF7 | AF8 | AF9 | 模拟 | 说明 |
|------|-----|-----|-----|-----|-----|-----|-----|-----|-----|------|------|
| PB0 | TIM1_CH2N | TIM3_CH3 | **TIM8_CH2N** | — | — | — | — | — | — | ADC1/2_IN8 | **Fan PWM** |
| PB1 | TIM1_CH3N | TIM3_CH4 | TIM8_CH3N | — | — | — | — | — | — | ADC1/2_IN9 | NTC ADC |
| PB2 | — | — | — | — | — | — | — | — | — | — | BOOT1；勿用 |
| PB3 | **TIM2_CH2** | — | — | — | SPI1_SCK/SPI3_SCK | — | — | — | — | — | **Encoder RL B** |
| PB4 | — | **TIM3_CH1** | — | — | SPI1_MISO/SPI3_MISO | — | — | — | — | — | **Encoder FR A** |
| PB5 | — | **TIM3_CH2** | — | I2C1_SMBA | SPI1_MOSI/SPI3_MOSI | — | — | — | CAN2_RX | — | **Encoder FR B** |
| PB6 | TIM4_CH1 | — | — | **I2C1_SCL** | — | — | USART1_TX | — | CAN2_TX | — | **I2C1_SCL (WP5/EXT)** |
| PB7 | TIM4_CH2 | — | — | **I2C1_SDA** | — | — | USART1_RX | — | — | — | **I2C1_SDA (WP5/EXT)** |
| PB8 | TIM4_CH3 | — | — | **I2C1_SCL** | — | — | — | — | CAN1_RX | — | **Motor FR IN1 (TIM10_CH1)** |
| PB9 | TIM4_CH4 | — | — | **I2C1_SDA** | SPI2_SSEL | — | — | — | CAN1_TX | — | **Motor FR IN2 (TIM11_CH1)** |
| PB10 | TIM2_CH3 | — | — | **I2C2_SCL** | SPI2_SCK | — | **USART3_TX** | — | — | — | **I2C2_SCL (IMU)** |
| PB11 | TIM2_CH4 | — | — | **I2C2_SDA** | — | — | **USART3_RX** | — | — | — | **I2C2_SDA (IMU)** |
| PB12 | TIM1_BKIN | — | — | I2C2_SMBA | **SPI2_SSEL** | — | USART3_CK | — | CAN2_RX | — | SPI2_NSS |
| PB13 | TIM1_CH1N | — | — | — | **SPI2_SCK** | — | USART3_CTS | — | CAN2_TX | — | SPI2_SCK |
| PB14 | TIM1_CH2N | TIM8_CH2N | TIM12_CH1 | — | **SPI2_MISO** | — | USART3_RTS | — | — | — | SPI2_MISO |
| PB15 | TIM1_CH3N | TIM8_CH3N | TIM12_CH2 | — | **SPI2_MOSI** | — | — | — | — | — | SPI2_MOSI |

### Port C

| 引脚 | AF2 | AF3 | AF4 | AF5 | AF6 | AF7 | AF8 | 模拟 | 说明 |
|------|-----|-----|-----|-----|-----|-----|-----|------|------|
| PC0 | — | — | — | — | — | — | — | ADC1/2/3_IN10 | EXT_ADC3 |
| PC1 | — | — | — | — | — | — | — | ADC1/2/3_IN11 | EXT_ADC (通用) |
| PC2 | — | — | — | SPI2_MISO | — | — | — | ADC1/2/3_IN12 | EXT_ADC / SPI2_MISO 备选 |
| PC3 | — | — | — | SPI2_MOSI | — | — | — | ADC1/2/3_IN13 | EXT_ADC / SPI2_MOSI 备选 |
| PC4 | — | — | — | — | — | — | — | ADC1/2_IN14 | IPROPI_FL |
| PC5 | — | — | — | — | — | — | — | ADC1/2_IN15 | IPROPI_FR |
| PC6 | TIM3_CH1 | **TIM8_CH1** | — | — | I2S2_MCK | USART6_TX | — | — | **Motor RR IN1** |
| PC7 | TIM3_CH2 | **TIM8_CH2** | — | — | I2S3_MCK | USART6_RX | — | — | **Motor RR IN2** |
| PC8 | TIM3_CH3 | TIM8_CH3 | — | — | — | USART6_CK | — | — | EXT_PWM1 备选 |
| PC9 | TIM3_CH4 | TIM8_CH4 | **I2C3_SDA** | — | I2S_CKIN | MCO2 | — | — | **I2C3_SDA** |
| PC10 | — | — | — | — | **SPI3_SCK** | USART3_TX | UART4_TX | — | **LED Strip SCK** |
| PC11 | — | — | — | — | **SPI3_MISO** | USART3_RX | UART4_RX | — | **LED Strip MISO** |
| PC12 | — | — | — | — | **SPI3_MOSI** | UART5_TX | — | — | **LED Strip MOSI** |
| PC13 | — | — | — | — | — | — | — | — | RTC TAMP；低速，不建议高速 GPIO |

### Port D

| 引脚 | AF1 | AF2 | AF7 | AF9 | AF12 | 说明 |
|------|-----|-----|-----|-----|------|------|
| PD0 | — | — | — | CAN1_RX | FSMC | CAN1_RX 备选 |
| PD1 | — | — | — | CAN1_TX | FSMC | CAN1_TX 备选 |
| PD2 | — | TIM3_ETR | UART5_RX | — | SDIO_CMD | UART5_RX |
| PD3 | — | — | USART2_CTS | — | FSMC | |
| PD4 | — | — | USART2_RTS | — | FSMC | GPIO：PB_SHD_DETECT |
| PD5 | — | — | **USART2_TX** | — | FSMC | USART2_TX |
| PD6 | — | — | **USART2_RX** | — | FSMC | USART2_RX |
| PD7 | — | — | USART2_CK | — | FSMC | PB_SHD_CONFIRM |
| PD8 | — | — | **USART3_TX** | — | FSMC | USART3_TX 备选 |
| PD9 | — | — | **USART3_RX** | — | FSMC | USART3_RX 备选 |
| PD10 | — | — | USART3_CK | — | FSMC | |
| PD11 | — | — | USART3_CTS | — | FSMC | |
| PD12 | **TIM4_CH1** | — | — | — | FSMC | **Encoder FL A** |
| PD13 | **TIM4_CH2** | — | — | — | FSMC | **Encoder FL B** |
| PD14 | TIM4_CH3 | — | — | — | FSMC | EXT PWM 备选 |
| PD15 | TIM4_CH4 | — | — | — | FSMC | EXT PWM 备选 |

### Port E

| 引脚 | AF1 | AF3 | AF9 | 说明 |
|------|-----|-----|-----|------|
| PE0 | — | — | — | Right nFAULT (GPIO) |
| PE1 | — | — | — | Left nFAULT (GPIO) |
| PE2 | — | TIM3_ETR | — | EXT_GPIO |
| PE3 | — | — | — | GRN LED |
| PE4 | — | — | — | RED LED |
| PE5 | — | **TIM9_CH1** | — | **Motor RL IN1** |
| PE6 | — | **TIM9_CH2** | — | **Motor RL IN2** |
| PE7 | TIM1_ETR | — | — | EXT_GPIO |
| PE8 | TIM1_CH1N | — | — | EXT_GPIO |
| PE9 | **TIM1_CH1** | — | — | **Encoder RR A** |
| PE10 | TIM1_CH2N | — | — | |
| PE11 | **TIM1_CH2** | — | — | **Encoder RR B** |
| PE12 | TIM1_CH3N | — | — | |
| PE13 | TIM1_CH3 | — | — | EXT PWM 备选 |
| PE14 | **TIM1_CH4** | — | — | EXT_PWM2（CAN 占用 PA11 后的替代 PWM） |
| PE15 | TIM1_BKIN | — | — | |

---

## 4. 外设引脚汇总（按外设分类）

### 4.1 USART / UART

| 外设 | TX | RX | AF | 可用性 | 说明 |
|------|----|----|:--:|:------:|------|
| **USART1** | PA9 | PA10 | AF7 | ✅ | SBC 通信 |
| **USART2** | PD5 | PD6 | AF7 | ✅ | 调试/扩展 |
| **USART3** | PB10 | PB11 | AF7 | ❌ PB10/11=I2C2 | 与 IMU I2C2 冲突 |
| USART3 备选 | PD8 | PD9 | AF7 | ✅ | 如需第 3 路 UART |
| UART4 | PA0 | PA1 | AF8 | ✅ (PA0/1 空闲时) | 备用 |
| — | PC10 | PC11 | AF8 | ❌ | SPI3 占用 |
| UART5 | PC12 | PD2 | AF8 | ❌/⚠️ | PC12=SPI3_MOSI |
| USART6 | PC6 | PC7 | AF8 | ❌ | Motor RR 占用 |

**结论：2路（USART1 SBC + USART2 调试），如需3路可加 USART3 on PD8/PD9。**

### 4.2 I2C

| 外设 | SCL | SDA | AF | 用途 |
|------|-----|-----|:---:|------|
| **I2C1** | PB6 | PB7 | AF4 | WP5 电源板（共享 EXT1 总线） |
| I2C1 备选 | PB8 | PB9 | AF4 | ❌ 与 Motor FR (TIM10/11) 冲突 |
| **I2C2** | PB10 | PB11 | AF4 | IMU + EEPROM |
| **I2C3** | PA8/SCL | PC9/SDA | AF4 | EXT2 扩展 I2C3 |

> **⚠️ I2C2 只有 PB10/PB11 可用**（PF0/PF1 在 LQFP-100 不引出）

**结论：可实现 3 路独立 I2C（I2C1/2/3），满足目标。**

### 4.3 SPI

| 外设 | SCK | MISO | MOSI | NSS | AF | 用途 |
|------|-----|------|------|-----|:---:|------|
| SPI1 | PA5 | PA6 | PA7 | PA4/PA15 | AF5 | ❌ PA6/PA7 被 Motor FL 占用 |
| **SPI2** | PB13 | PB14 | PB15 | PB12 | AF5 | ✅ EXT1 扩展（标准引脚，AF5 无前 ETH 冲突） |
| SPI2 备选 | PB10 | PC2 | PC3 | — | AF5/AF5 | ❌ PB10=I2C2 |
| **SPI3** | PC10 | PC11 | PC12 | PA4/PA15 | AF6 | ✅ LED Strip 专用 |

**结论：SPI2 (EXT1) + SPI3 (LED Strip) = 2 路 SPI，满足目标。**

### 4.4 CAN

| 外设 | RX | TX | AF | 用途 |
|------|----|----|----|------|
| **CAN1** | PA11 | PA12 | AF9 | ✅ 主 CAN1 |
| CAN1 备选 | PD0 | PD1 | AF9 | ✅ 可选 |
| CAN1 备选 | PB8 | PB9 | AF9 | ❌ 与 Motor FR 冲突 |
| CAN2 | PB12 | PB13 | AF9 | ❌ 与 SPI2 冲突 |

**结论：CAN1 (PA11/PA12) 1 路，满足目标。PA11 保留 CAN，EXT PWM1 改用 PE14/TIM1_CH4。**

### 4.5 定时器（ROSbot Lite 分配）

| 定时器 | 通道 | 引脚 | 用途 | 频率 |
|--------|------|------|------|------|
| TIM1 | CH1/CH2 | PE9/PE11 | **Encoder RR A/B** | QEI |
| TIM2 | CH1/CH2 | PA15/PB3 | **Encoder RL A/B** | QEI |
| TIM3 | CH1/CH2 | PB4/PB5 | **Encoder FR A/B** | QEI |
| TIM4 | CH1/CH2 | PD12/PD13 | **Encoder FL A/B** | QEI |
| TIM5 | 32-bit | — | Runtime Stats | — |
| **TIM8** | CH1/CH2 | PC6/PC7 | **Motor RR IN1/IN2** | 20kHz |
| TIM8 | CH2N | PB0 | **Fan PWM** | 20kHz |
| **TIM9** | CH1/CH2 | PE5/PE6 | **Motor RL IN1/IN2** | 20kHz |
| **TIM10** | CH1 | PB8 | **Motor FR IN1** | 20kHz |
| **TIM11** | CH1 | PB9 | **Motor FR IN2** | 20kHz |
| **TIM13** | CH1 | PA6 | **Motor FL IN1** | 20kHz |
| **TIM14** | CH1 | PA7 | **Motor FL IN2** | 20kHz |
| TIM1 | CH4 | PE14 | EXT_PWM2（CAN 保留 PA11 后替代） | — |
| TIM8 | CH3 | PC8 | EXT_PWM1（Motor RR 只用 CH1/2） | — |
| TIM12 | CH1/CH2 | PB14/PB15 | ❌ 与 SPI2 冲突，不用 | — |

### 4.6 ADC（LQFP-100 可用通道）

| 引脚 | ADC 通道 | 用途 |
|------|---------|------|
| PA0 | ADC1/2/3_IN0 | 备用 ADC |
| PA5 | ADC1/2_IN5 | **Battery ADC** |
| PB0 | ADC1/2_IN8 | — |
| PB1 | ADC1/2_IN9 | **NTC ADC** |
| PC0 | ADC1/2/3_IN10 | **EXT_ADC3** |
| PC1 | ADC1/2/3_IN11 | **EXT_ADC (通用)** |
| PC2 | ADC1/2/3_IN12 | **EXT_ADC** |
| PC3 | ADC1/2/3_IN13 | **EXT_ADC** |
| PC4 | ADC1/2_IN14 | **IPROPI_FL** |
| PC5 | ADC1/2_IN15 | **IPROPI_FR** |
| PA3 | ADC1/2/3_IN3 | **IPROPI_RL** |
| PA2 | ADC1/2/3_IN2 | **IPROPI_RR** |

> **⚠️ 关键差异**：VET6 无 PF3/PF4/PF5/PF10 (ADC3 专属通道)，IPROPI 改用 PC4/PC5/PA3/PA2 (ADC1/2 共享通道)。

---

## 5. 冲突分析与设计约束

### 5.1 关键互斥关系

| 引脚 | 功能 A | 功能 B | 结论 |
|------|--------|--------|------|
| PA6 | TIM13_CH1 (AF9) = Motor FL IN1 | TIM3_CH1 (AF2) = Encoder FR A | **二选一**：PA6 用于 Motor FL，Encoder FR 用 PB4 (TIM3_CH1) ✅ |
| PA7 | TIM14_CH1 (AF9) = Motor FL IN2 | TIM3_CH2 (AF2) = Encoder FR B | **二选一**：PA7 用于 Motor FL，Encoder FR 用 PB5 (TIM3_CH2) ✅ |
| PB8 | TIM10_CH1 (AF3) = Motor FR IN1 | I2C1_SCL (AF4) = WP5 | **二选一**：PB8 用于 Motor FR，I2C1 迁至 PB6 ✅ |
| PB9 | TIM11_CH1 (AF3) = Motor FR IN2 | I2C1_SDA (AF4) = WP5 | **二选一**：PB9 用于 Motor FR，I2C1 迁至 PB7 ✅ |
| PB10 | I2C2_SCL (AF4) = IMU | USART3_TX (AF7) | **二选一**：PB10 用于 I2C2_SCL；USART3 用 PD8/PD9 ✅ |
| PB11 | I2C2_SDA (AF4) = IMU | USART3_RX (AF7) | 同上 ✅ |
| PA11 | CAN1_RX (AF9) | TIM1_CH4 / OTG_FS | 保留 CAN1；EXT PWM1 改用 PE14 ✅ |
| PC6 | TIM8_CH1 = Motor RR IN1 | TIM3_CH1 / USART6_TX | Motor RR 占用，Encoder FR 用 PB4 ✅ |
| PC7 | TIM8_CH2 = Motor RR IN2 | TIM3_CH2 / USART6_RX | Motor RR 占用，Encoder FR 用 PB5 ✅ |
| PC9 | I2C3_SDA (AF4) | TIM8_CH4 | I2C3 优先，TIM8_CH4 不用 ✅ |
| SPI2 PB14/15 | SPI2_MISO/MOSI | TIM12_CH1/2 | SPI2 优先，TIM12 不用 ✅ |

### 5.2 设计约束总结

1. **LQFP-100 无以太网**：不引出 ETH 相关引脚，完全避免 PA1/PA7/PC1/PC4/PC5 被以太网占用——这些引脚在 Lite 版全部可用于 ADC！
2. **SPI1 不可用**：PA6/PA7 被 Motor FL 占用，SPI1 标准引脚不全；故只用 SPI2/SPI3
3. **USART6 不可用**：PC6/PC7 被 Motor RR 占用
4. **UART5 受限**：PC12 被 SPI3_MOSI 占用；如需 UART5_TX 则无法使用
5. **I2C2 只有 PB10/PB11**：LQFP-100 无 PF，必须用 PB10/PB11，与 USART3 冲突——USART3 需使用 PD8/PD9 备选

---

## 6. ROSbot Lite 最优引脚分配方案

### 6.1 最优分配总表

| 功能 | 引脚 | 外设 | AF | 备注 |
|------|------|------|----|------|
| **通信** | | | | |
| USART1 TX/RX (SBC) | PA9/PA10 | USART1 | AF7 | 主通信 |
| USART2 TX/RX (调试) | PD5/PD6 | USART2 | AF7 | 调试串口 |
| I2C1 SCL/SDA (WP5/EXT) | PB6/PB7 | I2C1 | AF4 | ⚠️ PB8/9被Motor FR占用，故用PB6/7 |
| I2C2 SCL/SDA (IMU+EEPROM) | PB10/PB11 | I2C2 | AF4 | 唯一可用 I2C2 引脚 |
| I2C3 SCL/SDA (EXT2) | PA8/PC9 | I2C3 | AF4 | EXT2 扩展 |
| SPI2 NSS/SCK/MISO/MOSI (EXT1) | PB12/PB13/PB14/PB15 | SPI2 | AF5 | 标准引脚，无 ETH 冲突 |
| SPI3 SCK/MISO/MOSI (LED Strip) | PC10/PC11/PC12 | SPI3 | AF6 | LED Strip 专用 |
| CAN1 RX/TX | PA11/PA12 | CAN1 | AF9 | 保留 |
| **电机与编码器** | | | | |
| Motor FL IN1 (TIM13_CH1) | PA6 | TIM13 | AF9 | ⚠️ 无 PF9，用 PA6 代替 |
| Motor FL IN2 (TIM14_CH1) | PA7 | TIM14 | AF9 | ⚠️ 无 PF6，用 PA7 代替 |
| Motor FR IN1 (TIM10_CH1) | PB8 | TIM10 | AF3 | ⚠️ 无 PF8，用 PB8 代替 |
| Motor FR IN2 (TIM11_CH1) | PB9 | TIM11 | AF3 | ⚠️ 无 PF7，用 PB9 代替 |
| Motor RL IN1 (TIM9_CH1) | PE5 | TIM9 | AF3 | ✅ 同 PRO |
| Motor RL IN2 (TIM9_CH2) | PE6 | TIM9 | AF3 | ✅ 同 PRO |
| Motor RR IN1 (TIM8_CH1) | PC6 | TIM8 | AF3 | ✅ 同 PRO |
| Motor RR IN2 (TIM8_CH2) | PC7 | TIM8 | AF3 | ✅ 同 PRO |
| Fan PWM (TIM8_CH2N) | PB0 | TIM8 | AF3 | ✅ 同 PRO |
| Encoder FL A/B | PD12/PD13 | TIM4_CH1/2 | AF2 | ✅ 同 PRO |
| Encoder FR A/B | PB4/PB5 | TIM3_CH1/2 | AF2 | ✅ 同 PRO |
| Encoder RL A/B | PA15/PB3 | TIM2_CH1/2 | AF1 | ✅ 同 PRO |
| Encoder RR A/B | PE9/PE11 | TIM1_CH1/2 | AF1 | ✅ 同 PRO |
| **ADC / 模拟** | | | | |
| Battery ADC | PA5 | ADC1_CH5 | Analog | ✅ 同 PRO |
| NTC ADC | PB1 | ADC1_CH9 | Analog | ✅ 同 PRO |
| IPROPI FL | PC4 | ADC1/2_CH14 | Analog | ⚠️ 无 PF3，用 PC4（注意无 ETH 无冲突） |
| IPROPI FR | PC5 | ADC1/2_CH15 | Analog | ⚠️ 无 PF4，用 PC5 |
| IPROPI RL | PA3 | ADC1/2/3_CH3 | Analog | ⚠️ 无 PF5，用 PA3 |
| IPROPI RR | PA2 | ADC1/2/3_CH2 | Analog | ⚠️ 无 PF10，用 PA2 |
| Audio DAC | PA4 | DAC_OUT1 | Analog | ✅ 同 PRO |
| EXT_ADC1 | PC3 | ADC123_CH13 | Analog | ✅ PC3 直接可用（无 SPI2 在 PB） |
| EXT_ADC2 | PC2 | ADC123_CH12 | Analog | ✅ 同上 |
| EXT_ADC3 | PC0 | ADC123_CH10 | Analog | ✅ 同 PRO |
| **GPIO / 控制** | | | | |
| Audio SHDN | PE10 | GPIO | — | ✅ 普通 GPIO |
| Right nSLEEP | PD11 | GPIO | — | ⚠️ 无 PG9，用 PD11 |
| Left nSLEEP | PD10 | GPIO | — | ⚠️ 普通 GPIO |
| Right nFAULT | PE0 | GPIO | — | ✅ 同 PRO |
| Left nFAULT | PE1 | GPIO | — | ✅ 同 PRO |
| PB_SHD_DETECT | PD4 | GPIO | — | ✅ 同 PRO |
| PB_SHD_CONFIRM | PD7 | GPIO | — | ✅ 同 PRO |
| PUSH_BUTTON1 | PD0 | GPIO | — | ⚠️ 无 PF11，改用普通 GPIO |
| PUSH_BUTTON2 | PD1 | GPIO | — | ⚠️ 无 PF12，改用普通 GPIO |
| EN_LOC_5V | PD10 | GPIO | — | ⚠️ 无 PF13，用 PD10 |
| IMU INT | PD3 | EXTI | — | ⚠️ 无 PF2，用 PD3 |
| RED LED | PE4 | GPIO | — | ✅ 同 PRO |
| GRN LED | PE3 | GPIO | — | ✅ 同 PRO |
| Runtime Stats | TIM5 | 32-bit | — | ✅ 同 PRO |
| SWD Debug | PA13/PA14 | Debug | — | ✅ 同 PRO |
| EXT_PWM1 | PC8 → **PE14** | TIM1_CH4 | AF1 | ⚠️ PC8=nSLEEP，EXT PWM1 用 PE14 |
| EXT_PWM2 | PA6 → | TIM13_CH1 | AF9 | ❌ PA6=Motor FL，重叠 |

### 6.2 nSLEEP / nFAULT 分配调整说明

- **Right nSLEEP**: PC8 (GPIO)，Motor RR 只用 TIM8_CH1/CH2 (PC6/PC7)，PC8=TIM8_CH3 空闲
- **Left nSLEEP**: PD11 (GPIO)，PD11 无重要 AF 冲突
- 或者：两路 nSLEEP 统一使用 PD10/PD11，更整洁

### 6.3 外设资源总结

| 外设 | 数量 | 实现 | 说明 |
|------|:----:|------|------|
| UART | **2~3 路** | USART1(PA9/10), USART2(PD5/6), [USART3(PD8/9)] | ✅ 满足 2-3 路目标 |
| I2C | **2 路** | I2C1(PB6/7), I2C2(PB10/11) | ✅ 满足需求，I2C3 不启用 |
| SPI | **2 路** | SPI2(PB12-15), SPI3(PC10-12) | ✅ 满足 2 路目标 |
| CAN | **1 路** | CAN1(PD0/PD1) | ✅ 满足 1 路目标 |
| IPROPI | **4 路** | PC4/PC5/PA3/PA2 (ADC1/2) | ✅ 无 ETH 后 PC4/5 恢复可用 |
| ETH | **无** | STM32F407VET6 硅片不含 ETH MAC | ✅ 按设计要求 |

### 6.4 空闲/可扩展引脚

| 引脚 | 可用功能 |
|------|---------|
| PA8 | EXT GPIO |
| PA0/PA1 | ADC / UART4 / TIM2/5 |
| PC8 | EXT GPIO |
| PC14/PC15 | EXT GPIO / LSE 不使用时可用 |
| PD14/PD15 | EXT GPIO / PWM |
| PD0/PD1 | CAN1 |
| PD2 | UART5_RX (SBUS) |
| PE10 | Audio SHDN |

---

*本文档基于 ST 官方数据手册 DS8626 及 `framework-arduinoststm32` PeripheralPins.c (STM32F4xx/F407V(E-G)T) 编写，引脚编号以 ST 官方数据手册为权威。*
