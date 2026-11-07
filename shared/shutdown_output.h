#pragma once
#include <Arduino.h>

// Low-voltage shutdown-request output only. It is NOT a safety contact and
// must connect to a separately engineered, fail-safe hardware safety circuit.
// Default unconfigured state is deliberately non-operational.
class ShutdownRequestOutput {
 public:
  bool begin(int8_t pin) {
    pin_ = pin;
    asserted_ = false;
    if (pin_ < 0) return false;
    // Active-low open-drain request. External interface design must define
    // safe behavior on MCU loss/reset and must not rely on this GPIO alone.
    digitalWrite(pin_, HIGH);
    pinMode(pin_, OUTPUT_OPEN_DRAIN);
    configured_ = true;
    return true;
  }

  void assertShutdown() {
    asserted_ = true;
    if (configured_) digitalWrite(pin_, LOW);
  }

  // Release is permitted only after a deliberate physical/manual reset and
  // after the external safety circuit reports all trip conditions cleared.
  bool releaseAfterManualReset(bool manualResetAuthorized,
                               bool allTripInputsClear) {
    if (!manualResetAuthorized || !allTripInputsClear) return false;
    asserted_ = false;
    if (configured_) digitalWrite(pin_, HIGH);
    return configured_;
  }

  bool configured() const { return configured_; }
  bool asserted() const { return asserted_; }

 private:
  int8_t pin_ = -1;
  bool configured_ = false;
  bool asserted_ = false;
};
