#pragma once
#include <Arduino.h>

// Drives a LOW-VOLTAGE relay-module input used as one of two independent
// master-power permission contacts. The relay's normally-open contact is
// wired in series with the other controller's normally-open permit contact.
//
// This class is not a safety-rated device. External biasing must keep the
// relay released throughout reset, boot, brownout, and MCU power loss.
class MasterPowerPermit {
 public:
  bool begin(int8_t pin, bool activeHigh) {
    pin_ = pin;
    activeHigh_ = activeHigh;
    configured_ = false;
    permit_ = false;
    if (pin_ < 0) return false;

    const uint8_t inactive = activeHigh_ ? LOW : HIGH;
    digitalWrite(pin_, inactive);
    pinMode(pin_, OUTPUT);
    digitalWrite(pin_, inactive);
    configured_ = true;
    return true;
  }

  // False always drops this controller's own permit relay.
  void setPermit(bool allow) {
    permit_ = allow && configured_;
    if (!configured_) return;
    const uint8_t active = activeHigh_ ? HIGH : LOW;
    const uint8_t inactive = activeHigh_ ? LOW : HIGH;
    digitalWrite(pin_, permit_ ? active : inactive);
  }

  void drop() { setPermit(false); }
  bool configured() const { return configured_; }
  bool energized() const { return permit_; }

 private:
  int8_t pin_ = -1;
  bool activeHigh_ = true;
  bool configured_ = false;
  bool permit_ = false;
};
