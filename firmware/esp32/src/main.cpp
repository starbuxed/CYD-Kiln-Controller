#include <Arduino.h>
#include "board_config.h"
#include "controller_link.h"

#if KILN_UART_PINMAP_AVAILABLE
static HardwareSerial controllerSerial(2);
static ControllerLink controllerLink;
#endif

static void printBoardProfile() {
  Serial.println();
  Serial.println("CYD Kiln Controller - Operator Interface");
  Serial.printf("Board profile: %s\n", KILN_BOARD.name);
  Serial.printf("Model family: %s\n", KILN_BOARD.commonModel);
  Serial.printf("Panel: %u x %u\n", KILN_BOARD.width, KILN_BOARD.height);
  Serial.printf("Display driver: %u, touch driver: %u\n",
                static_cast<unsigned>(KILN_BOARD.displayDriver),
                static_cast<unsigned>(KILN_BOARD.touchDriver));
  Serial.printf("Pin map marked verified: %s\n",
                KILN_BOARD.pinMapVerified ? "yes" : "no");
#if KILN_UART_PINMAP_AVAILABLE
  Serial.printf("S3 UART2 RX/TX: %d / %d at %lu baud\n",
                KILN_UART_RX_PIN, KILN_UART_TX_PIN,
                static_cast<unsigned long>(KILN_UART_BAUD));
#else
  Serial.println("S3 UART disabled: this profile has no verified UART pin map.");
#endif
  Serial.println("CYD is the operator interface; S3 owns kiln control and interlocks.");
}

void setup() {
  Serial.begin(115200);
  delay(250);
  printBoardProfile();

#if KILN_UART_PINMAP_AVAILABLE
  controllerLink.begin(controllerSerial, KILN_UART_RX_PIN, KILN_UART_TX_PIN,
                       KILN_UART_BAUD);
#endif
}

void loop() {
#if KILN_UART_PINMAP_AVAILABLE
  controllerLink.update();

  static uint32_t lastReport = 0;
  const uint32_t now = millis();
  if (static_cast<uint32_t>(now - lastReport) >= 1000) {
    lastReport = now;
    Serial.printf("S3 link: %s, heartbeat age: %lu ms, controller state: %u\n",
                  controllerLink.connected() ? "ONLINE" : "OFFLINE",
                  static_cast<unsigned long>(controllerLink.heartbeatAgeMs()),
                  controllerLink.controllerState());
  }
#endif
  delay(2);
}
