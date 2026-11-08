#include "kiln_features.h"
#include <math.h>
#include <string.h>

namespace kiln {
namespace features {

TelemetryBuffer::TelemetryBuffer()
    : head_(0), count_(0), lastSampleMs_(0), hasSample_(false) {
  clear();
}

void TelemetryBuffer::clear() {
  head_ = 0;
  count_ = 0;
  lastSampleMs_ = 0;
  hasSample_ = false;
}

bool TelemetryBuffer::push(const TelemetrySample& sample, uint32_t interval) {
  if (hasSample_ &&
      static_cast<uint32_t>(sample.elapsedMs - lastSampleMs_) < interval) {
    return false;
  }
  samples_[head_] = sample;
  head_ = static_cast<uint16_t>((head_ + 1) % TELEMETRY_CAPACITY);
  if (count_ < TELEMETRY_CAPACITY) ++count_;
  lastSampleMs_ = sample.elapsedMs;
  hasSample_ = true;
  return true;
}

bool TelemetryBuffer::getOldestFirst(uint16_t index, TelemetrySample& sample) const {
  if (index >= count_) return false;
  const uint16_t start = static_cast<uint16_t>(
      (head_ + TELEMETRY_CAPACITY - count_) % TELEMETRY_CAPACITY);
  sample = samples_[(start + index) % TELEMETRY_CAPACITY];
  return true;
}

bool TelemetryBuffer::newest(TelemetrySample& sample) const {
  if (count_ == 0) return false;
  sample = samples_[(head_ + TELEMETRY_CAPACITY - 1) % TELEMETRY_CAPACITY];
  return true;
}

FiringHistory::FiringHistory() : head_(0), count_(0) {
  clear();
}

void FiringHistory::clear() {
  memset(records_, 0, sizeof(records_));
  head_ = 0;
  count_ = 0;
}

void FiringHistory::add(const FiringRecord& record) {
  records_[head_] = record;
  head_ = static_cast<uint8_t>((head_ + 1) % FIRING_HISTORY_CAPACITY);
  if (count_ < FIRING_HISTORY_CAPACITY) ++count_;
}

bool FiringHistory::newestFirst(uint8_t index, FiringRecord& record) const {
  if (index >= count_) return false;
  const uint8_t position = static_cast<uint8_t>(
      (head_ + FIRING_HISTORY_CAPACITY - 1 - index) % FIRING_HISTORY_CAPACITY);
  record = records_[position];
  return true;
}

void FiringHistory::exportRaw(
    FiringRecord (&records)[FIRING_HISTORY_CAPACITY], uint8_t& count) const {
  count = count_;
  for (uint8_t i = 0; i < count_; ++i) {
    newestFirst(static_cast<uint8_t>(count_ - 1 - i), records[i]);
  }
}

void FiringHistory::importRaw(const FiringRecord* records, uint8_t count) {
  clear();
  if (!records) return;
  if (count > FIRING_HISTORY_CAPACITY) count = FIRING_HISTORY_CAPACITY;
  for (uint8_t i = 0; i < count; ++i) add(records[i]);
}

EnergyCostTracker::EnergyCostTracker()
    : wattMilliseconds_(0), lastUpdateMs_(0), previousOutputMask_(0),
      previousFresh_(false), started_(false) {
  memset(&config_, 0, sizeof(config_));
}

void EnergyCostTracker::configure(const EnergyConfig& config) {
  config_ = config;
}

void EnergyCostTracker::reset(uint32_t nowMs) {
  wattMilliseconds_ = 0;
  lastUpdateMs_ = nowMs;
  previousOutputMask_ = 0;
  previousFresh_ = false;
  started_ = true;
}

bool EnergyCostTracker::configured() const {
  if (config_.rateMinorPerKWh == 0) return false;
  for (uint8_t i = 0; i < 4; ++i) {
    if (config_.zoneWatts[i] != 0) return true;
  }
  return false;
}

void EnergyCostTracker::update(uint32_t nowMs, uint8_t outputMask,
                               bool statusFresh) {
  if (!started_) {
    lastUpdateMs_ = nowMs;
    previousOutputMask_ = outputMask;
    previousFresh_ = statusFresh;
    started_ = true;
    return;
  }

  const uint32_t elapsed = static_cast<uint32_t>(nowMs - lastUpdateMs_);
  // Ignore long intervals and stale telemetry rather than inventing energy use.
  if (previousFresh_ && statusFresh && elapsed <= 2500UL) {
    uint32_t watts = 0;
    for (uint8_t zone = 0; zone < 4; ++zone) {
      if (previousOutputMask_ & (1U << zone)) watts += config_.zoneWatts[zone];
    }
    wattMilliseconds_ += static_cast<uint64_t>(watts) * elapsed;
  }
  lastUpdateMs_ = nowMs;
  previousOutputMask_ = outputMask;
  previousFresh_ = statusFresh;
}

double EnergyCostTracker::energyKWh() const {
  return static_cast<double>(wattMilliseconds_) / 3600000000.0;
}

uint32_t EnergyCostTracker::estimatedCostMinor() const {
  if (config_.rateMinorPerKWh == 0) return 0;
  const double cost = energyKWh() *
                      static_cast<double>(config_.rateMinorPerKWh);
  if (cost >= 4294967295.0) return 0xFFFFFFFFUL;
  return static_cast<uint32_t>(cost + 0.5);
}

uint64_t EnergyCostTracker::energyMilliWh() const {
  return wattMilliseconds_ / 3600ULL;
}

ScheduleQueue::ScheduleQueue() : nextId_(1) {
  memset(entries_, 0, sizeof(entries_));
}

bool ScheduleQueue::add(uint32_t startEpoch, const char* profileId,
                        uint32_t& assignedId) {
  if (!profileId || !profileId[0] || startEpoch == 0) return false;
  const size_t length = strlen(profileId);
  if (length >= PROFILE_ID_LENGTH) return false;

  int8_t freeSlot = -1;
  for (uint8_t i = 0; i < SCHEDULE_CAPACITY; ++i) {
    if (entries_[i].state == SCHEDULE_EMPTY) {
      freeSlot = static_cast<int8_t>(i);
      break;
    }
  }
  if (freeSlot < 0) return false;

  ScheduledFiring& entry = entries_[static_cast<uint8_t>(freeSlot)];
  memset(&entry, 0, sizeof(entry));
  entry.id = nextId_++;
  if (nextId_ == 0) nextId_ = 1;
  entry.startEpoch = startEpoch;
  memcpy(entry.profileId, profileId, length);
  entry.profileId[length] = '\0';
  entry.state = SCHEDULE_PENDING;
  assignedId = entry.id;
  return true;
}

bool ScheduleQueue::cancel(uint32_t id) {
  for (uint8_t i = 0; i < SCHEDULE_CAPACITY; ++i) {
    if (entries_[i].state != SCHEDULE_EMPTY && entries_[i].id == id) {
      memset(&entries_[i], 0, sizeof(entries_[i]));
      return true;
    }
  }
  return false;
}

uint8_t ScheduleQueue::markDue(uint32_t nowEpoch) {
  uint8_t dueCount = 0;
  if (nowEpoch == 0) return 0;
  for (uint8_t i = 0; i < SCHEDULE_CAPACITY; ++i) {
    if (entries_[i].state == SCHEDULE_PENDING &&
        entries_[i].startEpoch <= nowEpoch) {
      entries_[i].state = SCHEDULE_DUE;
      ++dueCount;
    }
  }
  return dueCount;
}

uint8_t ScheduleQueue::size() const {
  uint8_t result = 0;
  for (uint8_t i = 0; i < SCHEDULE_CAPACITY; ++i) {
    if (entries_[i].state != SCHEDULE_EMPTY) ++result;
  }
  return result;
}

bool ScheduleQueue::get(uint8_t index, ScheduledFiring& entry) const {
  if (index >= SCHEDULE_CAPACITY ||
      entries_[index].state == SCHEDULE_EMPTY) return false;
  entry = entries_[index];
  return true;
}

void ScheduleQueue::exportRaw(
    ScheduledFiring (&entries)[SCHEDULE_CAPACITY], uint8_t& count) const {
  count = 0;
  for (uint8_t i = 0; i < SCHEDULE_CAPACITY; ++i) {
    if (entries_[i].state != SCHEDULE_EMPTY) entries[count++] = entries_[i];
  }
}

void ScheduleQueue::importRaw(const ScheduledFiring* entries, uint8_t count) {
  memset(entries_, 0, sizeof(entries_));
  nextId_ = 1;
  if (!entries) return;
  if (count > SCHEDULE_CAPACITY) count = SCHEDULE_CAPACITY;
  for (uint8_t i = 0; i < count; ++i) {
    entries_[i] = entries[i];
    if (entries_[i].id >= nextId_) nextId_ = entries_[i].id + 1;
  }
  if (nextId_ == 0) nextId_ = 1;
}

AlertLedger::AlertLedger() : head_(0), count_(0), nextId_(1) {
  clear();
}

void AlertLedger::clear() {
  memset(events_, 0, sizeof(events_));
  head_ = 0;
  count_ = 0;
  nextId_ = 1;
}

int8_t AlertLedger::findActive(AlertCode code) const {
  for (uint8_t i = 0; i < count_; ++i) {
    const uint8_t position = static_cast<uint8_t>(
        (head_ + ALERT_CAPACITY - count_ + i) % ALERT_CAPACITY);
    if (events_[position].code == code && events_[position].active) {
      return static_cast<int8_t>(position);
    }
  }
  return -1;
}

void AlertLedger::append(AlertCode code, AlertSeverity severity, bool active,
                         uint32_t elapsedMs, int16_t valueDeciC) {
  AlertEvent& event = events_[head_];
  event = {nextId_++, elapsedMs, code, severity, active, false, valueDeciC};
  if (nextId_ == 0) nextId_ = 1;
  head_ = static_cast<uint8_t>((head_ + 1) % ALERT_CAPACITY);
  if (count_ < ALERT_CAPACITY) ++count_;
}

void AlertLedger::setCondition(AlertCode code, AlertSeverity severity,
                               bool active, uint32_t elapsedMs,
                               int16_t valueDeciC) {
  if (code == ALERT_NONE) return;
  const int8_t index = findActive(code);
  if (active) {
    if (index < 0) {
      append(code, severity, true, elapsedMs, valueDeciC);
    } else {
      events_[static_cast<uint8_t>(index)].valueDeciC = valueDeciC;
    }
  } else if (index >= 0) {
    AlertEvent& event = events_[static_cast<uint8_t>(index)];
    event.active = false;
    event.elapsedMs = elapsedMs;
    event.valueDeciC = valueDeciC;
  }
}

bool AlertLedger::acknowledge(uint32_t id) {
  for (uint8_t i = 0; i < count_; ++i) {
    const uint8_t position = static_cast<uint8_t>(
        (head_ + ALERT_CAPACITY - 1 - i) % ALERT_CAPACITY);
    if (events_[position].id == id) {
      events_[position].acknowledged = true;
      return true;
    }
  }
  return false;
}

bool AlertLedger::newestFirst(uint8_t index, AlertEvent& event) const {
  if (index >= count_) return false;
  const uint8_t position = static_cast<uint8_t>(
      (head_ + ALERT_CAPACITY - 1 - index) % ALERT_CAPACITY);
  event = events_[position];
  return true;
}

uint16_t AlertLedger::activeCount() const {
  uint16_t count = 0;
  for (uint8_t i = 0; i < count_; ++i) {
    const uint8_t position = static_cast<uint8_t>(
        (head_ + ALERT_CAPACITY - count_ + i) % ALERT_CAPACITY);
    if (events_[position].active) ++count;
  }
  return count;
}

KilnSimulator::KilnSimulator() : lastUpdateMs_(0), active_(false) {
  memset(&config_, 0, sizeof(config_));
  memset(temperaturesC_, 0, sizeof(temperaturesC_));
}

bool KilnSimulator::begin(const SimulationConfig& config, uint32_t nowMs) {
  if (!isfinite(config.ambientC) ||
      !isfinite(config.initialTemperatureC) ||
      !isfinite(config.maximumHeatRateCPerHour) ||
      !isfinite(config.coolingRateCPerHour) ||
      config.ambientC < -50.0f ||
      config.initialTemperatureC < config.ambientC ||
      config.maximumHeatRateCPerHour <= 0.0f ||
      config.maximumHeatRateCPerHour > 600.0f ||
      config.coolingRateCPerHour < 0.0f ||
      config.coolingRateCPerHour > 300.0f) return false;

  config_ = config;
  for (uint8_t i = 0; i < 4; ++i) {
    temperaturesC_[i] = config.initialTemperatureC;
  }
  lastUpdateMs_ = nowMs;
  active_ = true;
  return true;
}

bool KilnSimulator::update(uint32_t nowMs, uint8_t outputMask) {
  if (!active_) return false;
  const uint32_t elapsed = static_cast<uint32_t>(nowMs - lastUpdateMs_);
  if (elapsed > 60000UL) {
    lastUpdateMs_ = nowMs;
    return false;
  }

  const float hours = static_cast<float>(elapsed) / 3600000.0f;
  for (uint8_t zone = 0; zone < 4; ++zone) {
    const bool heating = (outputMask & (1U << zone)) != 0;
    const float delta = heating
        ? config_.maximumHeatRateCPerHour * hours
        : -config_.coolingRateCPerHour * hours;
    temperaturesC_[zone] += delta;
    if (temperaturesC_[zone] < config_.ambientC) {
      temperaturesC_[zone] = config_.ambientC;
    }
    if (temperaturesC_[zone] > 1800.0f) temperaturesC_[zone] = 1800.0f;
  }
  lastUpdateMs_ = nowMs;
  return true;
}

void KilnSimulator::stop() {
  active_ = false;
}

float KilnSimulator::temperatureC(uint8_t zone) const {
  return zone < 4 ? temperaturesC_[zone] : 0.0f;
}

PidDiagnostics::PidDiagnostics()
    : integral_(0.0f), previousMeasurement_(0.0f), hasPrevious_(false) {
  config_ = {0.0f, 0.0f, 0.0f, 0.0f, 100.0f};
}

void PidDiagnostics::configure(const PidConfig& config) {
  if (!isfinite(config.kp) || !isfinite(config.ki) ||
      !isfinite(config.kd) || !isfinite(config.outputMinimum) ||
      !isfinite(config.outputMaximum) || config.kp < 0.0f ||
      config.ki < 0.0f || config.kd < 0.0f ||
      config.outputMaximum <= config.outputMinimum) return;
  config_ = config;
  reset();
}

void PidDiagnostics::reset() {
  integral_ = 0.0f;
  previousMeasurement_ = 0.0f;
  hasPrevious_ = false;
}

PidTerms PidDiagnostics::update(float setpoint, float measured,
                                float dtSeconds) {
  PidTerms terms = {0.0f, 0.0f, 0.0f, 0.0f, config_.outputMinimum};
  if (!isfinite(setpoint) || !isfinite(measured) ||
      !isfinite(dtSeconds) || dtSeconds <= 0.0f || dtSeconds > 60.0f) {
    return terms;
  }

  terms.error = setpoint - measured;
  terms.p = config_.kp * terms.error;
  const float candidateIntegral = integral_ + terms.error * dtSeconds;
  const float candidateI = config_.ki * candidateIntegral;
  const float derivative = hasPrevious_
      ? -config_.kd * (measured - previousMeasurement_) / dtSeconds
      : 0.0f;
  const float raw = terms.p + candidateI + derivative;
  const float clipped = raw < config_.outputMinimum ? config_.outputMinimum
      : raw > config_.outputMaximum ? config_.outputMaximum : raw;
  if (raw == clipped ||
      (raw > config_.outputMaximum && terms.error < 0.0f) ||
      (raw < config_.outputMinimum && terms.error > 0.0f)) {
    integral_ = candidateIntegral;
  }

  terms.i = config_.ki * integral_;
  terms.d = derivative;
  const float output = terms.p + terms.i + terms.d;
  terms.output = output < config_.outputMinimum ? config_.outputMinimum
      : output > config_.outputMaximum ? config_.outputMaximum : output;
  previousMeasurement_ = measured;
  hasPrevious_ = true;
  return terms;
}

PidRecommendation PidTuningAdvisor::fromStepResponse(
    const PidStepResponse& response) {
  PidRecommendation result = {false, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
  const float deltaOutput = response.finalOutputPercent -
                            response.initialOutputPercent;
  const float deltaTemperature = response.finalTemperatureC -
                                 response.initialTemperatureC;
  if (!isfinite(deltaOutput) || !isfinite(deltaTemperature) ||
      !isfinite(response.deadTimeSeconds) ||
      !isfinite(response.timeTo63PercentSeconds) ||
      fabsf(deltaOutput) < 1.0f || fabsf(deltaTemperature) < 0.1f ||
      response.deadTimeSeconds < 1.0f ||
      response.timeTo63PercentSeconds <= response.deadTimeSeconds ||
      response.timeTo63PercentSeconds > 86400.0f) return result;

  const float processGain = deltaTemperature / (deltaOutput / 100.0f);
  if (!isfinite(processGain) || fabsf(processGain) < 0.0001f) return result;
  const float deadTime = response.deadTimeSeconds;
  const float timeConstant = response.timeTo63PercentSeconds - deadTime;
  // Conservative open-loop Ziegler-Nichols PID estimate; review before use.
  const float kp = 1.2f * timeConstant /
                   (fabsf(processGain) * deadTime) * 100.0f;
  const float integralTime = 2.0f * deadTime;
  const float derivativeTime = 0.5f * deadTime;
  if (!isfinite(kp) || kp <= 0.0f || kp > 100000.0f) return result;

  result.valid = true;
  result.processGainCPerFraction = processGain;
  result.kp = kp;
  result.integralTimeSeconds = integralTime;
  result.derivativeTimeSeconds = derivativeTime;
  result.ki = kp / integralTime;
  result.kd = kp * derivativeTime;
  return result;
}

}  // namespace features
}  // namespace kiln
