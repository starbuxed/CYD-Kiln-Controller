#include <assert.h>
#include <stdint.h>

// Host-side tests for the configuration rules used by the S3 relay driver.
// Hardware GPIO behavior still requires a PlatformIO build and bench test.

struct OutputConfig {
  int8_t pin;
  bool activeHigh;
  bool enabled;
};

static const uint8_t OUTPUT_COUNT = 4;

static bool validConfig(const OutputConfig config[OUTPUT_COUNT]) {
  if (config == 0) return false;
  for (uint8_t i = 0; i < OUTPUT_COUNT; ++i) {
    if (!config[i].enabled) continue;
    if (config[i].pin < 0) return false;
    for (uint8_t j = i + 1; j < OUTPUT_COUNT; ++j) {
      if (config[j].enabled && config[i].pin == config[j].pin) return false;
    }
  }
  return true;
}

int main() {
  const OutputConfig allDisabled[OUTPUT_COUNT] = {
    {-1, true, false}, {-1, true, false},
    {-1, false, false}, {-1, true, false}
  };
  assert(validConfig(allDisabled));

  const OutputConfig fourUniquePins[OUTPUT_COUNT] = {
    {4, true, true}, {5, false, true},
    {6, true, true}, {7, false, true}
  };
  assert(validConfig(fourUniquePins));

  const OutputConfig enabledWithoutPin[OUTPUT_COUNT] = {
    {-1, true, true}, {5, true, true},
    {6, true, true}, {7, true, true}
  };
  assert(!validConfig(enabledWithoutPin));

  const OutputConfig duplicatePins[OUTPUT_COUNT] = {
    {4, true, true}, {5, false, true},
    {4, true, true}, {7, false, true}
  };
  assert(!validConfig(duplicatePins));

  return 0;
}
