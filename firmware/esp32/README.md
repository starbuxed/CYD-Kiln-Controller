# CYD ESP32 Operator-Interface Firmware

This directory is the firmware target for the CYD touchscreen and local web-server device. It is not the authoritative heater-control processor.

## Responsibilities

- `display/` — CYD display and touch abstraction.
- `network/` — Wi-Fi station/AP provisioning and mDNS.
- `web/` — local HTTP server, API, and live event transport.
- `profiles/` — profile storage, schema validation, and editor.
- `control/` — operator request handling and UI-side state presentation.
- `io/` — framed communications client for the dedicated ESP32-S3 controller.
- `storage/` — persistent settings, history, and event log.

The dedicated controller firmware lives in [`firmware/controller_s3/`](../controller_s3/). The CYD must never directly drive heater outputs or bypass the S3 controller's local interlocks.

## Board configuration

Do not hard-code a CYD pin map until the exact CYD model/revision is selected. Different CYD variants and clones can differ in display/touch controller, GPIO wiring, flash, and PSRAM. Keep board-specific settings in one dedicated configuration header and document the verified schematic source.

## Build acceptance

- Clean compile with pinned toolchain/dependency versions.
- Unit tests for profile validation and UI command validation.
- UART parser tests with corrupt, truncated, and stale frames.
- Simulation mode must be available before any kiln power hardware is connected.
