### 7.4 完整引脚分配表（ZET6 最终方案）

| 功能 | 引脚 | 定时器/总线 | AF | 备注 |
|------|------|------------|:---:|------|
| I2C2_SDA (IMU+EEPROM) | PF0 | I2C2 | AF4 | OK | |
| I2C2_SCL (IMU+EEPROM) | PF1 | I2C2 | AF4 | OK | |
| I2C1_SCL (WP5) | PB8 | I2C1 | AF4 | OK | |
| I2C1_SDA (WP5) | PB9 | I2C1 | AF4 | OK | |
| I2C3_SCL (EXT I2C3) | PA8 | I2C3 | AF4 | OK EXT2 扩展 |
| I2C3_SDA (EXT I2C3) | PC9 | I2C3 | AF4 | OK EXT2 扩展；Motor RR 迁移后释放 |
| Motor FL IN1 | PF9 | TIM14_CH1 | AF9 | OK | |
| Motor FL IN2 | PF6 | TIM10_CH1 | AF3 | OK | |
| Motor FR IN1 | PF8 | TIM13_CH1 | AF9 | OK | |
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
| USART6 TX/RX | PG14/PG9 | USART6 | AF7 | SBC | OK | |
| ETH RMII | PA1/2/7,**PB11/12/13**,PC1/4/5 | ETH | AF11 | LAN9303 | OK | |
| Right nSLEEP | PG11 | GPIO | — | OK 释放 PG9 USART6 |
| Left nSLEEP | PG10 | GPIO | — | OK | |
| Right nFAULT | PE0 | GPIO | — | OK | |
| Left nFAULT | PE1 | GPIO | — | OK | |
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

注明：
USART3 确定改到 PD8/PD9，PB10 作为 SPI2_SCK 
由于我们必须要启用ethernet，并且需要启用SPI2（PB10，14，15），SPI2_SCK只能使用PB10和PB13，而PB13被ETH_RMII占用
由于PB12(NSS)被ETH_RMII占用，所以SPI2做出妥协，PB12(NSS) 换成 PE15
需要确认的是硬件的PB12(NSS) 换成 PE15软件CS区别大吗？性能是否够用？
然后 ETH_RMII 使用 PA1/2/7,**PB11/12/13**,PC1/4/5 这些引脚是否合适？

