# Firmware Implementation Status

## Architecture

- CYD ESP32: operator UI, web server, Wi-Fi, SD-card assets, profiles, charts, and history.
- ESP32-S3 secondary: four MAX31856 inputs, local control/safety logic, four digital output commands.
- Intended wired transport: CAN 2.0A with external transceiver per board.
- Independent hardwired shutdown request from each MCU plus physical latching E-stop and independent kiln high-limit.

## Existing source foundation

- UART protocol scaffolding with CRC, sequencing, heartbeat, status and command validation.
- CYD UART client and S3 UART server.
- Four-channel output abstraction, all pins unassigned/disabled by default.
- Ramp/soak engine, four-zone hysteresis controller, sensor manager, local fault latch.
- MAX31856 SPI abstraction with per-channel thermocouple type selection.
- Native tests and GitHub Actions.

## Remaining / not yet verified

- ESP32 PlatformIO builds and on-device CAN integration tests.
- Exact secondary-board CAN TX/RX pins, transceivers, cable, topology, and termination validation.
- Main-board CAN GPIO27/22 are configured only for the default classic 2.8-inch profile; the actual board header must still be checked.
- Independent hardwired shutdown interface implementation for both boards.
- Physical E-stop/high-limit/contactor circuit design and verification.
- MAX31856 SPI and CS pins pending exact S3 board.
- Relay output pins pending exact secondary board and external interface.
- CYD display/touch UI, splash renderer, SD asset loader, icon/theme manager.
- CYD Wi-Fi, HTTP API, auth, WebSocket/live updates.
- Profile persistence/editor, history, graphs, exports.
- Watchdog integration and end-to-end simulation.
- No physical hardware tests have been performed; no kiln firing is enabled.

## Safety gate

START/RESUME must remain blocked unless verified interlocks exist. Default build with unassigned CAN, sensor, output, or shutdown pins must not arm. This branch is not ready for live kiln hardware.
