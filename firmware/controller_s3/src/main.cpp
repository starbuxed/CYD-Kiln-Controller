#include <Arduino.h>
#include "relay_outputs.h"
#include "controller_link.h"
#include "kiln_control_core.h"
#include "sensor_manager.h"
#include "max31856_array.h"
#include "shutdown_output.h"

// Dedicated ESP32-S3: thermocouple acquisition, local control, outputs,
// and local safety. All physical pin assignments remain disabled until the
// exact board and interface hardware are confirmed.

static constexpr uint8_t SENSOR_CHANNEL_COUNT = 4;
static constexpr uint8_t HEATER_ZONE_COUNT = 4;

#ifndef KILN_S3_CAN_TX_PIN
#define KILN_S3_CAN_TX_PIN -1
#endif
#ifndef KILN_S3_CAN_RX_PIN
#define KILN_S3_CAN_RX_PIN -1
#endif
#ifndef KILN_S3_CAN_BITRATE
#define KILN_S3_CAN_BITRATE 500000
#endif
#ifndef KILN_S3_SHUTDOWN_REQUEST_PIN
#define KILN_S3_SHUTDOWN_REQUEST_PIN -1
#endif

kiln::RelayOutputs relayOutputs;
kiln::ControllerState controllerState = kiln::STATE_BOOT;
kiln::KilnControlCore controlCore;
kiln::SensorManager sensorManager;
kiln::Max31856Array sensorFrontend;
float kilnTemperaturesC[4] = {0, 0, 0, 0};

static ControllerLink controllerLink;
static kiln::CanStream controllerBus(kiln::CAN_ROLE_SECONDARY);
static ShutdownRequestOutput secondaryShutdownRequest;

// Deliberately disabled until the exact S3 board pinout, relay input
// electrical interface, and external safety circuit have been verified.
#ifndef KILN_RELAY1_PIN
#define KILN_RELAY1_PIN -1
#endif
#ifndef KILN_RELAY2_PIN
#define KILN_RELAY2_PIN -1
#endif
#ifndef KILN_RELAY3_PIN
#define KILN_RELAY3_PIN -1
#endif
#ifndef KILN_RELAY4_PIN
#define KILN_RELAY4_PIN -1
#endif
#ifndef KILN_RELAY_ACTIVE_HIGH
#define KILN_RELAY_ACTIVE_HIGH 1
#endif
#ifndef KILN_RELAY1_ACTIVE_HIGH
#define KILN_RELAY1_ACTIVE_HIGH KILN_RELAY_ACTIVE_HIGH
#endif
#ifndef KILN_RELAY2_ACTIVE_HIGH
#define KILN_RELAY2_ACTIVE_HIGH KILN_RELAY_ACTIVE_HIGH
#endif
#ifndef KILN_RELAY3_ACTIVE_HIGH
#define KILN_RELAY3_ACTIVE_HIGH KILN_RELAY_ACTIVE_HIGH
#endif
#ifndef KILN_RELAY4_ACTIVE_HIGH
#define KILN_RELAY4_ACTIVE_HIGH KILN_RELAY_ACTIVE_HIGH
#endif
static const kiln::RelayOutputConfig outputConfig[kiln::RELAY_OUTPUT_COUNT] = {
  {KILN_RELAY1_PIN, KILN_RELAY1_ACTIVE_HIGH != 0, KILN_RELAY1_PIN >= 0},
  {KILN_RELAY2_PIN, KILN_RELAY2_ACTIVE_HIGH != 0, KILN_RELAY2_PIN >= 0},
  {KILN_RELAY3_PIN, KILN_RELAY3_ACTIVE_HIGH != 0, KILN_RELAY3_PIN >= 0},
  {KILN_RELAY4_PIN, KILN_RELAY4_ACTIVE_HIGH != 0, KILN_RELAY4_PIN >= 0}
};

void setup() {
  Serial.begin(115200);
  delay(200);
  controllerState = kiln::STATE_SELF_TEST;
  secondaryShutdownRequest.begin(KILN_S3_SHUTDOWN_REQUEST_PIN);

  const bool outputsReady = relayOutputs.begin(outputConfig);
  relayOutputs.allInactive();

  Serial.println();
  Serial.println("Kiln Controller - ESP32-S3 Secondary");
  Serial.printf("Temperature channels: %u\n", SENSOR_CHANNEL_COUNT);
  Serial.printf("Independent zones: %u\n", HEATER_ZONE_COUNT);
  Serial.printf("Relay output channels: %u\n", kiln::RELAY_OUTPUT_COUNT);
  Serial.printf("Output configuration valid: %s\n", outputsReady ? "yes" : "no");
  Serial.printf("Secondary shutdown-request configured: %s\n",
                secondaryShutdownRequest.configured() ? "yes" : "no");
  const bool sensorsReady = sensorFrontend.begin();
  Serial.printf("MAX31856 front-end ready: %s\n", sensorsReady ? "yes" : "no");
  Serial.println("Sensor SPI/CS GPIOs and relay GPIOs remain disabled until verified.");
  Serial.println("All outputs remain inactive unless explicitly configured.");

  if (!outputsReady) {
    controllerState = kiln::STATE_FAULT;
    secondaryShutdownRequest.assertShutdown();
  }

  if (!controllerLink.begin(controllerBus, KILN_S3_CAN_TX_PIN,
                            KILN_S3_CAN_RX_PIN, KILN_S3_CAN_BITRATE)) {
    Serial.println("CAN initialization unavailable; START/RESUME must remain blocked.");
  }
}

void loop() {
  controllerLink.update();

  sensorFrontend.update(sensorManager, millis());
  sensorManager.update(millis());
  for (uint8_t i = 0; i < SENSOR_CHANNEL_COUNT; ++i)
    kilnTemperaturesC[i] = sensorManager.temperatureC(i);

  if (controllerState == kiln::STATE_FIRING ||
      controllerState == kiln::STATE_PAUSED) {
    controlCore.update(millis(), kilnTemperaturesC,
                       sensorManager.validMask(),
                       sensorManager.controlEnabledMask(),
                       sensorManager.safetyEnabledMask(),
                       controllerLink.hostLinkHealthy());
    if (controlCore.faulted()) {
      relayOutputs.allInactive();
      controllerState = kiln::STATE_FAULT;
    } else if (controllerState == kiln::STATE_PAUSED) {
      relayOutputs.allInactive();
    } else if (controlCore.profileState() == kiln::PROFILE_COMPLETE) {
      relayOutputs.allInactive();
      controllerState = kiln::STATE_COMPLETE;
    } else {
      for (uint8_t i = 0; i < kiln::RELAY_OUTPUT_COUNT; ++i)
        relayOutputs.set(i, controlCore.outputOn(i));
    }
  }

  if (controllerState == kiln::STATE_FAULT) {
    relayOutputs.allInactive();
    secondaryShutdownRequest.assertShutdown();
  }
  delay(2);
}
