# CYD ESP32 Operator-Interface Firmware

This directory is the firmware target for the CYD touchscreen and local web-server device. It is not the authoritative kiln-control processor.

## Responsibilities

- Touchscreen and display.
- Wi-Fi station/access-point setup and local web server.
- Browser API, dashboard, profiles, history, and event log.
- Operator command validation and presentation.
- Framed UART client for the dedicated ESP32-S3 controller.

The CYD UART client is implemented in include/controller_link.h and src/controller_link.cpp. It sends HELLO and periodic heartbeats, sends typed operator commands, receives controller status and acknowledgements, and reports link health. It does not write heater GPIOs.

The dedicated controller firmware lives in firmware/controller_s3/. The S3 owns the local kiln state machine, sensor acquisition, output requests, and safety interlocks.

## Board configuration

Do not hard-code a CYD pin map until the exact CYD model/revision is selected. Different CYD variants and clones can differ in display/touch controller, GPIO wiring, flash, and PSRAM. The current UART2 pin mapping is only enabled for supported classic 2.8-inch CYD profiles.

## Build acceptance

- Clean compile with pinned toolchain/dependency versions.
- Host tests for protocol, profile validation, and command validation.
- UART parser tests with corrupt, truncated, and stale frames.
- Simulation mode must be available before any kiln power hardware is connected.
