# CYD Kiln Controller

Locally hosted ceramic kiln controller with a CYD ESP32 operator interface and a dedicated ESP32-S3 kiln I/O/control processor.

## Architecture

- **CYD main:** touchscreen, local web application, Wi-Fi, profiles, graphs, history, SD-card files, startup screen, icons, themes, operator commands.
- **ESP32-S3 secondary:** four MAX31856 thermocouple channels, four independent low-voltage output commands, local control logic, local safety supervision.
- **CAN bus:** wired inter-controller communications using ESP32 TWAI and one external CAN transceiver per board.
- **Dual master-power permit:** one relay from each MCU; their normally-open contacts are wired in series, so both boards must permit the main contactor.
- **Independent safety:** physical latching E-stop and independent kiln-side high-limit interrupt the safety-control path without CAN or firmware.

> **Safety notice:** Experimental development firmware, not a certified kiln controller. Keep kiln power disconnected during software bring-up. Never drive bare relay coils or heater loads directly from ESP32 GPIO. Independent E-stop, high-limit, contactor, fusing, grounding, and power isolation are mandatory.

## Goals

- CYD touchscreen and self-hosted browser interface.
- SD startup art, icons, themes, profiles, logs, backups, reports.
- Dedicated ESP32-S3 with four MAX31856 thermocouple inputs.
- Four independent digital output commands.
- CAN 2.0A with heartbeats and fault handling.
- Separate master-power permit relay from each MCU; both normally-open contacts are wired in series so both boards must permit contactor operation. The opt-in 2.8-inch CYD build uses onboard GPIO16 (green LED channel) as its permit output; a soldered connection and buffer are required.
- Physical latching E-stop and independent high-limit interrupt the safety-control path independently.
- Ramp/soak, cone-fire, candling, controlled cooling, PID/zone control, alarms, history.
- Local operation without cloud/subscription.
- Safe inactive outputs on boot/reset/link loss/fault.
- No automatic firing restart.

## Repository layout

- `firmware/esp32/` — CYD main firmware.
- `firmware/controller_s3/` — secondary ESP32-S3 firmware.
- `shared/` — shared protocol.
- `web/` — local browser application (pending).
- `assets/` — default startup graphics/icons (pending).
- `profiles/` — example profiles.
- `docs/` — architecture, board, safety, hardware, protocol, CAN wiring, and master-power interlock.
- `tests/` — portable tests.

## Status

The repository has a safety-oriented foundation: ramp/soak, four-zone control, sensor manager, MAX31856 driver abstraction, relay abstraction, UART scaffolding, and native tests. This branch records the CAN migration and independent shutdown requirements. Full CAN firmware, UI/web, SD asset manager, and physical safety circuit have not been implemented/validated yet.

Do not connect kiln heating power. Test with simulation and low-voltage indicators until hardware and independent safety circuit are reviewed.

## License

No license has been selected yet.
