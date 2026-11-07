# Firmware Implementation Status

## Current architecture

- CYD ESP32: operator interface, later touchscreen and self-hosted browser UI.
- Dedicated ESP32-S3: local authority for four temperature channels, four independent zone loops, four digital ON/OFF outputs, profile execution, and safety supervision.
- External relay/SSR equipment is selected by the user. Firmware provides four low-voltage ON/OFF command signals only.

## Implemented in source

- CYD-to-S3 UART protocol v1 with framing, CRC-16/CCITT, sequence numbers, bounded payloads, and parser.
- CYD UART client with HELLO, heartbeat, command send, status receive, and link-health reporting.
- S3 UART server with heartbeat, status snapshots, command validation, acknowledgements, stale-sequence rejection, and safe STOP/PAUSE handling.
- Four-channel relay output abstraction with active-high/active-low polarity, disabled/unassigned handling, and inactive startup.
- Hardware-independent ramp/soak profile engine with bounded profile validation, ramp interpolation, soak timing, pause/resume, and completion.
- Four independent hysteresis zone controllers with configurable deadband and minimum on/off dwell.
- Local latched safety manager for sensor loss, sensor range, overtemperature, and host-link faults.
- Four-zone control core that coordinates profile, independent zones, and fail-safe output masks.
- Host-side protocol, relay configuration, and controller-core test sources.
- CYD build source filter excludes the obsolete Pro Micro transport from the current S3 architecture.

## Not yet integrated or verified

- Thermocouple front-end drivers and real four-channel temperature acquisition. Sensor type selection is not connected to hardware yet.
- S3 control core is hardware-independent source; the Arduino main loop does not yet feed it real sensor readings or map its output mask to GPIO.
- Profile persistence, profile transfer from CYD, and user profile editor.
- Verified physical GPIO assignments for the exact S3 board and external relay input interface.
- CYD display/touchscreen UI, Wi-Fi setup, HTTP API, browser authentication, and live web updates.
- Firing history, charts, data export, and watchdog integration.
- A separate independent hardware overtemperature cutoff is not replaced by firmware.

## Safety behavior

- START and RESUME remain blocked at the S3 command handler until hardware sensor acquisition, output mapping, profile transfer, and end-to-end interlocks are integrated and tested.
- Outputs are configured inactive at boot; all current output pins are disabled/unassigned.
- Loss of the CYD heartbeat during a firing must force all outputs inactive and latch a fault.
- No automatic restart or resume after reset, brownout, sensor loss, or communication failure.

## Validation status

- Host test source files are present for protocol framing, relay configuration, and the hardware-independent controller core.
- No PlatformIO build, ESP32 board build, or physical hardware test has yet been run.
- The current firmware cannot energize a kiln and cannot start a firing.
