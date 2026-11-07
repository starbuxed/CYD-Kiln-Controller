# Architecture Decisions

This is the working decision log. Open questions remain open until hardware evidence and test results support a decision.

| Topic | Current direction | Status |
|---|---|---|
| Operator interface | CYD ESP32 | Selected at family level; exact variant TBD |
| Dedicated kiln controller | ESP32-S3 DevKitC-1 development board | Recommended; exact board/pin map to verify |
| Legacy Pro Micro | Removed from target architecture | Not recommended for this four-zone design |
| Alternative controller | RP2040/RP2350 | Technically viable, but not selected |
| Temperature channels | Four independent channels | User selected |
| Sensor mix | Per-channel configurable thermocouple types | Assumed meaning of "mixed"; RTD support requires separate front end |
| Thermocouple front end | One MAX31856-class interface per thermocouple channel | Candidate; module and isolation TBD |
| RTD front end | MAX31865-class interface if an RTD is required | Optional, not selected |
| Heater zones | Four independently controlled circuits | User selected |
| Zone mapping | Zone 1–4 to sensor 1–4 and output 1–4 by default | Proposed; configurable mapping |
| Outputs | Four isolated low-voltage control outputs | Required; driver and feedback design TBD |
| Kiln supply | Other / not identified | Open; no voltage/phase/current assumptions |
| Local UI | CYD touchscreen | Required |
| Remote UI | CYD-hosted web app | Required |
| API | Versioned JSON REST + live event transport | Proposed |
| Inter-processor link | CYD-to-S3 UART with framed, versioned, CRC-protected messages | Proposed |
| Local control authority | Dedicated ESP32-S3 | Proposed architecture |
| Fault on sensor/link failure | Safe output state and latched fault | Required behavior |
| Restart after interruption | Never resume automatically | Required behavior |
| Independent high limit | Hardware protection independent of both MCUs | Required; design TBD |
| Profiles | Versioned ramp/soak profiles with four zone targets/limits | Proposed |

## Decisions required before pin-level or power-stage implementation

1. Exact CYD board model/revision, display controller, touch controller, flash/PSRAM.
2. Exact ESP32-S3 development board/module, pin map, flash/PSRAM, and power arrangement.
3. Exact thermocouple type, probe model, range, and installation location for each channel.
4. Whether any channel is RTD (PT100/PT1000) rather than thermocouple.
5. Exact front-end modules and whether isolation is required for each sensor channel.
6. Kiln nameplate voltage, frequency, phase/conductor arrangement, total current/power, and existing wiring diagram.
7. Electrical ratings and grouping of each of the four heater circuits.
8. Driver/SSR/contactor topology, default polarity, feedback, and independent disconnect.
9. Independent high-limit device(s), sensor placement, and final power-disconnect design.
10. Firing pause/stop/power-failure behavior for the selected kiln.

A change to any hardware-dependent decision must be reflected in the hardware documentation, configuration schema, tests, and wiring drawings.
