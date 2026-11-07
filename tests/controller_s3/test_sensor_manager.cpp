#include <assert.h>
#include "sensor_manager.h"
int main() {
  using namespace kiln;
  SensorManager sensors;
  assert(sensors.configure(0, TC_K));
  assert(sensors.configure(1, TC_S));
  assert(sensors.configure(2, TC_J));
  assert(sensors.configure(3, TC_T));
  assert(sensors.type(1) == TC_S);
  // Configured sensors default to safety monitoring, never silently to OFF.
  assert(sensors.mode(0) == SENSOR_MODE_LOG_SAFETY);
  assert(sensors.safetyEnabledMask() == 0x0F);
  assert(!sensors.submitSample(4, 20.0f, SENSOR_OK, 0));
  for (uint8_t i = 0; i < 4; ++i)
    assert(sensors.submitSample(i, 25.0f, SENSOR_OK, 100));
  assert(sensors.validMask() == 0x0F);
  assert(sensors.submitSample(0, 1500.0f, SENSOR_OK, 200));
  assert(!sensors.valid(0));
  assert(sensors.fault(0) == SENSOR_OUT_OF_RANGE);
  assert(sensors.submitSample(0, 30.0f, SENSOR_OK, 300));
  assert(sensors.fault(0) == SENSOR_RATE_IMPLAUSIBLE);
  sensors.update(3000);
  assert(!sensors.valid(1));
  assert(sensors.fault(1) == SENSOR_STALE);

  // The four user modes have independent responsibilities.
  assert(sensors.setMode(0, SENSOR_MODE_OFF));
  assert(!sensors.monitoringEnabled(0));
  assert(!sensors.controlEnabled(0));
  assert(!sensors.safetyEnabled(0));
  assert(!sensors.submitSample(0, 25.0f, SENSOR_OK, 3100));
  assert(sensors.setMode(1, SENSOR_MODE_LOG_ONLY));
  assert(sensors.monitoringEnabled(1));
  assert(!sensors.controlEnabled(1));
  assert(!sensors.safetyEnabled(1));
  assert(sensors.setMode(2, SENSOR_MODE_LOG_CONTROL));
  assert(sensors.monitoringEnabled(2));
  assert(sensors.controlEnabled(2));
  assert(sensors.safetyEnabled(2));
  assert(sensors.setMode(3, SENSOR_MODE_LOG_SAFETY));
  assert(sensors.monitoringEnabled(3));
  assert(!sensors.controlEnabled(3));
  assert(sensors.safetyEnabled(3));
  assert(sensors.monitoringMask() == 0x0E);
  assert(sensors.controlEnabledMask() == 0x04);
  assert(sensors.safetyEnabledMask() == 0x0C);
  return 0;
}
