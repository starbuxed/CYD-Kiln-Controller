#include <assert.h>
#include <stdint.h>
#include <string.h>
#include "kiln_protocol.h"
#include "can_wire.h"

int main() {
  kiln::Frame tx;
  tx.type = kiln::MSG_PROFILE_SEGMENT;
  tx.sequence = 0x4567;
  tx.length = 9;
  tx.payload[0] = 2;
  kiln::putI16(tx.payload + 1, 12345);
  kiln::putI16(tx.payload + 3, 678);
  kiln::putU32(tx.payload + 5, 987654UL);

  uint8_t encoded[kiln::MAX_FRAME_SIZE] = {};
  const size_t encodedLength = kiln::encode(tx, encoded, sizeof(encoded));
  assert(encodedLength == kiln::FRAME_OVERHEAD + tx.length);

  kiln::canwire::Reassembler rx;
  uint8_t assembled[kiln::canwire::MAX_REASSEMBLED_BYTES] = {};
  size_t assembledLength = 0;
  const uint16_t canId = 0x181;
  const uint8_t fragments = static_cast<uint8_t>(
      (encodedLength + kiln::canwire::FRAGMENT_DATA_BYTES - 1) /
      kiln::canwire::FRAGMENT_DATA_BYTES);

  bool complete = false;
  for (uint8_t i = 0; i < fragments; ++i) {
    kiln::canwire::Fragment fragment;
    assert(kiln::canwire::makeFragment(encoded, encodedLength, i, fragment));
    complete = rx.accept(canId, fragment.bytes, fragment.dlc,
                         static_cast<uint32_t>(10 + i), assembled,
                         sizeof(assembled), assembledLength) || complete;
  }
  assert(complete);
  assert(assembledLength == encodedLength);
  assert(memcmp(encoded, assembled, encodedLength) == 0);

  // Wrong fragment order must be rejected and must not yield a message.
  rx.reset();
  kiln::canwire::Fragment first;
  kiln::canwire::Fragment second;
  assert(kiln::canwire::makeFragment(encoded, encodedLength, 0, first));
  assert(kiln::canwire::makeFragment(encoded, encodedLength, 1, second));
  assert(!rx.accept(canId, first.bytes, first.dlc, 100, assembled,
                    sizeof(assembled), assembledLength));
  assert(!rx.accept(canId, second.bytes, second.dlc, 101, assembled,
                    sizeof(assembled), assembledLength));

  // An interrupted transfer is discarded after its timeout.
  rx.reset();
  assert(!rx.accept(canId, first.bytes, first.dlc, 200, assembled,
                    sizeof(assembled), assembledLength));
  assert(!rx.accept(canId, second.bytes, second.dlc, 400, assembled,
                    sizeof(assembled), assembledLength));

  return 0;
}
