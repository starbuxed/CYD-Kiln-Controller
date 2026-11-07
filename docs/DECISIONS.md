# Architecture Decisions

| Topic | Current direction | Status |
|---|---|---|
| Operator interface | CYD ESP32 | Selected; exact revision TBD |
| Dedicated controller | ESP32-S3 DevKitC-1 | Recommended; pin map to verify |
| Legacy Pro Micro | Not used in target architecture | Retained as historical prototype |
| Temperature channels | Four independent channels | User selected |
| Sensor mix | Per-channel configurable thermocouple types | Assumed meaning of "mixed"; RTD requires separate front end |
| Thermocouple front end | One MAX31856-class interface per channel | Candidate; verify module |
| Heater zones | Four independent zones | User selected |
| Output interface | Four independent digital ON/OFF commands | Selected behavior |
| External relay/SSR hardware | User selects and supplies it | Outside firmware power-stage scope |
| Output polarity | Configurable active-high/active-low per channel | Required |
| Output electrical interface | Protected/isolated low-voltage interface matched to relay module | Exact interface depends on module input |
| Output feedback | Optional, if external hardware provides it | Not assumed |
| Kiln power supply | Not required to implement logical ON/OFF firmware | User handles external power equipment |
| Local UI | CYD touchscreen | Required |
| Remote UI | CYD-hosted web app | Required |
| Inter-controller link | Framed, versioned, CRC-protected UART | Proposed |
| Local control authority | Dedicated ESP32-S3 | Selected architecture |
| Fault response | Outputs inactive and fault latched | Required |
| Restart after interruption | Never resume automatically | Required |

## Remaining controller-interface detail

The firmware can be developed against a generic four-channel digital output abstraction without knowing kiln mains voltage or heater current. Before selecting the physical output-interface circuit or connecting a relay module, record that module's input voltage/current, active polarity, isolation, and whether its input is logic-level or a bare relay coil.

Exact sensor probes/front ends, CYD revision, ESP32-S3 pin map, and relay-module input interface remain to be verified.
