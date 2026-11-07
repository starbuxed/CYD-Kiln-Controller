# CYD ↔ ESP32-S3 CAN Protocol

## Transport

Inter-controller link: classic CAN 2.0A, standard 11-bit identifiers, using ESP32 TWAI and one compatible external CAN transceiver on each board. Initial target bitrate is 500 kbit/s, subject to validation against cable length, topology, transceiver, and EMI conditions.

The TWAI peripheral is not a physical transceiver. Both boards require an external transceiver. Exact TX/RX GPIO assignments remain unset until their board revisions and pin availability are verified.

CANH/CANL use twisted pair; terminate only the two physical ends of a linear bus with 120 ohms. Keep stubs short. Choose reference grounding or galvanic isolation intentionally. Emergency shutdown is separate hardwired circuitry.

## CAN identifiers

Lower standard identifiers have higher arbitration priority.

| ID | Name | Direction | Purpose |
|---:|---|---|---|
| 0x080 | HARD_STATUS | S3 → CYD | Fast state/fault/output summary |
| 0x081 | TEMPERATURES_0_1 | S3 → CYD | Channels 0 and 1: signed deci-C and status |
| 0x082 | TEMPERATURES_2_3 | S3 → CYD | Channels 2 and 3: signed deci-C and status |
| 0x090 | HEARTBEAT_CYD | CYD → S3 | Main heartbeat and session |
| 0x091 | HEARTBEAT_S3 | S3 → CYD | Secondary heartbeat, state, safety flags |
| 0x100 | OPERATOR_COMMAND | CYD → S3 | Typed command; never raw GPIO |
| 0x101 | COMMAND_ACK | S3 → CYD | Accepted/rejected result |
| 0x110 | PROFILE_BEGIN | CYD → S3 | Start staged transfer |
| 0x111 | PROFILE_SEGMENT | CYD → S3 | Segment fragment |
| 0x112 | PROFILE_COMMIT | CYD → S3 | Validate and install staged profile |
| 0x120 | ZONE_CONFIG | CYD → S3 | Validated zone settings |
| 0x121 | SENSOR_CONFIG | CYD → S3 | Channel type/offset/mode |
| 0x122 | SAFETY_CONFIG | CYD → S3 | Bounded safety config while idle |
| 0x130 | GET_STATUS | CYD → S3 | Request extended status |
| 0x131 | EXTENDED_STATUS | S3 → CYD | Diagnostics |
| 0x140 | EVENT | S3 → CYD | Fault/transition event |
| 0x150 | TIME_SYNC | CYD → S3 | Wall-clock sync; not safety-critical |

Classic CAN payload is limited to 8 bytes. Multi-frame transfers require transaction ID, fragment index/count, bounded total size, timeout, and CRC at assembled-message layer. Partial transfer must never be actionable.

## Message safety

- Validate exact DLC for every identifier.
- Include sequence counters and a boot/session identifier for commands/heartbeats.
- Reject duplicate, stale, malformed, unsupported, or out-of-range commands.
- Commands are requests, never direct output writes.
- Accept configuration only while safely idle and outputs inactive.
- Stage and atomically commit profile/config transfers only after full validation.
- Missing CYD heartbeat during firing causes S3 outputs inactive and a latched link fault.
- Reconnection never replays commands.
- CAN bus-off or persistent error state is a fault; S3 outputs go inactive.
- Fault/status traffic has higher priority than ordinary profile transfers.
- The separate hardwired shutdown path is the emergency-stop mechanism; CAN is not.

## Numeric encoding

All integers little-endian. Temperatures use signed deci-degrees Celsius. Every sensor value is paired with validity/fault status. Zero is not a representation for an invalid sensor.

## Heartbeats

Both nodes transmit at 5–10 Hz. S3 monitors CYD heartbeat. A timeout while firing, paused, or armed forces outputs inactive and latches a communications fault. Boot requires a fresh session handshake; firing requires a deliberate new operator start.

## Shutdown distinction

Software STOP/FAULT messages provide state and diagnostics only. Each ESP32 has a separate shutdown-request output/interface wired to an engineered fail-safe circuit. A physical latching E-stop and independent high-limit interrupt the safety-contactor path without firmware. No CAN ID or touchscreen command may override an active hardware trip.

## Compatibility

The prior UART protocol is superseded as the intended inter-board transport. UART may remain as a diagnostic console only; it is not the kiln control/safety link.
