# Arduino Pro Micro I/O Firmware

This firmware implements the dedicated sensor/output I/O coprocessor.

## Planned modules

- Protocol framing, CRC, and sequence validation.
- Sensor drivers behind a common interface.
- Four temperature channel abstractions.
- Four output-channel abstractions.
- Communications watchdog and safe-state handler.
- Fault reporting and diagnostics.

## Board configuration is required

Pro Micro boards are sold in different MCU, voltage, and clock variants. Record the exact board before selecting the Arduino board target or connecting UART. The ESP32 is a 3.3 V logic device; do not connect a possible 5 V Pro Micro TX signal directly to an ESP32 RX pin.

Do not assign output pins, output polarity, or relay semantics until the exact Pro Micro, driver hardware, and electrical design are documented.

## Startup and fault behavior

- Initialize all outputs to the documented safe state before normal application work.
- Never restore prior output states from EEPROM after reset.
- Refresh the communications watchdog only after a valid, correctly sequenced host frame.
- On watchdog timeout, drive controlled outputs to safe state and latch/report the fault.
- Reconnection must not restore the pre-fault state; explicit revalidation and a new operator start sequence are required.

The watchdog is not a substitute for independent hardware high-limit protection.
