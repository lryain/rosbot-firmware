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

#include <Arduino.h>

#include "battery_adc.hpp"
#include "battery_interface.hpp"
#include "communication_manager.hpp"
#include "config.hpp"
#include "encoder_array.hpp"
#include "hardware_encoder.hpp"
#include "imu_bno055.hpp"
#include "led_indicator.hpp"
#include "led_strip.hpp"
#include "motor_array.hpp"
#include "motor_drv8874.hpp"
#ifdef USE_WITTYPI5_POWER
#include "power_board_wp5.hpp"
#endif
#include "ros/ros_node.hpp"
#include "rtos.hpp"

// ───────── Board Revision ─────────
static BoardRevision board_revision(board_revision_config);

// ───────── Battery (ADC or Witty Pi 5 power board) ─────────
#ifdef USE_WITTYPI5_POWER
PowerBoardWP5 wp5_power(wp5_config);
#else
static BatteryAdc battery_adc(battery_adc_config);
#endif

// ───────── Encoders ─────────
static HardwareEncoder enc_fl(enc_fl_config);
static HardwareEncoder enc_fr(enc_fr_config);
static HardwareEncoder enc_rl(enc_rl_config);
static HardwareEncoder enc_rr(enc_rr_config);
static EncoderInterface* encoders[] = {&enc_fl, &enc_fr, &enc_rl, &enc_rr};
static constexpr uint8_t ENCODER_COUNT = sizeof(encoders) / sizeof(encoders[0]);

// ───────── Fan ─────────
FanController g_fan;

// ───────── IMU ─────────
static ImuBno055 imu_bno055(imu_bno055_config);

// ───────── LED Strip ─────────
static SpiTransport s_transport(spi_config);

// ───────── Motors (DRV8874 IN/IN mode) ─────────
static MotorDrv8874 motor_fl(motor_fl_config, &enc_fl,
                             PIDController(pid_config));
static MotorDrv8874 motor_fr(motor_fr_config, &enc_fr,
                             PIDController(pid_config));
static MotorDrv8874 motor_rl(motor_rl_config, &enc_rl,
                             PIDController(pid_config));
static MotorDrv8874 motor_rr(motor_rr_config, &enc_rr,
                             PIDController(pid_config));
static MotorInterface* motors[] = {&motor_fl, &motor_fr, &motor_rl, &motor_rr};
static constexpr uint8_t MOTOR_COUNT = sizeof(motors) / sizeof(motors[0]);
static constexpr uint8_t DRIVER_GROUP_COUNT =
    sizeof(driver_groups) / sizeof(driver_groups[0]);

// ───────── Extern variables ─────────
#ifdef USE_WITTYPI5_POWER
BatteryInterface* g_battery = &wp5_power;
#else
BatteryInterface* g_battery = &battery_adc;
#endif
EncoderArray g_encoders(encoders, ENCODER_COUNT);
ImuInterface* g_imu = &imu_bno055;
LedIndicator g_indicator(led_status_config);
LedStrip g_led_strip;
MotorArray g_motors(motors, MOTOR_COUNT, driver_groups, DRIVER_GROUP_COUNT);

bool useAlt() { return digitalRead(PUSH_BUTTON1) == LOW; }

void confirmAlt() { digitalWrite(GRN_LED, HIGH); }

CommunicationManagerConfig communication_config = {
    .primary_type = TransportType::kEthernet,
    .diagnostic_serial = DIAGNOSTIC_SERIAL_CONFIG,
    .useDiagnosticCondition = useAlt,
    .onDiagnosticSelected = confirmAlt};

CommunicationManager g_comm_mgr(communication_config);

void boardPheripheralsInit() {
  // Audio
  pinMode(AUDIO_SHDN, OUTPUT);
  digitalWrite(AUDIO_SHDN, HIGH);

  // Buttons
  pinMode(PUSH_BUTTON1, INPUT_PULLUP);
  pinMode(PUSH_BUTTON2, INPUT_PULLUP);

  // LEDs
  pinMode(RED_LED, OUTPUT);
  pinMode(GRN_LED, OUTPUT);
  digitalWrite(RED_LED, HIGH);

  // Peripheral Power
  pinMode(EN_LOC_5V, OUTPUT);
  digitalWrite(EN_LOC_5V, HIGH);

  // Power board shutdown pins are kept identical to ROSbot XL.
  pinMode(PB_SHD_DETECT, INPUT_PULLUP);
  pinMode(PB_SHD_CONFIRM, OUTPUT);
  digitalWrite(PB_SHD_CONFIRM, LOW);

  // I2C (IMU bus)
  i2c.begin();
  i2c.setClock(400000);

  delay(50);
}

/*───────── Setup ─────────*/
void setup() {
  boardPheripheralsInit();

  // Pre-communication
  g_comm_mgr.init();
  const auto* transport = g_comm_mgr.selectTransport();
  g_comm_mgr.configureNamespace();
  g_ros_node.setNamespace(g_comm_mgr.getNamespace());

  // Revision specific configuration
  board_revision.init();

  // Components initialization
  Ethernet.begin(MAC, CLIENT_IP);
#ifdef USE_WITTYPI5_POWER
  wp5_power.init();
#else
  battery_adc.init();
#endif
  g_encoders.init();
  ntc.init();
  g_fan.init(fan_config);
  imu_bno055.init();
  g_indicator.init();
  g_led_strip.init(strip_config, &s_transport);
  g_motors.init();

  if (g_comm_mgr.isSerialTransport()) {
    g_ros_node.serialTransportInit(*transport);
  } else {
    g_ros_node.ethernetTransportInit(AGENT_IP, AGENT_PORT);
  }
  g_ros_node.setDiagnosticSerial(g_comm_mgr.debugSerial());

  // RTOS
  createQueues();
  createTasks();
  vTaskStartScheduler();
}

/*───────── Loop ─────────*/
void loop() {}

/*───────── Runtime stats ─────────*/
// TIM5: 32-bit general-purpose timer on APB1 (84 MHz timer clock).
// Prescaler 840 → 100 kHz (10 µs/tick), sufficient for FreeRTOS runtime stats.
HardwareTimer RunTimeStatsTimer(TIM5);

void vConfigureTimerForRunTimeStats(void) {
  RunTimeStatsTimer.setPrescaleFactor(840);  // 84 MHz / 840 = 100 kHz
  RunTimeStatsTimer.setOverflow(0xFFFFFFFF);  // 32-bit max
  RunTimeStatsTimer.refresh();
  RunTimeStatsTimer.resume();
}

uint32_t vGetTimerValueForRunTimeStats(void) {
  return RunTimeStatsTimer.getCount();
}
