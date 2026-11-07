# ESP32 / CYD Firmware

This directory will contain the CYD application firmware.

## Planned modules

- `app/` — application startup and dependency wiring.
- `display/` — CYD display and touch abstraction.
- `network/` — Wi-Fi station/AP provisioning and mDNS.
- `web/` — local HTTP server, API, and live event transport.
- `profiles/` — profile storage, schema validation, and ramp/soak scheduler.
- `control/` — supervisory state machine and command validation.
- `io/` — Pro Micro serial client and telemetry parser.
- `storage/` — persistent settings, history, and event log.
- `safety/` — sensor freshness, fault handling, watchdog supervision.

## Board configuration

Do not hard-code a CYD pin map until the exact CYD model/revision is selected. Different CYD variants and clones can differ in display/touch controller, GPIO wiring, flash, and PSRAM. Keep board-specific settings in one dedicated configuration header and document the verified schematic source.

The initial host build target should be a generic ESP32 compile/test target only. It is not evidence that the selected CYD display/touch hardware is configured correctly.

## Build acceptance

- Clean compile with pinned toolchain/dependency versions.
- Unit tests for profile validation and state transitions.
- UART parser tests with corrupt, truncated, and stale frames.
- Simulation mode must be available before any kiln power hardware is connected.
