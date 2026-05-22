// Copyright 2024 ROSbot PRO
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "motor_drv8874.hpp"

void MotorDrv8874::init() {
  // Initialize encoder first (if present)
  if (encoder_) {
    encoder_->init();
  }

  // Resolve PinNames: use explicit ALT override when provided to avoid
  // conflicts with encoder timers (TIM1-TIM4 PRIMARY entries in PinMap_PWM).
  PinName in1_pn = (cfg_.in1_pname != NC) ? cfg_.in1_pname
                                           : digitalPinToPinName(cfg_.in1_pin);
  PinName in2_pn = (cfg_.in2_pname != NC) ? cfg_.in2_pname
                                           : digitalPinToPinName(cfg_.in2_pin);

  // Initialize IN1 PWM channel
  TIM_TypeDef* tim1 = (TIM_TypeDef*)pinmap_peripheral(in1_pn, PinMap_PWM);
  pwm_channel_1_ = STM_PIN_CHANNEL(pinmap_function(in1_pn, PinMap_PWM));
  pwm_timer_1_ = new HardwareTimer(tim1);
  pwm_timer_1_->setPWM(pwm_channel_1_, in1_pn, cfg_.pwm_freq, 0);

  // Initialize IN2 PWM channel
  TIM_TypeDef* tim2 = (TIM_TypeDef*)pinmap_peripheral(in2_pn, PinMap_PWM);
  pwm_channel_2_ = STM_PIN_CHANNEL(pinmap_function(in2_pn, PinMap_PWM));

  if (tim1 == tim2) {
    pwm_timer_2_ = pwm_timer_1_;
  } else {
    pwm_timer_2_ = new HardwareTimer(tim2);
  }
  pwm_timer_2_->setPWM(pwm_channel_2_, in2_pn, cfg_.pwm_freq, 0);

  pwm_arr_ = pwm_timer_1_->getOverflow(TICK_FORMAT);

  // ✅ 修复：强制设置初始模式，绕过去重检查
  current_mode_ = MotorMode8874::COAST_8874;  // 先设为COAST
  // 然后强制切换到BRAKE（因为COAST无法仅靠IN1/IN2实现）
  // IN1=LOW, IN2=LOW = 低侧制动，这是上电最安全的状态
  pwm_timer_1_->setCaptureCompare(pwm_channel_1_, 0);
  pwm_timer_2_->setCaptureCompare(pwm_channel_2_, 0);
  current_mode_ = MotorMode8874::BRAKE_8874;  // 记录实际状态
  
  setMode(MotorMode8874::COAST_8874);
}

MotorData MotorDrv8874::getData() const {
  MotorData d;
  if (encoder_) {
    const auto enc = encoder_->getData();
    d.position = enc.position;
    d.velocity = enc.velocity;
  }
  d.effort          = current_effort_.load(std::memory_order_relaxed);
  d.target_velocity = target_velocity_.load(std::memory_order_relaxed);
  d.current         = filtered_current_;  // cached by update(), no ADC re-read
  d.stall           = stall_active_;
  return d;
}

void MotorDrv8874::setMode(MotorMode8874 mode) {
  if (mode == current_mode_) return;
  current_mode_ = mode;

  switch (mode) {
    case MotorMode8874::FORWARD_8874:
      // IN1=PWM (or IN2=PWM if inverted), other=LOW
      if (cfg_.inv_dir) {
        pwm_timer_1_->setCaptureCompare(pwm_channel_1_, 0);
      } else {
        pwm_timer_2_->setCaptureCompare(pwm_channel_2_, 0);
      }
      break;

    case MotorMode8874::REVERSE_8874:
      // IN2=PWM (or IN1=PWM if inverted), other=LOW
      if (cfg_.inv_dir) {
        pwm_timer_2_->setCaptureCompare(pwm_channel_2_, 0);
      } else {
        pwm_timer_1_->setCaptureCompare(pwm_channel_1_, 0);
      }
      break;

    case MotorMode8874::BRAKE_8874:
      // IN1=HIGH, IN2=HIGH → DRV8874 both outputs LOW (active low-side brake / fast decay)
      pwm_timer_1_->setCaptureCompare(pwm_channel_1_, pwm_arr_);
      pwm_timer_2_->setCaptureCompare(pwm_channel_2_, pwm_arr_);
      break;

    case MotorMode8874::COAST_8874:
    default:
      pwm_timer_1_->setCaptureCompare(pwm_channel_1_, 0);
      pwm_timer_2_->setCaptureCompare(pwm_channel_2_, 0);
      break;
  }
}

