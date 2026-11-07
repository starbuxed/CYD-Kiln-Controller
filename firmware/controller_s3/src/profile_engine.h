#pragma once
#include <stdint.h>

namespace kiln {
static constexpr uint8_t MAX_PROFILE_SEGMENTS = 32;
static constexpr float PROFILE_MAX_TEMP_C = 1800.0f;
static constexpr float PROFILE_MAX_RAMP_C_PER_HOUR = 600.0f;
static constexpr uint32_t PROFILE_MAX_HOLD_SECONDS = 86400UL;

struct RampSoakSegment {
  float targetC;
  float rampCPerHour;
  uint32_t holdSeconds;
};

enum ProfileRunState : uint8_t {
  PROFILE_IDLE = 0,
  PROFILE_RAMPING = 1,
  PROFILE_SOAKING = 2,
  PROFILE_PAUSED = 3,
  PROFILE_COMPLETE = 4,
  PROFILE_INVALID = 5
};

class ProfileEngine {
 public:
  ProfileEngine();
  bool validate(const RampSoakSegment* segments, uint8_t count) const;
  bool start(const RampSoakSegment* segments, uint8_t count,
             float startingTempC, uint32_t nowMs);
  void update(uint32_t nowMs);
  void pause(uint32_t nowMs);
  bool resume(uint32_t nowMs);
  void stop();
  ProfileRunState state() const { return state_; }
  float setpointC() const { return setpointC_; }
  uint8_t segmentIndex() const { return segmentIndex_; }
  uint8_t segmentCount() const { return count_; }
  uint32_t elapsedInSegmentMs(uint32_t nowMs) const;

 private:
  RampSoakSegment segments_[MAX_PROFILE_SEGMENTS];
  uint8_t count_;
  uint8_t segmentIndex_;
  ProfileRunState state_;
  float segmentStartC_;
  float setpointC_;
  uint32_t phaseStartMs_;
  uint32_t pausedAtMs_;
  uint32_t pausedTotalMs_;
  bool beginSegment(uint8_t index, float startC, uint32_t nowMs);
  void advance(uint32_t nowMs);
};
} // namespace kiln
