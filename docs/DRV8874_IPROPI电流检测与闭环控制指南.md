# DRV8874 IPROPI — 电流检测、保护与闭环控制完全指南

> 文档基于：德州仪器 DRV8874PWPR 数据手册 SLVSER5  
> 适用固件：ROSbot PRO (STM32F407ZET6 + DRV8874)  
> 最后更新：2025-06

---

## 目录

1. [IPROPI 硬件原理（数据手册解读）](#1-ipropi-硬件原理)
2. [ROSbot PRO 实际电路参数](#2-rosbot-pro-实际电路参数)
3. [IPROPI 的四大用途](#3-ipropi-的四大用途)
4. [局限性与常见误区](#4-局限性与常见误区)
5. [应用一：电流实时监测](#5-应用一电流实时监测)
6. [应用二：堵转检测与保护](#6-应用二堵转检测与保护)
7. [应用三：软件过流保护](#7-应用三软件过流保护)
8. [应用四：为什么不采用电流闭环](#8-应用四为什么不采用电流闭环)
9. [ROS 2 集成方案](#9-ros-2-集成方案)
10. [固件现状与扩展路径](#10-固件现状与扩展路径)
11. [参数整定指南](#11-参数整定指南)
12. [电流闭环专业性评估](#12-电流闭环专业性评估)
13. [后续建议](#13-后续建议)

---

## 1. IPROPI 硬件原理

### 1.1 电流镜工作机制

DRV8874 内部包含一个**精密电流镜（Current Mirror）**，其结构为：

---

## 9. ROS 2 集成方案

### 9.1 当前集成状态

IPROPI 数据在 ROS 2 侧只做三件事：

```
/rosbot_pro/_motors/current  [std_msgs/Float32MultiArray]
/rosbot_pro/_motors/stall    [std_msgs/UInt8]
/rosbot_pro/_motors/feedback [sensor_msgs/JointState]
```

### 9.2 JointState.effort 的使用

`JointState.effort` 现在直接填入电流值 [A]，用于可视化、记录和诊断，不再承担力矩控制语义。

### 9.3 上位机怎么用

推荐用法很简单：

1. 导航时看 `_motors/current` 判断负载和异常。
2. 遇到堵转时看 `_motors/stall` 做安全处理。
3. `JointState.effort` 只作为监控数据，不做控制输入。

### 9.4 nav2 对电流的使用

nav2 本身不直接使用电机电流数据，但上位机可以把它转成诊断信息：

1. 电流异常时上报 diagnostics。
2. 电流持续偏高时提醒可能的地形负载或机械阻塞。
3. 必要时触发安全停止。

---

## 10. 固件现状与扩展路径

### 10.1 已实现（V1.1，当前版本）

```
已实现 ✅
├── hardware: IPROPI 采样（ADC3，4通道）
├── firmware: update() 中 IIR 滤波采样（α=0.5）
├── firmware: 软件硬 OCP（电流 > max_current → 立即刹车）
├── firmware: 软件软 OCP（电流 80%~100% 时线性降低 PWM）
├── firmware: 堵转检测（速度低 + 电流高 + 有指令，250ms 去抖）
├── firmware: stall_active 标志位，由 getData() 暴露
├── firmware: JointState.effort = 电流值 [A]（非 PWM 占空比）
├── firmware: MotorCurrentPublisher → _motors/current @ 200Hz
├── firmware: MotorStallPublisher → _motors/stall @ 200Hz（UInt8 位掩码，bit i = 电机 i）
└── firmware: 不包含电流闭环，保持速度环 + 保护层的简单架构
```

### 10.2 待实现功能

```
待实现（按优先级）

1. ADC 升至 12-bit（可选精度提升）
   └── 约 2h，修改 IPROPI_ADC_MAX = 4095.0f + HAL 配置

2. K_IPROPI 出厂校准存 EEPROM（可选）
   └── 约 4h，消除 ±20% 偏差

3. ros2_control effort 接口集成（如未来需要力矩控制再评估）
    └── 仅在明确需要力矩控制时再推进，不作为当前版本目标
```

### 10.3 固件版本路线图

| 版本 | 功能 | 状态 |
|-----|------|------|
| V1.0 | IPROPI 电流监测，发布 topic | ✅ 完成 |
| V1.1 | 软件 OCP + 堵转检测 + JointState.effort = 电流 | ✅ 完成 |
| V2.0 | 仅在未来确有力矩控制需求时再考虑 ros2_control effort 接口 | 计划中 |

---

## 11. 参数整定指南

### 11.1 ADC 精度提升（可选）

当前使用 `analogRead()` 返回 10-bit 值。STM32F407 ADC 支持 12-bit（4096量程）。若精度不足，可通过 HAL 直接配置 ADC3：

```cpp
// 12-bit ADC 配置（精度提升 4 倍 → 约 1.73 mA/count）
ADC_HandleTypeDef hadc3;
hadc3.Init.Resolution = ADC_RESOLUTION_12B;  // vs. ADC_RESOLUTION_10B
```

对应修改 `IPROPI_ADC_MAX = 4095.0f`。

### 11.2 电流读数滤波（已实现）

> **当前固件已在 `update()` 中实现 IIR 低通滤波，无需额外修改。**

滤波在 `motor_drv8874.cpp` 的 `update()` 中执行（每 5ms @ 200 Hz）：

```cpp
// IIR 低通滤波（α = 0.5）：每次 update() 采样并滤波，getData() 直接返回缓存值
constexpr float kAlpha = 0.5f;
const float raw = static_cast<float>(analogRead(cfg_.ipropi_pin)) * cfg_.ipropi_scale;
filtered_current_ = kAlpha * raw + (1.0f - kAlpha) * filtered_current_;
```

如需更强滤波（牺牲响应速度），将 `kAlpha` 改为 0.1~0.3。

### 11.3 K_IPROPI 工厂偏差校准（可选）

由于 K 偏差 ±20%，可通过外部电流计校准每块 PCB：

1. 以已知电流驱动电机（使用外部精密电流计）
2. 读取 ADC 原始值并计算实际换算系数
3. 将校准值存入 EEPROM

```cpp
// EEPROM 校准读取
float calibrated_scale = eeprom_read_float(EEPROM_IPROPI_SCALE_ADDR);
if (calibrated_scale > 0.0f) cfg_.ipropi_scale = calibrated_scale;
```

### 11.4 过流和堵转阈值参考

| 运行状态 | 典型电流 | 固件阈值（`motor_drv8874.hpp` 默认值） |
|---------|---------|------------------------------------|
| 空载 | 0.1-0.5 A | — |
| 正常负载行驶 | 0.5-2.0 A | — |
| 爬坡负载 | 1.0-3.5 A | — |
| 堵转检测触发电流阈值 | — | **3.5 A**（`stall_current`）|
| 堵转检测速度阈值 | — | **0.15 rad/s**（`stall_vel`）|
| 堵转去抖时间 | — | **250 ms**（`stall_ms`）|
| 软件软 OCP 开始缩减 | — | **4.0 A**（max_current × 80%）|
| 软件硬 OCP 立即刹车 | — | **5.0 A**（`max_current`）|
| DRV8874 硬件 OCP（芯片内置） | — | ~8 A（芯片自动关断 nFAULT）|

> **注意**：DRV8874 内部有硬件过流保护（OCP），当电流超过约 8A 时芯片自动关断并拉低 nFAULT。软件 OCP 是在这个硬件保护之**前**的额外保护层。三层保护：软件软 OCP（5A 以下降额）→ 软件硬 OCP（5A 刹车）→ 硬件 OCP（8A 芯片关断）。

---

## 12. 电流闭环专业性评估

### 12.1 结论

电流闭环在工业电机控制里是专业方案，但**对 ROSbot PRO 当前的室内导航任务不必需**。我们保留 IPROPI 的原因是做监测、保护和诊断，不是为了把它升级成完整力矩控制链路。

### 12.2 评估要点

对当前硬件和任务来说，电流闭环的问题主要不是“理论上能不能做”，而是“值不值得做”：

- 控制任务频率只有 200 Hz，和典型电流环带宽不是一个量级。
- IPROPI 本身有器件偏差，做高精度力矩控制前要先标定。
- 室内导航更需要稳定、可维护，而不是增加一层难调的内环。

### 12.3 建议

当前版本不启用电流闭环，是更稳妥的工程选择。若未来要做接触操作、力矩控制或机械臂类项目，再单独设计新的电流/力矩控制方案。

---

## 13. 后续建议

### 13.1 当前版本

保持现状即可：IPROPI 只用于电流上报、软件过流保护和堵转检测，不做电流闭环。

### 13.2 未来如果要做

如果以后真的需要电流/力矩控制，建议先补齐这些前提：

- 先做电机电阻、电感和 K_IPROPI 的实测标定。
- 先把控制频率提升到更适合内环的水平。
- 先明确 ROS2 侧是否真的需要 effort 接口，而不是只做监测。

### 13.3 本文档结论

本仓库当前的推荐方案是：**速度环 + 软件 OCP + 堵转检测 + 电流监测**。不引入电流闭环，能让系统更简单，也更容易稳定交付。

---

## 附录：关键代码索引

| 功能 | 文件 | 关键位置 |
|------|------|---------|
| IPROPI 常量 + OCP 阈值 | `include/rosbot_pro/config.hpp` | `IPROPI_*`, `IPROPI_OC_*` |
| OCP + 堵转 + 电流环逻辑 | `lib/motor/motor_drv8874.cpp` | `update()` 函数 |
| 电流/堵转数据结构 | `lib/motor/motor_interface.hpp` | `MotorData` struct |
| 聚合数据 | `lib/motor/motor_array.hpp` | `MotorsData` struct |
| 电流 ROS 2 发布器 | `lib/ros/ros/publishers/motor_current_publisher.hpp` | 全文 |
| **堵转 ROS 2 发布器** | `lib/ros/ros/publishers/motor_stall_publisher.hpp` | 全文（新增）|
| JointState.effort = 电流 | `lib/ros/ros/publishers/joint_state_publisher.hpp` | `fillMsg()` |
| FreeRTOS 任务 | `src/rosbot_pro/rtos.cpp` | `motorControlTask`, `encoderTask` |
| 发布器注册 | `src/rosbot_pro/ros.cpp` | publishers vector |
