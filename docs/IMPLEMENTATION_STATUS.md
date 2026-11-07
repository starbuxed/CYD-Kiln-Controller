# Firmware Implementation Status

## Architecture

- CYD ESP32: main operator interface, web server, Wi-Fi, SD-card assets, profiles, charts, and history.
- ESP32-S3 secondary: four MAX31856 inputs, local control/safety logic, four heater output commands.
- CAN 2.0A/TWAI: wired transport using one external transceiver on each board.
- Master-power interlock: dedicated permit relay output from each board; normally-open contacts in series.
- Physical latching E-stop and independent kiln high-limit remain hardware interlocks.

## Existing control foundation

- Ramp/soak profile engine and four-zone hysteresis controller.
- Four-channel sensor manager with modes Off, Log only, Log and control, Log and safety.
- Combined safety monitoring for all sensors not set Off.
- MAX31856 driver abstraction with per-channel thermocouple type selection.
- Four low-voltage output abstractions; hardware GPIO mapping disabled pending verification.
- Latched local safety fault management and no automatic restart.
- Existing CRC-protected typed controller protocol and native tests.

## Added on this branch

- Classic CAN 2.0A TWAI stream adapter for ESP32 main and S3 secondary.
- Protocol fragmentation/reassembly for classic CAN's 8-byte frame payload.
- Per-CAN-identifier receive reassembly streams to avoid interleaved message corruption.
- Heartbeats, CAN bus-health checks, timeout behavior, and existing command sequence checks retained over CAN.
- Latched emergency-stop protocol command.
- Independent master-power permit output abstraction on both boards.
- CYD permit is enabled only when fresh S3 status says FIRING, CAN/heartbeat are healthy, and no fault is reported.
- S3 permit is enabled only while locally FIRING, configured local heater outputs exist, safety sensors are valid, no control fault is latched, and CAN/host heartbeat is healthy.
- Optional classic 2.8-inch CYD build profile uses onboard RGB GPIO16 as the master-permit driver signal; disabled in the default build.
- Documentation for CAN wiring and series permit-contact architecture.
- Native CAN fragmentation/reassembly test.

## Not implemented or not yet verified

- Full CYD touchscreen UI and display/touch driver.
- Startup-screen renderer, SD asset loader, icon/theme manager.
- CYD Wi-Fi setup, embedded HTTP API, authentication, WebSocket updates.
- Persistent profile editor, firing history, graph rendering, report export.
- Physical MAX31856 SPI/CS pin assignment and module validation.
- Physical four heater relay output pin assignment and driver validation.
- Exact CYD revision and GPIO16 LED-pad verification.
- S3 DevKitC-1 pin mapping confirmation on the physical board.
- Actual transceiver selection, CAN harness, termination, EMI, and two-board hardware test.
- Relay contact feedback and contactor auxiliary-contact monitoring.
- Physical safety-rated circuit design, E-stop/high-limit/contactor test.
- End-to-end power-loss/watchdog testing and simulation integration.

## Validation completed

- Standalone host test of CAN fragment generation/reassembly passed in the development environment.
- Test covers multi-fragment reassembly, order rejection, and timeout rejection.
- The repository native test script includes the CAN wire test. The full repository native suite has not been run against a local checkout in this session.
- ESP32 PlatformIO builds have not been run.
- No boards or kiln power hardware have been physically tested.

## Safety gate

Both master-power permit relays are OFF by default. The CYD permit remains disabled in the standard build until the exact CYD revision and output connection are verified. The secondary relay output pins and MAX31856 SPI/CS pins also remain disabled pending hardware configuration.

START/RESUME must remain blocked unless verified sensor, output, communications, and hardware interlock conditions are present. This branch is not ready for live kiln use.
