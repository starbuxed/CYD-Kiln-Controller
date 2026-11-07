# Architecture Decisions

This is the working decision log. Open questions remain open until hardware evidence and test results support a decision.

| Topic | Current direction | Status |
|---|---|---|
| Main controller | CYD ESP32 | Selected at family level; exact variant TBD |
| I/O coprocessor | Arduino Pro Micro | Selected at family level; MCU/voltage/clock TBD |
| Temperature channels | Up to four | Sensor technology/front end TBD |
| Output channels | Four configurable channels | Driver topology, loads, polarity TBD |
| Local UI | CYD touchscreen | Display/touch controller and pins TBD |
| Remote UI | ESP32-hosted web app | Required; implementation TBD |
| API | Versioned JSON REST + live event transport | Proposed |
| MCU link | UART with framed, versioned, CRC-protected messages | Proposed |
| Fault on MCU-link loss | Controlled outputs to safe state; latch fault | Required behavior |
| Restart after interruption | Never resume automatically | Required behavior |
| Independent high limit | Hardware protection independent of MCUs | Required; design TBD |
| Profiles | Versioned ramp/soak JSON | Proposed |

## Decisions required before firmware pin-level implementation

1. Exact CYD board model, revision, display controller, touch controller, flash/PSRAM.
2. Exact Pro Micro MCU, voltage, clock, bootloader, and serial pin availability.
3. Thermocouple/RTD type and front-end module for each of the four inputs.
4. What each of the four outputs controls and the electrical ratings of those loads.
5. Driver/contactor topology, feedback contacts, default polarity, and isolation.
6. Independent high-limit and final power-disconnect design.
7. Firing pause/stop/power-failure behavior for the selected kiln.

A change to any hardware-dependent decision should be recorded here and reflected in the hardware documentation and tests.
