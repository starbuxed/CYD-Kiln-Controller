# CYD Kiln Controller

Locally hosted ceramic kiln controller with a CYD ESP32 operator interface and a dedicated ESP32-S3 kiln I/O/control processor.

## Architecture

- **CYD main:** touchscreen, locally hosted browser dashboard, Wi-Fi access point, profiles, recent telemetry, history, schedules, alerts, and energy estimates.
- **ESP32-S3 secondary:** four MAX31856 thermocouple channels, four independent output commands, local firing/control/safety.
- **CAN bus:** wired inter-controller communications using ESP32 TWAI and one external CAN transceiver per board.
- **Dual master-power permit:** one relay from each MCU; normally-open contacts wired in series so both boards must permit the main contactor.
- **Independent safety:** physical latching E-stop and independent kiln-side high-limit interrupt the contactor control path without CAN or firmware.

> **Safety notice:** Experimental development firmware, not a certified kiln controller. Keep kiln power disconnected during software bring-up. Never drive bare relay coils or heater loads directly from ESP32 GPIO. Independent E-stop, high-limit, contactor, fusing, grounding, and power isolation are mandatory.

## Touchscreen and browser interfaces

The first touchscreen target is the classic 2.8-inch ILI9341 + XPT2046 CYD profile. It includes Home, Profile, Sensors, Zones, Info, and touch calibration.

The CYD now also starts a local WPA2 access point and browser dashboard. The serial monitor prints its generated SSID/password and the browser URL. Pages include live readings/graphs, profile upload, firing history, alerts, scheduled reminders, energy estimates, isolated simulation, and offline PID diagnostics.

See [Touchscreen Interface](docs/TOUCH_UI.md), [Web Application](docs/WEB_APP.md), and [Feature Reference](docs/REFERENCE_FEATURES.md) for hardware scope and limitations. Other CYD variants remain disabled until their display/touch pin maps are verified.

## Goals

- CYD touchscreen and self-hosted browser interface.
- SD startup art, icons, themes, profiles, logs, backups, reports.
- Dedicated ESP32-S3 with four MAX31856 thermocouple inputs.
- Four independent digital output commands.
- CAN 2.0A with heartbeats and fault handling.
- Separate master-power permit relay from each MCU; both normally-open contacts are wired in series.
- Physical latching E-stop and independent high-limit interrupt the safety-control path independently.
- Ramp/soak, cone-fire, candling, controlled cooling, PID/zone control, alarms, history.
- Local operation without cloud/subscription.
- Safe inactive outputs on boot/reset/link loss/fault.
- No automatic firing restart.

## Repository layout

- \`firmware/esp32/\` — CYD main firmware, touchscreen, web server, and feature utilities.
- \`firmware/controller_s3/\` — secondary ESP32-S3 firmware.
- \`shared/\` — CAN transport and protocol.
- \`web/\` — browser application notes (current dashboard is embedded in firmware).
- \`assets/\` — default startup graphics/icons (pending).
- \`docs/\` — architecture, board, safety, hardware, protocol, CAN wiring, master-power interlock, touchscreen, web API, and feature reference.
- \`tests/\` — portable tests.

## Status

The project now has a compiling touchscreen, local browser dashboard, feature utility modules, native tests, and an S3/CAN foundation. Physical board pin maps, thermocouple and heater I/O assignments, full PID control, long-term detailed firing logs, Wi-Fi station onboarding, and the independent safety circuit still need work and verification.

**Do not connect kiln heating power.** The controller has not had physical end-to-end testing or qualified electrical safety commissioning. Simulation and low-voltage indicator loads only.

## Reference project

Feature ideas were independently implemented after reviewing [jbruce12000/kiln-controller](https://github.com/jbruce12000/kiln-controller). Its source code has not been copied. Review its GPL-3.0 notice before adapting any code from it.

## License

No license has been selected for this repository yet.
