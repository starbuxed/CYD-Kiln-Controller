#pragma once
#include <stdint.h>

namespace kiln {

static constexpr uint8_t MAPPING_SENSOR_COUNT = 4;
static constexpr uint8_t MAPPING_ZONE_COUNT = 4;
static constexpr uint8_t MAPPING_OUTPUT_COUNT = 4;

enum SensorCombineMode : uint8_t {
  SENSOR_COMBINE_DESIGNATED = 0,
  SENSOR_COMBINE_HIGHEST = 1,
  SENSOR_COMBINE_LOWEST = 2,
  SENSOR_COMBINE_AVERAGE = 3
};

struct ZoneMappingConfig {
  bool enabled;
  uint8_t sensorMask;
  uint8_t outputMask;
  SensorCombineMode combineMode;
  uint8_t designatedSensor;
};

struct ZoneMeasurement {
  bool valid;
  float temperatureC;
  uint8_t contributingSensorMask;
};

class ZoneMapping {
 public:
  ZoneMapping();
  bool configure(uint8_t zone, const ZoneMappingConfig& config);
  bool configureAll(const ZoneMappingConfig configs[MAPPING_ZONE_COUNT]);
  bool configured(uint8_t zone) const;
  const ZoneMappingConfig& config(uint8_t zone) const;
  bool validate(const ZoneMappingConfig configs[MAPPING_ZONE_COUNT]) const;
  ZoneMeasurement measure(uint8_t zone, const float temperaturesC[MAPPING_SENSOR_COUNT],
                          uint8_t validSensorMask) const;
  uint8_t requiredSensorMask() const;
  uint8_t enabledZoneMask() const;
  uint8_t assignedOutputMask() const;
  uint8_t outputMaskForZone(uint8_t zone) const;
  uint8_t zoneMaskForSensor(uint8_t sensor) const;

 private:
  ZoneMappingConfig zones_[MAPPING_ZONE_COUNT];
  bool configured_[MAPPING_ZONE_COUNT];
};

} // namespace kiln
