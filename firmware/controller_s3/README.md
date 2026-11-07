# Dedicated ESP32-S3 Kiln Controller

This is the target firmware for the dedicated local kiln-control processor. The existing `firmware/esp32/` directory remains the CYD operator-interface prototype, and `firmware/promicro/` is retained as a legacy experiment only.

## Initial target

- Recommended development board: Espressif ESP32-S3-DevKitC-1.
- PlatformIO environment: `esp32-s3-devkitc-1`.
- Four sensor interfaces, one per zone.
- Four independent zone-control loops.
- Four isolated low-voltage output-driver requests.
- Framed UART link to the CYD operator interface.

## Safety and pin policy

No GPIO pins are assigned to heater outputs or sensor chip-selects yet. The exact ESP32-S3 DevKitC-1 variant, sensor breakout modules, output driver board, and carrier PCB must be selected before pin assignments are committed.

The firmware must keep heater requests inactive at boot and during all unconfigured states. Do not infer a safe electrical level from a GPIO default. The final carrier hardware must provide external biasing so driver inputs remain inactive while the MCU is reset, unpowered, or booting.

The ESP32-S3 board is a development platform, not a certified safety controller. Independent high-limit protection and final power isolation remain mandatory.

## Build

From this directory:

```sh
pio run -e controller_s3_devkitc1
```

No live heater outputs are implemented in this initial scaffold.
