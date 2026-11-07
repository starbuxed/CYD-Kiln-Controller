#pragma once

#include <stddef.h>
#include <stdint.h>

namespace kiln {
static constexpr uint8_t FRAME_START_1 = 0xA5;
static constexpr uint8_t FRAME_START_2 = 0x5A;
static constexpr uint8_t PROTOCOL_VERSION = 1;
static constexpr size_t MAX_PAYLOAD = 64;
static constexpr size_t HEADER_SIZE = 6; // version,type,seqLE[2],lengthLE[2]
static constexpr size_t FRAME_OVERHEAD = 2 + HEADER_SIZE + 2;

enum class MessageType : uint8_t {
  HELLO = 1,
  HEARTBEAT = 2,
  TELEMETRY = 3,
  OUTPUT_STATUS = 4,
  FAULT = 5,
  ACK = 6,
  HOST_HEARTBEAT = 16,
  SET_OUTPUTS = 17,
  REQUEST_STATUS = 18
};

enum class Result : uint8_t {
  OK = 0,
  BAD_VERSION = 1,
  BAD_LENGTH = 2,
  BAD_CRC = 3,
  BAD_SEQUENCE = 4,
  UNSUPPORTED = 5,
  INTERLOCKED = 6,
  FAULT_LATCHED = 7
};

struct Frame {
  uint8_t version = PROTOCOL_VERSION;
  MessageType type = MessageType::HELLO;
  uint16_t sequence = 0;
  uint16_t length = 0;
  uint8_t payload[MAX_PAYLOAD] = {};
};

inline uint16_t crc16Ccitt(const uint8_t* data, size_t length) {
  uint16_t crc = 0xFFFF;
  for (size_t i = 0; i < length; ++i) {
    crc ^= static_cast<uint16_t>(data[i]) << 8;
    for (uint8_t bit = 0; bit < 8; ++bit) {
      crc = (crc & 0x8000) ? static_cast<uint16_t>((crc << 1) ^ 0x1021)
                           : static_cast<uint16_t>(crc << 1);
    }
  }
  return crc;
}
} // namespace kiln
