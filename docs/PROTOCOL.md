# CYD ↔ Dedicated ESP32-S3 Communications Protocol

The target architecture uses a CYD ESP32 operator interface and a separate ESP32-S3 local kiln controller. The former Pro Micro protocol descriptions are legacy and must not be used as the target design.

## Transport

Use a hardware UART between the CYD and dedicated ESP32-S3 during initial implementation. Both boards use 3.3 V logic, but exact board pins, cable routing, grounding, and any required isolation must be verified. For a noisy kiln environment or long cable, evaluate an isolated differential transport such as isolated RS-485 rather than assuming an unprotected TTL UART is robust enough.

Do not assume UART pins from a board marketing name; verify the exact board schematic and carrier-board design.

## Authority and safety boundary

The dedicated ESP32-S3 is authoritative for local sensor acquisition, four zone-control loops, and heater output requests. The CYD may request profile selection, arm, start, pause, or stop actions, but the dedicated controller validates every request and owns the local state machine and safety interlocks.

The CYD and browser must never send raw GPIO values. The controller accepts typed, bounded commands only. Loss of the CYD link must not stop local fault monitoring. Initial commissioning policy is to make all heater outputs inactive and latch a UI-link fault on link loss during firing; any alternative requires a kiln-specific safety review.

## Frame requirements

Every frame includes:
- Protocol version.
- Message type.
- Sequence number.
- Payload length.
- Payload.
- CRC.

The receiver rejects unsupported versions, invalid lengths, checksum failures, malformed payloads, stale/replayed sequence numbers, and invalid state transitions.

The exact byte-level framing must be finalized in the shared protocol implementation and covered by host-side tests.

## CYD → ESP32-S3 messages

- `HELLO`: UI firmware and protocol versions.
- `HEARTBEAT`: UI uptime and link status.
- `GET_STATUS`: request controller snapshot.
- `PROFILE_PROPOSE`: submit a validated-profile candidate for controller-side validation.
- `ARM_REQUEST`: request arming after local checks.
- `START_REQUEST`: explicit operator start request.
- `PAUSE_REQUEST` / `STOP_REQUEST`: requests subject to the kiln-specific state policy.
- `CONFIG_PROPOSE`: propose sensor, zone, and profile configuration changes; only accepted while outputs are safe.

## ESP32-S3 → CYD messages

- `HELLO`: controller firmware/protocol versions and hardware capabilities.
- `HEARTBEAT`: uptime, state, watchdog status, and fault flags.
- `STATUS`: four temperature channels, sensor types, freshness, four setpoints, four zone states, output request states, and feedback availability.
- `FAULT`: fault code, severity, affected channel/zone, and latched state.
- `ACK`: acknowledged request sequence and result.
- `EVENT`: state transitions and operator-visible event records.

## Four-channel telemetry

Every temperature channel reports:
- Channel ID (1–4).
- Sensor family and configured thermocouple type.
- Front-end identity.
- Raw/front-end value where useful.
- Converted temperature and unit.
- Validity and freshness.
- Front-end fault flags (open circuit, short, over/undervoltage, cold-junction fault, etc., as supported).
- Monotonic sample sequence or timestamp.

Every zone/output reports:
- Zone ID (1–4).
- Assigned sensor channel and heater output.
- Setpoint and control demand.
- Requested/commanded output state.
- Reported driver state.
- Feedback-present flag and verified physical state only when actual feedback hardware exists.
- Interlock and fault flags.

## Timing and reconnect behavior

- Heartbeat cadence and timeout must be derived from measured worst-case loop and serial latency with a documented margin.
- The dedicated controller continues local safety supervision if the CYD disconnects.
- Initial commissioning policy on UI-link loss is all heater outputs inactive and a latched fault.
- Reconnection begins with version/capability exchange and authoritative status/fault snapshot.
- Never replay queued arm/start/resume/output commands.
- A new explicit operator start sequence is required after interruption.

## Protocol test cases

- Valid frame and acknowledgement.
- Corrupt CRC, unsupported version, invalid length, truncated/oversized frame.
- Replayed/out-of-order sequence and duplicate command.
- UART noise and partial-frame recovery.
- CYD link timeout while idle and while firing.
- Controller reset during firing request.
- CYD reset and reconnect.
- Invalid sensor type, sensor fault, stale data, and implausible temperature.
- One-zone fault and all-zones-safe default.
- Output feedback mismatch and heater current present while commanded off.
