#include <Arduino.h>
#include "board_config.h"
#include "controller_link.h"
#include "shutdown_output.h"

#if KILN_CAN_PINMAP_AVAILABLE
static kiln::CanStream controllerBus(kiln::CAN_ROLE_MAIN);
static ControllerLink controllerLink;
#endif

static ShutdownRequestOutput mainShutdownRequest;

static void printBoardProfile() {
  Serial.println();
  Serial.println("CYD Kiln Controller - Main Operator Interface");
  Serial.printf("Board profile: %s\n", KILN_BOARD.name);
  Serial.printf("Model family: %s\n", KILN_BOARD.commonModel);
  Serial.printf("Panel: %u x %u\n", KILN_BOARD.width, KILN_BOARD.height);
  Serial.printf("Display driver: %u, touch driver: %u\n",
                static_cast<unsigned>(KILN_BOARD.displayDriver),
                static_cast<unsigned>(KILN_BOARD.touchDriver));
  Serial.printf("Pin map marked verified: %s\n",
                KILN_BOARD.pinMapVerified ? "yes" : "no");
#if KILN_CAN_PINMAP_AVAILABLE
  Serial.printf("CAN TX/RX: %d / %d at %lu bit/s\n",
                KILN_CAN_TX_PIN, KILN_CAN_RX_PIN,
                static_cast<unsigned long>(KILN_CAN_BITRATE));
#else
  Serial.println("CAN disabled: exact CYD pins are not yet verified.");
#endif
  Serial.printf("Main shutdown-request GPIO configured: %s\n",
                mainShutdownRequest.configured() ? "yes" : "no");
  Serial.println("CYD owns UI/web/SD; the ESP32-S3 owns sensor and output control.");
}

// Call this from a touchscreen emergency control or a local emergency input.
// The hardwired request is asserted first; CAN is supplementary.
void requestEmergencyShutdown() {
  mainShutdownRequest.assertShutdown();
#if KILN_CAN_PINMAP_AVAILABLE
  controllerLink.sendCommand(kiln::CMD_ESTOP);
#endif
}

void setup() {
  Serial.begin(115200);
  delay(250);
  mainShutdownRequest.begin(KILN_MAIN_SHUTDOWN_REQUEST_PIN);
  printBoardProfile();

#if KILN_CAN_PINMAP_AVAILABLE
  if (!controllerLink.begin(controllerBus, KILN_CAN_TX_PIN, KILN_CAN_RX_PIN,
                            KILN_CAN_BITRATE)) {
    Serial.println("CAN initialization failed; firing must remain unavailable.");
  }
#endif
}

void loop() {
#if KILN_CAN_PINMAP_AVAILABLE
  controllerLink.update();

  static uint32_t lastReport = 0;
  const uint32_t now = millis();
  if (static_cast<uint32_t>(now - lastReport) >= 1000) {
    lastReport = now;
    Serial.printf("S3 link: %s, heartbeat age: %lu ms, state: %u, faults: 0x%04X\n",
                  controllerLink.connected() ? "ONLINE" : "OFFLINE",
                  static_cast<unsigned long>(controllerLink.heartbeatAgeMs()),
                  controllerLink.controllerState(), controllerLink.faultFlags());
  }
#endif
  delay(2);
}
