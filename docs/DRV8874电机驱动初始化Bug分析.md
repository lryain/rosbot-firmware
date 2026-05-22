
# DRV8874 电机驱动初始化Bug分析报告

> 现象：ROSbot Lite控制板上电后，未连接SBC，电机持续转动不停

---

## 一、DRV8874 IN/IN 模式真值表（来自数据手册）

这是分析的基础，必须首先明确：

| IN1 | IN2 | 输出状态 | 说明 |
|-----|-----|---------|------|
| PWM | LOW | 正转 | IN1占空比控制速度 |
| LOW | PWM | 反转 | IN2占空比控制速度 |
| LOW | LOW | **低侧制动** | 两个低侧FET导通，电机短路制动 |
| HIGH | HIGH | **高侧制动** | 两个高侧FET导通，电机短路制动 |
| — | — | **滑行(Coast)** | **必须nSLEEP=LOW**，输出Hi-Z高阻态 |

> ⚠️ **关键点**：DRV8874的COAST（滑行）模式**只能通过nSLEEP=LOW实现**，不存在IN1/IN2的组合可以实现COAST！

---

## 二、Bug #1：BRAKE和COAST模式实现错误（严重）

### 问题代码

`lib/motor/motor_drv8874.cpp` 第84-95行：

```cpp
case MotorMode8874::BRAKE_8874:
    // 注释写的是：IN1=HIGH, IN2=HIGH → DRV8874 both outputs LOW (active low-side brake / fast decay)
    pwm_timer_1_->setCaptureCompare(pwm_channel_1_, pwm_arr_);  // IN1 = 100% = HIGH
    pwm_timer_2_->setCaptureCompare(pwm_channel_2_, pwm_arr_);  // IN2 = 100% = HIGH
    break;

case MotorMode8874::COAST_8874:
default:
    pwm_timer_1_->setCaptureCompare(pwm_channel_1_, 0);  // IN1 = 0% = LOW
    pwm_timer_2_->setCaptureCompare(pwm_channel_2_, 0);  // IN2 = 0% = LOW
    break;
```

### 错误分析

| 代码模式 | 代码设置 | 实际DRV8874行为 | 应有行为 |
|---------|---------|---------------|---------|
| BRAKE | IN1=HIGH, IN2=HIGH | 高侧制动 ✅ | 制动 ✅ |
| COAST | IN1=LOW, IN2=LOW | **低侧制动** ❌ | **滑行(Hi-Z)** |

**代码注释说"IN1=HIGH, IN2=HIGH → both outputs LOW"是错误的！** 根据DRV8874真值表：
- IN1=HIGH, IN2=HIGH → 两个高侧FET导通 → **高侧制动**
- IN1=LOW, IN2=LOW → 两个低侧FET导通 → **低侧制动**

两者都是制动，都不是滑行！**COAST模式被错误实现为低侧制动。**

虽然这个Bug本身不会导致电机转动（制动会让电机停），但它意味着**代码中永远无法实现真正的滑行模式**。

### 正确的COAST实现

DRV8874的COAST需要nSLEEP=LOW：

```cpp
case MotorMode8874::COAST_8874:
    // 必须通过nSLEEP=LOW实现Hi-Z，不能仅靠IN1/IN2
    // 需要调用 disableDrivers() 或单独控制nSLEEP引脚
    // 当前代码设置IN1=LOW, IN2=LOW → 实际是低侧制动，不是滑行！
    break;
```

---

## 三、Bug #2：setMode状态去重导致初始化失效（🔥根本原因）

### 问题代码

`lib/motor/motor_drv8874.hpp` 第90行：
```cpp
MotorMode8874 current_mode_ = MotorMode8874::COAST_8874;  // 默认值 = COAST
```

`lib/motor/motor_drv8874.cpp` 第61-63行：
```cpp
void MotorDrv8874::setMode(MotorMode8874 mode) {
  if (mode == current_mode_) return;  // ← 相同模式直接返回！
  current_mode_ = mode;
  ...
```

