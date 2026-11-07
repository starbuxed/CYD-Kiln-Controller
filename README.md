# CYD Kiln Controller

A locally hosted ceramic kiln controller with a CYD ESP32 operator interface and a dedicated ESP32-S3 control processor.

The CYD provides the touchscreen, Wi-Fi, local web application, profile editing, charts, and history. The dedicated ESP32-S3 reads four temperature channels and operates four independent digital ON/OFF control outputs. The user selects and supplies the external relay/SSR/power-switching equipment; this project focuses on temperature control and four compatible low-voltage relay-control signals.

The design assumes mixed thermocouple types configurable per channel. RTD channels require a different sensor front end.

> **Safety notice:** This is a development project, not a certified kiln controller. The ESP32 GPIOs must not directly drive bare relay coils or heater loads. Use a compatible protected/isolated interface for the external relay modules. Independent kiln-side over-temperature and power protection remain necessary. Never test initial firmware with heating elements connected.

## Project goals

- CYD touchscreen and self-hosted browser interface.
- Dedicated ESP32-S3 local control processor.
- Four independently configurable temperature channels.
- Mixed thermocouple types selectable per channel.
- Four independent zone-control loops.
- Four digital ON/OFF relay-control outputs with configurable polarity.
- Ramp/soak firing profiles, alarms, and history.
- Local operation without cloud hosting or subscriptions.
- Safe inactive outputs on boot, reset, communications loss, and critical faults.
- No automatic firing restart after interruption.

## Architecture

- **CYD ESP32:** touchscreen, web server, Wi-Fi, profiles, configuration, history, and operator commands.
- **Dedicated ESP32-S3:** four sensor channels, four control loops, four digital output commands, and local fault supervision.
- **External relay/SSR hardware:** selected and supplied by the user. The controller only issues low-voltage ON/OFF commands through compatible interface circuitry.

See [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md), [docs/HARDWARE.md](docs/HARDWARE.md), [docs/SAFETY.md](docs/SAFETY.md), and [docs/PROTOCOL.md](docs/PROTOCOL.md).

## Repository layout

- `firmware/esp32/` — CYD operator-interface firmware.
- `firmware/controller_s3/` — dedicated ESP32-S3 controller firmware.
- `web/` — browser application served by the CYD.
- `shared/` — protocol definitions and schemas.
- `docs/` — architecture, safety, hardware, and API documentation.
- `profiles/` — example firing profiles.
- `tests/` — protocol and control tests.

## Current status

The repository contains initial board-selection, protocol, and ESP32-S3 startup scaffolding. Four-channel sensor acquisition, control loops, physical output-interface drivers, web app, and firing history remain to be implemented and validated. The firmware can be developed against generic ON/OFF outputs; external relay-module compatibility must be confirmed before connecting that hardware.

## Development principles

1. Keep local control and fault handling independent of Wi-Fi/browser behavior.
2. Reject invalid, stale, wrong-type, or out-of-range sensor data.
3. Force all four digital outputs inactive on critical faults.
4. Never replay a queued start/resume command after reconnection.
5. Validate all four zone configurations before firing.
6. Distinguish commanded output state from verified physical state.
7. Keep hazardous power switching outside low-voltage logic circuitry.
8. Do not automatically resume after reset or interruption.

## License

No license has been selected yet.
