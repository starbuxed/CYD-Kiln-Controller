#pragma once

#include <Arduino.h>
#include <stdint.h>

namespace kiln {

static constexpr uint8_t RELAY_OUTPUT_COUNT = 4;

struct RelayOutputConfig {
  int8_t pin;
  bool activeHigh;
  bool enabled;
};

class RelayOutputs {
 public:
  RelayOutputs();

  // Validates the complete configuration before configuring any GPIO.
  // Disabled or unassigned channels cannot be commanded ON.
  bool begin(const RelayOutputConfig config[RELAY_OUTPUT_COUNT]);

  // Applies the logical state to one enabled output.
  bool set(uint8_t channel, bool active);

  // Commands all configured outputs to their logical inactive state.
  void allInactive();

  bool isConfigured(uint8_t channel) const;
  bool commandedActive(uint8_t channel) const;
  uint8_t configuredCount() const;

 private:
  RelayOutputConfig config_[RELAY_OUTPUT_COUNT];
  bool commanded_[RELAY_OUTPUT_COUNT];
  bool ready_;

  bool validate(const RelayOutputConfig config[RELAY_OUTPUT_COUNT]) const;
  void writeLevel(uint8_t channel, bool active);
};

} // namespace kiln
