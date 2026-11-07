#pragma once
#include <stdint.h>
namespace kiln {
struct ZoneConfig {
  float hysteresisC;
  uint32_t minimumOnMs;
  uint32_t minimumOffMs;
};
class ZoneController {
 public:
  ZoneController();
  bool configure(const ZoneConfig& config);
  bool update(uint32_t nowMs, float temperatureC, float setpointC,
              bool sensorValid, bool heatPermitted);
  void forceOff();
  bool outputOn() const { return outputOn_; }
  bool configured() const { return configured_; }
 private:
  ZoneConfig config_;
  bool configured_;
  bool outputOn_;
  bool hasChanged_;
  uint32_t lastChangeMs_;
  uint32_t lastUpdateMs_;
};
} // namespace kiln
