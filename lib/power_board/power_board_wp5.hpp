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
#include <Wire.h>

#include <cmath>

#include "battery_interface.hpp"

// ── Witty Pi 5 I2C Register Map ─────────────────────────────────

namespace wp5 {

// Read-only registers
constexpr uint8_t REG_FW_ID = 0x00;
constexpr uint8_t REG_FW_VERSION_MAJOR = 0x01;
constexpr uint8_t REG_FW_VERSION_MINOR = 0x02;
constexpr uint8_t REG_VUSB_MV_MSB = 0x03;
constexpr uint8_t REG_VUSB_MV_LSB = 0x04;
constexpr uint8_t REG_VIN_MV_MSB = 0x05;
constexpr uint8_t REG_VIN_MV_LSB = 0x06;
constexpr uint8_t REG_VOUT_MV_MSB = 0x07;
constexpr uint8_t REG_VOUT_MV_LSB = 0x08;
constexpr uint8_t REG_IOUT_MA_MSB = 0x09;
constexpr uint8_t REG_IOUT_MA_LSB = 0x0A;
constexpr uint8_t REG_POWER_MODE = 0x0B;
constexpr uint8_t REG_MISSED_HEARTBEAT = 0x0C;
constexpr uint8_t REG_RPI_STATE = 0x0D;
constexpr uint8_t REG_ACTION_REASON = 0x0E;

// Configuration registers
constexpr uint8_t REG_CONF_DEFAULT_ON_DELAY = 0x11;
constexpr uint8_t REG_CONF_POWER_CUT_DELAY = 0x12;
constexpr uint8_t REG_CONF_LOW_VOLTAGE = 0x16;
constexpr uint8_t REG_CONF_RECOVERY_VOLTAGE = 0x17;
constexpr uint8_t REG_CONF_PS_PRIORITY = 0x18;
constexpr uint8_t REG_CONF_WATCHDOG = 0x1D;

// Admin registers
constexpr uint8_t REG_ADMIN_HEARTBEAT = 0x46;
constexpr uint8_t REG_ADMIN_SHUTDOWN = 0x47;

// Virtual registers (RTC)
constexpr uint8_t REG_VREG_RX8025_SEC = 0x50;

// Virtual registers (Temperature)
constexpr uint8_t REG_VREG_TMP112_TEMP_MSB = 0x60;
constexpr uint8_t REG_VREG_TMP112_TEMP_LSB = 0x61;

// State machine values
constexpr uint8_t STATE_OFF = 0;
constexpr uint8_t STATE_STARTING = 1;
constexpr uint8_t STATE_ON = 2;
constexpr uint8_t STATE_STOPPING = 3;

// Expected firmware ID
constexpr uint8_t EXPECTED_FW_ID = 0x51;

}  // namespace wp5

// ── Configuration ───────────────────────────────────────────────

struct PowerBoardWP5Config {
  TwoWire& i2c_bus;
  uint8_t i2c_addr = 0x51;
  float v_min = 9.6f;
  float v_max = 12.6f;
  uint8_t shd_detect_pin = 0xFF;  // 0xFF = use I2C polling
  uint32_t heartbeat_interval_ms = 30000;

  // WP5 configuration (written during init)
  uint8_t low_voltage = 96;        // ×100mV → 9.6V
  uint8_t recovery_voltage = 105;  // ×100mV → 10.5V
  uint8_t watchdog_count = 3;
  uint8_t default_on_delay = 3;    // seconds
  uint8_t power_cut_delay = 20;    // seconds
  uint8_t ps_priority = 1;         // 1 = VIN priority
};

// ── Class ───────────────────────────────────────────────────────

class PowerBoardWP5 : public BatteryInterface {
 public:
  struct BoardInfo {
    uint8_t fw_id = 0;
    uint8_t fw_major = 0;
    uint8_t fw_minor = 0;

    bool isValid() const { return fw_id == wp5::EXPECTED_FW_ID; }
  };

  explicit PowerBoardWP5(const PowerBoardWP5Config& config);

  // ── BatteryInterface ────────────────────────────────────────

  void init() override;
  void update() override;
  const char* name() const override { return "WittyPi5"; }

  // ── WP5-specific API ────────────────────────────────────────

  bool sendHeartbeat();
  bool isShutdownRequested();
  uint8_t readState();
  float readTemperature();

  const BoardInfo& boardInfo() const { return board_info_; }

  uint16_t vinMv() const { return vin_mv_; }
  uint16_t vusbMv() const { return vusb_mv_; }
  uint16_t voutMv() const { return vout_mv_; }
  uint16_t ioutMa() const { return iout_ma_; }
  uint8_t powerMode() const { return power_mode_; }
  uint8_t rpiState() const { return rpi_state_; }

 private:
  bool readReg(uint8_t reg, uint8_t* data, uint8_t len);
  bool writeReg(uint8_t reg, uint8_t value);
  uint16_t readU16(uint8_t reg_msb);

  void readBoardInfo();
  void readBatteryState();
  void configureWP5();

  PowerBoardWP5Config cfg_;
  BoardInfo board_info_;

  uint16_t vusb_mv_ = 0;
  uint16_t vin_mv_ = 0;
  uint16_t vout_mv_ = 0;
  uint16_t iout_ma_ = 0;
  uint8_t power_mode_ = 0;
  uint8_t rpi_state_ = wp5::STATE_OFF;
  uint32_t last_heartbeat_ms_ = 0;
  bool configured_ = false;
};
