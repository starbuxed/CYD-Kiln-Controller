# CYD Kiln Controller

Locally hosted ceramic kiln controller with a CYD ESP32 operator interface and a dedicated ESP32-S3 kiln I/O/control processor.

## Architecture

- **CYD main:** touchscreen, local web application, Wi-Fi, SD profiles/logs, user settings.
- **ESP32-S3 secondary:** four MAX31856 thermocouple channels, four independent output commands, local firing/control/safety.
- **CAN bus:** wired inter-controller communications using ESP32 TWAI and one external CAN transceiver per board.
- **Dual master-power permit:** one relay from each MCU; normally-open contacts wired in series so both boards must permit the main contactor.
- **Independent safety:** physical latching E-stop and independent kiln-side high-limit interrupt the contactor control path without CAN or firmware.

> **Safety notice:** Experimental development firmware, not a certified kiln controller. Keep kiln power disconnected during software bring-up. Never drive bare relay coils or heater loads directly from ESP32 GPIO. Independent E-stop, high-limit, contactor, fusing, grounding, and power isolation are mandatory.

## Touchscreen

The first touchscreen implementation targets the classic 2.8-inch ILI9341 + XPT2046 CYD profile. It includes Home, Profile, Sensors, Zones, Info, and touch calibration screens. See [Touchscreen Interface](docs/TOUCH_UI.md) for supported hardware, screen behavior, and limitations. Other CYD variants remain disabled until their display/touch pin maps are verified.

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

- `firmware/esp32/` — CYD main firmware and touchscreen.
- `firmware/controller_s3/` — secondary ESP32-S3 firmware.
- `shared/` — shared protocol.
- `web/` — local browser application (pending).
- `assets/` — default startup graphics/icons (pending).
- `docs/` — architecture, board, safety, hardware, protocol, CAN wiring, master-power interlock, and touchscreen.
- `tests/` — portable tests.

## Status

The repository has a safety-oriented firmware foundation and an initial touchscreen operator interface. The embedded web application, SD asset manager, expanded persistence, exact hardware configuration, and physical safety circuit are not complete or validated. Keep kiln heating power disconnected until the hardware and independent safety circuit are reviewed and commissioned.

## License

No license has been selected yet.
