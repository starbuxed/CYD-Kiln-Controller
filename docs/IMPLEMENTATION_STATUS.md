# Firmware Implementation Status

## Increment 1: board selection and serial bring-up

Added:
- PlatformIO environment for the common 2.8-inch ILI9341/XPT2046 CYD.
- Alternate PlatformIO environment for the 2.8-inch ST7789/XPT2046 revision.
- Compile-time board profile selector and a catalog of common CYD family variants.
- Explicit compile-time rejection for unverified 2.4-inch, 2.8-inch capacitive, 3.2-inch, 3.5-inch, and Guition profiles.
- ESP32 UART2 initialization using the selected board profile's proposed UART pins.
- Pro Micro 5V/16MHz and 3.3V/8MHz PlatformIO environments.
- Shared protocol constants and CRC16-CCITT helper.
- Pro Micro safe-output/watchdog bring-up skeleton with no guessed output pins.

Not implemented yet:
- Complete UART frame encoder/parser and ACK/retry behavior.
- Sensor drivers and four-channel telemetry.
- Relay driver pins or physical output switching.
- ESP32 display/touch driver integration and UI.
- Profile engine, web server, authentication, and history.
- Hardware-in-the-loop testing.

## Safety limitation

The Pro Micro `forceSafeOutputs()` function is intentionally a placeholder because output pin numbers, driver topology, active polarity, and actual safe states are not known. This firmware increment does not control a kiln and must not be connected to energized kiln outputs.

## Build validation

No PlatformIO compile or physical hardware test has been run as part of this repository update. Treat the code as an initial scaffold until it is built with the selected toolchain and tested with the exact board revisions.
