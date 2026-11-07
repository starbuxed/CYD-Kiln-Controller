#pragma once
#include <stdint.h>
namespace kiln {
enum SafetyFault : uint16_t {
  FAULT_NONE = 0,
  FAULT_SENSOR_LOSS = 1u << 0,
  FAULT_OVERTEMP = 1u << 1,
  FAULT_HOST_LINK = 1u << 2,
  FAULT_SENSOR_RANGE = 1u << 3,
  FAULT_PROFILE = 1u << 4,
  FAULT_ESTOP = 1u << 5
};
class SafetyManager {
 public:
  SafetyManager();
  void update(uint32_t nowMs, uint8_t sensorValidMask,
              const float temperaturesC[4], bool hostLinkHealthy,
              bool firing, float maximumAllowedC);
  void update(uint32_t nowMs, uint8_t safetySensorMask,
              uint8_t sensorValidMask, const float temperaturesC[4],
              bool hostLinkHealthy, bool firing, float maximumAllowedC);
  bool acknowledge(uint8_t sensorValidMask, bool hostLinkHealthy,
                   const float temperaturesC[4], float maximumAllowedC);
  bool acknowledge(uint8_t safetySensorMask, uint8_t sensorValidMask,
                   bool hostLinkHealthy, const float temperaturesC[4],
                   float maximumAllowedC);
  void latch(uint16_t fault);
  uint16_t faults() const { return faults_; }
  bool faulted() const { return faults_ != FAULT_NONE; }
 private:
  uint16_t faults_;
  uint32_t invalidSinceMs_;
  bool invalidTimerActive_;
};
} // namespace kiln
