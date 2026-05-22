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

#include "power_board_wp5.hpp"

// ═══════════════════════════════════════════════════════════════════
//  Constructor
// ═══════════════════════════════════════════════════════════════════

PowerBoardWP5::PowerBoardWP5(const PowerBoardWP5Config& config)
    : cfg_(config) {}

// ═══════════════════════════════════════════════════════════════════
//  init() — setup GPIO, read board info, configure WP5
// ═══════════════════════════════════════════════════════════════════

void PowerBoardWP5::init() {
  // Setup shutdown detection GPIO if configured
  if (cfg_.shd_detect_pin != 0xFF) {
    pinMode(cfg_.shd_detect_pin, INPUT_PULLUP);
  }

  cfg_.i2c_bus.begin();
  cfg_.i2c_bus.setClock(100000);  // WP5 slave runs at 100kHz

  // Read board identification
  readBoardInfo();

  // Configure WP5 with our parameters
  if (board_info_.isValid()) {
    configureWP5();
    sendHeartbeat();
  }
}

// ═══════════════════════════════════════════════════════════════════
//  update() — read battery state, auto heartbeat
// ═══════════════════════════════════════════════════════════════════

void PowerBoardWP5::update() {
  readBatteryState();

  // Auto heartbeat
  uint32_t now = millis();
  if (now - last_heartbeat_ms_ >= cfg_.heartbeat_interval_ms) {
    sendHeartbeat();
    last_heartbeat_ms_ = now;
  }

  // One-time configuration retry if init failed
  if (!configured_ && board_info_.isValid()) {
    configureWP5();
  }
}

// ═══════════════════════════════════════════════════════════════════
//  I2C register read/write
// ═══════════════════════════════════════════════════════════════════

bool PowerBoardWP5::readReg(uint8_t reg, uint8_t* data, uint8_t len) {
  cfg_.i2c_bus.beginTransmission(cfg_.i2c_addr);
  cfg_.i2c_bus.write(reg);
  if (cfg_.i2c_bus.endTransmission(false) != 0) {
    return false;
  }

  uint8_t received = cfg_.i2c_bus.requestFrom(cfg_.i2c_addr, len);
  if (received != len) {
    return false;
  }

  for (uint8_t i = 0; i < len; i++) {
    data[i] = cfg_.i2c_bus.read();
  }
  return true;
}

bool PowerBoardWP5::writeReg(uint8_t reg, uint8_t value) {
  cfg_.i2c_bus.beginTransmission(cfg_.i2c_addr);
  cfg_.i2c_bus.write(reg);
  cfg_.i2c_bus.write(value);
  return cfg_.i2c_bus.endTransmission() == 0;
}

uint16_t PowerBoardWP5::readU16(uint8_t reg_msb) {
  uint8_t buf[2];
  if (!readReg(reg_msb, buf, 2)) return 0;
  return (static_cast<uint16_t>(buf[0]) << 8) | buf[1];
}

// ═══════════════════════════════════════════════════════════════════
//  readBoardInfo() — read firmware ID and version
// ═══════════════════════════════════════════════════════════════════

void PowerBoardWP5::readBoardInfo() {
  uint8_t buf[3];
  if (readReg(wp5::REG_FW_ID, buf, 3)) {
    board_info_.fw_id = buf[0];
    board_info_.fw_major = buf[1];
    board_info_.fw_minor = buf[2];
  }
}

// ═══════════════════════════════════════════════════════════════════
//  readBatteryState() — read voltages, current, state
// ═══════════════════════════════════════════════════════════════════

