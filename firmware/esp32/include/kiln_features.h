#pragma once
#include <stdint.h>

namespace kiln {
namespace features {

static constexpr uint16_t TELEMETRY_CAPACITY = 240;
static constexpr uint8_t FIRING_HISTORY_CAPACITY = 12;
static constexpr uint8_t SCHEDULE_CAPACITY = 8;
static constexpr uint8_t ALERT_CAPACITY = 24;
static constexpr uint8_t PROFILE_ID_LENGTH = 24;

struct TelemetrySample {
  uint32_t elapsedMs;
  int16_t temperatureDeciC[4];
  int16_t setpointDeciC[4];
  uint8_t validMask;
  uint8_t outputMask;
  uint8_t state;
};

class TelemetryBuffer {
 public:
  TelemetryBuffer();
  void clear();
  bool push(const TelemetrySample& sample, uint32_t minimumIntervalMs = 1000);
  uint16_t size() const { return count_; }
  bool getOldestFirst(uint16_t index, TelemetrySample& sample) const;
  bool newest(TelemetrySample& sample) const;
 private:
  TelemetrySample samples_[TELEMETRY_CAPACITY];
  uint16_t head_;
  uint16_t count_;
  uint32_t lastSampleMs_;
  bool hasSample_;
};

struct FiringRecord {
  uint32_t id;
  uint32_t startedEpoch;
  uint32_t durationSeconds;
  char profileId[PROFILE_ID_LENGTH];
  int16_t peakTemperatureDeciC[4];
  uint64_t energyMilliWh;
  uint32_t estimatedCostMinor;
  uint8_t finalState;
};

class FiringHistory {
 public:
  FiringHistory();
  void clear();
  void add(const FiringRecord& record);
  uint8_t size() const { return count_; }
  bool newestFirst(uint8_t index, FiringRecord& record) const;
  void exportRaw(FiringRecord (&records)[FIRING_HISTORY_CAPACITY], uint8_t& count) const;
  void importRaw(const FiringRecord* records, uint8_t count);
 private:
  FiringRecord records_[FIRING_HISTORY_CAPACITY];
  uint8_t head_;
  uint8_t count_;
};

struct EnergyConfig {
  uint16_t zoneWatts[4];
  uint32_t rateMinorPerKWh;
};

class EnergyCostTracker {
 public:
  EnergyCostTracker();
  void configure(const EnergyConfig& config);
  void reset(uint32_t nowMs = 0);
  void update(uint32_t nowMs, uint8_t outputMask, bool statusFresh);
  double energyKWh() const;
  uint32_t estimatedCostMinor() const;
  uint64_t energyMilliWh() const;
  bool configured() const;
 private:
  EnergyConfig config_;
  uint64_t wattMilliseconds_;
  uint32_t lastUpdateMs_;
  uint8_t previousOutputMask_;
  bool previousFresh_;
  bool started_;
};

enum ScheduleState : uint8_t {
  SCHEDULE_EMPTY = 0,
  SCHEDULE_PENDING = 1,
  SCHEDULE_DUE = 2
};

struct ScheduledFiring {
  uint32_t id;
  uint32_t startEpoch;
  char profileId[PROFILE_ID_LENGTH];
  ScheduleState state;
};

class ScheduleQueue {
 public:
  ScheduleQueue();
  bool add(uint32_t startEpoch, const char* profileId, uint32_t& assignedId);
  bool cancel(uint32_t id);
  uint8_t markDue(uint32_t nowEpoch);
  uint8_t size() const;
  bool get(uint8_t index, ScheduledFiring& entry) const;
  void exportRaw(ScheduledFiring (&entries)[SCHEDULE_CAPACITY], uint8_t& count) const;
  void importRaw(const ScheduledFiring* entries, uint8_t count);
 private:
  ScheduledFiring entries_[SCHEDULE_CAPACITY];
  uint32_t nextId_;
};

enum AlertSeverity : uint8_t {
  ALERT_INFO = 0,
  ALERT_WARNING = 1,
  ALERT_CRITICAL = 2
};

enum AlertCode : uint8_t {
  ALERT_NONE = 0,
  ALERT_CAN_LOST = 1,
  ALERT_STATUS_STALE = 2,
  ALERT_CONTROLLER_FAULT = 3,
  ALERT_SENSOR_INVALID = 4,
  ALERT_OVERTEMP = 5,
  ALERT_RUN_STARTED = 6,
  ALERT_RUN_COMPLETED = 7,
  ALERT_RUN_STOPPED = 8,
  ALERT_SCHEDULE_DUE = 9
};

struct AlertEvent {
  uint32_t id;
  uint32_t elapsedMs;
  AlertCode code;
  AlertSeverity severity;
  bool active;
  bool acknowledged;
  int16_t valueDeciC;
};

class AlertLedger {
 public:
  AlertLedger();
  void clear();
  void setCondition(AlertCode code, AlertSeverity severity, bool active,
                    uint32_t elapsedMs, int16_t valueDeciC = 0);
  bool acknowledge(uint32_t id);
  uint8_t size() const { return count_; }
  bool newestFirst(uint8_t index, AlertEvent& event) const;
  uint16_t activeCount() const;
 private:
  AlertEvent events_[ALERT_CAPACITY];
  uint8_t head_;
  uint8_t count_;
  uint32_t nextId_;
  int8_t findActive(AlertCode code) const;
  void append(AlertCode code, AlertSeverity severity, bool active,
              uint32_t elapsedMs, int16_t valueDeciC);
};

struct SimulationConfig {
  float ambientC;
  float initialTemperatureC;
  float maximumHeatRateCPerHour;
  float coolingRateCPerHour;
};

class KilnSimulator {
 public:
  KilnSimulator();
  bool begin(const SimulationConfig& config, uint32_t nowMs);
  bool update(uint32_t nowMs, uint8_t simulatedOutputMask);
  void stop();
  bool active() const { return active_; }
  float temperatureC(uint8_t zone) const;
 private:
  SimulationConfig config_;
  float temperaturesC_[4];
  uint32_t lastUpdateMs_;
  bool active_;
};

struct PidConfig {
  float kp;
  float ki;
  float kd;
  float outputMinimum;
  float outputMaximum;
};

struct PidTerms {
  float error;
  float p;
  float i;
  float d;
  float output;
};

class PidDiagnostics {
 public:
  PidDiagnostics();
  void configure(const PidConfig& config);
  void reset();
  PidTerms update(float setpointC, float measuredC, float dtSeconds);
 private:
  PidConfig config_;
  float integral_;
  float previousMeasurement_;
  bool hasPrevious_;
};

struct PidStepResponse {
  float initialOutputPercent;
  float finalOutputPercent;
  float initialTemperatureC;
  float finalTemperatureC;
  float deadTimeSeconds;
  float timeTo63PercentSeconds;
};

struct PidRecommendation {
  bool valid;
  float processGainCPerFraction;
  float kp;
  float ki;
  float kd;
  float integralTimeSeconds;
  float derivativeTimeSeconds;
};

class PidTuningAdvisor {
 public:
  // Offline calculation from recorded process response; it never changes heater outputs.
  static PidRecommendation fromStepResponse(const PidStepResponse& response);
};

}  // namespace features
}  // namespace kiln
