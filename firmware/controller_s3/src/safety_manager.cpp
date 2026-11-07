#include "safety_manager.h"
#include <math.h>
namespace kiln {
static constexpr uint32_t SENSOR_LOSS_GRACE_MS = 1500UL;
SafetyManager::SafetyManager()
    : faults_(FAULT_NONE), invalidSinceMs_(0), invalidTimerActive_(false) {}
void SafetyManager::latch(uint16_t fault) { faults_ |= fault; }
void SafetyManager::update(uint32_t nowMs, uint8_t safetyMask,
                           uint8_t validMask, const float temps[4],
                           bool linkHealthy, bool firing,
                           float maximumAllowedC) {
  if (!firing) {
    invalidTimerActive_ = false;
    return;
  }
  safetyMask &= 0x0F;
  const bool missingSafetySensor = (validMask & safetyMask) != safetyMask;
  if (missingSafetySensor) {
    if (!invalidTimerActive_) {
      invalidTimerActive_ = true;
      invalidSinceMs_ = nowMs;
    } else if (static_cast<uint32_t>(nowMs - invalidSinceMs_) >= SENSOR_LOSS_GRACE_MS) {
      latch(FAULT_SENSOR_LOSS);
    }
  } else {
    invalidTimerActive_ = false;
  }
  if (!linkHealthy) latch(FAULT_HOST_LINK);
  if (!temps || !isfinite(maximumAllowedC) || maximumAllowedC <= 0.0f) {
    latch(FAULT_SENSOR_RANGE);
    return;
  }
  for (uint8_t i = 0; i < 4; ++i) {
    const uint8_t bit = static_cast<uint8_t>(1u << i);
    if ((safetyMask & bit) == 0 || (validMask & bit) == 0) continue;
    if (!isfinite(temps[i]) || temps[i] < -50.0f || temps[i] > 2000.0f) {
      latch(FAULT_SENSOR_RANGE);
    } else if (temps[i] >= maximumAllowedC) {
      latch(FAULT_OVERTEMP);
    }
  }
}
bool SafetyManager::acknowledge(uint8_t safetyMask, uint8_t validMask,
                               bool linkHealthy, const float temps[4],
                               float maximumAllowedC) {
  safetyMask &= 0x0F;
  if (!temps || (validMask & safetyMask) != safetyMask || !linkHealthy ||
      !isfinite(maximumAllowedC) || maximumAllowedC <= 0.0f) return false;
  for (uint8_t i = 0; i < 4; ++i) {
    const uint8_t bit = static_cast<uint8_t>(1u << i);
    if ((safetyMask & bit) == 0) continue;
    if (!isfinite(temps[i]) || temps[i] < -50.0f ||
        temps[i] >= maximumAllowedC) return false;
  }
  faults_ = FAULT_NONE;
  invalidTimerActive_ = false;
  return true;
}
} // namespace kiln
