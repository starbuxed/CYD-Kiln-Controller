#include <assert.h>
#include <stdint.h>
#include "kiln_protocol.h"

int main() {
  kiln::Frame tx;
  tx.version = kiln::PROTOCOL_VERSION;
  tx.type = kiln::MSG_HOST_HEARTBEAT;
  tx.sequence = 0x1234;
  tx.length = 2;
  tx.payload[0] = 0xA5;
  tx.payload[1] = 0x5A;

  uint8_t wire[kiln::FRAME_OVERHEAD + kiln::MAX_PAYLOAD] = {};
  const size_t wireLength = kiln::encode(tx, wire, sizeof(wire));
  assert(wireLength == kiln::FRAME_OVERHEAD + tx.length);

  kiln::Parser parser;
  kiln::Frame rx;
  bool decoded = false;
  for (size_t i = 0; i < wireLength; ++i)
    decoded = parser.feed(wire[i], rx) || decoded;

  assert(decoded);
  assert(rx.version == tx.version);
  assert(rx.type == tx.type);
  assert(rx.sequence == tx.sequence);
  assert(rx.length == tx.length);
  assert(rx.payload[0] == tx.payload[0]);
  assert(rx.payload[1] == tx.payload[1]);

  // Corrupt one payload byte: parser must not emit a valid frame.
  wire[7] ^= 0x01;
  parser.reset();
  decoded = false;
  for (size_t i = 0; i < wireLength; ++i)
    decoded = parser.feed(wire[i], rx) || decoded;
  assert(!decoded);

  // Oversized payloads must be rejected by the encoder.
  tx.length = kiln::MAX_PAYLOAD + 1;
  assert(kiln::encode(tx, wire, sizeof(wire)) == 0);
  return 0;
}
