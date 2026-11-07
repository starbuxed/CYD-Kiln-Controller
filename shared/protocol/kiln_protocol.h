#pragma once
#include <stdint.h>
#include <stddef.h>

// Wire format: SOF(2), version(1), type(1), sequence(2 LE),
// payload_length(1), payload(0..32), CRC16(2 LE).
namespace kiln {
static constexpr uint8_t SOF_0 = 0xA5;
static constexpr uint8_t SOF_1 = 0x5A;
static constexpr uint8_t PROTOCOL_VERSION = 1;
static constexpr uint8_t MAX_PAYLOAD = 32;
static constexpr size_t FRAME_OVERHEAD = 9;

enum MessageType : uint8_t {
  MSG_HELLO = 0x01,
  MSG_HOST_HEARTBEAT = 0x02,
  MSG_TELEMETRY = 0x03,
  MSG_OUTPUT_STATUS = 0x04,
  MSG_FAULT = 0x05,
  MSG_ACK = 0x06,
  MSG_SET_OUTPUTS = 0x10,
  MSG_REQUEST_STATUS = 0x11
};

enum ResultCode : uint8_t {
  RESULT_OK = 0,
  RESULT_BAD_LENGTH = 1,
  RESULT_BAD_SEQUENCE = 2,
  RESULT_UNSUPPORTED = 3,
  RESULT_FAULT_LATCHED = 4,
  RESULT_NOT_CONFIGURED = 5
};

struct Frame {
  uint8_t version = PROTOCOL_VERSION;
  uint8_t type = 0;
  uint16_t sequence = 0;
  uint8_t length = 0;
  uint8_t payload[MAX_PAYLOAD] = {};
};

inline uint16_t crc16(const uint8_t* data, size_t length) {
  uint16_t crc = 0xFFFF;
  for (size_t i = 0; i < length; ++i) {
    crc ^= static_cast<uint16_t>(data[i]) << 8;
    for (uint8_t bit = 0; bit < 8; ++bit)
      crc = (crc & 0x8000) ? static_cast<uint16_t>((crc << 1) ^ 0x1021)
                           : static_cast<uint16_t>(crc << 1);
  }
  return crc;
}

inline size_t encode(const Frame& frame, uint8_t* out, size_t capacity) {
  if (frame.length > MAX_PAYLOAD || capacity < FRAME_OVERHEAD + frame.length)
    return 0;
  out[0] = SOF_0;
  out[1] = SOF_1;
  out[2] = frame.version;
  out[3] = frame.type;
  out[4] = static_cast<uint8_t>(frame.sequence & 0xFF);
  out[5] = static_cast<uint8_t>(frame.sequence >> 8);
  out[6] = frame.length;
  for (uint8_t i = 0; i < frame.length; ++i) out[7 + i] = frame.payload[i];
  const uint16_t crc = crc16(out + 2, 5 + frame.length);
  out[7 + frame.length] = static_cast<uint8_t>(crc & 0xFF);
  out[8 + frame.length] = static_cast<uint8_t>(crc >> 8);
  return FRAME_OVERHEAD + frame.length;
}

class Parser {
 public:
  Parser() : used_(0), expected_(0) {}

  // Feed one byte. Returns true only when a complete, CRC-valid frame is ready.
  bool feed(uint8_t byte, Frame& result) {
    if (used_ == 0 && byte != SOF_0) return false;
    if (used_ == 1 && byte != SOF_1) {
      used_ = (byte == SOF_0) ? 1 : 0;
      return false;
    }
    if (used_ < sizeof(buffer_)) buffer_[used_++] = byte;
    else { reset(); return false; }

    if (used_ == 7) {
      const uint8_t payloadLength = buffer_[6];
      if (payloadLength > MAX_PAYLOAD) { reset(); return false; }
      expected_ = FRAME_OVERHEAD + payloadLength;
    }
    if (expected_ == 0 || used_ < expected_) return false;
    if (used_ != expected_) { reset(); return false; }

    const uint8_t length = buffer_[6];
    const uint16_t received = static_cast<uint16_t>(buffer_[7 + length]) |
                              (static_cast<uint16_t>(buffer_[8 + length]) << 8);
    const uint16_t calculated = crc16(buffer_ + 2, 5 + length);
    if (received != calculated) { reset(); return false; }

    result.version = buffer_[2];
    result.type = buffer_[3];
    result.sequence = static_cast<uint16_t>(buffer_[4]) |
                      (static_cast<uint16_t>(buffer_[5]) << 8);
    result.length = length;
    for (uint8_t i = 0; i < length; ++i) result.payload[i] = buffer_[7 + i];
    reset();
    return true;
  }

  void reset() { used_ = 0; expected_ = 0; }

 private:
  uint8_t buffer_[FRAME_OVERHEAD + MAX_PAYLOAD];
  size_t used_;
  size_t expected_;
};
}  // namespace kiln
