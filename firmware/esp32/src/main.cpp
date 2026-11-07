#include <Arduino.h>
#include "board_profiles.h"

static void printBoardProfile() {
  Serial.println();
  Serial.println("CYD Kiln Controller");
  Serial.printf("Board profile: %s\n", KILN_BOARD.name);
  Serial.printf("Model family: %s\n", KILN_BOARD.commonModel);
  Serial.printf("Panel: %u x %u\n", KILN_BOARD.width, KILN_BOARD.height);
  Serial.printf("Pin map marked verified: %s\n", KILN_BOARD.pinMapVerified ? "yes (classic 2.8 reference only)" : "no");
  Serial.println("No kiln outputs are controlled by this bring-up firmware.");
}

void setup() {
  Serial.begin(115200);
  delay(250);
  printBoardProfile();
}

void loop() {
  // Hardware bring-up scaffold only.
  // Display, touch, network, and kiln-control tasks will be added in later phases.
  delay(1000);
}
