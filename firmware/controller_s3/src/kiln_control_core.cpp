#include "kiln_control_core.h"
#include <math.h>

namespace kiln {
KilnControlCore::KilnControlCore()
    : configuredCount_(0), profileConfigured_(false), running_(false), paused_(false),
      maximumAllowedC_(1400.0f), soakToleranceC_(5.0f) {
  for (uint8_t i = 0; i < CONTROL_ZONE_COUNT; ++i) {
    zoneConfigured_[i] = false;
    outputs_[i] = false;
    zoneConfig_[i].control = {5.0f, 30000UL, 30000UL};
    zoneConfig_[i].setpointOffsetC = 0.0f;
  }
}
bool KilnControlCore::setMaximumAllowedTemperatureC(float maximumC) {
  if (running_ || paused_ || !isfinite(maximumC) ||
      maximumC < 200.0f || maximumC > PROFILE_MAX_TEMP_C) return false;
  for (uint8_t i = 0; i < configuredCount_; ++i) {
    if (configuredSegments_[i].targetC >= maximumC) return false;
    for (uint8_t z = 0; z < CONTROL_ZONE_COUNT; ++z) {
      if (zoneConfigured_[z]) {
        const float target = configuredSegments_[i].targetC + zoneConfig_[z].setpointOffsetC;
        if (target < 0.0f || target >= maximumC) return false;
      }
    }
  }
  maximumAllowedC_ = maximumC;
  return true;
}
bool KilnControlCore::setSoakToleranceC(float toleranceC) {
  if (running_ || paused_ || !isfinite(toleranceC) ||
      toleranceC < 0.5f || toleranceC > 50.0f) return false;
  soakToleranceC_ = toleranceC;
  return true;
}
bool KilnControlCore::configureProfile(const RampSoakSegment* segments,
                                       uint8_t count) {
  if (running_ || paused_ || !profile_.validate(segments, count)) return false;
  // Store the validated profile by starting only when START is requested.
  // ProfileEngine::start copies the segment array at that time; retain a
  // bounded copy here so configuration is independent of caller memory.
  for (uint8_t i = 0; i < count; ++i) {
    if (segments[i].targetC >= maximumAllowedC_) return false;
    for (uint8_t z = 0; z < CONTROL_ZONE_COUNT; ++z) {
      if (zoneConfigured_[z]) {
        const float zoneTarget = segments[i].targetC + zoneConfig_[z].setpointOffsetC;
        if (zoneTarget < 0.0f || zoneTarget >= maximumAllowedC_) return false;
      }
    }
  }
  configuredCount_ = count;
  for (uint8_t i = 0; i < count; ++i) configuredSegments_[i] = segments[i];
  profileConfigured_ = true;
  return true;
}
bool KilnControlCore::configureZone(uint8_t zone,
                                    const ZoneRuntimeConfig& config) {
  if (zone >= CONTROL_ZONE_COUNT || running_ || paused_ ||
      !isfinite(config.setpointOffsetC) || fabsf(config.setpointOffsetC) > 200.0f)
    return false;
  if (profileConfigured_) {
    for (uint8_t i = 0; i < configuredCount_; ++i) {
      const float zoneTarget = configuredSegments_[i].targetC + config.setpointOffsetC;
      if (zoneTarget < 0.0f || zoneTarget >= maximumAllowedC_) return false;
    }
  }
  if (!zones_[zone].configure(config.control)) return false;
  zoneConfig_[zone] = config;
  zoneConfigured_[zone] = true;
  return true;
}
bool KilnControlCore::allSensorsValid(uint8_t mask) const {
  return (mask & 0x0F) == 0x0F;
}
bool KilnControlCore::start(uint32_t nowMs, const float temps[4],
                            uint8_t validMask, bool linkHealthy) {
  if (running_ || paused_ || safety_.faulted() || !profileConfigured_ ||
      !temps || !allSensorsValid(validMask) || !linkHealthy) return false;
  for (uint8_t i = 0; i < CONTROL_ZONE_COUNT; ++i)
    if (!zoneConfigured_[i]) return false;
  for (uint8_t i = 0; i < CONTROL_ZONE_COUNT; ++i)
    if (!isfinite(temps[i]) || temps[i] < -50.0f ||
        temps[i] >= maximumAllowedC_) return false;
  if (!profile_.start(configuredSegments_, configuredCount_, temps[0], nowMs))
    return false;
  running_ = true;
  paused_ = false;
  allOff();
  return true;
}
void KilnControlCore::update(uint32_t nowMs, const float temps[4],
                             uint8_t validMask, bool linkHealthy) {
  if (!running_ || paused_) return;
  safety_.update(nowMs, validMask, temps, linkHealthy, true, maximumAllowedC_);
  if (safety_.faulted()) {
    allOff();
    running_ = false;
    return;
  }
  profile_.update(nowMs);
  if (profile_.state() == PROFILE_WAITING) {
    bool allZonesAtTarget = true;
    for (uint8_t i = 0; i < CONTROL_ZONE_COUNT; ++i) {
      const float zoneTarget = profile_.setpointC() + zoneConfig_[i].setpointOffsetC;
      if ((validMask & (1u << i)) == 0 ||
          fabsf(temps[i] - zoneTarget) > soakToleranceC_) allZonesAtTarget = false;
    }
    if (allZonesAtTarget) profile_.beginSoak(nowMs);
  }
  if (profile_.state() == PROFILE_COMPLETE) {
    allOff();
    running_ = false;
    return;
  }
  for (uint8_t i = 0; i < CONTROL_ZONE_COUNT; ++i) {
    const float target = profile_.setpointC() + zoneConfig_[i].setpointOffsetC;
    outputs_[i] = zones_[i].update(nowMs, temps[i], target,
                                   (validMask & (1u << i)) != 0, true);
  }
}
bool KilnControlCore::pause(uint32_t nowMs) {
  if (!running_ || paused_) return false;
  profile_.pause(nowMs);
  allOff();
  paused_ = true;
  return true;
}
bool KilnControlCore::resume(uint32_t nowMs, const float temps[4],
                             uint8_t validMask, bool linkHealthy) {
  if (!paused_ || safety_.faulted() || !temps ||
      !allSensorsValid(validMask) || !linkHealthy) return false;
  for (uint8_t i = 0; i < CONTROL_ZONE_COUNT; ++i)
    if (!isfinite(temps[i]) || temps[i] < -50.0f ||
        temps[i] >= maximumAllowedC_) return false;
  if (!profile_.resume(nowMs)) return false;
  paused_ = false;
  running_ = true;
  allOff();
  return true;
}
void KilnControlCore::stop() {
  allOff();
  profile_.stop();
  running_ = false;
  paused_ = false;
}
void KilnControlCore::latchFault(uint16_t fault) {
  safety_.latch(fault);
  allOff();
  running_ = false;
  paused_ = false;
}
bool KilnControlCore::acknowledgeFault(const float temps[4],
                                       uint8_t validMask, bool linkHealthy) {
  if (!safety_.acknowledge(validMask, linkHealthy, temps, maximumAllowedC_))
    return false;
  if (running_ || paused_) stop();
  return true;
}
void KilnControlCore::allOff() {
  for (uint8_t i = 0; i < CONTROL_ZONE_COUNT; ++i) {
    outputs_[i] = false;
    zones_[i].forceOff();
  }
}
float KilnControlCore::setpointC(uint8_t zone) const {
  if (zone >= CONTROL_ZONE_COUNT || !profileConfigured_) return 0.0f;
  return profile_.setpointC() + zoneConfig_[zone].setpointOffsetC;
}
bool KilnControlCore::outputOn(uint8_t zone) const {
  return zone < CONTROL_ZONE_COUNT && outputs_[zone];
}
uint8_t KilnControlCore::outputMask() const {
  uint8_t mask = 0;
  for (uint8_t i = 0; i < CONTROL_ZONE_COUNT; ++i)
    if (outputs_[i]) mask |= static_cast<uint8_t>(1u << i);
  return mask;
}
} // namespace kiln
