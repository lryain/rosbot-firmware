# ROSbot Lite 设计与实施文档

> **版本**: V1.0  
> **日期**: 2026-04-13  
> **目标板**: ROSbot Lite  
> **MCU**: STM32F407VET6 (LQFP-100, 512KB Flash, 192KB SRAM)  
> **当前编译验证板型**: `rosbot_xl_digital_board`  
> **引脚参考**: [STM32F407VET6_LQFP100_Pinmap.md](STM32F407VET6_LQFP100_Pinmap.md)

## 1. 设计目标

ROSbot Lite 作为无以太网版本，目标是用更少的封装引脚保留尽可能完整的机器人外设能力：

- 串口保留 2-3 路
- I2C 保留 3 路
- SPI 保留 2 路
- CAN 保留 1 路
- 电机、编码器、IMU、LED Strip、Witty Pi 5、电池采样全部保留
- micro-ROS 走串口，不依赖 Ethernet / LwIP / LAN9303

## 2. 总体策略

Lite 版与 PRO 版的最大差异只有两点：

1. **无 Ethernet MAC**：不再分配任何以太网相关资源
2. **封装缩减到 LQFP-100**：PF/PG 不可用，所有高编号外设必须重排

因此 Lite 版的核心思路是：

- 用 `USART1` 作为主 micro-ROS 通信口
- 用 `USART2` 作为调试/扩展串口
- 用 `USART3` 作为第 3 路备用串口
- 用 `PB10/PB11` 作为 `I2C2`
- 用 `PB6/PB7` 作为 `I2C1`
- 用 `PA8/PC9` 作为 `I2C3`
- 用 `PB12/PB13/PB14/PB15` 作为标准 `SPI2`
- 用 `PC10/PC11/PC12` 作为 `SPI3`
- 保留 `PA11/PA12` 给 `OTG FS`，`CAN1` 迁到 `PD0/PD1`

## 3. 最终外设分配摘要

### 通信

| 外设 | 引脚 | 说明 |
|------|------|------|
| USART1 | PA9 / PA10 | 主 micro-ROS 通信口 |
| USART2 | PD5 / PD6 | 调试 / 扩展串口 |
| USART3 | PD8 / PD9 | 备用串口 |
| USART6 | PG14 / PG9 | 备用串口 |
| I2C1 | PB6 / PB7 | WP5 / EXT1 扩展 |
| I2C2 | PB10 / PB11 | IMU + EEPROM |
| SPI2 | PB12 / PB13 / PB14 / PB15 | EXT1 扩展 SPI |
| SPI3 | PC10 / PC11 / PC12 | LED Strip |
| USB OTG FS | PA11 / PA12 | 主 USB 口 |
| CAN1 | PD0 / PD1 | 保留 |

### 电机与编码器

| 功能 | 引脚 | 定时器 |
|------|------|--------|
| Motor FL | PA6 (TIM13\_CH1) / PA7 (TIM14\_CH1) | TIM13 / TIM14 |
| Motor FR | PB8 (TIM10\_CH1) / PB9 (TIM11\_CH1) | TIM10 / TIM11 |
| Motor RL | PE5 (TIM9\_CH1) / PE6 (TIM9\_CH2) | TIM9 |
| Motor RR | PC6 (TIM8\_CH1) / PC7 (TIM8\_CH2) | TIM8 |
| Fan PWM | PB0 | TIM8_CH2N |
| Encoder FL | PD12 / PD13 | TIM4 |
| Encoder FR | PB4 / PB5 | TIM3 |
| Encoder RL | PA15 / PB3 | TIM2 |
| Encoder RR | PE9 / PE11 | TIM1 |

### 模拟量

| 功能 | 引脚 | 说明 |
|------|------|------|
| Battery ADC | PA5 | 电池电压采样 |
| NTC ADC | PB1 | 温度采样 |
| IPROPI_FL | PC4 | 电流检测 |
| IPROPI_FR | PC5 | 电流检测 |
| IPROPI_RL | PC1 | 电流检测 |
| IPROPI_RR | PC0 | 电流检测 |
| Audio DAC | PA4 | 音频输出 |
| Audio SHDN | PE10 | 音频功放关闭 |
| EXT_ADC1 | PC3 | 扩展模拟输入 |
| EXT_ADC2 | PC2 | 扩展模拟输入 |
| EXT_ADC3 | PC1 | 扩展模拟输入 |
| EXT_ADC4 | PC0 | 扩展模拟输入 |

## 4. 完整引脚分配表（VET6 LQFP-100 最终方案）

