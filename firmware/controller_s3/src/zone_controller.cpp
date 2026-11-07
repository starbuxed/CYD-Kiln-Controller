#include "zone_controller.h"
#include <math.h>
namespace kiln {
ZoneController::ZoneController()
    : configured_(false), outputOn_(false), hasChanged_(false), lastChangeMs_(0) {
  config_ = {5.0f, 30000UL, 30000UL};
}
bool ZoneController::configure(const ZoneConfig& c) {
  if (!isfinite(c.hysteresisC) || c.hysteresisC < 0.5f ||
      c.hysteresisC > 100.0f || c.minimumOnMs > 3600000UL ||
      c.minimumOffMs > 3600000UL) return false;
  config_ = c;
  configured_ = true;
  forceOff();
  hasChanged_ = false;
  return true;
}
bool ZoneController::update(uint32_t nowMs, float temperatureC,
                            float setpointC, bool sensorValid,
                            bool heatPermitted) {
  if (!configured_ || !sensorValid || !heatPermitted ||
      !isfinite(temperatureC) || !isfinite(setpointC)) {
    forceOff();
    return false;
  }
  bool desired = outputOn_;
  const float halfBand = config_.hysteresisC * 0.5f;
  if (temperatureC <= setpointC - halfBand) desired = true;
  if (temperatureC >= setpointC + halfBand) desired = false;
  if (desired != outputOn_) {
    const uint32_t dwell = outputOn_ ? config_.minimumOnMs : config_.minimumOffMs;
    if (!hasChanged_ || static_cast<uint32_t>(nowMs - lastChangeMs_) >= dwell) {
      outputOn_ = desired;
      lastChangeMs_ = nowMs;
      hasChanged_ = true;
    }
  }
  return outputOn_;
}
} // namespace kiln
