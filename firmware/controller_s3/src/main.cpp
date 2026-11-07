#include <Arduino.h>
#include "relay_outputs.h"

// Dedicated four-zone controller bring-up.
// Physical GPIO assignments remain intentionally unset until the exact
// ESP32-S3 board revision and external relay interface are confirmed.

static constexpr uint8_t SENSOR_CHANNEL_COUNT = 4;
static constexpr uint8_t HEATER_ZONE_COUNT = 4;

enum class ControllerState : uint8_t {
  BOOT,
  SELF_TEST,
  IDLE,
  VALIDATING,
  ARMED,
  FIRING,
  STOPPING,
  COMPLETE,
  FAULT
};

static ControllerState controllerState = ControllerState::BOOT;
static kiln::RelayOutputs relayOutputs;

static const kiln::RelayOutputConfig outputConfig[kiln::RELAY_OUTPUT_COUNT] = {
  {-1, true, false}, // Output 1: pin unassigned, active-high, disabled
  {-1, true, false}, // Output 2: pin unassigned, active-high, disabled
  {-1, true, false}, // Output 3: pin unassigned, active-high, disabled
  {-1, true, false}  // Output 4: pin unassigned, active-high, disabled
};

static void forceAllHeaterRequestsInactive() {
  relayOutputs.allInactive();
}

static void enterLatchedFault(const char* reason) {
  forceAllHeaterRequestsInactive();
  controllerState = ControllerState::FAULT;
  Serial.printf("LATCHED FAULT: %s\n", reason);
}

void setup() {
  Serial.begin(115200);
  delay(200);

  // Initialize outputs before any other controller activity. Unassigned
  // channels remain unavailable and no output can be energized at startup.
  const bool outputsReady = relayOutputs.begin(outputConfig);
  forceAllHeaterRequestsInactive();

  controllerState = ControllerState::SELF_TEST;

  Serial.println();
  Serial.println("CYD Kiln Controller - dedicated ESP32-S3");
  Serial.printf("Sensor channels: %u\n", SENSOR_CHANNEL_COUNT);
  Serial.printf("Independent heater zones: %u\n", HEATER_ZONE_COUNT);
  Serial.printf("Relay output channels: %u\n", kiln::RELAY_OUTPUT_COUNT);
  Serial.printf("Output configuration valid: %s\n", outputsReady ? "yes" : "no");
  Serial.printf("Configured output channels: %u\n", relayOutputs.configuredCount());
  Serial.println("Physical GPIO assignments are intentionally unset.");
  Serial.println("No live heater control is enabled.");

  // Remain non-firing until sensors, safety supervision, communications,
  // and verified physical output interfaces are implemented and tested.
  if (!outputsReady) {
    enterLatchedFault("Relay output configuration invalid");
    return;
  }
  controllerState = ControllerState::IDLE;
}

void loop() {
  // No firing or output-energizing path is enabled in this bring-up stage.
  // Later stages add sensor acquisition, local zone control, protocol,
  // watchdog supervision, and explicit start/stop state transitions.
  delay(10);
}
