#include <assert.h>
#include <stdint.h>
#include "kiln_protocol.h"

int main() {
  kiln::Frame tx;
  tx.type = kiln::MSG_HEARTBEAT;
  tx.sequence = 0x1234;
  tx.length = 4;
  kiln::putU32(tx.payload, 0x78563412UL);

  uint8_t wire[kiln::MAX_FRAME_SIZE] = {};
  const size_t wireLength = kiln::encode(tx, wire, sizeof(wire));
  assert(wireLength == kiln::FRAME_OVERHEAD + tx.length);

  kiln::Parser parser;
  kiln::Frame rx;
  bool decoded = false;
  for (size_t i = 0; i < wireLength; ++i)
    decoded = parser.feed(wire[i], rx) || decoded;

  assert(decoded);
  assert(rx.version == kiln::PROTOCOL_VERSION);
  assert(rx.type == tx.type);
  assert(rx.sequence == tx.sequence);
  assert(rx.length == tx.length);
  assert(kiln::getU32(rx.payload) == 0x78563412UL);

  wire[7] ^= 0x01;
  parser.reset();
  decoded = false;
  for (size_t i = 0; i < wireLength; ++i)
    decoded = parser.feed(wire[i], rx) || decoded;
  assert(!decoded);

  tx.length = kiln::MAX_PAYLOAD + 1;
  assert(kiln::encode(tx, wire, sizeof(wire)) == 0);

  assert(kiln::sequenceIsNewer(11, 10));
  assert(!kiln::sequenceIsNewer(10, 10));
  assert(!kiln::sequenceIsNewer(9, 10));
  assert(kiln::sequenceIsNewer(0, 65535));

  uint8_t tempBytes[2];
  kiln::putI16(tempBytes, -1234);
  assert(kiln::getI16(tempBytes) == -1234);
  return 0;
}
