# Firmware Implementation Status

## Existing prototype

The repository contains an initial CYD board-profile catalogue and a legacy Pro Micro heartbeat prototype. The Pro Micro is retained for reference only and is no longer the target I/O controller.

## Current target architecture

- CYD ESP32: touchscreen, local web server, profile editing, configuration, charts, and history.
- Dedicated ESP32-S3: local authority for four temperature channels, four independent zone loops, and four digital ON/OFF output commands.
- Four thermocouple channels with per-channel type configuration.
- Four independent outputs with configurable active-high/active-low polarity.
- User selects and supplies the external relay/SSR equipment. This project provides compatible low-voltage control signals, not the kiln power stage.

## Added

- Updated architecture, hardware, decision, safety, roadmap, protocol, and README documentation for CYD + dedicated ESP32-S3.
- Added a PlatformIO target and non-firing startup scaffold in `firmware/controller_s3/`.
- Added a four-zone configuration example in `shared/schemas/four-zone-config.example.json`.
- Retained the Pro Micro firmware as a legacy prototype.

## Important assumptions and open details

- "Mixed" currently means different thermocouple types across the four channels. Each channel has an independent type setting.
- MAX31856 is a candidate thermocouple interface, not an RTD interface. PT100/PT1000 channels need a separate RTD front end such as MAX31865.
- Exact CYD and ESP32-S3 board revisions, sensor modules, GPIO pin map, and relay-module input interface remain to be verified.
- Kiln mains voltage, phase, and heater current are not needed to implement the logical ON/OFF commands; the user is selecting the external relay/power equipment.
- Before connecting a particular relay module, its control-input voltage/current, active polarity, and isolation requirements must be checked so the low-voltage interface can be matched.

## Not implemented yet

- Sensor front-end drivers and four-channel temperature acquisition.
- Four independent zone-control loops.
- Four GPIO output channels and polarity configuration.
- Output interface hardware and optional feedback.
- CYD-to-S3 communications implementation.
- Dedicated controller watchdog/fault manager.
- CYD display/touch integration, web server, authentication, profiles, and firing history.

## Validation status

The ESP32-S3 firmware is currently only a non-firing startup scaffold. No sensor pins or output pins are assigned, and there is no code path to switch relay modules yet.

The earlier standalone protocol test was compiled with g++ using C++11 and passed round-trip, CRC-corruption, and oversized-payload checks. No PlatformIO build or physical hardware test has been run for the new ESP32-S3 target.
