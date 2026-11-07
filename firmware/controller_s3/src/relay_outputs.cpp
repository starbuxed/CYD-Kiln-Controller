#include "relay_outputs.h"

namespace kiln {

RelayOutputs::RelayOutputs() : ready_(false) {
  for (uint8_t i = 0; i < RELAY_OUTPUT_COUNT; ++i) {
    config_[i] = {-1, true, false};
    commanded_[i] = false;
  }
}

bool RelayOutputs::validate(
    const RelayOutputConfig config[RELAY_OUTPUT_COUNT]) const {
  if (config == nullptr) return false;

  for (uint8_t i = 0; i < RELAY_OUTPUT_COUNT; ++i) {
    if (!config[i].enabled) continue;
    if (config[i].pin < 0) return false;

    for (uint8_t j = i + 1; j < RELAY_OUTPUT_COUNT; ++j) {
      if (config[j].enabled && config[i].pin == config[j].pin) {
        return false;
      }
    }
  }
  return true;
}

bool RelayOutputs::begin(
    const RelayOutputConfig config[RELAY_OUTPUT_COUNT]) {
  ready_ = false;
  allInactive();

  if (!validate(config)) return false;

  for (uint8_t i = 0; i < RELAY_OUTPUT_COUNT; ++i) {
    config_[i] = config[i];
    commanded_[i] = false;
  }

  // Preload the inactive output latch before switching each pin to OUTPUT.
  // External hardware biasing is still required during reset/boot/power loss.
  for (uint8_t i = 0; i < RELAY_OUTPUT_COUNT; ++i) {
    if (!config_[i].enabled) continue;
    const uint8_t inactiveLevel = config_[i].activeHigh ? LOW : HIGH;
    digitalWrite(config_[i].pin, inactiveLevel);
    pinMode(config_[i].pin, OUTPUT);
  }

  ready_ = true;
  return true;
}

void RelayOutputs::writeLevel(uint8_t channel, bool active) {
  const uint8_t activeLevel = config_[channel].activeHigh ? HIGH : LOW;
  digitalWrite(config_[channel].pin, active ? activeLevel : (activeLevel == HIGH ? LOW : HIGH));
  commanded_[channel] = active;
}

bool RelayOutputs::set(uint8_t channel, bool active) {
  if (!ready_ || channel >= RELAY_OUTPUT_COUNT || !config_[channel].enabled) {
    return false;
  }
  writeLevel(channel, active);
  return true;
}

void RelayOutputs::allInactive() {
  for (uint8_t i = 0; i < RELAY_OUTPUT_COUNT; ++i) {
    commanded_[i] = false;
    if (ready_ && config_[i].enabled) {
      const uint8_t inactiveLevel = config_[i].activeHigh ? LOW : HIGH;
      digitalWrite(config_[i].pin, inactiveLevel);
    }
  }
}

bool RelayOutputs::isConfigured(uint8_t channel) const {
  return ready_ && channel < RELAY_OUTPUT_COUNT && config_[channel].enabled;
}

bool RelayOutputs::commandedActive(uint8_t channel) const {
  return channel < RELAY_OUTPUT_COUNT && commanded_[channel];
}

uint8_t RelayOutputs::configuredCount() const {
  uint8_t count = 0;
  for (uint8_t i = 0; i < RELAY_OUTPUT_COUNT; ++i) {
    if (config_[i].enabled) ++count;
  }
  return count;
}

} // namespace kiln
