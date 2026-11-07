# Firmware Implementation Status

## Existing prototype increment

The repository contains an initial CYD board-profile catalogue and a legacy Pro Micro heartbeat prototype. Those files are retained as historical bring-up work; the Pro Micro is no longer the target I/O controller.

## Current target architecture

- CYD ESP32: operator touchscreen, local web server, profile editing, configuration, charts, and history.
- Dedicated ESP32-S3: local authority for four sensor channels, four independent heater zones, four low-voltage output requests, and local safety supervision.
- Four independently configurable thermocouple channels, with per-channel type selection.
- Four independent zone-control loops, mapped one-to-one to sensor channels and heater outputs by default.
- Independent hardware high-limit and final power-disconnect chain.

## Added in this architecture revision

- Rewrote `docs/ARCHITECTURE.md` around CYD + dedicated ESP32-S3.
- Rewrote `docs/HARDWARE.md` for four zones, configurable thermocouples, output isolation, and unknown kiln supply.
- Updated `docs/DECISIONS.md`, `docs/SAFETY.md`, `docs/ROADMAP.md`, `README.md`, and `docs/PROTOCOL.md`.
- Added a PlatformIO target and safe startup scaffold in `firmware/controller_s3/`.
- Added `shared/schemas/four-zone-config.example.json` with four unconfigured sensor channels, four zones, and four inactive outputs.
- Kept the legacy Pro Micro firmware directory for reference; it is not the planned controller.

## Important hardware assumptions and open questions

- "Mixed" is currently interpreted as different thermocouple types across the four channels. Each channel has its own thermocouple-type setting.
- MAX31856 is a candidate thermocouple interface, not an RTD interface. Any PT100/PT1000 channel requires a separate RTD front end such as a MAX31865-class device and separate validation.
- The kiln supply was reported as "other / not sure". Voltage, frequency, phase, current, power, and heater circuit ratings remain unknown. No SSR, contactor, fuse, conductor, or disconnect ratings have been selected.
- Exact CYD and ESP32-S3 board revisions, pin maps, sensor modules, output driver circuits, and independent safety hardware remain to be selected and verified.

## Not implemented yet

- Sensor front-end drivers and four-channel temperature acquisition.
- Four independent PID/time-proportioning loops.
- Output-driver GPIO mapping, hardware feedback, and driver fault detection.
- CYD-to-S3 framed communications implementation.
- Dedicated controller watchdog/fault manager.
- CYD display/touch integration, web server, authentication, profiles, and firing history.
- Complete electrical drawings and independent high-limit/power-disconnect design.

## Safety and validation status

The new ESP32-S3 firmware is only a non-firing startup scaffold. No sensor pins or heater output pins are assigned, and there is no code path to energize a heater output.

The earlier standalone protocol test was compiled with g++ using C++11 and passed its round-trip, CRC-corruption, and oversized-payload checks. No PlatformIO build or physical hardware test has been run for the new ESP32-S3 target. Do not connect energized kiln outputs.
