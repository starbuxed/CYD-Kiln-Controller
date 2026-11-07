#include <Arduino.h>
#include "relay_outputs.h"
#include "controller_link.h"
#include "kiln_control_core.h"
#include "sensor_manager.h"

// Dedicated ESP32-S3 control processor. Board-specific sensor and output
// pin assignments remain disabled until the exact hardware is selected.

static constexpr uint8_t SENSOR_CHANNEL_COUNT = 4;
static constexpr uint8_t HEATER_ZONE_COUNT = 4;

#ifndef KILN_S3_UART_RX_PIN
#define KILN_S3_UART_RX_PIN -1
#endif
#ifndef KILN_S3_UART_TX_PIN
#define KILN_S3_UART_TX_PIN -1
#endif

kiln::RelayOutputs relayOutputs;
kiln::ControllerState controllerState = kiln::STATE_BOOT;
kiln::KilnControlCore controlCore;
kiln::SensorManager sensorManager;
float kilnTemperaturesC[4] = {0, 0, 0, 0};

static ControllerLink controllerLink;
static HardwareSerial controllerSerial(1);

// Deliberately disabled until relay input polarity, voltage and S3 GPIOs are
// confirmed. Firmware cannot energize any external output in this state.
static const kiln::RelayOutputConfig outputConfig[kiln::RELAY_OUTPUT_COUNT] = {
  {-1, true, false}, {-1, true, false},
  {-1, true, false}, {-1, true, false}
};

void setup() {
  Serial.begin(115200);
  delay(200);
  controllerState = kiln::STATE_SELF_TEST;
  const bool outputsReady = relayOutputs.begin(outputConfig);
  relayOutputs.allInactive();

  Serial.println();
  Serial.println("Kiln Controller - ESP32-S3");
  Serial.printf("Temperature channels: %u\n", SENSOR_CHANNEL_COUNT);
  Serial.printf("Independent zones: %u\n", HEATER_ZONE_COUNT);
  Serial.printf("Relay output channels: %u\n", kiln::RELAY_OUTPUT_COUNT);
  Serial.printf("Output configuration valid: %s\n", outputsReady ? "yes" : "no");
  Serial.println("Sensor front-end driver and board pin map are not configured.");
  Serial.println("No thermocouple channels are enabled; START remains blocked.");
  Serial.println("All relay outputs are disabled and unassigned.");

  if (!outputsReady) {
    controllerState = kiln::STATE_FAULT;
    return;
  }

#if KILN_S3_UART_RX_PIN >= 0 && KILN_S3_UART_TX_PIN >= 0
  controllerLink.begin(controllerSerial, KILN_S3_UART_RX_PIN,
                       KILN_S3_UART_TX_PIN, 115200);
#else
  Serial.println("S3 UART pins not configured; communications disabled.");
#endif
  controllerState = kiln::STATE_IDLE;
}

void loop() {
#if KILN_S3_UART_RX_PIN >= 0 && KILN_S3_UART_TX_PIN >= 0
  controllerLink.update();
#endif

  // SensorManager accepts readings from the future MAX31856 (or other)
  // hardware adapter. No adapter is attached in this hardware-neutral build.
  sensorManager.update(millis());
  for (uint8_t i = 0; i < SENSOR_CHANNEL_COUNT; ++i)
    kilnTemperaturesC[i] = sensorManager.temperatureC(i);

  if (controllerState == kiln::STATE_FIRING) {
    controlCore.update(millis(), kilnTemperaturesC,
                       sensorManager.validMask(),
#if KILN_S3_UART_RX_PIN >= 0 && KILN_S3_UART_TX_PIN >= 0
                       true
#else
                       false
#endif
    );
    if (controlCore.faulted()) {
      relayOutputs.allInactive();
      controllerState = kiln::STATE_FAULT;
    } else {
      for (uint8_t i = 0; i < kiln::RELAY_OUTPUT_COUNT; ++i)
        relayOutputs.set(i, controlCore.outputOn(i));
    }
  }
  delay(2);
}