void PowerBoardWP5::readBatteryState() {
  vusb_mv_ = readU16(wp5::REG_VUSB_MV_MSB);
  vin_mv_ = readU16(wp5::REG_VIN_MV_MSB);
  vout_mv_ = readU16(wp5::REG_VOUT_MV_MSB);
  iout_ma_ = readU16(wp5::REG_IOUT_MA_MSB);

  uint8_t pm;
  if (readReg(wp5::REG_POWER_MODE, &pm, 1)) {
    power_mode_ = pm;
  }

  uint8_t st;
  if (readReg(wp5::REG_RPI_STATE, &st, 1)) {
    rpi_state_ = st;
  }

  // Update BatteryInterface data
  if (vin_mv_ > 0) {
    data_.voltage = vin_mv_ * 0.001f;
    data_.current = -(iout_ma_ * 0.001f);  // discharge is negative
    data_.present = true;

    if (cfg_.v_max > cfg_.v_min) {
      float pct = (data_.voltage - cfg_.v_min) / (cfg_.v_max - cfg_.v_min);
      data_.percentage = fmaxf(0.0f, fminf(1.0f, pct));
    }
  } else {
    // I2C read failed or no power — keep previous data
    data_.present = false;
  }

  // Read temperature from TMP112 virtual registers
  data_.temperature = readTemperature();

  data_.status = rpi_state_;
  data_.health = 0;

  // Retry board info if not valid yet
  if (!board_info_.isValid()) {
    readBoardInfo();
  }
}

// ═══════════════════════════════════════════════════════════════════
//  configureWP5() — write configuration parameters
// ═══════════════════════════════════════════════════════════════════

void PowerBoardWP5::configureWP5() {
  bool ok = true;
  ok &= writeReg(wp5::REG_CONF_PS_PRIORITY, cfg_.ps_priority);
  ok &= writeReg(wp5::REG_CONF_DEFAULT_ON_DELAY, cfg_.default_on_delay);
  ok &= writeReg(wp5::REG_CONF_POWER_CUT_DELAY, cfg_.power_cut_delay);
  ok &= writeReg(wp5::REG_CONF_LOW_VOLTAGE, cfg_.low_voltage);
  ok &= writeReg(wp5::REG_CONF_RECOVERY_VOLTAGE, cfg_.recovery_voltage);
  ok &= writeReg(wp5::REG_CONF_WATCHDOG, cfg_.watchdog_count);
  configured_ = ok;
}

// ═══════════════════════════════════════════════════════════════════
//  sendHeartbeat() — reset WP5 watchdog timer
// ═══════════════════════════════════════════════════════════════════

bool PowerBoardWP5::sendHeartbeat() {
  bool ok = writeReg(wp5::REG_ADMIN_HEARTBEAT, 0x01);
  if (ok) {
    last_heartbeat_ms_ = millis();
  }
  return ok;
}

// ═══════════════════════════════════════════════════════════════════
//  isShutdownRequested() — check if WP5 wants us to shut down
// ═══════════════════════════════════════════════════════════════════

bool PowerBoardWP5::isShutdownRequested() {
  // GPIO-based detection (faster, optional)
  if (cfg_.shd_detect_pin != 0xFF) {
    return digitalRead(cfg_.shd_detect_pin) == HIGH;
  }

  // I2C polling — check if state is STOPPING
  return readState() == wp5::STATE_STOPPING;
}

// ═══════════════════════════════════════════════════════════════════
//  readState() — read current WP5 state machine value
// ═══════════════════════════════════════════════════════════════════

uint8_t PowerBoardWP5::readState() {
  uint8_t st;
  if (readReg(wp5::REG_RPI_STATE, &st, 1)) {
    rpi_state_ = st;
  }
  return rpi_state_;
}

// ═══════════════════════════════════════════════════════════════════
//  readTemperature() — read TMP112 via WP5 virtual registers
// ═══════════════════════════════════════════════════════════════════

float PowerBoardWP5::readTemperature() {
  uint8_t buf[2];
  if (!readReg(wp5::REG_VREG_TMP112_TEMP_MSB, buf, 2)) {
    return NAN;
  }

  // TMP112 format: 12-bit signed, MSB-first, right-justified in 16 bits
  int16_t raw = (static_cast<int16_t>(buf[0]) << 4) | (buf[1] >> 4);
  if (raw & 0x0800) {
    raw |= 0xF000;  // sign-extend
  }
  return raw * 0.0625f;
}