| 功能 | 引脚 | 外设/总线 | AF | 备注 |
|------|------|------------|:---:|------|
| I2C2_SCL (IMU+EEPROM) | PB10 | I2C2 | AF4 | 唯一可用 I2C2 引脚（无 PF0/1） |
| I2C2_SDA (IMU+EEPROM) | PB11 | I2C2 | AF4 | 同上 |
| I2C1_SCL (WP5/EXT1) | PB6 | I2C1 | AF4 | PB8 被 Motor FR 占用 |
| I2C1_SDA (WP5/EXT1) | PB7 | I2C1 | AF4 | PB9 被 Motor FR 占用 |

| USB OTG FS DM | PA11 | USB OTG FS | AF10 | 主 USB 口 |
| USB OTG FS DP | PA12 | USB OTG FS | AF10 | 主 USB 口 |
| Motor FL IN1 | PA6 | TIM13_CH1 | AF9 | 替代 PRO PF8 (TIM13) |
| Motor FL IN2 | PA7 | TIM14_CH1 | AF9 | 替代 PRO PF9 (TIM14) |
| Motor FR IN1 | PB8 | TIM10_CH1 | AF3 | 替代 PRO PF6 (TIM10) |
| Motor FR IN2 | PB9 | TIM11_CH1 | AF3 | 替代 PRO PF7 (TIM11) |
| Motor RL IN1 | PE5 | TIM9_CH1 | AF3 | 同 PRO |
| Motor RL IN2 | PE6 | TIM9_CH2 | AF3 | 同 PRO |
| Motor RR IN1 | PC8 | TIM8_CH3 | AF3 | OK | |
| Motor RR IN2 | PC9 | TIM8_CH4 | AF3 | OK | |
| Encoder FL A/B | PD12/PD13 | TIM4_CH1/2 | AF2 | 同 PRO |
| Encoder FR A/B | PB4/PB5 | TIM3_CH1/2 | AF2 | 同 PRO |
| Encoder RL A/B | PA15/PB3 | TIM2_CH1/2 | AF1 | 同 PRO |
| Encoder RR A/B | PE9/PE11 | TIM1_CH1/2 | AF1 | 同 PRO |
| SPI3 SCK/MISO/MOSI | PC10/PC11/PC12 | SPI3 | AF6 | LED Strip |
| SPI2 NSS/SCK/MISO/MOSI | PB12/PB13/PB14/PB15 | SPI2 | AF5 | EXT1/NRF24L01；无 ETH 无冲突 |
| USART1 TX/RX | PA9/PA10 | USART1 | AF7 | SBC 主串口 |
| USART2 TX/RX | PD5/PD6 | USART2 | AF7 | 调试/扩展 |
| USART3 TX/RX | PD8/PD9 | USART3 | AF7 | 备用；PB10/11 由 I2C2 占用 |
| USART6 TX/RX | PC6/PC7 | USART6 | AF7 | 舵机 | OK | |
| CAN1 RX/TX | PD0/PD1 | CAN1 | AF9 | 迁移自 PA11/PA12，保留给 OTG FS |
| Fan PWM | PB0 | TIM8_CH2N | AF3 | 20kHz complementary |
| NTC ADC | PB1 | ADC1_CH9 | Analog | |
| Battery ADC | PA5 | ADC1_CH5 | Analog | |
| Audio DAC | PA4 | DAC_OUT1 | Analog | |
| Audio SHDN | PE10 | GPIO | 改到别处，PD2 预留 SBUS | |
| IPROPI FL/FR/RL/RR | PC4/PC5/PA3/PA2=>PC1/PC0 | ADC1/2 | Analog | VET6 无 PF3~10 ADC3，用 ADC1/2 |
| EXT_ADC1/2/3/4 | PC3/PC2/PC1/PC0 | ADC123 | Analog | PC1 预留 |
| Left nSLEEP | PD10 | GPIO | — | |
| Right nSLEEP | PD11 | GPIO | — | PC8 改作 EXT |
| Left nFAULT | PE0 | GPIO | — | 同 PRO |
| Right nFAULT | PE1 | GPIO | — | 同 PRO |
| PB_SHD_DETECT | PD4 | GPIO | — | |
| PB_SHD_CONFIRM | PD7 | GPIO | — | |
| IMU INT | PD3 | EXTI | — | 替代 PRO PF2 |
| PUSH_BUTTON1 | PE12 | GPIO | — | 普通 GPIO |
| PUSH_BUTTON2 | PE15 | GPIO | — | 普通 GPIO |
| EN_LOC_5V | PE2 | GPIO | — | 替代 PRO PF13 |
| SBUS | PD2 | GPIO | — | SBUS |
| RED LED | PE4 | GPIO | — | 同 PRO |
| GRN LED | PE3 | GPIO | — | 同 PRO |
| Runtime Stats | TIM5 | 32-bit | — | 仅在开发时启用 |所以可以释放出 PA0-PA3
| SWD Debug | PA13/PA14 | Debug | — | |
| SWD Debug | PA13/PA14 | Debug | — | |

