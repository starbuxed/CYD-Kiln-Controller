# Shared Definitions

This directory contains definitions shared by the CYD ESP32, dedicated ESP32-S3 controller, and host-side tests.

Keep these items language-neutral wherever possible:
- Protocol version and message type registry.
- Frame format, field widths, byte order, and CRC definition.
- Error/fault code registry.
- Temperature and output telemetry schema.
- Profile JSON schema and version migration rules.

Do not copy protocol constants independently into multiple code files. Generate or include them from a single reviewed source where practical. Any change to a wire-format field requires a protocol version update and compatibility tests.
