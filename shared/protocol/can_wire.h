#pragma once
#include <stddef.h>
#include <stdint.h>

namespace kiln {
namespace canwire {

// Classic CAN carries at most eight data bytes. One byte is reserved for
// fragmentation metadata, leaving seven bytes of protocol data per frame.
static constexpr uint8_t MAX_CAN_DLC = 8;
static constexpr uint8_t FRAGMENT_DATA_BYTES = 7;
static constexpr size_t MAX_REASSEMBLED_BYTES = 64;
static constexpr uint8_t FRAGMENT_START = 0x80;
static constexpr uint8_t FRAGMENT_END = 0x40;
static constexpr uint8_t FRAGMENT_INDEX_MASK = 0x3F;
static constexpr uint32_t FRAGMENT_TIMEOUT_MS = 150;

struct Fragment {
  uint8_t bytes[MAX_CAN_DLC] = {};
  uint8_t dlc = 0;
};

inline bool makeFragment(const uint8_t* source, size_t sourceLength,
                         uint8_t fragmentIndex, Fragment& out) {
  if (source == nullptr || sourceLength == 0) return false;
  const size_t offset = static_cast<size_t>(fragmentIndex) * FRAGMENT_DATA_BYTES;
  if (offset >= sourceLength || fragmentIndex > FRAGMENT_INDEX_MASK) return false;
  const size_t remaining = sourceLength - offset;
  const uint8_t copied = static_cast<uint8_t>(
      remaining > FRAGMENT_DATA_BYTES ? FRAGMENT_DATA_BYTES : remaining);
  out.bytes[0] = static_cast<uint8_t>(fragmentIndex & FRAGMENT_INDEX_MASK);
  if (fragmentIndex == 0) out.bytes[0] |= FRAGMENT_START;
  if (copied == remaining) out.bytes[0] |= FRAGMENT_END;
  for (uint8_t i = 0; i < copied; ++i) out.bytes[i + 1] = source[offset + i];
  out.dlc = static_cast<uint8_t>(copied + 1);
  return true;
}

// One reassembly stream per receiving node is sufficient for this two-node
// point-to-point topology. A new start fragment discards an incomplete frame.
// CAN identifier and strict fragment order prevent cross-message splicing.
class Reassembler {
 public:
  Reassembler() { reset(); }

  bool accept(uint16_t canId, const uint8_t* data, uint8_t dlc,
              uint32_t nowMs, uint8_t* output, size_t outputCapacity,
              size_t& outputLength) {
    outputLength = 0;
    if (data == nullptr || output == nullptr || dlc < 2 || dlc > MAX_CAN_DLC)
      return false;

    if (active_ && static_cast<uint32_t>(nowMs - lastFragmentMs_) > FRAGMENT_TIMEOUT_MS)
      reset();

    const uint8_t header = data[0];
    const uint8_t index = static_cast<uint8_t>(header & FRAGMENT_INDEX_MASK);
    const bool start = (header & FRAGMENT_START) != 0;
    const bool end = (header & FRAGMENT_END) != 0;

    if (start) {
      if (index != 0) { reset(); return false; }
      reset();
      active_ = true;
      canId_ = canId;
      expectedIndex_ = 0;
    } else if (!active_ || canId != canId_ || index != expectedIndex_) {
      reset();
      return false;
    }

    if (index != expectedIndex_) { reset(); return false; }
    const uint8_t payloadLength = static_cast<uint8_t>(dlc - 1);
    if (length_ + payloadLength > sizeof(buffer_) ||
        length_ + payloadLength > outputCapacity) {
      reset();
      return false;
    }

    for (uint8_t i = 0; i < payloadLength; ++i)
      buffer_[length_++] = data[i + 1];
    ++expectedIndex_;
    lastFragmentMs_ = nowMs;

    if (!end) return false;
    for (size_t i = 0; i < length_; ++i) output[i] = buffer_[i];
    outputLength = length_;
    reset();
    return true;
  }

  void reset() {
    active_ = false;
    canId_ = 0;
    expectedIndex_ = 0;
    length_ = 0;
    lastFragmentMs_ = 0;
  }

 private:
  uint8_t buffer_[MAX_REASSEMBLED_BYTES] = {};
  bool active_ = false;
  uint16_t canId_ = 0;
  uint8_t expectedIndex_ = 0;
  size_t length_ = 0;
  uint32_t lastFragmentMs_ = 0;
};

} // namespace canwire
} // namespace kiln
