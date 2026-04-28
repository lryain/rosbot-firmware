# ROSbot PRO STM32F407ZET6 LQFP-144 引脚优化（V3.1）

## 关键优化点（相对 V3.0）

### ETH TX 引脚迁移
- 旧: PB11(TX_EN), PB12(TXD0), PB13(TXD1) — 全部 AF11
- 新: **PG11(TX_EN), PG13(TXD0), PG14(TXD1)** — 全部 AF11 ✅ 已在 PeripheralPins.c 验证

### 级联释放：USART3 + SPI2 标准引脚
- PB10 → USART3_TX (AF7) — 原 SPI2_SCK，现空闲
- PB11 → USART3_RX (AF7) — 原 ETH_TX_EN，现空闲
- PB12 → SPI2_NSS (AF5) — 原 ETH_TXD0，现空闲
- PB13 → SPI2_SCK (AF5) — 原 ETH_TXD1，现标准 SPI2
- PB14 → SPI2_MISO (AF5) — 未变，现标准 SPI2
- PB15 → SPI2_MOSI (AF5) — 未变，现标准 SPI2

### Motor RR + I2C3 释放
- Motor RR: PC8/PC9 → **PC6(TIM8_CH1) + PC7(TIM8_CH2)** （与编码器 PC6/PC7 不冲突，因 Encoder FR 已迁至 PB4/PB5）
- PC9 → **I2C3_SDA (AF4)** EXT2 扩展 I2C3（与 PA8=I2C3_SCL 组合）
- PC8 → TIM8_CH3 EXT_PWM1（空闲，Motor RR 迁移后）

### ADC 冲突修复
- PC1 是 ETH_MDC，不可用作 ADC ❌
- EXT_ADC1 改用 **PC3 (ADC123_CH13)** ✅
- EXT_ADC2 = PC2 (ADC123_CH12)（SPI2 迁到 PB 后无冲突）✅
- EXT_ADC3 = PC0 (ADC123_CH10)✅

### 新增串口
- USART2: PD5(TX)/PD6(RX) AF7 — 完全空闲 ✅
- USART3: PB10(TX)/PB11(RX) AF7 — ETH TX 迁移后释放 ✅
- 总计: **3路 UART**（USART1 SBC + USART2 + USART3）

## 最终外设规格
| 外设 | 数量 | 引脚 |
|------|----|-----|
| UART | 3路 | USART1(PA9/10), USART2(PD5/6), USART3(PB10/11) |
| I2C | 3路 | I2C1(PB8/9,WP5), I2C2(PF0/1,IMU), I2C3(PA8/PC9,EXT2) |
| SPI | 2路 | SPI2(PB12-15,EXT1), SPI3(PC10-12,LED) |
| CAN | 1路 | CAN1(PA11/PA12) |
| ETH | 1路 | RMII via PG11/13/14(TX) + PA1/2/7+PC1/4/5 |

## 关键文档
- `docs/STM32F407ZET6_LQFP144_Pinmap.md` — 完整 GPIO AF 参考（从 PeripheralPins.c 提取）
- `docs/ROSbot_PRO综合评估与修订方案.md` — PRO 板设计方案（V3.1）

## USART6 不可用原因
PC6/PC7(Motor RR占用) + PG9(nSLEEP占用) + PG14(ETH TXD1占用) → USART6 完全没有可用引脚
