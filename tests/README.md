# Test Strategy

Tests must be runnable without a connected kiln or energized heater circuit.

## Host-side tests

- Protocol framing, CRC, malformed and truncated frames.
- Sequence numbers, duplicate frames, stale commands, and version negotiation.
- Temperature validity, freshness, and range checking.
- Profile JSON schema and ramp/soak parameter validation.
- Supervisory state-machine transition table.
- Fault latching and recovery requirements.
- Browser reconnect behavior and command replay prevention.

## Hardware-in-the-loop tests

Begin with simulated sensor inputs and low-voltage indicator loads only.

- Pro Micro reset during active command.
- ESP32 reset and reconnection.
- UART cable disconnect, noise, and timeout.
- Sensor open/short/front-end fault.
- Stale temperature data.
- Output feedback mismatch (when feedback hardware is present).
- Brownout and watchdog reset.

Never use live heating elements as the initial test load. See [docs/SAFETY.md](../docs/SAFETY.md).


## Protocol unit test

The standalone C++ test in `tests/protocol/test_protocol.cpp` exercises frame encoding, byte-at-a-time parsing, and CRC rejection.

From the repository root with a C++11 compiler:

```sh
g++ -std=c++11 -Ishared/protocol tests/protocol/test_protocol.cpp -o /tmp/kiln_protocol_test
/tmp/kiln_protocol_test
```

This host-side test does not replace compiling both MCU firmware targets or testing on the exact physical boards.
