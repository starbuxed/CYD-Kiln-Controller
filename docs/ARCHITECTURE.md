# System Architecture

## Dual-controller design

- **CYD ESP32 main board:** touchscreen, local web server, Wi-Fi, profiles, charts, configuration, history, SD-card assets, startup screen, icons, themes, and operator commands.
- **Dedicated ESP32-S3 secondary board:** four MAX31856 thermocouple channels, four independent control/output channels, local sensor validation, local safety state machine, and physical output control.
- **Wired CAN 2.0A data/control bus:** one external CAN transceiver per board. ESP32 TWAI is a controller peripheral, not a physical transceiver.
- **Independent hardwired shutdown path:** separate from CAN. Either MCU may assert a shutdown request through its own electrically isolated interface. A physical latching E-stop and independent kiln high-limit/power-disconnect hardware must work without either MCU.

This is a development architecture, not a certified kiln safety system. CAN and firmware shutdown requests supplement, but do not replace, independent power-interruption hardware.

## Block diagram

```text
 Phone / Tablet / PC -- local Wi-Fi --> CYD ESP32 MAIN
                                         | UI / Web / SD assets
                                         | Profiles / Graphs / History
                                         | CAN controller + transceiver
                                         | Main-side shutdown request
                                         |
                                         | CANH / CANL / reference GND
                                         | 120-ohm termination at bus ends
                                         v
                                    ESP32-S3 SECONDARY
                                    4 x MAX31856 thermocouples
                                    Local safety supervisor
                                    4 protected output commands
                                    CAN controller + transceiver
                                    Secondary shutdown request
                                         |
                                         v
                                    External relay/SSR input interfaces
                                         |
                                    User-engineered kiln power stage

 HARDWIRED SAFETY (independent of CAN):
 CYD shutdown request -------- Secondary shutdown request ---+--> fail-safe safety circuit / contactor
 Physical latching E-stop ------+
 Independent high-limit -------/
```

## CYD responsibilities

- Touchscreen and locally hosted browser application.
- Wi-Fi, authentication, mDNS, profile editor, SD-card management.
- Startup artwork, icons, themes, firing history, logs, reports.
- Typed operator requests over CAN.
- CAN heartbeat and link health monitor.
- Independent main-side emergency shutdown request.
- Never directly energize secondary relay outputs.

## Secondary ESP32-S3 responsibilities

- Acquire four independent MAX31856 channels.
- Per-channel thermocouple type and calibration.
- Sensor validity, freshness, converter fault, range, and rate checks.
- Local firing/control state machine and four output channels.
- Local temperature limits and interlocks.
- CAN heartbeat monitoring and command validation.
- Safe inactive outputs and latched fault on critical faults or host timeout.
- Independent secondary-side hardwired shutdown request.
- Local safety monitoring continues without the CYD.

## CAN physical layer

Use classic CAN 2.0A (11-bit identifiers) using the ESP32 TWAI peripheral and one external 3.3V-compatible CAN transceiver per board. The transceiver must be selected by its datasheet for MCU logic voltage, bus speed, common-mode range, isolation requirements, and expected electrical environment.

- CANH connects to CANH; CANL connects to CANL.
- Use twisted pair for CANH/CANL.
- Provide a reference ground conductor as appropriate to the isolation design.
- Fit 120-ohm termination only at the two physical ends of a linear bus.
- Keep stubs short.
- Never connect GPIO pins directly as a CAN bus.
- Start with 500 kbit/s only after confirming cable length, topology, and transceiver; make bitrate configurable.
- Use galvanically isolated transceivers/modules and suitable isolated power where installation noise/ground potential requires it.

Exact CYD CAN TX/RX GPIO assignments are intentionally unset until the physical CYD revision and exposed connector pins are confirmed. Do not consume display, touch, or SD pins.

## Independent emergency shutdown

CAN carries ordinary data and commands. Emergency shutdown must not depend on CAN delivery, polling, Wi-Fi, browser, or either MCU's main loop.

Provide:
- Dedicated CYD shutdown-request electrical output/interface.
- Dedicated ESP32-S3 shutdown-request electrical output/interface.
- Physical latching E-stop directly in the safety-contactor control circuit.
- Independent kiln-side high-limit device.
- Fail-safe circuit so either board request can de-energize the safety enable path.
- Optional E-stop/high-limit monitoring inputs to both controllers.
- Latched software alarm identifying trip source.
- Manual inspection/reset after a trip.

Do not connect MCU GPIO outputs together. Use separate isolated/open-collector or otherwise engineered interfaces feeding a safety circuit designed by a qualified person. An MCU pin must not be the sole safety contact. Loss of MCU power/reset/wire should result in the safe state of the engineered circuit.

A touchscreen emergency-stop control is a software shutdown request, not a physical E-stop substitute.

## Control authority

The secondary is authoritative for sensor validity, local interlocks, output switching, and local fault latching. The CYD owns operator experience and supervisory requests. Both boards may request shutdown; neither can clear an independent hardware trip.

| Function | CYD main | ESP32-S3 secondary |
|---|---:|---:|
| Touch UI and web server | Yes | No |
| SD startup screen, icons, themes | Yes | No |
| Profile editing/history | Yes | Optional event buffer |
| Thermocouple acquisition | Receives telemetry | Yes |
| Local sensor validation | Displays status | Yes |
| Local control execution | Supervisory requests | Authoritative |
| Output switching | No | Yes |
| CAN heartbeat | Yes | Yes |
| Independent shutdown request | Yes | Yes |
| Physical E-stop/high-limit | External hardware | External hardware |

## Boot/restart

Both boards boot with output commands inactive. The secondary performs local I/O checks; the CYD loads SD assets and establishes CAN communication. A successful handshake is not authorization to energize outputs. A deliberate operator start is required after readiness checks.

After power loss, watchdog reset, CAN loss, E-stop, high-limit, or critical fault: never auto-resume, keep outputs inactive, preserve recovery information, and require manual inspection/re-arming. Software acknowledgement cannot override active hardware E-stop/high-limit.

## Hardware abstraction

CAN pins/transceiver, sensor SPI pins/chip-selects, relay pins, shutdown interface pins, and polarity remain board-specific. Unverified pins are disabled. Do not assume CYD variants share an accessible GPIO map.
