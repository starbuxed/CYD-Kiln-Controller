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
- `sensor_manager.*`: per-channel thermocouple type selection, reading validation, stale-data detection, range checking, and rate plausibility checks.
- `safety_manager.*`: latched sensor-loss, sensor-range, overtemperature, and host-link faults.
- `kiln_control_core.*`: coordinates the profile, four zones, and fail-safe logical output states.
- `relay_outputs.*`: validates enabled pin assignments, rejects duplicate GPIO assignments, supports per-channel active-high/active-low polarity, and initializes configured pins inactive.
- `controller_link.*`: UART heartbeat, status telemetry, command validation, acknowledgements, and host-link timeout handling.

The modules are deliberately separated from hardware drivers so the profile, zone, sensor-validation, and safety logic can be host-tested without the ESP32 boards.

## Hardware integration still required

The sensor manager currently has no physical thermocouple front-end driver attached. It accepts samples from a future MAX31856 or compatible hardware adapter. Four channel thermocouple types can be selected independently in software, but the actual SPI chip-select pins, bus wiring, board revision, and driver must be configured before readings are available.

All relay outputs remain disabled and unassigned in `main.cpp`. No physical GPIO assignments are committed until the exact ESP32-S3 board and external relay input interface are selected. The relay input interface must be compatible with the GPIO voltage/current, polarity, and isolation requirements.

Do not connect bare relay coils or heater loads directly to ESP32 GPIO pins. A commanded OFF state does not prove the external relay or heater power is physically off. Use an independent hardware overtemperature limit and appropriate power-stage protection.

## Safety boundary

START and RESUME are intentionally rejected by the UART command handler. The current application cannot start a firing or energize the disabled output channels. These commands must remain blocked until sensor acquisition, profile transfer, physical output mapping, and end-to-end safety validation are complete.

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

Neither the ESP32 board build nor physical hardware has been tested yet.
