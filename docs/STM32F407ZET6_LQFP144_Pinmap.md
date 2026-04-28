# STM32F407ZET6 LQFP-144 完整引脚映射参考手册

> **版本**: V1.0  
> **日期**: 2026-04-13  
> **芯片**: STM32F407ZET6 (LQFP-144)  
> **数据来源**: ST 官方数据手册 DS8626 + `framework-arduinoststm32/variants/STM32F4xx/F407Z(E-G)T_F417Z(E-G)T/PeripheralPins.c`  
> **⚠️ 物理引脚编号**: LQFP-144 封装从引脚 1（缺口标记处）开始逆时针编号，具体编号以 ST 官方数据手册表格为准。

---

## 目录

1. [芯片概述](#1-芯片概述)
2. [LQFP-144 引脚编号速查](#2-lqfp-144-引脚编号速查)
3. [GPIO 端口 AF 功能完整表](#3-gpio-端口-af-功能完整表)
4. [外设引脚汇总（按外设分类）](#4-外设引脚汇总按外设分类)
5. [冲突分析与设计约束](#5-冲突分析与设计约束)
6. [ROSbot PRO 最优引脚分配方案](#6-rosbot-pro-最优引脚分配方案)

---

## 1. 芯片概述

| 参数 | 值 |
|------|-----|
| 型号 | STM32F407ZET6 |
| 封装 | LQFP-144 |
| 内核 | ARM Cortex-M4 @ 168MHz |
| Flash | 512KB |
| SRAM | 192KB (128+64KB CCM) |
| GPIO 端口 | PA~PG（完整 Port A-G，共 112 个 I/O） |
| ADC | ADC1/2/3（12-bit，共 24 通道） |
| DAC | DAC1（2路输出 PA4/PA5） |
| 定时器 | TIM1~TIM14（含高级/通用/基础） |
| USART | USART1/2/3 + UART4/5/USART6（共 6 路） |
| SPI | SPI1/2/3（含 I2S） |
| I2C | I2C1/2/3 |
| CAN | CAN1/CAN2 |
| USB | OTG FS + OTG HS |
| ETH | 10/100M MAC（RMII/MII） |
| SDIO | 1路 |
| 数据手册 | DS8626 Rev9 |

---

## 2. LQFP-144 引脚编号速查

引脚从封装缺口/圆点处编号，逆时针排列，每侧 36 个引脚。

| 区间 | 引脚范围 | 主要 GPIO |
|------|---------|----------|
| 侧1 (顶部) | 1 ~ 36 | PE2~PE6, PC13~PC15, PF0~PF10, PH0/PH1, NRST, PC0~PC3, PA0~PA3 |
| 侧2 (右侧) | 37 ~ 72 | PA4~PA7, PC4~PC5, PB0~PB2, PF11~PF15, PG0~PG1, PE7~PE15, PB10~PB12 |
| 侧3 (底部) | 73 ~ 108 | PB13~PB15, PD8~PD15, PG2~PG8, PC6~PC9, PA8~PA15 |
| 侧4 (左侧) | 109 ~ 144 | PC10~PC12, PD0~PD7, PG9~PG15, PB3~PB9, PE0~PE1 |

**部分关键引脚位置（以 DS8626 为权威）**：

| LQFP-144 引脚# | GPIO | LQFP-144 引脚# | GPIO |
|:---:|------|:---:|------|
| 68 | PB10 | 69 | PB11 |
| 72 | PB12 | 73 | PB13 |
| 74 | PB14 | 75 | PB15 |
| 95 | PC6 | 96 | PC7 |
| 97 | PC8 | 98 | PC9 |
| 99 | PA8 | 100 | PA9 |
| 102 | PA11 | 103 | PA12 |
| 109 | PC10 | 111 | PC12 |
| 117 | PD5 | 120 | PD6 |
| 122 | PG9 | 124 | PG11 |
| 126 | PG13 | 127 | PG14 |
| 131 | PB3 | 132 | PB4 |
| 133 | PB5 | 134 | PB6 |
| 137 | PB8 | 138 | PB9 |

---

## 3. GPIO 端口 AF 功能完整表

> AF = Alternate Function。GPIO 默认为 AF0 (SYSTEM)；各 AF 编号参考 STM32F4xx 参考手册 RM0090 表 9。  
> 标注 `⚡ETH` 表示以太网使用该引脚时与其他功能互斥。

### Port A

| 引脚 | Pin# | AF1 | AF2 | AF3 | AF4 | AF5 | AF6 | AF7 | AF8 | AF9 | AF10 | AF11 | 模拟 | 说明 |
|------|:----:|-----|-----|-----|-----|-----|-----|-----|-----|-----|------|------|------|------|
| PA0 | 33 | TIM2_CH1 | TIM5_CH1 | — | — | — | — | USART2_CTS | UART4_TX | — | OTG_HS_ULPI | ETH_CRS | ADC1/2/3_IN0 | WKUP pin |
| PA1 | 34 | TIM2_CH2 | TIM5_CH2 | — | — | — | — | USART2_RTS | UART4_RX | — | OTG_HS_ULPI | **ETH_RMII_REF_CLK** ⚡ETH | ADC1/2/3_IN1 | ETH RMII 必需 |
| PA2 | 35 | TIM2_CH3 | TIM5_CH3 | TIM9_CH1 | — | — | — | USART2_TX | — | — | OTG_HS_ULPI | **ETH_MDIO** ⚡ETH | ADC1/2/3_IN2 | ETH MDIO 必需 |
| PA3 | 36 | TIM2_CH4 | TIM5_CH4 | TIM9_CH2 | — | — | — | USART2_RX | — | — | OTG_HS_ULPI | ETH_COL | ADC1/2/3_IN3 | |
| PA4 | 39 | TIM2_CH1 | — | — | — | SPI1_SSEL / SPI3_SSEL | — | USART2_CK | — | — | OTG_HS_SOF | — | ADC1/2_IN4; DAC_OUT1 | |
| PA5 | 40 | TIM2_CH1 | — | TIM8_CH1N | — | SPI1_SCK | — | — | — | — | OTG_HS_ULPI_CK | — | ADC1/2_IN5; DAC_OUT2 | Battery ADC |
| PA6 | 41 | TIM1_BKIN | TIM3_CH1 | TIM8_BKIN | — | SPI1_MISO | — | — | — | TIM13_CH1 | — | — | ADC1/2_IN6 | EXT PWM 候选 |
| PA7 | 42 | TIM1_CH1N | TIM3_CH2 | TIM8_CH1N | — | SPI1_MOSI | — | — | — | TIM14_CH1 | — | **ETH_CRS_DV / ETH_RX_DV** ⚡ETH | ADC1/2_IN7 | ETH RMII 必需 |
| PA8 | 99 | TIM1_CH1 | — | — | I2C3_SCL | — | — | USART1_CK | — | — | OTG_FS_SOF | — | — | **I2C3_SCL** 首选 |
| PA9 | 100 | TIM1_CH2 | — | — | — | — | — | **USART1_TX** | — | — | OTG_FS_VBUS | — | — | SBC 通信 |
| PA10 | 101 | TIM1_CH3 | — | — | — | — | — | **USART1_RX** | — | — | OTG_FS_ID | — | — | SBC 通信 |
| PA11 | 102 | TIM1_CH4 | — | — | — | — | — | USART1_CTS | — | **CAN1_RX** | OTG_FS_DM | — | — | **CAN1 保留** |
| PA12 | 103 | TIM1_ETR | — | — | — | — | — | USART1_RTS | — | **CAN1_TX** | OTG_FS_DP | — | — | **CAN1 保留** |
| PA13 | 104 | JTMS/SWDIO | — | — | — | — | — | — | — | — | — | — | — | SWD 调试，勿占用 |
| PA14 | 107 | JTCK/SWCLK | — | — | — | — | — | — | — | — | — | — | — | SWD 调试，勿占用 |
| PA15 | 108 | JTDI / TIM2_CH1 | — | — | — | SPI1_SSEL / SPI3_SSEL | — | — | — | — | — | — | — | Encoder RL A |

### Port B

| 引脚 | Pin# | AF1 | AF2 | AF3 | AF4 | AF5 | AF6 | AF7 | AF8 | AF9 | AF10 | AF11 | 模拟 | 说明 |
|------|:----:|-----|-----|-----|-----|-----|-----|-----|-----|-----|------|------|------|------|
| PB0 | 45 | TIM1_CH2N | TIM3_CH3 | **TIM8_CH2N** | — | — | — | — | — | — | OTG_HS_ULPI_D1 | ETH_RXD2 | ADC1/2_IN8 | Fan PWM |
| PB1 | 46 | TIM1_CH3N | TIM3_CH4 | TIM8_CH3N | — | — | — | — | — | — | OTG_HS_ULPI_D2 | ETH_RXD3 | ADC1/2_IN9 | NTC ADC |
| PB2 | 47 | — | — | — | — | — | — | — | — | — | — | — | — | BOOT1；勿用作普通 GPIO |
| PB3 | 131 | TIM2_CH2 | — | — | — | SPI1_SCK / SPI3_SCK | — | — | — | — | — | — | — | JTDO/TRACESWO; Encoder RL B |
| PB4 | 132 | — | TIM3_CH1 | — | — | SPI1_MISO / SPI3_MISO | — | — | — | — | — | — | — | NJTRST; **Encoder FR A** (新分配) |
| PB5 | 133 | — | TIM3_CH2 | — | I2C1_SMBA | SPI1_MOSI / SPI3_MOSI | — | — | — | CAN2_RX | OTG_HS_ULPI_D7 | ETH_PPS_OUT | — | **Encoder FR B** (新分配) |
| PB6 | 134 | TIM4_CH1 | — | — | **I2C1_SCL** | — | — | USART1_TX | — | CAN2_TX | — | — | — | EXT1 I2C1 |
| PB7 | 135 | TIM4_CH2 | — | — | **I2C1_SDA** | — | — | USART1_RX | — | — | — | — | — | EXT1 I2C1 |
| PB8 | 137 | TIM4_CH3 | TIM10_CH1 | — | **I2C1_SCL** | — | — | — | — | CAN1_RX | — | ETH_TXD3 | — | I2C1_SCL (WP5) |
| PB9 | 138 | TIM4_CH4 | TIM11_CH1 | — | **I2C1_SDA** | SPI2_SSEL | — | — | — | CAN1_TX | — | — | — | I2C1_SDA (WP5) |
| PB10 | 68 | TIM2_CH3 | — | — | I2C2_SCL | SPI2_SCK | — | **USART3_TX** | — | — | OTG_HS_ULPI_D3 | ETH_RX_ER | — | **USART3_TX** 首选 |
| PB11 | 69 | TIM2_CH4 | — | — | I2C2_SDA | — | — | **USART3_RX** | — | — | OTG_HS_ULPI_D4 | ~~ETH_TX_EN~~ → **PG11** | — | **USART3_RX** (ETH TX 挪至 PG11 后释放) |
| PB12 | 72 | TIM1_BKIN | — | — | I2C2_SMBA | **SPI2_SSEL** | — | USART3_CK | — | CAN2_RX | OTG_HS_ULPI_D5 | ~~ETH_TXD0~~ → **PG13** | — | **SPI2_NSS** (ETH TX 挪至 PG13 后释放) |
| PB13 | 73 | TIM1_CH1N | — | — | — | **SPI2_SCK** | — | USART3_CTS | — | CAN2_TX | OTG_HS_ULPI_D6 | ~~ETH_TXD1~~ → **PG14** | — | **SPI2_SCK** 标准引脚 |
| PB14 | 74 | TIM1_CH2N | TIM8_CH2N | TIM12_CH1 | — | **SPI2_MISO** | — | USART3_RTS | — | — | OTG_HS_DM | — | — | **SPI2_MISO** 标准引脚 |
| PB15 | 75 | TIM1_CH3N | TIM8_CH3N | TIM12_CH2 | — | **SPI2_MOSI** | — | — | — | — | OTG_HS_DP | — | — | **SPI2_MOSI** 标准引脚 |

### Port C

| 引脚 | Pin# | AF1 | AF2 | AF3 | AF4 | AF5 | AF6 | AF7 | AF8 | AF9 | 模拟 | 说明 |
|------|:----:|-----|-----|-----|-----|-----|-----|-----|-----|-----|------|------|
| PC0 | 26 | — | — | — | — | — | — | — | — | — | ADC1/2/3_IN10 | EXT_ADC3 |
| PC1 | 27 | — | — | — | — | — | — | — | — | — | ADC1/2/3_IN11 | ⚡ETH_MDC (AF11)，**与 ADC 互斥** |
| PC2 | 28 | — | — | — | — | SPI2_MISO | — | — | — | — | ADC1/2/3_IN12 | EXT_ADC2；SPI2_MISO 备选（主 SPI2 迁至 PB14） |
| PC3 | 29 | — | — | — | — | SPI2_MOSI | — | — | — | — | ADC1/2/3_IN13 | EXT_ADC1；SPI2_MOSI 备选（主 SPI2 迁至 PB15） |
| PC4 | 43 | — | — | — | — | — | — | — | — | — | ADC1/2_IN14 | ⚡ETH_RMII_RXD0，与 ADC 互斥 |
| PC5 | 44 | — | — | — | — | — | — | — | — | — | ADC1/2_IN15 | ⚡ETH_RMII_RXD1，与 ADC 互斥 |
| PC6 | 95 | — | TIM3_CH1 | **TIM8_CH1** | — | — | I2S2_MCK | USART6_TX | — | — | — | **Motor RR IN1** (TIM8_CH1) |
| PC7 | 96 | — | TIM3_CH2 | **TIM8_CH2** | — | — | I2S3_MCK | USART6_RX | — | — | — | **Motor RR IN2** (TIM8_CH2) |
| PC8 | 97 | — | TIM3_CH3 | TIM8_CH3 | — | — | — | USART6_CK | — | — | — | 目前空闲（Motor RR 迁至 PC6/PC7 后） |
| PC9 | 98 | — | TIM3_CH4 | TIM8_CH4 | **I2C3_SDA** | — | I2S_CKIN | MCO2 | — | — | — | **I2C3_SDA**（Motor RR 迁至 PC6/PC7 后） |
| PC10 | 109 | — | — | — | — | — | **SPI3_SCK** | USART3_TX | UART4_TX | — | — | LED Strip SCK |
| PC11 | 110 | — | — | — | — | — | **SPI3_MISO** | USART3_RX | UART4_RX | — | — | LED Strip MISO |
| PC12 | 111 | — | — | — | — | — | **SPI3_MOSI** | UART5_TX | — | — | — | LED Strip MOSI |
| PC13 | 7 | — | — | — | — | — | — | — | — | — | — | RTC 专用，低速 GPIO，不推荐高速使用 |
| PC14 | 8 | — | — | — | — | — | — | — | — | — | — | OSC32_IN，用于 RTC 晶振 |
| PC15 | 9 | — | — | — | — | — | — | — | — | — | — | OSC32_OUT，用于 RTC 晶振 |

### Port D

| 引脚 | Pin# | AF1 | AF2 | AF3 | AF4 | AF7 | AF12 | 说明 |
|------|:----:|-----|-----|-----|-----|-----|------|------|
| PD0 | 112 | — | — | — | — | — | CAN1_RX / FSMC_D2 | CAN1 备选 RX |
| PD1 | 113 | — | — | — | — | — | CAN1_TX / FSMC_D3 | CAN1 备选 TX |
| PD2 | 114 | — | TIM3_ETR | — | — | UART5_RX | SDIO_CMD | UART5_RX 可用 |
| PD3 | 115 | — | — | — | — | USART2_CTS | FSMC_CLK | |
| PD4 | 116 | — | — | — | — | USART2_RTS | FSMC_NOE | WP5 关机检测 GPIO |
| PD5 | 117 | — | — | — | — | **USART2_TX** | FSMC_NWE | **USART2_TX 首选** |
| PD6 | 120 | — | — | — | — | **USART2_RX** | FSMC_NWAIT | **USART2_RX 首选** |
| PD7 | 121 | — | — | — | — | USART2_CK | FSMC_NE1 | |
| PD8 | 76 | — | — | — | — | **USART3_TX** | FSMC_D13 | USART3_TX 备选 |
| PD9 | 77 | — | — | — | — | **USART3_RX** | FSMC_D14 | USART3_RX 备选 |
| PD10 | 78 | — | — | — | — | USART3_CK | FSMC_D15 | |
| PD11 | 79 | — | — | — | — | USART3_CTS | FSMC_A16 | |
| PD12 | 80 | TIM4_CH1 | — | — | — | USART3_RTS | FSMC_A17 | Encoder FL A |
| PD13 | 81 | TIM4_CH2 | — | — | — | — | FSMC_A18 | Encoder FL B |
| PD14 | 84 | TIM4_CH3 | — | — | — | — | FSMC_D0 | |
| PD15 | 85 | TIM4_CH4 | — | — | — | — | FSMC_D1 | |

### Port E

| 引脚 | Pin# | AF1 | AF2 | AF3 | 说明 |
|------|:----:|-----|-----|-----|------|
| PE0 | 139 | — | TIM4_ETR | — | Right nFAULT GPIO |
| PE1 | 140 | — | — | — | Left nFAULT GPIO |
| PE2 | 1 | — | TIM3_ETR | — | EXT GPIO / Trace |
| PE3 | 2 | — | — | — | 绿色 LED |
| PE4 | 3 | — | — | — | 红色 LED |
| PE5 | 4 | — | — | **TIM9_CH1** | Motor RL IN1 |
| PE6 | 5 | — | — | **TIM9_CH2** | Motor RL IN2 |
| PE7 | 57 | TIM1_ETR | — | — | EXT GPIO |
| PE8 | 58 | TIM1_CH1N | — | — | EXT GPIO |
| PE9 | 59 | **TIM1_CH1** | — | — | Encoder RR A |
| PE10 | 62 | TIM1_CH2N | — | — | |
| PE11 | 63 | **TIM1_CH2** | — | — | Encoder RR B |
| PE12 | 64 | TIM1_CH3N | — | — | |
| PE13 | 65 | TIM1_CH3 | — | — | |
| PE14 | 66 | **TIM1_CH4** | — | — | EXT PWM 备选（如 PA11 被 CAN 占用时） |
| PE15 | 67 | TIM1_BKIN | — | — | |

### Port F

| 引脚 | Pin# | AF4 | AF9 | 模拟 | 说明 |
|------|:----:|-----|-----|------|------|
| PF0 | 10 | **I2C2_SDA** | — | — | I2C2_SDA (IMU+EEPROM) |
| PF1 | 11 | **I2C2_SCL** | — | — | I2C2_SCL (IMU+EEPROM) |
| PF2 | 12 | I2C2_SMBA | — | — | IMU INT (EXTI) |
| PF3 | 13 | — | — | ADC3_IN9 | IPROPI FL |
| PF4 | 14 | — | — | ADC3_IN14 | IPROPI FR |
| PF5 | 15 | — | — | ADC3_IN15 | IPROPI RL |
| PF6 | 16 | — | TIM10_CH1 | ADC3_IN4 | Motor FL IN2 (TIM10_CH1) |
| PF7 | 19 | — | TIM11_CH1 | ADC3_IN5 | Motor FR IN2 (TIM11_CH1) |
| PF8 | 20 | — | TIM13_CH1 | ADC3_IN6 | Motor FR IN1 (TIM13_CH1) |
| PF9 | 21 | — | TIM14_CH1 | ADC3_IN7 | Motor FL IN1 (TIM14_CH1) |
| PF10 | 22 | — | — | ADC3_IN8 | IPROPI RR |
| PF11 | 48 | — | — | — | PUSH_BUTTON1 |
| PF12 | 49 | — | — | — | PUSH_BUTTON2 |
| PF13 | 52 | — | — | — | EN_LOC_5V |
| PF14 | 53 | — | — | — | 扩展 GPIO 预留 |
| PF15 | 54 | — | — | — | 扩展 GPIO 预留 |

### Port G

| 引脚 | Pin# | AF7 | AF8 | AF11 | 说明 |
|------|:----:|-----|-----|------|------|
| PG0 | 55 | — | — | — | EXT GPIO |
| PG1 | 56 | — | — | — | EXT GPIO |
| PG2 | 86 | — | — | — | EXT GPIO |
| PG3 | 87 | — | — | — | EXT GPIO |
| PG4 | 88 | — | — | — | EXT GPIO |
| PG5 | 89 | — | — | — | EXT GPIO |
| PG6 | 90 | — | — | — | EXT GPIO |
| PG7 | 91 | USART6_CK | — | — | |
| PG8 | 92 | — | USART6_RTS | ETH_PPS_OUT | |
| PG9 | 122 | — | **USART6_RX** | — | Right nSLEEP (GPIO，无法用 USART6) |
| PG10 | 123 | — | — | — | Left nSLEEP |
| PG11 | 124 | — | — | **ETH_RMII_TX_EN** | **ETH TX_EN 新分配**（从 PB11 迁移） |
| PG12 | 125 | — | USART6_RTS | — | 扩展 GPIO |
| PG13 | 126 | USART6_CTS | — | **ETH_RMII_TXD0** | **ETH TXD0 新分配**（从 PB12 迁移） |
| PG14 | 127 | — | **USART6_TX** | **ETH_RMII_TXD1** | **ETH TXD1 新分配**（从 PB13 迁移；USART6_TX 因此不可用） |
| PG15 | 130 | — | USART6_CTS | — | 扩展 GPIO |

---

## 4. 外设引脚汇总（按外设分类）

### 4.1 以太网 ETH RMII

RMII 模式最少需要 9 根信号线（不含 MDC/MDIO 管理通道也需 2 根），全部使用 AF11。

| 信号 | 推荐引脚（新方案） | 原始引脚 | 说明 |
|------|---------|---------|------|
| ETH_REF_CLK | **PA1** | PA1 | 不可更换，固定 |
| ETH_MDIO | **PA2** | PA2 | 不可更换，固定 |
| ETH_CRS_DV | **PA7** | PA7 | 不可更换，固定 |
| ETH_MDC | **PC1** | PC1 | 不可更换，占用 ADC_IN11 |
| ETH_RXD0 | **PC4** | PC4 | 不可更换，占用 ADC_IN14 |
| ETH_RXD1 | **PC5** | PC5 | 不可更换，占用 ADC_IN15 |
| ETH_TX_EN | **PG11** ✅ | PB11 | 迁移！释放 PB11 给 USART3_RX |
| ETH_TXD0 | **PG13** ✅ | PB12 | 迁移！释放 PB12 给 SPI2_NSS |
| ETH_TXD1 | **PG14** ✅ | PB13 | 迁移！释放 PB13 给 SPI2_SCK |

> **⚠️ 注意**：PG14 兼有 USART6_TX 功能，ETH TXD1 占用后 USART6_TX 不可用。但 USART6 在 PRO 设计中无法启用（RX 端 PC7/PG9 均已占用），故不影响。

### 4.2 USART / UART

| 外设 | TX | RX | AF | 可用性 | 说明 |
|------|----|----|:--:|:------:|------|
| **USART1** | PA9 | PA10 | AF7 | ✅ 已用 | SBC 通信 |
| **USART2** | PD5 | PD6 | AF7 | ✅ 新增 | 调试/扩展串口 |
| **USART3** | PB10 | PB11 | AF7 | ✅ 新增 | ETH TX 迁移至 PG 后释放 |
| USART3 备选 | PD8 | PD9 | AF7 | ✅ | 如 PB10/PB11 被占用 |
| UART4 | PA0 | PA1 | AF8 | ❌ | PA1 是 ETH_REF_CLK |
| UART4 | PC10 | PC11 | AF8 | ❌ | SPI3 占用 |
| UART5 | PC12 | PD2 | AF8 | ❌/⚠️ | PC12 是 SPI3_MOSI，PD2 是 UART5_RX，TX 无法使用 |
| USART6 | PG14 | PG9 | AF8 | ❌ | PG14 被 ETH 占用，PG9 是 nSLEEP |

**结论：PRO 可实现 3 路 UART（USART1/2/3），满足需求。**

### 4.3 I2C

| 外设 | SCL | SDA | AF | 用途 |
|------|-----|-----|:---:|------|
| **I2C1** | PB8 | PB9 | AF4 | WP5 电源板（主板内部，独立总线） |
| I2C1 扩展 | PB6 | PB7 | AF4 | EXT1 扩展（同一 I2C1 总线，共享） |
| **I2C2** | PF1 | PF0 | AF4 | IMU + EEPROM（主板内部） |
| **I2C3** | PA8 | PC9 | AF4 | EXT2 扩展 I2C（PC9 因 Motor RR 迁移至 PC6/PC7 后释放） |

**结论：PRO 可实现 3 路 I2C（I2C1/2/3），满足需求。**

### 4.4 SPI

| 外设 | SCK | MISO | MOSI | NSS | AF | 用途 |
|------|-----|------|------|-----|:---:|------|
| SPI1 | PA5/PB3 | PA6/PB4 | PA7/PB5 | PA4/PA15 | AF5 | ❌ PA7=ETH，PA5=Battery ADC，SPI1 不可用 |
| **SPI2** | **PB13** | **PB14** | **PB15** | PB12 | AF5 | ✅ 标准引脚（ETH TX 迁移至 PG 后全部释放） |
| SPI2 备选 | PB10 | PC2 | PC3 | — | AF5 | ⚠️ PB10=USART3_TX 冲突，不推荐 |
| **SPI3** | PC10 | PC11 | PC12 | PA4/PA15 | AF6 | ✅ LED Strip 专用 |

**结论：PRO 可实现 2 路 SPI（SPI2 标准引脚 / SPI3 LED Strip），满足需求。**

### 4.5 CAN

| 外设 | RX | TX | AF | 用途 |
|------|----|----|----|------|
| **CAN1** | **PA11** | **PA12** | AF9 | ✅ 保留，主要 CAN1 |
| CAN1 备选 | PB8 | PB9 | AF9 | ❌ 被 I2C1 (WP5) 占用 |
| CAN1 备选 | PD0 | PD1 | AF9 | ✅ 可选（目前未用） |
| CAN2 | PB12 | PB13 | AF9 | ⚠️ 与 SPI2_NSS/SCK 冲突，不可同时启用 |

**结论：PRO 实现 CAN1 一路（PA11/PA12），满足需求。**

### 4.6 定时器（与 ROSbot PRO 相关）

| 定时器 | 通道 | 引脚 | 用途 |
|--------|------|------|------|
| TIM1 | CH1/CH2 | PE9/PE11 | Encoder RR A/B |
| **TIM8** | CH1/CH2 | PC6/PC7 | Motor RR IN1/IN2 |
| **TIM8** | CH2N | PB0 | Fan PWM（互补通道） |
| TIM2 | CH1/CH2 | PA15/PB3 | Encoder RL A/B |
| TIM3 | CH1/CH2 | PB4/PB5 | Encoder FR A/B（从 PC6/PC7 迁移） |
| TIM4 | CH1/CH2 | PD12/PD13 | Encoder FL A/B |
| TIM9 | CH1/CH2 | PE5/PE6 | Motor RL IN1/IN2 |
| TIM10 | CH1 | PF6 | Motor FL IN2 |
| TIM11 | CH1 | PF7 | Motor FR IN2 |
| TIM13 | CH1 | PF8 | Motor FR IN1 |
| TIM14 | CH1 | PF9 | Motor FL IN1 |
| TIM1 | CH4 | PE14 | EXT PWM 备选（PA11 被 CAN 保留） |
| TIM5 | 32-bit | — | Runtime Stats |

---

## 5. 冲突分析与设计约束

### 5.1 关键互斥关系

| 引脚 | 功能 A | 功能 B | 结论 |
|------|--------|--------|------|
| PA1 | ETH_RMII_REF_CLK | ADC1_IN1 / UART4_RX | **ETH 优先，ADC/UART4 不可用** |
| PA7 | ETH_CRS_DV | SPI1_MOSI / ADC1_IN7 | **ETH 优先，SPI1/ADC 不可用** |
| PC1 | ETH_MDC | ADC123_IN11 | **ETH 优先，EXT ADC1 改用 PC3** |
| PC4 | ETH_RXD0 | ADC1_IN14 | **ETH 优先** |
| PC5 | ETH_RXD1 | ADC1_IN15 | **ETH 优先** |
| PB10 | USART3_TX | I2C2_SCL | 二选一；PRO 选 USART3_TX |
| PB11 | USART3_RX | ~~ETH_TX_EN~~ | **ETH TX 已迁 PG11**，PB11 可用为 USART3_RX |
| PG14 | ETH_TXD1 | USART6_TX | ETH 优先，USART6_TX 不可用 |
| PC9 | I2C3_SDA | TIM8_CH4 | Motor RR 迁至 PC6/PC7 后，PC9 可用为 I2C3_SDA |
| PC6 | TIM8_CH1 | TIM3_CH1 / USART6_TX | Motor RR 占用；Encoder FR 迁至 PB4 |
| PC7 | TIM8_CH2 | TIM3_CH2 / USART6_RX | Motor RR 占用；Encoder FR 迁至 PB5 |
| PB4 | TIM3_CH1 | SPI3_MISO / NJTRST | Encoder FR A；注意 JTAG 功能但 SWD 不受影响 |
| PA11 | CAN1_RX | TIM1_CH4 / USB_DM | 保留 CAN1；不可用于 PWM |
| PA12 | CAN1_TX | TIM1_ETR / USB_DP | 保留 CAN1 |

### 5.2 设计原则

1. **ETH 引脚不可妥协**：PA1/PA2/PA7/PC1/PC4/PC5 一旦使用以太网必须留给 ETH
2. **ETH TX 可以在 PB 和 PG 之间选择**：选 PG11/PG13/PG14 释放 PB11/PB12/PB13 给 USART3 和 SPI2
3. **SPI1 在 PRO 设计中不可用**（PA7 被 ETH 占用，PA5 被 Battery ADC 占用）
4. **USART6 在 PRO 设计中不可用**（PC6/PC7 被 Motor RR 占用，PG9 被 nSLEEP 占用，PG14 被 ETH 占用）
5. **NJTRST（PB4）解除后 TIM3_CH1 可用**：实际开发板上 JTAG 可能已经通过 `stm32f4xx_hal_rcc.c` 中 `__HAL_RCC_AFIO_FORCE_RESET` 等初始化解除复用，具体需确认固件启动代码

---

## 6. ROSbot PRO 最优引脚分配方案

> 相比 V3.0 方案的核心变化：  
> 1. ETH TX 引脚从 PB11/PB12/PB13 迁移至 **PG11/PG13/PG14**  
> 2. SPI2 使用**标准引脚 PB12/13/14/15**（替代原 PB10/PC2/PC3 方案）  
> 3. 新增 **USART2 (PD5/PD6)** 和 **USART3 (PB10/PB11)**  
> 4. Motor RR 使用 **PC6/PC7 (TIM8_CH1/2)**，释放 PC9 给 I2C3_SDA  
> 5. EXT ADC1 从 PC1（ETH冲突）改为 **PC3**  
> 6. EXT1 SPI2 引脚更新为 PB12/13/14/15

### 6.1 最优引脚分配总表

| 功能 | 引脚 | 外设/总线 | AF | 备注 |
|------|------|------------|:---:|------|
| **通信** | | | | |
| USART1 TX/RX (SBC) | PA9/PA10 | USART1 | AF7 | ✅ 保留 |
| **USART2 TX/RX (Debug/扩展)** | PD5/PD6 | USART2 | AF7 | 🆕 新增 |
| **USART3 TX/RX (Debug/扩展)** | PB10/PB11 | USART3 | AF7 | 🆕 ETH TX 迁移后释放 |
| I2C1 SCL/SDA (WP5) | PB8/PB9 | I2C1 | AF4 | ✅ 保留 |
| I2C2 SCL/SDA (IMU+EEPROM) | PF1/PF0 | I2C2 | AF4 | ✅ 保留 |
| **I2C3 SCL/SDA (EXT I2C3)** | PA8/PC9 | I2C3 | AF4 | 🔄 PC9 因 Motor RR 迁移而释放 |
| **SPI2 (EXT 扩展)** | PB13/PB14/PB15 (NSS:PB12) | SPI2 | AF5 | 🔄 标准引脚，ETH TX 迁移后释放 |
| SPI3 (LED Strip) | PC10/PC11/PC12 | SPI3 | AF6 | ✅ 保留 |
| **CAN1 (保留)** | PA11(RX)/PA12(TX) | CAN1 | AF9 | ✅ 保留 |
| **以太网** | | | | |
| ETH REF_CLK | PA1 | ETH | AF11 | 固定 |
| ETH MDIO/MDC | PA2/PC1 | ETH | AF11 | 固定，PC1 占用 ADC_IN11 |
| ETH CRS_DV | PA7 | ETH | AF11 | 固定 |
| ETH RXD0/1 | PC4/PC5 | ETH | AF11 | 固定 |
| **ETH TX_EN** | **PG11** | ETH | AF11 | 🔄 从 PB11 迁移 |
| **ETH TXD0** | **PG13** | ETH | AF11 | 🔄 从 PB12 迁移 |
| **ETH TXD1** | **PG14** | ETH | AF11 | 🔄 从 PB13 迁移 |
| **电机与编码器** | | | | |
| Motor FL IN1/IN2 | PF9/PF6 | TIM14_CH1 / TIM10_CH1 | AF9/AF3 | ✅ 保留 |
| Motor FR IN1/IN2 | PF8/PF7 | TIM13_CH1 / TIM11_CH1 | AF9/AF3 | ✅ 保留 |
| Motor RL IN1/IN2 | PE5/PE6 | TIM9_CH1/2 | AF3 | ✅ 保留 |
| **Motor RR IN1/IN2** | **PC6/PC7** | **TIM8_CH1/2** | AF3 | 🔄 从 PC8/PC9 迁移 |
| Fan PWM | PB0 | TIM8_CH2N | AF3 | ✅ 保留 |
| Encoder FL A/B | PD12/PD13 | TIM4_CH1/2 | AF2 | ✅ 保留 |
| **Encoder FR A/B** | **PB4/PB5** | **TIM3_CH1/2** | AF2 | 🔄 从 PC6/PC7 迁移 |
| Encoder RL A/B | PA15/PB3 | TIM2_CH1/2 | AF1 | ✅ 保留 |
| Encoder RR A/B | PE9/PE11 | TIM1_CH1/2 | AF1 | ✅ 保留 |
| **ADC** | | | | |
| Battery ADC | PA5 | ADC1_CH5 | Analog | ✅ 保留 |
| NTC ADC | PB1 | ADC1_CH9 | Analog | ✅ 保留 |
| IPROPI FL/FR/RL/RR | PF3/PF4/PF5/PF10 | ADC3 | Analog | ✅ 保留 |
| **EXT_ADC1** | **PC3** | ADC1/2/3_CH13 | Analog | 🔄 从 PC1(ETH冲突) 改为 PC3 |
| EXT_ADC2 | PC2 | ADC1/2/3_CH12 | Analog | ✅ 无冲突（SPI2 迁走后） |
| EXT_ADC3 | PC0 | ADC1/2/3_CH10 | Analog | ✅ 保留 |
| **音频/DAC** | | | | |
| Audio DAC | PA4 | DAC_OUT1 | Analog | ✅ 保留 |
| Audio SHDN | PB2 | GPIO | — | ✅ 保留 |
| **GPIO / 特殊** | | | | |
| Right nSLEEP | PG9 | GPIO | — | ✅ 保留 |
| Left nSLEEP | PG10 | GPIO | — | ✅ 保留 |
| Right nFAULT | PE0 | GPIO | — | ✅ 保留 |
| Left nFAULT | PE1 | GPIO | — | ✅ 保留 |
| PB_SHD_DETECT | PD4 | GPIO | — | ✅ 保留 |
| PUSH_BUTTON1/2 | PF11/PF12 | GPIO | — | ✅ 保留 |
| EN_LOC_5V | PF13 | GPIO | — | ✅ 保留 |
| IMU INT | PF2 | EXTI | — | ✅ 保留 |
| RED/GRN LED | PE4/PE3 | GPIO | — | ✅ 保留 |
| Runtime Stats | TIM5 | 32-bit | — | ✅ 保留 |
| SWD Debug | PA13/PA14 | Debug | — | ✅ 保留 |
| **EXT 扩展 PWM** | PE14 | TIM1_CH4 | AF1 | 🔄 替代 PA11(CAN 保留) |

### 6.2 外设资源总结

| 外设 | 数量 | 实现 | 说明 |
|------|:----:|------|------|
| UART | **3路** | USART1, USART2, USART3 | ✅ 满足 3-4 路目标 |
| I2C | **3路** | I2C1, I2C2, I2C3 | ✅ 满足 3 路目标 |
| SPI | **2路** | SPI2, SPI3 | ✅ 满足 2 路目标 |
| CAN | **1路** | CAN1 (PA11/PA12) | ✅ 满足 1 路目标 |
| ETH | **1路** | RMII, LAN9303 | ✅ 保留 |
| ADC | **11通道** | ADC3×4(IPROPI), ADC1×2(Bat/NTC), ADC3×3(EXT) | ✅ |

### 6.3 空闲/可扩展引脚

| 引脚 | 可用功能 | 备注 |
|------|---------|------|
| PC8 | GPIO / TIM8_CH3 / SDIO | Motor RR 迁走后空闲 |
| PG0~PG6 | GPIO | 通用扩展 |
| PG12/PG15 | GPIO / USART6_RTS | 通用扩展 |
| PD0/PD1 | GPIO / CAN1 备选 | FSMC/CAN 扩展 |
| PD2 | UART5_RX | 可扩展第4路 UART（TX 仅 PC12/SPI3冲突） |
| PD14/PD15 | TIM4_CH3/4 | 扩展定时器 |
| PE7/PE8 | GPIO / TIM1_CH1N | EXT GPIO |
| PE10/PE12-15 | TIM1_CH2N-4 | 扩展 PWM |
| PF14/PF15 | GPIO | 通用扩展 |
| PA3 | TIM9_CH2 / ADC / USART2_RX | USART2 备选 RX |
| PA0 | WKUP / TIM2_CH1 / ADC | 唤醒或通用 |

---

*本文档由 STM32F407ZET6 PeripheralPins.c (framework-arduinoststm32 variant) 自动提取并经人工校验生成。如有疑问以 ST 官方数据手册 DS8626 为准。*