void MotorDrv8874::applyPWM(float duty) {
  duty = constrain(duty, -1.0f, 1.0f);
  current_effort_.store(duty, std::memory_order_relaxed);

  if (fabs(duty) < 0.01f) {
    setMode(MotorMode8874::BRAKE_8874);
    return;
  }

  uint16_t pwm_value = static_cast<uint16_t>(fabs(duty) * pwm_arr_);

  if (duty > 0) {
    setMode(MotorMode8874::FORWARD_8874);
    // Apply PWM to the active channel
    HardwareTimer* t_active = cfg_.inv_dir ? pwm_timer_2_ : pwm_timer_1_;
    uint32_t ch_active = cfg_.inv_dir ? pwm_channel_2_ : pwm_channel_1_;
    t_active->setCaptureCompare(ch_active, pwm_value);
  } else {
    setMode(MotorMode8874::REVERSE_8874);
    HardwareTimer* t_active = cfg_.inv_dir ? pwm_timer_1_ : pwm_timer_2_;
    uint32_t ch_active = cfg_.inv_dir ? pwm_channel_1_ : pwm_channel_2_;
    t_active->setCaptureCompare(ch_active, pwm_value);
  }
}

void MotorDrv8874::setVelocity(float vel) {
  float v = constrain(vel, -cfg_.max_velocity, cfg_.max_velocity);
  if (fabs(v) < cfg_.min_velocity) v = 0.0f;
  target_velocity_.store(v, std::memory_order_relaxed);
}

void MotorDrv8874::setNeutral() {
  target_velocity_.store(0.0f, std::memory_order_relaxed);
  setMode(MotorMode8874::COAST_8874);
  pid_.reset();
}

void MotorDrv8874::brake() {
  target_velocity_.store(0.0f, std::memory_order_relaxed);
  current_effort_.store(0.0f, std::memory_order_relaxed);
  setMode(MotorMode8874::BRAKE_8874);
  pid_.reset();
}

void MotorDrv8874::update(float dt, bool move) {
  // --- Step 0: Update encoder (must be called first to get fresh data) ---
  if (encoder_) {
    encoder_->update();
  }

  // --- Step 1: Sample IPROPI (always, even when stopped, for accurate monitoring) ---
  if (cfg_.ipropi_pin != 0xFF) {
    constexpr float kAlpha = 0.1f;  // IIR low-pass: τ≈47ms @ 200Hz, reduces ADC noise for stall/OCP
    const float raw = static_cast<float>(analogRead(cfg_.ipropi_pin)) * cfg_.ipropi_scale;
    filtered_current_ = kAlpha * raw + (1.0f - kAlpha) * filtered_current_;
  }

  if (!move) {
    stall_start_ms_ = 0xFFFFFFFFUL;
    stall_active_   = false;
    brake();
    return;
  }

  const float target = target_velocity_.load(std::memory_order_relaxed);
  const float vel    = encoder_ ? encoder_->getData().velocity : 0.0f;

  // --- Step 2: Hard OCP — immediate brake if current exceeds max_current ---
  if (cfg_.max_current > 0.0f && filtered_current_ > cfg_.max_current) {
    brake();
    return;
  }

  // --- Step 3: Velocity PID ---
  float output = pid_.compute(target, vel, dt);

  // --- Step 4: Soft OCP — proportionally reduce effort in upper 20 % current band ---
  if (cfg_.max_current > 0.0f && filtered_current_ > cfg_.max_current * 0.8f) {
    const float headroom = cfg_.max_current - filtered_current_;
    const float band     = cfg_.max_current * 0.2f;  // 20 % band
    output *= constrain(headroom / band, 0.0f, 1.0f);
  }

  // --- Step 5: Stall detection (non-braking — alert only via getData().stall) ---
  // Condition: motor commanded but barely moving AND drawing significant current.
  const bool stall_cond =
    (cfg_.stall_current > 0.0f) &&
    (fabsf(vel)    < cfg_.stall_vel) &&
    (filtered_current_ > cfg_.stall_current) &&
    (fabsf(target) > cfg_.min_velocity);

  if (!stall_cond) {
    stall_start_ms_ = 0xFFFFFFFFUL;
    stall_active_   = false;
  } else if (stall_start_ms_ == 0xFFFFFFFFUL) {
    stall_start_ms_ = millis();  // begin debounce window
  } else if ((millis() - stall_start_ms_) >= cfg_.stall_ms) {
    stall_active_ = true;        // confirmed stall after debounce
  }

  // --- Step 6: Apply PWM ---
  applyPWM(output);
}

void MotorDrv8874::reset() {
  brake();
  target_velocity_.store(0.0f, std::memory_order_relaxed);
  current_effort_.store(0.0f, std::memory_order_relaxed);
  pid_.reset();
  filtered_current_ = 0.0f;
  stall_start_ms_   = 0xFFFFFFFFUL;
  stall_active_     = false;
  if (encoder_) encoder_->reset();
}
