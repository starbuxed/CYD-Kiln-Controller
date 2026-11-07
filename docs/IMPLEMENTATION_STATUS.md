# Firmware Implementation Status

## Current architecture

- CYD ESP32: operator interface, later touchscreen and self-hosted browser UI.
- Dedicated ESP32-S3: local authority for four temperature channels, four independent zone loops, four digital ON/OFF outputs, profile execution, and safety supervision.
- External relay/SSR equipment is selected by the user. Firmware provides four low-voltage ON/OFF command signals only.

## Implemented in source

- CYD-to-S3 UART protocol v1 with framing, CRC-16/CCITT, sequence numbers, bounded payloads, and parser.
- CYD UART client with HELLO, heartbeat, command send, status receive, and link-health reporting.
- S3 UART server with heartbeat, status snapshots, command validation, acknowledgements, stale-sequence rejection, and safe STOP/PAUSE handling.
- Kiln telemetry uses signed deci-degrees Celsius so the protocol can represent normal ceramic-kiln temperatures.
- Four-channel relay output abstraction with active-high/active-low polarity, disabled/unassigned handling, and inactive startup.
- Hardware-independent ramp/soak profile engine with bounded profile validation, ramp interpolation, measured-temperature arrival wait before soak timing, pause/resume, and completion.
- Four independent hysteresis zone controllers with configurable deadband and minimum on/off dwell.
- Four-channel sensor manager with per-channel thermocouple type selection (K/J/N/R/S/T/E/B), range checks, sample freshness, driver-fault handling, and implausible-rate detection.
- Local latched safety manager for sensor loss, sensor range, overtemperature, and host-link faults.
- Four-zone control core coordinating profile execution, waiting for all four measured zones to reach target before starting soak timers, independent zone control, and fail-safe output masks.
- S3 status telemetry reports sensor-valid bits and latched control-core fault flags.
- Portable host test sources and a reproducible native C++ test script; GitHub Actions runs the portable tests on pushes and pull requests.
- CYD build source filter excludes the obsolete Pro Micro transport from the current S3 architecture.

## Not yet implemented or verified

- Physical thermocouple acquisition driver. The sensor manager is ready to receive readings, but no MAX31856/MAX31865 or other hardware driver is connected.
- Profile storage and profile transfer/configuration from the CYD.
- Verified physical GPIO assignments for the exact S3 board and external relay input interface.
- CYD display/touchscreen UI, Wi-Fi setup, HTTP API, browser authentication, and live web updates.
- Firing history, charts, data export, hardware watchdog integration, and complete end-to-end firing state machine.
- Independent hardware overtemperature cutoff. Firmware must not be treated as a substitute for a separate safety limit device.

## Safety behavior

- START and RESUME remain blocked at the S3 command boundary. The controller cannot start a firing until a real sensor driver, profile transfer, physical output map, and end-to-end interlocks are integrated and validated.
- All relay output pins remain disabled and unassigned in the current build.
- Outputs are initialized inactive and commanded inactive on STOP, PAUSE, and latched faults.
- Loss of the CYD heartbeat during a firing forces all outputs inactive and latches a host-link fault.
- Fault acknowledgement requires all four sensor channels valid, readings below the configured safety limit, and a healthy host link.
- No automatic restart or resume after reset, brownout, sensor loss, or communication failure.

## Validation status

- Portable test sources cover protocol framing, profile timing, zone dwell behavior, safety fault latching, and sensor validation.
- Run them with `sh tests/run_native_tests.sh` on a computer with g++ installed.
- The ESP32-S3 and CYD PlatformIO builds have not yet been run in this environment.
- No physical hardware tests have been performed. The current firmware cannot energize a kiln or start a firing.