### EXT 扩展端口

| 名称 | 引脚 | 说明 |
|------|------|------|
| EXT_GPIO1 | PA8 | EXT 扩展 / GPIO |
| EXT_GPIO2 | PA0 | EXT 扩展 / ADC / GPIO |
| EXT_GPIO3 | PA1 | EXT 扩展 / ADC / GPIO |
| EXT_GPIO5 | PC14 | EXT 扩展 / 仅在不使用外部 32.768kHz 晶振时可用 |
| EXT_GPIO7 | PD14 | EXT 扩展 / GPIO 或 PWM |
| EXT_GPIO8 | PD15 | EXT 扩展 / GPIO 或 PWM |
PA0、PA1 = UART4
PE10、PE12、PE13、PD14、PD15 = EXT_GPIO1,2,3,4,5

## 5. 软件实现

ROSbot Lite 的代码目录已经建立：

- [include/rosbot_lite/config.hpp](../include/rosbot_lite/config.hpp)
- [src/rosbot_lite/main.cpp](../src/rosbot_lite/main.cpp)
- [src/rosbot_lite/ros.cpp](../src/rosbot_lite/ros.cpp)
- [src/rosbot_lite/rtos.cpp](../src/rosbot_lite/rtos.cpp)

实现方式：

- `include/config.hpp` 通过 `ROSBOT_LITE` 选择 Lite 配置
- `main.cpp` 初始化电机、编码器、IMU、LED、串口 micro-ROS
- `ros.cpp` 注册 battery / imu / joint_state / current / stall 等发布器
- `rtos.cpp` 运行控制循环、IMU 采样、LED Strip、通信任务

## 6. 编译配置

| IPROPI FL/FR/RL/RR | PC4/PC5/PC1/PC0 | ADC1/2 | Analog | VET6 无 PF3~10 ADC3，用 ADC1/2 (PA3 → WS2812 数据线) |
### PlatformIO

Lite 环境当前编译验证使用现有 husarion board preset，后续可再补 VET6 专用 board JSON：

- `board = rosbot_xl_digital_board`
- `-D ROSBOT_LITE`
- `build_src_filter = +<rosbot_lite/*>`

### 编译命令

```bash
platformio run -e rosbot_lite
```

如需发布版本：

```bash
platformio run -e rosbot_lite_release
```

## 7. 设计注意事项

1. `PB10/PB11` 仅能二选一给 `I2C2` 或 `USART3`，当前选择 `I2C2`，`USART3` 改用 `PD8/PD9`。
2. `PA11/PA12` 始终保留给 `CAN1`，不再作为 PWM 或 USB 使用。
3. Motor RR 使用 PC6/PC7 (TIM8\_CH1/2)，**PC8 就此释放为 Right nSLEEP**，**PC9 释放为 I2C3\_SDA**。
4. `PUSH_BUTTON1/2` 已改用 `PE12/PE15`，因此 `PC0/PC1` 可以保留给模拟扩展或其他普通 IO。
5. `PD0/PD1` 现作为 `CAN1`，`PA11/PA12` 保留给 `OTG FS`。
6. `I2C3` 不再启用，因为 `PC9` 已经被最终方案占用，Lite 版不强制保留第三路 I2C。
7. 无 Ethernet MAC，不需要 LAN9303 或 RMII 相关布线；`micro_ros_arduino` 编译依赖里保留 `STM32Ethernet` 和 `LwIP`，但运行路径不使用它们。
8. FL 电机用 PA6 (TIM13\_CH1) + PA7 (TIM14\_CH1)，FR 用 PB8 (TIM10\_CH1) + PB9 (TIM11\_CH1)，这与 PRO 的 TIM 通道组合一致（TIM13+TIM14 为 FL，TIM10+TIM11 为 FR）。
9. SPI2 在 LITE 中可用标准引脚 PB12(NSS)/PB13(SCK)/PB14(MISO)/PB15(MOSI)，因无以太网 PG 引脚冲突问题。

## 8. 当前状态

- 引脚参考文档已完成
- Lite 板级配置已接入 `config.hpp`
- Lite 平台已改用 `genericSTM32F407VET6`
- 下一步是完成 `platformio run -e rosbot_lite` 的编译验证

## 9. 结论

ROSbot Lite 在 LQFP-100 约束下仍然能保留完整的机器人控制能力，并且实现了：

- 3 路 UART
- 2 路 I2C
- 2 路 SPI
- 1 路 CAN
- 4 路编码器
- 4 路电机驱动
- 4 路 IPROPI 电流检测

这使其适合做一个没有 Ethernet 的轻量版 ROS 开发板。
