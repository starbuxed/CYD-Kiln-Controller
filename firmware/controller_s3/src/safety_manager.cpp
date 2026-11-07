#include "safety_manager.h"
#include <math.h>
namespace kiln {
static constexpr uint32_t SENSOR_LOSS_GRACE_MS = 1500UL;
SafetyManager::SafetyManager()
    : faults_(FAULT_NONE), invalidSinceMs_(0), invalidTimerActive_(false) {}
void SafetyManager::latch(uint16_t fault) { faults_ |= fault; }
void SafetyManager::update(uint32_t nowMs, uint8_t validMask,
                           const float temps[4], bool linkHealthy,
                           bool firing, float maximumAllowedC) {
  if (!firing) {
    invalidTimerActive_ = false;
    return;
  }
  if ((validMask & 0x0F) != 0x0F) {
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
    if (!(validMask & (1u << i))) continue;
    if (!isfinite(temps[i]) || temps[i] < -50.0f || temps[i] > 2000.0f) {
      latch(FAULT_SENSOR_RANGE);
    } else if (temps[i] >= maximumAllowedC) {
      latch(FAULT_OVERTEMP);
    }
  }
}
bool SafetyManager::acknowledge(uint8_t validMask, bool linkHealthy,
                               const float temps[4], float maximumAllowedC) {
  if (!temps || (validMask & 0x0F) != 0x0F || !linkHealthy ||
      !isfinite(maximumAllowedC) || maximumAllowedC <= 0.0f) return false;
  for (uint8_t i = 0; i < 4; ++i) {
    if (!isfinite(temps[i]) || temps[i] < -50.0f ||
        temps[i] >= maximumAllowedC) return false;
  }
  faults_ = FAULT_NONE;
  invalidTimerActive_ = false;
  return true;
}
} // namespace kiln
