#include "profile_engine.h"
#include <math.h>

namespace kiln {
ProfileEngine::ProfileEngine()
    : count_(0), segmentIndex_(0), state_(PROFILE_IDLE), stateBeforePause_(PROFILE_IDLE), segmentStartC_(0),
      setpointC_(0), phaseStartMs_(0), pausedAtMs_(0), pausedTotalMs_(0) {}

bool ProfileEngine::validate(const RampSoakSegment* segments, uint8_t count) const {
  if (!segments || count == 0 || count > MAX_PROFILE_SEGMENTS) return false;
  for (uint8_t i = 0; i < count; ++i) {
    const RampSoakSegment& s = segments[i];
    if (!isfinite(s.targetC) || !isfinite(s.rampCPerHour)) return false;
    if (s.targetC < 0.0f || s.targetC > PROFILE_MAX_TEMP_C) return false;
    if (s.rampCPerHour < 0.0f ||
        s.rampCPerHour > PROFILE_MAX_RAMP_C_PER_HOUR) return false;
    if (s.rampCPerHour == 0.0f &&
        (i == 0 || fabsf(s.targetC - segments[i - 1].targetC) >= 0.001f))
      return false;
    if (s.holdSeconds > PROFILE_MAX_HOLD_SECONDS) return false;
  }
  return true;
}

bool ProfileEngine::start(const RampSoakSegment* segments, uint8_t count,
                          float startingTempC, uint32_t nowMs) {
  stop();
  if (!validate(segments, count) || !isfinite(startingTempC) ||
      startingTempC < 0.0f || startingTempC > PROFILE_MAX_TEMP_C) {
    state_ = PROFILE_INVALID;
    return false;
  }
  count_ = count;
  for (uint8_t i = 0; i < count; ++i) segments_[i] = segments[i];
  segmentIndex_ = 0;
  pausedTotalMs_ = 0;
  return beginSegment(0, startingTempC, nowMs);
}

bool ProfileEngine::beginSegment(uint8_t index, float startC, uint32_t nowMs) {
  if (index >= count_) {
    state_ = PROFILE_COMPLETE;
    setpointC_ = count_ ? segments_[count_ - 1].targetC : startC;
    return true;
  }
  segmentIndex_ = index;
  segmentStartC_ = startC;
  setpointC_ = startC;
  phaseStartMs_ = nowMs;
  state_ = (fabsf(segments_[index].targetC - startC) < 0.001f)
             ? PROFILE_WAITING : PROFILE_RAMPING;
  return true;
}

uint32_t ProfileEngine::elapsedInSegmentMs(uint32_t nowMs) const {
  if (state_ == PROFILE_PAUSED)
    return static_cast<uint32_t>(pausedAtMs_ - phaseStartMs_ - pausedTotalMs_);
  return static_cast<uint32_t>(nowMs - phaseStartMs_ - pausedTotalMs_);
}

void ProfileEngine::advance(uint32_t nowMs) {
  const float nextStart = segments_[segmentIndex_].targetC;
  if (segmentIndex_ + 1 >= count_) {
    state_ = PROFILE_COMPLETE;
    setpointC_ = nextStart;
    return;
  }
  beginSegment(static_cast<uint8_t>(segmentIndex_ + 1), nextStart, nowMs);
}

void ProfileEngine::update(uint32_t nowMs) {
  if (state_ != PROFILE_RAMPING && state_ != PROFILE_SOAKING) return;
  const RampSoakSegment& seg = segments_[segmentIndex_];
  const uint32_t elapsed = elapsedInSegmentMs(nowMs);
  if (state_ == PROFILE_RAMPING) {
    const float distance = fabsf(seg.targetC - segmentStartC_);
    const float durationMs = distance / seg.rampCPerHour * 3600000.0f;
    const float fraction = durationMs <= 0.0f ? 1.0f :
        static_cast<float>(elapsed) / durationMs;
    if (fraction >= 1.0f) {
      setpointC_ = seg.targetC;
      state_ = PROFILE_WAITING;
    } else {
      const float direction = seg.targetC >= segmentStartC_ ? 1.0f : -1.0f;
      setpointC_ = segmentStartC_ + direction * distance * fraction;
    }
  } else if (elapsed >= seg.holdSeconds * 1000UL) {
    advance(nowMs);
  }
}

bool ProfileEngine::beginSoak(uint32_t nowMs) {
  if (state_ != PROFILE_WAITING) return false;
  phaseStartMs_ = nowMs;
  pausedTotalMs_ = 0;
  state_ = PROFILE_SOAKING;
  return true;
}

void ProfileEngine::pause(uint32_t nowMs) {
  if (state_ == PROFILE_RAMPING || state_ == PROFILE_WAITING ||
      state_ == PROFILE_SOAKING) {
    pausedAtMs_ = nowMs;
    stateBeforePause_ = state_;
    state_ = PROFILE_PAUSED;
  }
}
bool ProfileEngine::resume(uint32_t nowMs) {
  if (state_ != PROFILE_PAUSED) return false;
  if (stateBeforePause_ == PROFILE_RAMPING || stateBeforePause_ == PROFILE_SOAKING)
    pausedTotalMs_ += static_cast<uint32_t>(nowMs - pausedAtMs_);
  state_ = stateBeforePause_;
  return true;
}
void ProfileEngine::stop() {
  count_ = 0;
  segmentIndex_ = 0;
  state_ = PROFILE_IDLE;
  stateBeforePause_ = PROFILE_IDLE;
  segmentStartC_ = setpointC_ = 0.0f;
  phaseStartMs_ = pausedAtMs_ = pausedTotalMs_ = 0;
}
} // namespace kiln
