#pragma once
#include <stdint.h>
namespace kiln {
static constexpr uint8_t TEMPERATURE_CHANNELS = 4;
enum ThermocoupleType : uint8_t {
  TC_K = 0, TC_J = 1, TC_N = 2, TC_R = 3,
  TC_S = 4, TC_T = 5, TC_E = 6, TC_B = 7
};
enum SensorFault : uint8_t {
  SENSOR_OK = 0, SENSOR_OPEN = 1, SENSOR_SHORT = 2,
  SENSOR_CONVERSION = 3, SENSOR_STALE = 4,
  SENSOR_OUT_OF_RANGE = 5, SENSOR_RATE_IMPLAUSIBLE = 6,
  SENSOR_NOT_CONFIGURED = 7
};
struct TemperatureReading {
  float temperatureC;
  uint32_t sampledAtMs;
  SensorFault fault;
  bool valid;
};
class SensorManager {
 public:
  SensorManager();
  bool configure(uint8_t channel, ThermocoupleType type,
                 float calibrationOffsetC = 0.0f);
  bool submitSample(uint8_t channel, float temperatureC,
                    SensorFault driverFault, uint32_t nowMs);
  void update(uint32_t nowMs);
  bool valid(uint8_t channel) const;
  bool configured(uint8_t channel) const;
  uint8_t validMask() const;
  float temperatureC(uint8_t channel) const;
  SensorFault fault(uint8_t channel) const;
  ThermocoupleType type(uint8_t channel) const;
 private:
  struct Channel {
    ThermocoupleType type;
    TemperatureReading reading;
    bool configured;
    float calibrationOffsetC;
    bool hasPrevious;
    float previousC;
    uint32_t previousAtMs;
  };
  Channel channels_[TEMPERATURE_CHANNELS];
  float maximumFor(ThermocoupleType type) const;
};
} // namespace kiln
