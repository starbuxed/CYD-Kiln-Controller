#pragma once
#include <stdint.h>
#include "profile_engine.h"
#include "zone_controller.h"
#include "safety_manager.h"
namespace kiln {
static constexpr uint8_t CONTROL_ZONE_COUNT = 4;
struct ZoneRuntimeConfig {
  ZoneConfig control;
  float setpointOffsetC;
};
class KilnControlCore {
 public:
  KilnControlCore();
  bool configureProfile(const RampSoakSegment* segments, uint8_t count);
  bool setMaximumAllowedTemperatureC(float maximumC);
  bool setSoakToleranceC(float toleranceC);
  bool configureZone(uint8_t zone, const ZoneRuntimeConfig& config);
  bool start(uint32_t nowMs, const float temperaturesC[4],
             uint8_t sensorValidMask, bool hostLinkHealthy);
  bool start(uint32_t nowMs, const float temperaturesC[4],
             uint8_t sensorValidMask, uint8_t controlSensorMask,
             uint8_t safetySensorMask, bool hostLinkHealthy);
  void update(uint32_t nowMs, const float temperaturesC[4],
              uint8_t sensorValidMask, bool hostLinkHealthy);
  void update(uint32_t nowMs, const float temperaturesC[4],
              uint8_t sensorValidMask, uint8_t controlSensorMask,
              uint8_t safetySensorMask, bool hostLinkHealthy);
  bool pause(uint32_t nowMs);
  bool resume(uint32_t nowMs, const float temperaturesC[4],
              uint8_t sensorValidMask, bool hostLinkHealthy);
  bool resume(uint32_t nowMs, const float temperaturesC[4],
              uint8_t sensorValidMask, uint8_t controlSensorMask,
              uint8_t safetySensorMask, bool hostLinkHealthy);
  void stop();
  void latchFault(uint16_t fault);
  bool acknowledgeFault(const float temperaturesC[4],
                       uint8_t sensorValidMask, bool hostLinkHealthy);
  bool acknowledgeFault(const float temperaturesC[4],
                       uint8_t sensorValidMask, uint8_t safetySensorMask,
                       bool hostLinkHealthy);
  uint16_t faults() const { return safety_.faults(); }
  bool faulted() const { return safety_.faulted(); }
  ProfileRunState profileState() const { return profile_.state(); }
  float setpointC(uint8_t zone) const;
  bool outputOn(uint8_t zone) const;
  uint8_t outputMask() const;
  bool running() const { return running_; }
 private:
  ProfileEngine profile_;
  SafetyManager safety_;
  ZoneController zones_[CONTROL_ZONE_COUNT];
  ZoneRuntimeConfig zoneConfig_[CONTROL_ZONE_COUNT];
  bool zoneConfigured_[CONTROL_ZONE_COUNT];
  bool outputs_[CONTROL_ZONE_COUNT];
  RampSoakSegment configuredSegments_[MAX_PROFILE_SEGMENTS];
  uint8_t configuredCount_;
  bool profileConfigured_;
  bool running_;
  bool paused_;
  float maximumAllowedC_;
  float soakToleranceC_;
  void allOff();
  bool sensorsValid(uint8_t validMask, uint8_t requiredMask) const;
};
} // namespace kiln
