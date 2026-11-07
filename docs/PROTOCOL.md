# CYD ↔ Dedicated ESP32-S3 Communications Protocol

The CYD is the operator interface. The dedicated ESP32-S3 is authoritative for sensor acquisition, zone control, output requests, and local safety decisions.

## Transport

The initial transport is framed hardware UART at 115200 baud, 8-N-1, using 3.3 V logic. Exact pins, cable routing, grounding, and any required isolation must be verified against the actual board revisions. For a noisy kiln environment or long cable, evaluate isolated RS-485.

The CYD UART pin map is only enabled for supported classic 2.8-inch CYD profiles. S3 UART pins remain compile-time configurable and unset until the exact board is selected.

## Frame format

| Field | Bytes | Description |
|---|---:|---|
| Start of frame | 2 | A5 5A |
| Version | 1 | Protocol version 1 |
| Message type | 1 | Typed message identifier |
| Sequence | 2 | Little-endian sequence number |
| Payload length | 1 | 0–32 bytes |
| Payload | 0–32 | Message-specific data |
| CRC | 2 | CRC-16/CCITT over version through payload |

The shared implementation is shared/protocol/kiln_protocol.h. Frames with invalid CRC, unsupported version, or invalid message payload length are ignored or rejected. Commands use monotonically increasing sequence numbers; the S3 rejects duplicate or stale command sequences.

## Message types implemented

| Type | ID | Direction | Payload |
|---|---:|---|---|
| HELLO | 0x01 | CYD → S3 | Protocol version byte |
| HEARTBEAT | 0x02 | Both | CYD: uptime u32; S3: uptime u32 + state byte |
| STATUS | 0x03 | S3 → CYD | Fixed 21-byte snapshot |
| ACK | 0x05 | S3 → CYD | Command byte + result byte |
| COMMAND | 0x10 | CYD → S3 | One command byte |
| GET_STATUS | 0x11 | CYD → S3 | Empty |

All multibyte integers are little-endian.

## Command IDs

| Command | ID | Current behavior |
|---|---:|---|
| START | 0x01 | Rejected as NOT_READY until profile, sensors, and interlocks exist |
| PAUSE | 0x02 | Accepted only while FIRING; forces outputs inactive |
| RESUME | 0x03 | Rejected as NOT_READY until resume checks exist |
| STOP | 0x04 | Forces all outputs inactive and returns to IDLE unless fault-latched |
| ACK_FAULT | 0x05 | Not ready; fault recovery checks are not implemented |

The browser and CYD must never send raw GPIO values. The S3 validates typed requests and owns the controller state machine. The current application has no code path to start a firing or energize the disabled, unassigned output channels.

## Status payload (21 bytes)

| Offset | Size | Meaning |
|---:|---:|---|
| 0 | 1 | Controller state |
| 1 | 2 | Fault flags (currently zero; fault manager pending) |
| 3 | 1 | Sensor-valid bitmask (currently zero; sensor drivers pending) |
| 4 | 1 | Commanded output bitmask |
| 5 | 8 | Four signed temperatures, centi-degrees C |
| 13 | 8 | Four signed setpoints, centi-degrees C |

A zero sensor-valid mask means no valid temperature channels are currently available. Zero temperature values are placeholders, not measured ambient readings. Output bits describe logical commands only, not verified physical relay state.

## Heartbeats and loss behavior

Both processors send heartbeats every 500 ms. The S3 monitors the host heartbeat. If a previously established host heartbeat becomes stale while the controller is FIRING, the S3 commands all outputs inactive and enters FAULT. The S3 must continue local safety supervision independently of the CYD.

The current S3 firmware deliberately blocks START and RESUME because sensor acquisition, profile validation, and full safety interlocks are not implemented. Reconnection never automatically replays a command.

## Current implementation boundary

Implemented: shared frame encoding/parsing, CRC, sequence comparison, CYD UART client, S3 UART server, heartbeat exchange, status snapshots, command acknowledgements, stale command rejection, and safe STOP/PAUSE handling.

Not yet implemented: profile transfer, sensor configuration, real temperature telemetry, firing state machine, fault manager, zone control loops, and enabled physical GPIO mapping.
