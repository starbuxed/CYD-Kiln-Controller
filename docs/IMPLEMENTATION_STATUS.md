# Firmware Implementation Status

## Architecture

- CYD ESP32: touchscreen, local browser dashboard, Wi-Fi AP, user/profile settings, recent telemetry and firing summaries.
- ESP32-S3 secondary: four MAX31856 thermocouple inputs, local firing/safety/control state machine, four heater output commands.
- CAN 2.0A/TWAI: inter-board transport with one external 3.3 V transceiver per board.
- Master-power interlock: one independent permit relay from each board; normally-open contacts in series.
- Physical latching E-stop and independent kiln high-limit remain hardware interlocks.

## Implemented software foundations

- Classic CAN transport, fragmentation/reassembly, heartbeat and timeout behavior.
- CRC-protected typed protocol, sequence validation, configuration and command messages.
- Four-channel sensor manager with Off, Log only, Log and control, Log and safety modes.
- All non-Off sensors are included in the kiln-wide safety sensor mask; only Log and control sensors are eligible for PID/control.
- MAX31856 driver abstraction and per-channel thermocouple type selection.
- Four output abstractions with active-high/active-low configuration and inactive-by-default GPIO handling.
- Ramp/soak profile engine and four independent hysteresis zone controllers.
- Sensor/zone/output mapping validation module (not yet integrated into firing core).
- Separate master-power permit abstraction on both MCUs.
- CYD touchscreen for live status, profile entry, sensor settings, zone hysteresis, controller diagnostics and touch calibration.
- CYD-hosted WPA2 access point and local browser dashboard with HTTP Basic Authentication and CSRF checks on write requests.
- Browser dashboard with temperature graph, profile upload, firing-summary history, CSV exports, alerts, scheduled reminders, estimated energy/cost, isolated simulation preview and offline PID diagnostic/advisor tools.
- Portable feature modules for telemetry buffering, history records, schedule queue, alerts, energy estimate, simulation and PID advisory calculations.
- Native tests cover protocol, CAN fragmentation, control core, sensor manager, zone mapping, telemetry/history, estimated energy, reminders, alert ledger, simulation and PID advisory math.

## Validation status

On the corrected ArduinoJson dependency revision, GitHub Actions passed:
- Portable native test suite.
- ESP32-S3 firmware build.
- Classic CYD touchscreen and web firmware build.

The immediately following change only updates hardware documentation. No physical hardware has been tested.

## Hardware still not assigned or verified

- Exact CYD model/revision in the user's possession.
- ESP32-S3 DevKitC-1 physical module variant/pinout.
- MAX31856 SPI SCLK/MISO/MOSI GPIOs and four separate chip-select GPIOs.
- Four heater-output GPIOs and their external driver circuit.
- CYD master-power permit output pin (disabled by default).
- Actual MAX31856 modules, thermocouples, CAN transceivers, relay/SSR modules and control power.
- CAN cabling, termination, EMC and ground/isolation scheme.
- Relay/contactor auxiliary feedback.

## Functional work remaining

- Make S3 safety evaluation explicitly continuous while idle as well as during firing/paused states; verify that every enabled sensor's invalid/stale/faulted condition is handled exactly as intended.
- Integrate configurable sensor-to-zone-to-output mapping into the live S3 control core.
- Implement and test real PID zone control if PID is still the intended control algorithm. Current zone loop remains hysteresis-based; the new PID tools are diagnostic/advisory only.
- Persist full S3-side sensor, zone, profile and safety configuration with versioning and validation after reboot.
- Add profile names/library management, read-back and explicit controller acknowledgements.
- Persist detailed per-firing telemetry. Current graph history is a bounded RAM ring; firing summaries are persisted in NVS.
- Persist and review alert/event history; implement any alert output hardware (buzzer/beacon) that is selected.
- Implement SD startup graphics/assets, themes/icons and asset management.
- Implement Wi-Fi station-mode onboarding, credential-management UX and optional stronger LAN security/TLS.
- Complete browser profile editor, live push updates (WebSocket/SSE), richer firing detail and report export.
- Add relay feedback and contactor auxiliary-contact monitoring where supported.

## Hardware commissioning still required

- Verify actual board revisions and every GPIO against schematics.
- Bench-test CAN with the two actual transceiver modules.
- Bench-test all four thermocouple channels with known probes and open/short/wrong-type fault cases.
- Test all four output channels into low-voltage indicator loads and verify reset/boot/brownout/fault inactive behavior.
- Test each master permit independently, CAN-loss drop-out, controller power loss, watchdog/fault behavior, and the series NO contact path.
- Test physical E-stop and independent high-limit directly interrupt the hardware safety path with both MCUs disconnected.
- Have the final contactor/safety-relay and power circuit reviewed and commissioned by a qualified electrical professional.

## Safety gate

The ESP32 GPIOs must never switch kiln mains or bare relay coils directly. The S3's four heater GPIOs and MAX31856 pins remain disabled pending exact hardware configuration. CYD K1 remains disabled in the default build. Do not connect kiln heating power. This is experimental firmware, not a certified kiln controller.
