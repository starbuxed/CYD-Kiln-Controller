# CYD Kiln Controller

A locally hosted, touchscreen-operated ceramic kiln controller built around a Cheap Yellow Display (CYD) ESP32 and an Arduino Pro Micro I/O coprocessor.

The ESP32 is the user-interface, profile-management, logging, and local web-server host. The Pro Micro provides a dedicated I/O layer for sensor acquisition and relay-output control. The final division of responsibilities, sensor interfaces, and hardware pin assignments must be confirmed against the exact board revisions before hardware is energized.

> **Safety notice:** This repository is a development project, not a certified kiln controller. Kiln installations involve lethal voltage, high current, extreme temperatures, fire risk, and potentially hazardous failure modes. Software, a microcontroller, and ordinary PCB relays must not be treated as substitutes for correctly rated contactors, independent over-temperature protection, fusing, grounding, enclosure design, and code-compliant installation. Have the power system designed and inspected by a qualified professional. Never test initial firmware with heating elements connected.

## Project goals

- CYD ESP32 touchscreen interface.
- Up to four independently monitored kiln temperature channels.
- Four configurable relay/output channels.
- Ramp/soak firing profiles with validation and history.
- Self-hosted responsive web application and versioned local API.
- Live temperature and setpoint charts.
- Local operation without cloud hosting or subscriptions.
- Explicit fault handling, communication watchdogs, and safe output defaults.
- No automatic firing restart after a reset, fault, or communication outage.

## Architecture at a glance

- **ESP32/CYD:** display, user interaction, profile execution/orchestration, web server, network management, history, configuration, and supervisory state machine.
- **Pro Micro:** dedicated I/O coprocessor for the configured sensors and outputs, with a communication watchdog and fail-safe output behavior.
- **Independent safety hardware:** high-limit cutoff and appropriately rated power switching must remain effective without either microcontroller or the network.

See [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md), [docs/SAFETY.md](docs/SAFETY.md), and [docs/PROTOCOL.md](docs/PROTOCOL.md).

## Repository layout

- `firmware/esp32/` — ESP32/CYD firmware.
- `firmware/promicro/` — Pro Micro I/O firmware.
- `web/` — browser application assets served by the ESP32.
- `shared/` — protocol definitions and shared schemas.
- `docs/` — architecture, safety, hardware, and API documentation.
- `profiles/` — example firing profiles.
- `tests/` — protocol, profile-validation, and state-machine tests.

## Current status

This is the initial specification and scaffolding stage. Hardware-dependent pin maps, sensor drivers, electrical interface details, and firmware implementation are intentionally not guessed. Confirm the exact CYD model/revision, Pro Micro voltage/clock variant, temperature sensor type, and relay/driver hardware before implementation.

## Development principles

1. Keep the local safety path independent of Wi-Fi and browser behavior.
2. Reject invalid, stale, or out-of-range sensor data.
3. Treat communications loss as a fault; outputs must move to their defined safe state.
4. Never replay a queued start/resume command after reconnection.
5. Validate profiles before saving and again before firing.
6. Log faults, state transitions, and operator commands.
7. Keep dangerous high-power switching outside low-voltage logic circuitry.

## License

No license has been selected yet. Do not assume this project is open for reuse until a license is added.
