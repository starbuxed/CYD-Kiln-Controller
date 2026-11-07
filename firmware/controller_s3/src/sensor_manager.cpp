#include "sensor_manager.h"
#include <math.h>
namespace kiln {
static constexpr uint32_t SENSOR_STALE_MS = 2000UL;
static constexpr float MAX_RATE_C_PER_MIN = 100.0f;
SensorManager::SensorManager() {
  for (uint8_t i = 0; i < TEMPERATURE_CHANNELS; ++i) {
    channels_[i].type = TC_K;
    channels_[i].reading = {0.0f, 0, SENSOR_NOT_CONFIGURED, false};
    channels_[i].configured = false;
    channels_[i].mode = SENSOR_MODE_LOG_SAFETY;
    channels_[i].calibrationOffsetC = 0.0f;
    channels_[i].hasPrevious = false;
    channels_[i].previousC = 0.0f;
    channels_[i].previousAtMs = 0;
  }
}
float SensorManager::maximumFor(ThermocoupleType t) const {
  switch (t) {
    case TC_K: return 1372.0f;
    case TC_J: return 1200.0f;
    case TC_N: return 1300.0f;
    case TC_R: return 1768.0f;
    case TC_S: return 1768.0f;
    case TC_T: return 400.0f;
    case TC_E: return 1000.0f;
    case TC_B: return 1700.0f;
    default: return 0.0f;
  }
}
bool SensorManager::configure(uint8_t ch, ThermocoupleType t, float offsetC) {
  if (ch >= TEMPERATURE_CHANNELS || t > TC_B || !isfinite(offsetC) ||
      fabsf(offsetC) > 50.0f) return false;
  channels_[ch].type = t;
  channels_[ch].configured = true;
  channels_[ch].calibrationOffsetC = offsetC;
  channels_[ch].hasPrevious = false;
  channels_[ch].reading.valid = false;
  channels_[ch].reading.fault = SENSOR_NOT_CONFIGURED;
  return true;
}
bool SensorManager::setMode(uint8_t ch, SensorOperatingMode modeValue) {
  if (ch >= TEMPERATURE_CHANNELS || modeValue > SENSOR_MODE_LOG_SAFETY)
    return false;
  Channel& c = channels_[ch];
  c.mode = modeValue;
  // Turning a channel off discards its current validity. Re-enabling it must
  // wait for a fresh sample before it can be used for control or safety.
  c.reading.valid = false;
  c.reading.fault = c.configured ? SENSOR_STALE : SENSOR_NOT_CONFIGURED;
  c.hasPrevious = false;
  return true;
}
SensorOperatingMode SensorManager::mode(uint8_t ch) const {
  return ch < TEMPERATURE_CHANNELS ? channels_[ch].mode : SENSOR_MODE_OFF;
}
bool SensorManager::monitoringEnabled(uint8_t ch) const {
  return ch < TEMPERATURE_CHANNELS && channels_[ch].configured &&
         channels_[ch].mode != SENSOR_MODE_OFF;
}
bool SensorManager::controlEnabled(uint8_t ch) const {
  return ch < TEMPERATURE_CHANNELS && channels_[ch].configured &&
         channels_[ch].mode == SENSOR_MODE_LOG_CONTROL;
}
bool SensorManager::safetyEnabled(uint8_t ch) const {
  return ch < TEMPERATURE_CHANNELS && channels_[ch].configured &&
         (channels_[ch].mode == SENSOR_MODE_LOG_CONTROL ||
          channels_[ch].mode == SENSOR_MODE_LOG_SAFETY);
}
uint8_t SensorManager::monitoringMask() const {
  uint8_t mask = 0;
  for (uint8_t i = 0; i < TEMPERATURE_CHANNELS; ++i)
    if (monitoringEnabled(i)) mask |= static_cast<uint8_t>(1u << i);
  return mask;
}
uint8_t SensorManager::controlEnabledMask() const {
  uint8_t mask = 0;
  for (uint8_t i = 0; i < TEMPERATURE_CHANNELS; ++i)
    if (controlEnabled(i)) mask |= static_cast<uint8_t>(1u << i);
  return mask;
}
uint8_t SensorManager::safetyEnabledMask() const {
  uint8_t mask = 0;
  for (uint8_t i = 0; i < TEMPERATURE_CHANNELS; ++i)
    if (safetyEnabled(i)) mask |= static_cast<uint8_t>(1u << i);
  return mask;
}
bool SensorManager::submitSample(uint8_t ch, float temp,
                                 SensorFault driverFault, uint32_t nowMs) {
  if (ch >= TEMPERATURE_CHANNELS || !monitoringEnabled(ch)) return false;
  Channel& c = channels_[ch];
  c.reading.sampledAtMs = nowMs;
  c.reading.temperatureC = temp + c.calibrationOffsetC;
  c.reading.fault = driverFault;
  c.reading.valid = false;
  if (driverFault != SENSOR_OK) return true;
  temp = c.reading.temperatureC;
  if (!isfinite(temp) || temp < -270.0f || temp > maximumFor(c.type)) {
    c.reading.fault = SENSOR_OUT_OF_RANGE;
    return true;
  }
  if (c.hasPrevious) {
    const uint32_t dt = static_cast<uint32_t>(nowMs - c.previousAtMs);
    if (dt > 0) {
      const float rate = fabsf(temp - c.previousC) * 60000.0f /
                         static_cast<float>(dt);
      if (rate > MAX_RATE_C_PER_MIN) {
        c.reading.fault = SENSOR_RATE_IMPLAUSIBLE;
        return true;
      }
    }
  }
  c.reading.valid = true;
  c.reading.fault = SENSOR_OK;
  c.previousC = temp;
  c.previousAtMs = nowMs;
  c.hasPrevious = true;
  return true;
}
void SensorManager::update(uint32_t nowMs) {
  for (uint8_t i = 0; i < TEMPERATURE_CHANNELS; ++i) {
    Channel& c = channels_[i];
    if (!monitoringEnabled(i)) continue;
    if (static_cast<uint32_t>(nowMs - c.reading.sampledAtMs) > SENSOR_STALE_MS) {
      c.reading.valid = false;
      c.reading.fault = SENSOR_STALE;
    }
  }
}
bool SensorManager::configured(uint8_t ch) const {
  return ch < TEMPERATURE_CHANNELS && channels_[ch].configured;
}
bool SensorManager::valid(uint8_t ch) const {
  return ch < TEMPERATURE_CHANNELS && monitoringEnabled(ch) &&
         channels_[ch].reading.valid;
}
uint8_t SensorManager::validMask() const {
  uint8_t mask = 0;
  for (uint8_t i = 0; i < TEMPERATURE_CHANNELS; ++i)
    if (valid(i)) mask |= static_cast<uint8_t>(1u << i);
  return mask;
}
float SensorManager::temperatureC(uint8_t ch) const {
  return ch < TEMPERATURE_CHANNELS ? channels_[ch].reading.temperatureC : 0.0f;
}
SensorFault SensorManager::fault(uint8_t ch) const {
  return ch < TEMPERATURE_CHANNELS ? channels_[ch].reading.fault : SENSOR_NOT_CONFIGURED;
}
ThermocoupleType SensorManager::type(uint8_t ch) const {
  return ch < TEMPERATURE_CHANNELS ? channels_[ch].type : TC_K;
}
} // namespace kiln
