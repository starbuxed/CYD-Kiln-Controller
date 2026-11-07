# Dedicated ESP32-S3 Kiln Controller

This firmware target is the local control processor. The CYD ESP32 remains the touchscreen and web-server device; the Pro Micro is a legacy prototype.

## Target functions

- Four temperature inputs, individually configurable for supported thermocouple types.
- Four independent zone-control loops.
- Four independent digital ON/OFF control outputs.
- Per-output active-high/active-low configuration.
- Safe inactive outputs during boot, reset, brownout, watchdog timeout, and latched faults.
- Framed UART link to the CYD operator interface.

## Output scope

The firmware issues four logical relay-control commands. The user selects and supplies the external relay/SSR equipment. The controller's physical output interface must be matched to the external module's control-input voltage, current, polarity, and isolation requirements.

Do not connect bare relay coils or heater loads directly to ESP32 GPIO pins. Use appropriate protected/isolated interface circuitry. Do not assume that a commanded OFF state proves that the external relay or heater power is physically off.

No GPIO pin assignments are committed until the exact ESP32-S3 board and output-interface hardware are selected. External biasing must keep all output interfaces inactive while the MCU is reset, unpowered, or booting.

## Build

From this directory:

```sh
pio run -e controller_s3_devkitc1
```

The current firmware is only a non-firing startup scaffold. Sensor acquisition and the four output drivers are not yet implemented.
