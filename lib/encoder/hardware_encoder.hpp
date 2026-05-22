// Copyright 2022 Husarion sp. z o.o.
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

#include "encoder_interface.hpp"

struct HardwareEncoderConfig {
  uint8_t pin_a;         // TIMx_CH1 pin
  uint8_t pin_b;         // TIMx_CH2 pin
  TIM_TypeDef* timer;    // e.g. TIM1, TIM3, TIM8
  bool inv_dir;          // polarity
  float rad_per_tick;    // radians per encoder tick
  const char* frame_id;  // human name, e.g. "FR"
  float alpha = 1.0f;    // low-pass filter coefficient for velocity
};

class HardwareEncoder : public EncoderInterface {
 public:
  explicit HardwareEncoder(const HardwareEncoderConfig& cfg);

  void init() override;
  void update() override;
  void reset() override;
  const char* name() const override { return cfg_.frame_id; }

 private:
  const uint32_t getTicks() const { return timer_handle_->Instance->CNT; }
  float lowPass(float prev, float input) const {
    float filtered = cfg_.alpha * input + (1.0f - cfg_.alpha) * prev;
    return fabs(filtered) > ZERO_THRESHOLD ? filtered : 0.0f;
  }
  static inline int32_t compute_delta(uint32_t cnt, uint32_t last_cnt, bool is_32bit);

  HardwareEncoderConfig cfg_ = {};
  TIM_HandleTypeDef* timer_handle_ = nullptr;
  TIM_Encoder_InitTypeDef encoder_cfg_ = {};
  uint32_t last_cnt_ = 0;
  uint32_t last_time_us_ = 0;
  float last_velocity_ = 0.0f;
  bool is_32bit_timer_ = false;  // true for TIM2/TIM5 (32-bit timers)

  // Timer counter max value (16-bit or 32-bit)
  static constexpr uint32_t CNT_MAX_16BIT = 0xFFFF;
  static constexpr uint32_t CNT_MAX_32BIT = 0xFFFFFFFF;
  static constexpr int32_t CNT_HALF_16BIT = 0x8000;
  static constexpr int32_t CNT_HALF_32BIT = 0x80000000;
  
  static constexpr uint32_t MIN_DT_US = 100;
  static constexpr float ZERO_THRESHOLD = 0.01f;  // rad/s
  static constexpr float US_TO_SEC = 1.0f / 1000000.0f;
};
