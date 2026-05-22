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

#pragma once

#include <Arduino.h>
#include <HardwareTimer.h>

#include <atomic>

#include "encoder_interface.hpp"
#include "motor_interface.hpp"
#include "pid.hpp"

enum MotorMode8874 : uint8_t { FORWARD_8874, REVERSE_8874, BRAKE_8874, COAST_8874 };

/// DRV8874 motor driver configuration for IN/IN mode (PMODE=VCC).
/// Each motor requires two PWM channels (IN1 and IN2).
struct MotorDrv8874Config {
  uint8_t in1_pin;          // IN1 — PWM output pin (Arduino pin number)
  uint8_t in2_pin;          // IN2 — PWM output pin (Arduino pin number)
  // Optional ALT PinName overrides to force a specific timer/AF mapping.
  // When set to NC (default), digitalPinToPinName(in1_pin/in2_pin) is used,
  // which picks the PRIMARY PinMap_PWM entry (may conflict with encoder timers).
  // Use ALT variants (e.g. PA_6_ALT1, PB_8_ALT1) to avoid such conflicts.
  PinName in1_pname = NC;   // NC = use in1_pin lookup
  PinName in2_pname = NC;   // NC = use in2_pin lookup
  bool inv_dir;             // polarity inversion
  float max_velocity;    // [rad/s] clamp
  float min_velocity;    // [rad/s] deadband
  uint32_t pwm_freq;     // [Hz]
  const char* frame_id;
  uint8_t ipropi_pin    = 0xFF;   // IPROPI ADC pin (0xFF = not connected)
  float   ipropi_scale  = 0.0f;   // A per ADC count
  // ── Software OCP & stall ──────────────────────────────────────
  float   max_current   = 5.0f;   // [A] software OCP limit (0 = disabled)
  float   stall_current = 3.5f;   // [A] stall detection current threshold
  float   stall_vel     = 0.15f;  // [rad/s] stall velocity threshold
  uint32_t stall_ms     = 250;    // [ms] debounce before confirming stall
};

/// DRV8874-based motor with IN/IN control mode.
/// IN1 and IN2 are both PWM pins; direction is set by which pin
/// receives the PWM signal while the other is held LOW.
class MotorDrv8874 : public MotorInterface {
 public:
  MotorDrv8874() = default;
  explicit MotorDrv8874(const MotorDrv8874Config& cfg,
                        EncoderInterface* encoder, PIDController pid)
      : cfg_(cfg), encoder_(encoder), pid_(pid) {}

  void init() override;
  void update(float dt, bool move = true) override;
  void reset() override;
  void setVelocity(float vel) override;
  void brake() override;
  void setNeutral() override;
  void setEnabled(bool en) override { enabled_ = en; }

  MotorData getData() const override;
  const char* name() const override { return cfg_.frame_id; }

 private:
  void setMode(MotorMode8874 mode);
  void applyPWM(float duty);

  MotorDrv8874Config cfg_ = {};
  EncoderInterface* encoder_ = nullptr;
  PIDController pid_;

  HardwareTimer* pwm_timer_1_ = nullptr;
  HardwareTimer* pwm_timer_2_ = nullptr;
  uint32_t pwm_channel_1_ = 0;
  uint32_t pwm_channel_2_ = 0;
  uint16_t pwm_arr_ = 0;

  std::atomic<float> target_velocity_{0.0f};
  std::atomic<float> current_effort_{0.0f};
  MotorMode8874 current_mode_ = MotorMode8874::COAST_8874;
  bool enabled_ = false;

  // IPROPI state (updated every update() call)
  float    filtered_current_ = 0.0f;           // IIR-filtered current [A]
  uint32_t stall_start_ms_   = 0xFFFFFFFFUL;   // debounce timer (0xFFFF… = idle)
  bool     stall_active_     = false;          // latched stall flag
};
