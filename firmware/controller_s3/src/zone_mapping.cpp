#include "zone_mapping.h"
#include <math.h>

namespace kiln {

ZoneMapping::ZoneMapping() {
  for (uint8_t i = 0; i < MAPPING_ZONE_COUNT; ++i) {
    zones_[i] = {false, 0, 0, SENSOR_COMBINE_DESIGNATED, 0};
    configured_[i] = false;
  }
}

bool ZoneMapping::validate(
    const ZoneMappingConfig configs[MAPPING_ZONE_COUNT]) const {
  if (!configs) return false;
  uint8_t usedOutputs = 0;
  for (uint8_t z = 0; z < MAPPING_ZONE_COUNT; ++z) {
    const ZoneMappingConfig& c = configs[z];
    if (!c.enabled) {
      if (c.sensorMask != 0 || c.outputMask != 0) return false;
      continue;
    }
    if (c.sensorMask == 0 || c.outputMask == 0 ||
        (c.sensorMask & 0xF0) != 0 || (c.outputMask & 0xF0) != 0 ||
        c.combineMode > SENSOR_COMBINE_AVERAGE) return false;
    if (c.combineMode == SENSOR_COMBINE_DESIGNATED &&
        (c.designatedSensor >= MAPPING_SENSOR_COUNT ||
         (c.sensorMask & (1u << c.designatedSensor)) == 0)) return false;
    if ((usedOutputs & c.outputMask) != 0) return false;
    usedOutputs |= c.outputMask;
  }
  return true;
}

bool ZoneMapping::configure(uint8_t zone, const ZoneMappingConfig& config) {
  if (zone >= MAPPING_ZONE_COUNT) return false;
  ZoneMappingConfig candidate[MAPPING_ZONE_COUNT];
  for (uint8_t i = 0; i < MAPPING_ZONE_COUNT; ++i)
    candidate[i] = configured_[i] ? zones_[i]
                                  : ZoneMappingConfig{false, 0, 0, SENSOR_COMBINE_DESIGNATED, 0};
  candidate[zone] = config;
  if (!validate(candidate)) return false;
  zones_[zone] = config;
  configured_[zone] = true;
  return true;
}

bool ZoneMapping::configureAll(
    const ZoneMappingConfig configs[MAPPING_ZONE_COUNT]) {
  if (!validate(configs)) return false;
  for (uint8_t i = 0; i < MAPPING_ZONE_COUNT; ++i) {
    zones_[i] = configs[i];
    configured_[i] = true;
  }
  return true;
}

bool ZoneMapping::configured(uint8_t zone) const {
  return zone < MAPPING_ZONE_COUNT && configured_[zone];
}

const ZoneMappingConfig& ZoneMapping::config(uint8_t zone) const {
  static const ZoneMappingConfig invalid = {
      false, 0, 0, SENSOR_COMBINE_DESIGNATED, 0};
  return zone < MAPPING_ZONE_COUNT ? zones_[zone] : invalid;
}

ZoneMeasurement ZoneMapping::measure(
    uint8_t zone, const float temperaturesC[MAPPING_SENSOR_COUNT],
    uint8_t validSensorMask) const {
  ZoneMeasurement result = {false, 0.0f, 0};
  if (zone >= MAPPING_ZONE_COUNT || !configured_[zone] || !temperaturesC ||
      !zones_[zone].enabled) return result;

  const ZoneMappingConfig& c = zones_[zone];
  const uint8_t available = static_cast<uint8_t>(c.sensorMask & validSensorMask & 0x0F);
  if (c.combineMode == SENSOR_COMBINE_DESIGNATED) {
    const uint8_t bit = static_cast<uint8_t>(1u << c.designatedSensor);
    if ((available & bit) == 0 || !isfinite(temperaturesC[c.designatedSensor]))
      return result;
    result.valid = true;
    result.temperatureC = temperaturesC[c.designatedSensor];
    result.contributingSensorMask = bit;
    return result;
  }

  // A group measurement is valid only when every assigned sensor is valid.
  // This prevents a failed sensor from silently disappearing from an average
  // or extrema calculation and masking a wiring or sensor fault.
  if (available != c.sensorMask) return result;

  bool first = true;
  float aggregate = 0.0f;
  float sum = 0.0f;
  for (uint8_t s = 0; s < MAPPING_SENSOR_COUNT; ++s) {
    const uint8_t bit = static_cast<uint8_t>(1u << s);
    if ((c.sensorMask & bit) == 0) continue;
    const float value = temperaturesC[s];
    if (!isfinite(value)) return result;
    if (first) {
      aggregate = value;
      first = false;
    } else if (c.combineMode == SENSOR_COMBINE_HIGHEST && value > aggregate) {
      aggregate = value;
    } else if (c.combineMode == SENSOR_COMBINE_LOWEST && value < aggregate) {
      aggregate = value;
    }
    sum += value;
  }
  if (first) return result;
  result.valid = true;
  result.temperatureC = c.combineMode == SENSOR_COMBINE_AVERAGE
                           ? sum / static_cast<float>(__builtin_popcount(c.sensorMask))
                           : aggregate;
  result.contributingSensorMask = c.sensorMask;
  return result;
}

uint8_t ZoneMapping::requiredSensorMask() const {
  uint8_t mask = 0;
  for (uint8_t z = 0; z < MAPPING_ZONE_COUNT; ++z)
    if (configured_[z] && zones_[z].enabled) mask |= zones_[z].sensorMask;
  return mask;
}

uint8_t ZoneMapping::enabledZoneMask() const {
  uint8_t mask = 0;
  for (uint8_t z = 0; z < MAPPING_ZONE_COUNT; ++z)
    if (configured_[z] && zones_[z].enabled) mask |= static_cast<uint8_t>(1u << z);
  return mask;
}

uint8_t ZoneMapping::assignedOutputMask() const {
  uint8_t mask = 0;
  for (uint8_t z = 0; z < MAPPING_ZONE_COUNT; ++z)
    if (configured_[z] && zones_[z].enabled) mask |= zones_[z].outputMask;
  return mask;
}

uint8_t ZoneMapping::outputMaskForZone(uint8_t zone) const {
  return zone < MAPPING_ZONE_COUNT && configured_[zone] && zones_[zone].enabled
             ? zones_[zone].outputMask : 0;
}

uint8_t ZoneMapping::zoneMaskForSensor(uint8_t sensor) const {
  if (sensor >= MAPPING_SENSOR_COUNT) return 0;
  uint8_t mask = 0;
  for (uint8_t z = 0; z < MAPPING_ZONE_COUNT; ++z)
    if (configured_[z] && zones_[z].enabled &&
        (zones_[z].sensorMask & (1u << sensor)))
      mask |= static_cast<uint8_t>(1u << z);
  return mask;
}

} // namespace kiln