`lib/motor/motor_drv8874.cpp` 第44行（init()末尾）：
```cpp
setMode(MotorMode8874::COAST_8874);  // ← 被跳过！
```

### 错误分析

执行流程：

```
1. MotorDrv8874对象构造 → current_mode_ = COAST_8874（默认值）
2. init() 被调用：
   a. setPWM(ch1, pin1, 20kHz, 0)  → IN1引脚配置为PWM，0%占空比
   b. setPWM(ch2, pin2, 20kHz, 0)  → IN2引脚配置为PWM，0%占空比
   c. setMode(COAST_8874)          → 因为current_mode_已经是COAST，直接return！
                                      COAST分支的代码从未执行！
```

**虽然setPWM设置了0%占空比，理论上IN1=LOW, IN2=LOW，但setMode被跳过意味着：**

1. **模式状态机从未正确初始化** — `current_mode_`停留在构造时的默认值，而不是通过setMode正确设置
2. **后续第一次setMode调用会执行完整切换** — 当PID首次调用applyPWM时，如果输出接近0，会调用`setMode(BRAKE_8874)`，此时因为`current_mode_ != BRAKE`，会执行完整的模式切换

但这不是电机持续转动的直接原因。让我继续深入...

---

## 四、🔥根本原因：看门狗未启用 + PID积分饱和 + 编码器噪声

### 完整的故障链

```
上电 → 初始化完成 → FreeRTOS启动 → motorControlTask以200Hz运行
    ↓
MotorArray::update()
    ↓
isWatchdogExpired() → false（看门狗未启用，因为从未收到速度指令）
    ↓
move = !false = true  ← ⚠️ 电机PID始终在运行！
    ↓
MotorDrv8874::update(dt, true)
    ↓
target = 0（初始目标速度）
vel = encoder_->getData().velocity  ← 编码器速度
    ↓
PID::compute(0, vel, dt)
error = 0 - vel = -vel
    ↓
如果编码器有微小噪声（vel ≠ 0）：
    P = 0.15 × (-vel)        ← 小，可能 < 0.01
    I = 0.55 × integral      ← ⚠️ 积分项持续累积！
    D = 0.007 × derivative   ← 很小
    ↓
output = P + I + D
    ↓
如果 |output| >= 0.01 → applyPWM(output) → 电机开始转动！
    ↓
电机转动 → 编码器检测到真实速度 → error变大 → PID输出更大
    ↓
正反馈循环 → 电机持续加速转动！
```

### 关键数值分析

PID参数：Kp=0.15, Ki=0.55, Kd=0.007
max_integral = 1/Ki = 1/0.55 ≈ 1.818

假设编码器噪声导致速度读数为0.1 rad/s（非常小的噪声）：
- 每次update: error = -0.1
- integral += -0.1 × 0.005 = -0.0005（dt=5ms @200Hz）
- 经过1秒（200次迭代）: integral ≈ -0.1
- I项 = 0.55 × (-0.1) = -0.055 → |output| > 0.01 → 电机开始转！

**即使编码器噪声只有0.01 rad/s，积分项也会在约10秒内累积到足以驱动电机的程度。**

### 为什么所有电机都转？

4个编码器都可能存在噪声，而且PID的积分项会持续累积直到max_integral（约1.818），对应的输出可达 Ki × max_integral = 0.55 × 1.818 ≈ 1.0（满输出！）。

---

## 五、修复方案

### 修复1：未连接SBC时电机制动（最关键）

**问题**：看门狗未启用时，`isWatchdogExpired()`返回false，导致`move=true`，PID持续运行。

**修复**：在`MotorArray::update()`中，当ROS未连接时应该停止电机。

`lib/motor/motor_array.cpp` 修改：

