# Reference Features and Implementation Notes

## Inspiration

The project at https://github.com/jbruce12000/kiln-controller was reviewed for feature ideas. It is a separate Raspberry Pi/Python controller with an extensive browser interface. This firmware uses its own C++ implementation for the CYD ESP32 and ESP32-S3 architecture; its source code has not been copied into this project.

The reference project declares GPL-3.0 in its license documentation. Do not copy or adapt its source files into this repository without a deliberate license decision and the corresponding notices/obligations. Feature concepts and independently written code are tracked here.

## Planned and implemented feature set

| Feature | Current implementation status |
|---|---|
| Live temperature dashboard | Touchscreen dashboard; browser dashboard phase is in progress |
| Live temperature/setpoint graph | Fixed-size telemetry ring buffer implemented; browser graph pending |
| PID performance diagnostics | Offline P/I/D diagnostics module implemented; active zone controller is still hysteresis-based |
| Firing history | Fixed-size summary history and raw persistence interface implemented; runtime integration in progress |
| Scheduling | Bounded queue and due-reminder logic implemented; reminders never start a firing automatically |
| Simulation | Independent four-zone thermal preview model implemented; it does not inject temperatures into the S3 or command outputs |
| Energy and firing cost | Estimate from configured zone wattage, output command history, and energy rate; actual wattage/rate must be entered by the operator |
| Configurable alerts | Bounded active/event ledger implemented; threshold evaluation and browser controls in progress |
| PID tuning | Offline step-response recommendation calculator implemented; no automatic live full-power heat test |
| Alerts and event history | Ledger model implemented; persistent runtime and UI integration pending |

## Safety and correctness boundaries

- The S3 remains the only authority for sensor validation, firing state, local interlocks, and heater outputs.
- A schedule reaching its due time creates a reminder. It does not issue START and does not restart after boot or power loss.
- Simulation data is separate from live telemetry and cannot be used as a control input. Simulation must keep the CYD master-power permit dropped.
- Cost is an estimate based on operator-entered zone wattage and energy rate. It is not an electrical meter.
- PID diagnostics and tuning recommendations do not change the running controller's gains. The current S3 output algorithm is hysteresis, not PID.
- The offline tuner expects a recorded process step response. It never drives heaters to generate that response.
- Browser commands are authenticated requests to the S3 state machine. No browser endpoint can write a GPIO or force an output on.
- No feature in this document makes the controller certified or ready for live kiln operation.

## Validation

Portable tests cover telemetry rollover, history serialization, energy calculations, schedule reminder behavior, alert activation/acknowledgement, simulation isolation, PID terms, and offline tuning recommendation validation.
