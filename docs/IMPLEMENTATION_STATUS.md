# Firmware Implementation Status

## Increment 1: CYD selection and Pro Micro serial heartbeat

Added:
- PlatformIO build profiles for classic 2.8-inch ILI9341/XPT2046 and ST7789/XPT2046 variants.
- Catalogue profiles for 2.4-inch, 2.8-inch capacitive, 3.2-inch, 3.5-inch resistive/capacitive, and ESP32-S3 4.3/7-inch CYD families.
- Compile-time profile selection using `KILN_BOARD_PROFILE`.
- Classic 2.8-inch reference display/touch pin map and proposed UART2 pins GPIO22/GPIO27.
- UART is disabled for other profiles until a board-specific UART pin map is verified.
- Shared framed UART protocol with SOF, version, message type, sequence, payload length, and CRC16-CCITT.
- ESP32 heartbeat sender and Pro Micro heartbeat receiver/status reporting.
- Pro Micro safe-output startup and communications-timeout handling.
- Host-side protocol round-trip and CRC-corruption test source.

## Current limitations

- The ESP32 firmware currently reports the board profile and maintains a Pro Micro heartbeat link; it does not yet initialize the display/touch hardware.
- No remote output-control command is issued by the ESP32.
- Pro Micro output pins remain unconfigured (`-1`) by design. No relay can be driven.
- Sensor drivers and four-channel telemetry are not implemented.
- Board profiles outside the classic 2.8-inch reference are catalogue/build-selection entries only; their GPIO maps and display/touch initialization are not validated.
- The 2.8-inch ST7789 profile shares the common classic CYD pin layout but panel color order/inversion and exact revision still require physical confirmation.
- No authentication, web server, firing-profile engine, or firing history is implemented yet.

## Safety limitations

This is a bring-up scaffold, not a kiln controller ready for operation. Do not connect it to energized kiln outputs. The Pro Micro output mapping remains disabled until the exact board, driver topology, active polarity, and independent safety chain are documented and tested.

A valid serial heartbeat only indicates that the Pro Micro is responding. It does not prove sensor health, relay state, contactor state, or kiln safety.

## Validation status

No PlatformIO build or physical hardware test has been run as part of this increment. The protocol test source is present but has not yet been executed against the repository toolchain. Build both MCU targets and run protocol tests before connecting hardware.
