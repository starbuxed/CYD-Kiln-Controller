# CYD Kiln Controller

A locally hosted, touchscreen-operated ceramic kiln controller with a CYD ESP32 operator interface and a dedicated ESP32-S3 kiln-control processor.

The CYD provides the touchscreen, Wi-Fi, local web application, profile editing, charts, and history. A separate ESP32-S3 is the local control authority for four temperature channels and four independently controlled heater zones. The Pro Micro is no longer the planned I/O controller. The design assumes mixed thermocouple types configurable per channel; RTD channels require a different sensor front end.

> **Safety notice:** This repository is a development project, not a certified kiln controller. Kiln installations involve lethal voltage, high current, extreme temperatures, fire risk, and potentially hazardous failure modes. Software and ordinary microcontroller boards are not substitutes for correctly rated contactors, independent over-temperature protection, fusing, grounding, enclosure design, and code-compliant installation. Have the power system designed and inspected by a qualified professional. Never test initial firmware with heating elements connected.

## Project goals

- CYD ESP32 touchscreen and self-hosted browser interface.
- Dedicated ESP32-S3 local kiln controller.
- Four individually configurable temperature channels.
- Mixed thermocouple types selectable per channel.
- Four independently controlled heater circuits/zones.
- Four independent local control loops with zone limits and fault monitoring.
- Ramp/soak firing profiles and history.
- Local operation without cloud hosting or subscriptions.
- Explicit fault handling, communication watchdogs, and safe output defaults.
- No automatic firing restart after reset, fault, or communication outage.

## Architecture at a glance

- **CYD ESP32:** touchscreen, web server, Wi-Fi, profile editor, configuration, history, and operator commands.
- **Dedicated ESP32-S3:** four sensor front ends, four zone-control loops, four isolated low-voltage output requests, local safety supervision, and communication with the CYD.
- **Independent safety hardware:** high-limit protection and appropriately rated final power switching must remain effective without either microcontroller or the network.

See [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md), [docs/HARDWARE.md](docs/HARDWARE.md), [docs/SAFETY.md](docs/SAFETY.md), [docs/PROTOCOL.md](docs/PROTOCOL.md), and [docs/CYD_BOARD_SELECTION.md](docs/CYD_BOARD_SELECTION.md).

## Repository layout

- `firmware/esp32/` — CYD operator-interface firmware.
- `firmware/promicro/` — legacy Pro Micro prototype; not the target control architecture.
- `firmware/controller_s3/` — dedicated ESP32-S3 controller firmware (planned).
- `web/` — browser application assets served by the CYD.
- `shared/` — protocol definitions and shared schemas.
- `docs/` — architecture, safety, hardware, and API documentation.
- `profiles/` — example firing profiles.
- `tests/` — protocol, profile-validation, and state-machine tests.

## Current status

The repository contains initial CYD board-selection and serial-protocol scaffolding. The four-zone ESP32-S3 architecture is now the target design, but the dedicated controller firmware, sensor acquisition, PID loops, output drivers, web app, and hardware safety chain are not yet implemented or validated. The kiln supply is unknown, so no mains power components, SSR ratings, contactor ratings, fuses, or wire sizes have been selected.

## Development principles

1. Keep the local safety path independent of Wi-Fi and browser behavior.
2. Reject invalid, stale, wrong-type, or out-of-range sensor data.
3. Treat communications loss as a fault; outputs must move to their defined safe state.
4. Never replay a queued start/resume command after reconnection.
5. Validate all four zone configurations and profiles before firing.
6. Log faults, state transitions, and operator commands.
7. Keep hazardous power switching outside low-voltage logic circuitry.
8. Preserve independent hardware high-limit and final power disconnect.

## License

No license has been selected yet. Do not assume this project is open for reuse until a license is added.
