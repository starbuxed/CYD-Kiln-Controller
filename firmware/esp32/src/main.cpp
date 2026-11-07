#include <Arduino.h>
#include "board_config.h"
#include "controller_link.h"
#include "master_power_permit.h"
#include "touch_ui.h"

#if KILN_CAN_PINMAP_AVAILABLE
static kiln::CanStream controllerBus(kiln::CAN_ROLE_MAIN);
#endif
static ControllerLink controllerLink;
static TouchUi touchUi;

#ifndef KILN_MAIN_POWER_PERMIT_PIN
#define KILN_MAIN_POWER_PERMIT_PIN -1
#endif
#ifndef KILN_MAIN_POWER_PERMIT_ACTIVE_HIGH
#define KILN_MAIN_POWER_PERMIT_ACTIVE_HIGH 1
#endif
static MasterPowerPermit mainPowerPermit;
static bool mainEmergencyLatched = false;

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
  Serial.printf("CYD master-power permit relay configured: %s\n",
                mainPowerPermit.configured() ? "yes" : "no");
  Serial.println("CYD owns UI/web/SD; the ESP32-S3 owns sensor and output control.");
}

void requestEmergencyShutdown() {
  mainEmergencyLatched = true;
  mainPowerPermit.drop();
#if KILN_CAN_PINMAP_AVAILABLE
  controllerLink.sendCommand(kiln::CMD_ESTOP);
#endif
}

void setup() {
  Serial.begin(115200);
  delay(250);
  mainPowerPermit.begin(KILN_MAIN_POWER_PERMIT_PIN,
                        KILN_MAIN_POWER_PERMIT_ACTIVE_HIGH != 0);
  printBoardProfile();
#if KILN_CAN_PINMAP_AVAILABLE
  if (!controllerLink.begin(controllerBus, KILN_CAN_TX_PIN, KILN_CAN_RX_PIN,
                            KILN_CAN_BITRATE)) {
    Serial.println("CAN initialization failed; firing must remain unavailable.");
  }
#endif
  touchUi.begin(controllerLink);
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
  touchUi.update();
#if KILN_CAN_PINMAP_AVAILABLE
  const bool mainPermit = !mainEmergencyLatched &&
      controllerLink.connected() && controllerLink.statusFresh() &&
      controllerLink.controllerState() == kiln::STATE_FIRING &&
      controllerLink.faultFlags() == 0;
#else
  const bool mainPermit = false;
#endif
  mainPowerPermit.setPermit(mainPermit);
  delay(2);
}
