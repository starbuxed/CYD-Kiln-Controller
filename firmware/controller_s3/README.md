# Dedicated ESP32-S3 Kiln Controller

This firmware target is the local control processor. The CYD ESP32 is the operator interface and will host the local web application. The Pro Micro is retained only as a legacy prototype.

## Target functions

- Four temperature inputs, individually configurable for K/J/N/R/S/T/E/B thermocouples.
- Four independent heating zones with independent hysteresis control.
- Four independent digital ON/OFF control outputs.
- Per-output active-high/active-low configuration.
- Local ramp/soak profile execution.
- Local latched safety supervision independent of the CYD.
- Framed UART link to the CYD operator interface.

## Implemented controller modules

- `profile_engine.*`: bounded ramp/soak profile validation, ramp interpolation, soak timing, pause/resume, and completion.
- `zone_controller.*`: four independent hysteresis controllers with configurable deadband and minimum on/off dwell.
- `sensor_manager.*`: per-channel thermocouple type selection, calibration offsets, reading validation, stale-data detection, range checking, rate plausibility checks, and Off / Log only / Log and control / Log and safety operating modes.
- `controller_link.*`: per-sensor operating-mode configuration over UART and per-sensor mode telemetry to the CYD. Log and safety sensors are checked independently of PID; only Log and control sensors are eligible for the current one-to-one zone PID inputs.
- `max31856_array.*`: four-chip SPI front end using the Adafruit MAX31856 library, with independent thermocouple type configuration and per-channel fault mapping.
- `safety_manager.*`: latched sensor-loss, sensor-range, overtemperature, and host-link faults.
- `kiln_control_core.*`: coordinates the profile, four zones, and fail-safe logical output states.
- `relay_outputs.*`: validates enabled pin assignments, rejects duplicate GPIO assignments, supports per-channel active-high/active-low polarity, and initializes configured pins inactive.
- `controller_link.*`: UART heartbeat, status telemetry, typed profile/zone/sensor/safety configuration, START/PAUSE/RESUME/STOP/ACK_FAULT handling, acknowledgements, and host-link timeout handling.

The modules are deliberately separated from hardware drivers so the profile, zone, sensor-validation, and safety logic can be host-tested without the ESP32 boards.

## Hardware integration still required

The MAX31856 SPI driver is implemented, but its SPI and chip-select pin macros default to -1. Set `KILN_TC_SPI_SCLK`, `KILN_TC_SPI_MISO`, `KILN_TC_SPI_MOSI`, and the required `KILN_TC1_CS_PIN` through `KILN_TC4_CS_PIN` only after checking the exact S3 board pinout. Configure each channel's thermocouple type over the UART link. No physical module has been tested yet.

All relay outputs remain disabled and unassigned in `main.cpp`. No physical GPIO assignments are committed until the exact ESP32-S3 board and external relay input interface are selected. The relay input interface must be compatible with the GPIO voltage/current, polarity, and isolation requirements.

Do not connect bare relay coils or heater loads directly to ESP32 GPIO pins. A commanded OFF state does not prove the external relay or heater power is physically off. Use an independent hardware overtemperature limit and appropriate power-stage protection.

## Safety boundary

START and RESUME are wired to the local control core. START is accepted only when a validated profile, all four zone configurations, four valid live sensor channels, a healthy host link, and four configured physical output channels are present. The current build has no physical thermocouple driver and all output pins are disabled, so it cannot start a firing or energize the outputs.

No automatic restart or resume is allowed after reset, brownout, sensor loss, communication failure, or a latched fault.

## Build

From this directory:

```sh
pio run -e controller_s3_devkitc1
```

Run portable host tests from the repository root:

```sh
sh tests/run_native_tests.sh
```

Native portable C++ tests run in GitHub Actions. Separate GitHub Actions jobs build the ESP32-S3 and classic CYD PlatformIO targets. Physical hardware has not been tested.