```cpp
void MotorArray::update() {
  if (!isDriversEnabled()) return;

  const uint32_t now = millis();
  const float dt = (now - last_update_) / 1000.0f;
  last_update_ = now;

  // ✅ 修复：看门狗未启用时（从未收到速度指令），电机制动
  bool should_move = isWatchdogEnabled() && !isWatchdogExpired();

  if (xSemaphoreTake(mutex_, pdMS_TO_TICKS(5)) == pdTRUE) {
    for (uint8_t i = 0; i < count_; ++i) {
      motors_[i]->update(dt, should_move);  // ← 使用should_move替代!isWatchdogExpired()
      // ...
    }
    xSemaphoreGive(mutex_);
  }
}
```

**逻辑**：
- 看门狗未启用（从未收到指令）→ `should_move = false` → 电机制动
- 看门狗已启用且未超时 → `should_move = true` → 电机正常运行
- 看门狗已启用但超时 → `should_move = false` → 电机制动

### 修复2：setMode初始化问题

`lib/motor/motor_drv8874.cpp` 修改init()：

```cpp
void MotorDrv8874::init() {
  // ... 现有PWM初始化代码 ...

  pwm_arr_ = pwm_timer_1_->getOverflow(TICK_FORMAT);

  // ✅ 修复：强制设置初始模式，绕过去重检查
  current_mode_ = MotorMode8874::COAST_8874;  // 先设为COAST
  // 然后强制切换到BRAKE（因为COAST无法仅靠IN1/IN2实现）
  // IN1=LOW, IN2=LOW = 低侧制动，这是上电最安全的状态
  pwm_timer_1_->setCaptureCompare(pwm_channel_1_, 0);
  pwm_timer_2_->setCaptureCompare(pwm_channel_2_, 0);
  current_mode_ = MotorMode8874::BRAKE_8874;  // 记录实际状态
}
```

或者更简洁地，将`current_mode_`的默认值改为一个不存在的模式值：

```cpp
// motor_drv8874.hpp
enum MotorMode8874 : uint8_t { FORWARD_8874, REVERSE_8874, BRAKE_8874, COAST_8874, UNINITIALIZED_8874 };

// motor_drv8874.hpp 成员变量
MotorMode8874 current_mode_ = MotorMode8874::UNINITIALIZED_8874;  // ← 修改默认值
```

这样init()中的`setMode(COAST_8874)`就不会被跳过了。

### 修复3：COAST模式正确实现（可选，低优先级）

真正的COAST需要nSLEEP=LOW，但当前架构中nSLEEP由MotorArray管理（按驱动组控制），单个电机无法独立控制nSLEEP。

**建议**：暂时保持COAST=低侧制动（IN1=LOW, IN2=LOW），这在功能上是安全的。如果未来需要真正的滑行模式，需要在MotorArray层面添加单独的COAST控制方法。

---

## 六、修复优先级

| 优先级 | 修复项 | 影响 | 难度 |
|--------|-------|------|------|
| 🔴 P0 | 修复1：看门狗未启用时电机制动 | **直接解决电机持续转动** | 低 |
| 🟡 P1 | 修复2：setMode初始化去重问题 | 防止潜在状态机错误 | 低 |
| 🟢 P2 | 修复3：COAST模式正确实现 | 功能完善 | 中 |

---

## 七、验证方法

修复后，按以下步骤验证：

1. **上电测试（无SBC）**：
   - [ ] 红灯常亮
   - [ ] 电机完全不动（无抖动、无转动）
   - [ ] LED灯带进入空闲动画

2. **连接SBC测试**：
   - [ ] 红灯熄灭（ROS连接成功）
   - [ ] 发送速度指令 → 电机正常转动
   - [ ] 停止发送指令超过500ms → 电机自动制动
   - [ ] 断开SBC → 电机自动制动

3. **边界测试**：
   - [ ] 上电后用手转动轮子 → 电机不应自行补偿（PID不应运行）
   - [ ] 连接SBC后发送零速度 → 电机制动不动
