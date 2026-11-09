#include "kiln_features.h"
#include <assert.h>
#include <stdio.h>
using namespace kiln::features;

static void testTelemetry() {
  TelemetryBuffer buffer;
  TelemetrySample sample = {0, {100,200,300,400}, {150,250,350,450}, 0x0F, 0, 2};
  assert(buffer.push(sample, 1000));
  assert(!buffer.push(sample, 1000));
  for (uint16_t i = 1; i < TELEMETRY_CAPACITY + 5; ++i) {
    sample.elapsedMs = static_cast<uint32_t>(i) * 1000;
    buffer.push(sample, 1000);
  }
  assert(buffer.size() == TELEMETRY_CAPACITY);
  TelemetrySample first, last;
  assert(buffer.getOldestFirst(0, first));
  assert(buffer.newest(last));
  assert(first.elapsedMs == 5000);
  assert(last.elapsedMs == (TELEMETRY_CAPACITY + 4) * 1000UL);
  assert(!buffer.getOldestFirst(TELEMETRY_CAPACITY, last));
}

static void testHistory() {
  FiringHistory history;
  FiringRecord record = {1, 100, 200, "cone-6", {1200,1300,1100,1000}, 1234, 56, 6};
  for (uint32_t i = 1; i <= FIRING_HISTORY_CAPACITY + 2; ++i) {
    record.id = i;
    history.add(record);
  }
  FiringRecord newest;
  assert(history.size() == FIRING_HISTORY_CAPACITY);
  assert(history.newestFirst(0, newest));
  assert(newest.id == FIRING_HISTORY_CAPACITY + 2);
  FiringRecord raw[FIRING_HISTORY_CAPACITY];
  uint8_t count = 0;
  history.exportRaw(raw, count);
  FiringHistory restored;
  restored.importRaw(raw, count);
  assert(restored.newestFirst(0, newest));
  assert(newest.id == FIRING_HISTORY_CAPACITY + 2);
}

static void testEnergyEstimate() {
  EnergyCostTracker energy;
  EnergyConfig config = {{1000, 500, 0, 0}, 150};
  energy.configure(config);
  energy.reset(0);
  energy.update(1000, 0x01, true);
  for (uint32_t second = 2; second <= 3601; ++second) {
    energy.update(second * 1000UL, 0x01, true);
  }
  assert(energy.energyKWh() > 0.99 && energy.energyKWh() < 1.01);
  assert(energy.estimatedCostMinor() >= 149);
  assert(energy.estimatedCostMinor() <= 151);
  energy.update(3602000, 0x01, false);
  const double before = energy.energyKWh();
  energy.update(3603000, 0x01, true);
  assert(energy.energyKWh() == before);
}

static void testScheduleRemindersDoNotStart() {
  ScheduleQueue queue;
  uint32_t id = 0;
  assert(queue.add(2000, "cone-6", id));
  assert(id != 0);
  assert(queue.markDue(1999) == 0);
  assert(queue.markDue(2000) == 1);
  assert(queue.markDue(3000) == 0);
  ScheduledFiring entry;
  bool found = false;
  for (uint8_t i = 0; i < SCHEDULE_CAPACITY; ++i) {
    if (queue.get(i, entry)) found = true;
  }
  assert(found && entry.state == SCHEDULE_DUE);
  assert(queue.cancel(id));
  assert(queue.size() == 0);
  assert(!queue.add(1000, "", id));
}

static void testAlerts() {
  AlertLedger alerts;
  alerts.setCondition(ALERT_CAN_LOST, ALERT_CRITICAL, true, 10);
  alerts.setCondition(ALERT_CAN_LOST, ALERT_CRITICAL, true, 20);
  assert(alerts.size() == 1 && alerts.activeCount() == 1);
  AlertEvent event;
  assert(alerts.newestFirst(0, event));
  assert(event.active && !event.acknowledged);
  assert(alerts.acknowledge(event.id));
  assert(alerts.newestFirst(0, event) && event.acknowledged);
  alerts.setCondition(ALERT_CAN_LOST, ALERT_CRITICAL, false, 30);
  assert(alerts.activeCount() == 0 && alerts.size() == 1);
}

static void testSimulationIsIndependent() {
  KilnSimulator simulator;
  SimulationConfig config = {20.0f, 20.0f, 100.0f, 20.0f};
  assert(simulator.begin(config, 0));
  for (uint32_t second = 1; second <= 60; ++second) {
    assert(simulator.update(second * 60000UL, 0x01));
  }
  assert(simulator.temperatureC(0) > 119.0f);
  assert(simulator.temperatureC(0) < 121.0f);
  assert(simulator.temperatureC(1) < 20.1f);
  simulator.stop();
  assert(!simulator.update(7200000, 0x0F));
  SimulationConfig invalid = {20.0f, 10.0f, 100.0f, 20.0f};
  assert(!simulator.begin(invalid, 0));
}

static void testPidDiagnosticsAndOfflineAdvisor() {
  PidDiagnostics pid;
  PidConfig config = {2.0f, 0.1f, 0.5f, 0.0f, 100.0f};
  pid.configure(config);
  const PidTerms terms = pid.update(200.0f, 190.0f, 1.0f);
  assert(terms.error == 10.0f);
  assert(terms.p > 0.0f && terms.output > 0.0f && terms.output <= 100.0f);
  PidStepResponse response = {20.0f, 40.0f, 100.0f, 150.0f, 30.0f, 330.0f};
  const PidRecommendation recommendation =
      PidTuningAdvisor::fromStepResponse(response);
  assert(recommendation.valid && recommendation.kp > 0.0f);
  assert(recommendation.ki > 0.0f && recommendation.kd > 0.0f);
  response.finalOutputPercent = response.initialOutputPercent;
  assert(!PidTuningAdvisor::fromStepResponse(response).valid);
}

int main() {
  testTelemetry();
  testHistory();
  testEnergyEstimate();
  testScheduleRemindersDoNotStart();
  testAlerts();
  testSimulationIsIndependent();
  testPidDiagnosticsAndOfflineAdvisor();
  puts("Kiln feature tests passed.");
}
