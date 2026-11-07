# Architecture Decisions

| Topic | Decision | Status |
|---|---|---|
| Main operator interface | CYD ESP32 | Selected; exact revision TBD |
| Secondary controller | ESP32-S3 development board | Selected; exact model TBD |
| Temperature inputs | Four independent channels | Selected |
| Thermocouple front ends | Four MAX31856 modules | Selected direction; exact modules/probes TBD |
| Output channels | Four digital ON/OFF outputs | Selected |
| External relay/SSR/power stage | User-selected protected equipment | Electrical details TBD |
| Main/secondary link | Classic CAN 2.0A with external transceiver per board | Selected; pins/transceivers/cable TBD |
| Initial bitrate | 500 kbit/s | Validate against physical topology |
| CYD responsibilities | UI, web, Wi-Fi, SD, profiles, history | Selected |
| Secondary responsibilities | Sensors, local control, outputs, local safety | Selected |
| Independent shutdown | Separate hardwired request from each MCU plus physical E-stop | Required; schematic/verification pending |
| Independent high-limit | Kiln-side hardware device | Required |
| Output polarity | Configurable after electrical confirmation | Required |
| Restart after interruption | Never auto-resume | Required |
| Exact pin maps | No assumptions | Pending board identification |

## Safety design boundary

CAN is the data/control transport, not the emergency-stop circuit. Each MCU requires its own external CAN transceiver. Each MCU also requires a separate shutdown request interface to a fail-safe hardware safety circuit. A physical latching E-stop and independent high-limit must interrupt the safety-contactor path without relying on either controller.

No GPIO mapping should be enabled until exact boards, schematics, transceivers, sensor modules, relay input interfaces, and shutdown circuit have been confirmed.
