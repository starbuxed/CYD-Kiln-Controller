# CYD ↔ Dedicated ESP32-S3 Communications Protocol

The CYD is the operator interface. The dedicated ESP32-S3 is authoritative for sensor acquisition, zone control, output requests, and local safety decisions.

## Transport

The initial transport is framed hardware UART at 115200 baud, 8-N-1, using 3.3 V logic. Exact pins, cable routing, grounding, and any required isolation must be verified against the actual board revisions. For a noisy kiln environment or long cable, evaluate isolated RS-485.

The default classic 2.8-inch CYD profile uses GPIO27 TX / GPIO22 RX. The ESP32-S3-DevKitC-1 PlatformIO target uses GPIO17 RX / GPIO18 TX. Override the S3 pin macros for a different board or wiring harness.

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
| STATUS | 0x03 | S3 → CYD | Fixed 29-byte snapshot |
| ACK | 0x05 | S3 → CYD | Command byte + result byte |
| COMMAND | 0x10 | CYD → S3 | One command byte |
| GET_STATUS | 0x11 | CYD → S3 | Empty |
| PROFILE_BEGIN | 0x20 | CYD → S3 | Segment count byte |
| PROFILE_SEGMENT | 0x21 | CYD → S3 | Index + target deci-C + ramp deci-C/hour + hold seconds u32 |
| PROFILE_COMMIT | 0x22 | CYD → S3 | Empty; validates and installs staged profile |
| ZONE_CONFIG | 0x23 | CYD → S3 | Zone + hysteresis deci-C + min-on ms + min-off ms + setpoint offset deci-C |
| SENSOR_CONFIG | 0x24 | CYD → S3 | Channel + thermocouple type + calibration offset deci-C |
| SENSOR_MODE_CONFIG | 0x26 | CYD → S3 | Channel + operating mode (0=Off, 1=Log only, 2=Log and control, 3=Log and safety) |
| SAFETY_CONFIG | 0x25 | CYD → S3 | Maximum allowed temperature deci-C u16/i16 |

All multibyte integers are little-endian. Thermocouple type IDs are K=0, J=1, N=2, R=3, S=4, T=5, E=6, B=7.

## Command IDs

| Command | ID | Current behavior |
|---|---:|---|
| START | 0x01 | Starts only when profile, four zone configs, four valid sensors, host link, and all four output channels are ready |
| PAUSE | 0x02 | Accepted only while FIRING; forces outputs inactive |
| RESUME | 0x03 | Resumes only from PAUSED after sensor and host-link checks |
| STOP | 0x04 | Forces all outputs inactive and returns to IDLE unless fault-latched |
| ACK_FAULT | 0x05 | Clears a latched fault only when all sensors are valid, below the safety limit, and host link is healthy |

The browser and CYD must never send raw GPIO values. The S3 validates typed requests and owns the controller state machine. Configuration is accepted only while IDLE. Profiles are staged by index and installed only after every segment is received and the complete profile validates. Zone, sensor, and safety settings are individually validated. START and RESUME now enter the local control core only when a complete profile, four zone configurations, all four valid PID-enabled sensor readings, all configured Log and safety sensors valid, a healthy host link, and all four configured relay outputs are present. The current hardware build has no sensor driver or enabled relay pin map, so these checks intentionally keep firing blocked.

## Status payload (29 bytes)

| Offset | Size | Meaning |
|---:|---:|---|
| 0 | 1 | Controller state |
| 1 | 2 | Latched safety fault flags |
| 3 | 1 | Sensor-valid bitmask |
| 4 | 1 | Commanded output bitmask |
| 5 | 8 | Four signed temperatures, deci-degrees C |
| 13 | 8 | Four signed setpoints, deci-degrees C |
| 21 | 4 | Per-channel sensor fault codes (see sensor manager enum) |
| 25 | 4 | Per-channel operating modes: 0=Off, 1=Log only, 2=Log and control, 3=Log and safety |

Sensor fault codes are: 0=OK, 1=open circuit, 2=short (reserved for front ends that report it), 3=conversion/electrical fault, 4=stale sample, 5=out of range, 6=implausible rate, 7=not configured. A zero sensor-valid mask means no valid temperature channels are currently available. Zero temperature values are placeholders, not measured ambient readings. Output bits describe logical commands only, not verified physical relay state.

## Heartbeats and loss behavior

Both processors send heartbeats every 500 ms. The S3 monitors the host heartbeat. If a previously established host heartbeat becomes stale while the controller is FIRING, the S3 commands all outputs inactive and enters FAULT. The S3 must continue local safety supervision independently of the CYD.

The S3 contains a hardware-independent profile engine, four zone controllers, a sensor-validation abstraction, and a local safety manager. Typed profile, zone, sensor, and safety configuration messages are implemented. START and RESUME are wired to the local control core but remain NOT_READY until a real sensor front-end, enabled output map, and complete end-to-end hardware validation are present. Reconnection never automatically replays a command.

## Current implementation boundary

Implemented: shared frame encoding/parsing, CRC, sequence comparison, CYD UART client and configuration senders, S3 UART server, heartbeat exchange, status snapshots, typed configuration staging, command acknowledgements, stale command rejection, and local control-core command handling.

Not yet implemented: physical SPI/CS pin configuration and sensor hardware validation, enabled physical relay GPIO mapping, CYD UI, and end-to-end firing validation.
