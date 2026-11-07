#include <assert.h>
#include <math.h>
#include "profile_engine.h"
#include "zone_controller.h"
#include "safety_manager.h"
#include "kiln_control_core.h"

int main() {
  using namespace kiln;
  RampSoakSegment profile[] = {
    {100.0f, 100.0f, 2},
    {200.0f, 50.0f, 0}
  };
  ProfileEngine engine;
  assert(engine.validate(profile, 2));
  assert(!engine.validate(profile, 0));
  assert(!engine.validate(profile, 33));
  RampSoakSegment bad = {200.0f, 0.0f, 0};
  assert(!engine.validate(&bad, 1));
  RampSoakSegment soakOnly[] = {{100.0f, 100.0f, 0}, {100.0f, 0.0f, 600}};
  assert(engine.validate(soakOnly, 2));
  assert(engine.start(profile, 2, 0.0f, 0));
  engine.update(1800000UL);
  assert(engine.state() == PROFILE_RAMPING);
  assert(fabsf(engine.setpointC() - 50.0f) < 0.1f);
  engine.pause(1800000UL);
  assert(engine.state() == PROFILE_PAUSED);
  assert(engine.resume(2800000UL));
  engine.update(4600000UL);
  assert(engine.state() == PROFILE_WAITING);
  assert(engine.beginSoak(4600000UL));
  engine.update(4602000UL);
  assert(engine.segmentIndex() == 1);

  ZoneController zone;
  ZoneConfig zc = {4.0f, 1000UL, 1000UL};
  assert(zone.configure(zc));
  assert(!zone.update(0, 20.0f, 100.0f, false, true));
  assert(!zone.update(0, 20.0f, 100.0f, true, true));
  assert(zone.update(1000, 20.0f, 100.0f, true, true));
  assert(zone.update(1500, 102.0f, 100.0f, true, true));
  assert(!zone.update(2000, 102.0f, 100.0f, true, true));

  SafetyManager safety;
  float temps[4] = {100, 101, 99, 100};
  safety.update(0, 0x0F, temps, true, true, 1200.0f);
  assert(!safety.faulted());
  safety.update(1, 0x07, temps, true, true, 1200.0f);
  safety.update(1502, 0x07, temps, true, true, 1200.0f);
  assert(safety.faults() & FAULT_SENSOR_LOSS);
  assert(!safety.acknowledge(0x07, true, temps, 1200.0f));
  assert(safety.acknowledge(0x0F, true, temps, 1200.0f));
  assert(!safety.faulted());

  KilnControlCore core;
  assert(core.configureProfile(profile, 2));
  for (uint8_t i = 0; i < 4; ++i) {
    ZoneRuntimeConfig cfg = {{5.0f, 1000UL, 1000UL}, 0.0f};
    assert(core.configureZone(i, cfg));
  }
  assert(core.start(0, temps, 0x0F, true));
  core.update(1000, temps, 0x0F, false);
  assert(core.faulted());
  assert(core.outputMask() == 0);

  KilnControlCore soakCore;
  RampSoakSegment soakProfile[] = {{100.0f, 100.0f, 600}};
  assert(soakCore.configureProfile(soakProfile, 1));
  for (uint8_t i = 0; i < 4; ++i) {
    ZoneRuntimeConfig cfg = {{5.0f, 1000UL, 1000UL}, 0.0f};
    assert(soakCore.configureZone(i, cfg));
  }
  float coldTemps[4] = {20, 20, 20, 20};
  assert(soakCore.start(0, coldTemps, 0x0F, true));
  float nearTemps[4] = {90, 90, 90, 80};
  soakCore.update(3000000UL, nearTemps, 0x0F, true);
  assert(soakCore.profileState() == PROFILE_WAITING);
  float targetTemps[4] = {100, 100, 100, 100};
  soakCore.update(3100000UL, targetTemps, 0x0F, true);
  assert(soakCore.profileState() == PROFILE_SOAKING);
  soakCore.update(3700000UL, targetTemps, 0x0F, true);
  assert(soakCore.profileState() == PROFILE_COMPLETE);
  assert(!soakCore.running());
  assert(soakCore.outputMask() == 0);
  return 0;
}
