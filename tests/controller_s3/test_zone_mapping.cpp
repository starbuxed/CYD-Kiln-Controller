#include <assert.h>
#include <math.h>
#include "zone_mapping.h"

int main() {
  using namespace kiln;
  ZoneMapping map;
  ZoneMappingConfig cfg[MAPPING_ZONE_COUNT] = {
    {true, 0x03, 0x01, SENSOR_COMBINE_HIGHEST, 0},
    {true, 0x0C, 0x0E, SENSOR_COMBINE_AVERAGE, 0},
    {false, 0, 0, SENSOR_COMBINE_DESIGNATED, 0},
    {false, 0, 0, SENSOR_COMBINE_DESIGNATED, 0}
  };
  assert(map.validate(cfg));
  assert(map.configureAll(cfg));
  assert(map.requiredSensorMask() == 0x0F);
  assert(map.enabledZoneMask() == 0x03);
  assert(map.assignedOutputMask() == 0x0F);
  assert(map.outputMaskForZone(0) == 0x01);
  assert(map.outputMaskForZone(1) == 0x0E);
  assert(map.zoneMaskForSensor(0) == 0x01);
  assert(map.zoneMaskForSensor(1) == 0x01);
  assert(map.zoneMaskForSensor(2) == 0x02);

  const float temps[4] = {900.0f, 920.0f, 940.0f, 960.0f};
  ZoneMeasurement z0 = map.measure(0, temps, 0x0F);
  assert(z0.valid && fabsf(z0.temperatureC - 920.0f) < 0.01f);
  assert(z0.contributingSensorMask == 0x03);
  ZoneMeasurement z1 = map.measure(1, temps, 0x0F);
  assert(z1.valid && fabsf(z1.temperatureC - 950.0f) < 0.01f);
  assert(!map.measure(1, temps, 0x07).valid); // no silent omission

  ZoneMappingConfig overlap[MAPPING_ZONE_COUNT] = {
    {true, 0x01, 0x01, SENSOR_COMBINE_DESIGNATED, 0},
    {true, 0x02, 0x01, SENSOR_COMBINE_DESIGNATED, 1},
    {false, 0, 0, SENSOR_COMBINE_DESIGNATED, 0},
    {false, 0, 0, SENSOR_COMBINE_DESIGNATED, 0}
  };
  assert(!map.validate(overlap)); // one relay cannot belong to two zones

  ZoneMappingConfig singleCircuit[MAPPING_ZONE_COUNT] = {
    {true, 0x0F, 0x01, SENSOR_COMBINE_HIGHEST, 0},
    {false, 0, 0, SENSOR_COMBINE_DESIGNATED, 0},
    {false, 0, 0, SENSOR_COMBINE_DESIGNATED, 0},
    {false, 0, 0, SENSOR_COMBINE_DESIGNATED, 0}
  };
  assert(map.configureAll(singleCircuit));
  ZoneMeasurement hottest = map.measure(0, temps, 0x0F);
  assert(hottest.valid && hottest.temperatureC == 960.0f);
  assert(!map.measure(0, temps, 0x07).valid);

  ZoneMappingConfig oneSensorFourOutputs[MAPPING_ZONE_COUNT] = {
    {true, 0x01, 0x0F, SENSOR_COMBINE_DESIGNATED, 0},
    {false, 0, 0, SENSOR_COMBINE_DESIGNATED, 0},
    {false, 0, 0, SENSOR_COMBINE_DESIGNATED, 0},
    {false, 0, 0, SENSOR_COMBINE_DESIGNATED, 0}
  };
  assert(map.configureAll(oneSensorFourOutputs));
  assert(map.requiredSensorMask() == 0x01);
  assert(map.assignedOutputMask() == 0x0F);
  assert(map.measure(0, temps, 0x01).valid);

  ZoneMappingConfig invalidDesignated[MAPPING_ZONE_COUNT] = {
    {true, 0x02, 0x01, SENSOR_COMBINE_DESIGNATED, 0},
    {false, 0, 0, SENSOR_COMBINE_DESIGNATED, 0},
    {false, 0, 0, SENSOR_COMBINE_DESIGNATED, 0},
    {false, 0, 0, SENSOR_COMBINE_DESIGNATED, 0}
  };
  assert(!map.validate(invalidDesignated));
  return 0;
}
