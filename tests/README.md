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
