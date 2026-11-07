# Firmware Implementation Status

## Current architecture

- CYD ESP32: touchscreen, local web server, profiles, configuration, charts, history, and operator requests.
- Dedicated ESP32-S3: local authority for four temperature channels, four independent zone loops, four digital ON/OFF outputs, and safety supervision.
- External relay/SSR equipment is selected by the user; this project supplies only compatible low-voltage ON/OFF control signals.

## Implemented

- CYD-to-S3 shared UART protocol v1 with framing, CRC-16/CCITT, sequence numbers, bounded payloads, and parser.
- CYD UART client with HELLO, heartbeat, command send, status receive, and link-health reporting.
- S3 UART server with heartbeat, status snapshots, command validation, acknowledgements, stale-sequence rejection, and safe STOP/PAUSE handling.
- START and RESUME are deliberately rejected as NOT_READY until profile validation, sensor acquisition, and safety interlocks exist.
- Four-channel relay output software abstraction with active-high/active-low polarity and disabled/unassigned output handling.
- Host-side protocol and relay configuration tests.

## Not implemented

- CYD display/touchscreen UI and local web application.
- Wi-Fi setup, HTTP API, browser authentication, and live web updates.
- Profile storage, profile transfer, and ramp/soak execution.
- Thermocouple front-end drivers and four-channel temperature acquisition.
- Four independent zone-control loops.
- Fault manager, watchdog supervision, and complete safety interlocks.
- Physical GPIO assignments for the S3 and validated external relay interface.
- Firing history and data export.

## Hardware-independent development

The firmware is being structured so the controller state machine, protocol, profile validation, and control algorithms can be host-tested without either controller board. Board-specific GPIO, sensor drivers, and display/touch drivers remain behind hardware interfaces.

## Validation status

The protocol and output logic have host-side test sources. No PlatformIO build, ESP32 board build, or physical hardware test has yet been run. All S3 relay outputs remain disabled and unassigned. The current S3 firmware cannot start a firing.
