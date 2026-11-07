# ESP32 ↔ Pro Micro Communications Protocol

This document defines the initial protocol requirements, not a finalized byte-level implementation. The framing, checksum, timeout, and version constants must be fixed before firmware implementation.

## Transport

Use a hardware UART between the CYD ESP32 and Pro Micro. Confirm the selected Pro Micro's logic voltage before connecting. A 5 V Pro Micro TX signal must not be connected directly to a 3.3 V ESP32 RX pin; use a suitable bidirectional/one-way level interface as appropriate. Share signal ground only where the complete isolation and grounding design permits it.

Do not assume a board's UART pins from its marketing name; verify the exact board schematic and pinout.

## Frame requirements

Every frame should include:
- Protocol version.
- Message type.
- Sequence number.
- Payload length.
- Payload.
- Error-detection checksum (CRC recommended).

The receiver must reject unsupported versions, invalid lengths, checksum failures, malformed payloads, and stale/replayed sequence numbers.

A byte-level framing format (including delimiter/escaping or fixed header, maximum frame size, CRC polynomial, and endian rules) remains to be selected.

## Message classes

### Pro Micro → ESP32

- `HELLO`: firmware/protocol version and hardware capability flags.
- `HEARTBEAT`: uptime, watchdog state, and current fault flags.
- `TELEMETRY`: per-channel raw value, converted value, validity, error flags, and sample sequence/time.
- `OUTPUT_STATUS`: requested/commanded/reported output states and feedback availability.
- `FAULT`: fault code, severity, channel, and latched state.
- `ACK`: acknowledged command sequence and result code.

### ESP32 → Pro Micro

- `HOST_HEARTBEAT`: valid host heartbeat; services Pro Micro communications watchdog.
- `SET_OUTPUTS`: bounded complete output-state request, never an unvalidated raw GPIO write.
- `CLEAR_NONCRITICAL_FAULT`: only for explicitly defined recoverable faults and never for a safety-critical latch.
- `REQUEST_STATUS`: request current I/O state and diagnostics.

No command may remotely clear a critical high-temperature, sensor-integrity, or communications safety fault without a documented physical inspection/recovery process.

## Timing and watchdog

- Define heartbeat cadence and timeout from measured worst-case loop and serial latency, with a documented safety margin.
- Pro Micro outputs must enter their safe state on watchdog timeout.
- ESP32 treats missing/late telemetry as stale and blocks firing actions.
- Reconnection begins with status exchange, not restoration of prior requested outputs.
- Do not automatically replay non-idempotent commands.

The final timeout values require validation under worst-case load and are not specified yet.

## Telemetry data model

Each temperature channel should provide:
- Channel ID (0–3).
- Raw sensor/front-end reading where useful.
- Engineering value and unit.
- Validity flag.
- Sensor/front-end error code.
- Monotonic sample sequence or timestamp.
- Freshness status derived by the ESP32.

Each output should provide:
- Output ID (0–3).
- Requested state.
- Commanded state.
- Reported state.
- Feedback-present flag and verified state when available.
- Interlock/fault flags.

## Protocol test cases

- Valid frame and valid acknowledgement.
- Corrupt checksum.
- Unsupported protocol version.
- Truncated and oversized frame.
- Invalid message type and payload length.
- Replayed/out-of-order sequence.
- Duplicate command.
- UART noise and partial frame recovery.
- Host heartbeat timeout.
- Pro Micro reset while firing request is active.
- ESP32 reset and reconnect.
- Sensor-invalid and output-feedback mismatch reporting.
