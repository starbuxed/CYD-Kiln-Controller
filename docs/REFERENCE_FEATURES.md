# Reference Features and Implementation Notes

## Inspiration

The project at https://github.com/jbruce12000/kiln-controller was reviewed for feature ideas. It is a separate Raspberry Pi/Python controller with an extensive browser interface. This firmware uses its own C++ implementation for the CYD ESP32 and ESP32-S3 architecture; its source code has not been copied into this project.

The reference project declares GPL-3.0 in its license documentation. Do not copy or adapt its source files into this repository without a deliberate license decision and the corresponding notices/obligations. Feature concepts and independently written code are tracked here.

## Feature status

| Feature | Current implementation status |
|---|---|
| Live temperature dashboard | Touchscreen and CYD-hosted browser dashboard |
| Live temperature/setpoint graph | 1,800-sample RAM ring (10-second sampling) with browser graph and CSV export |
| PID performance diagnostics | Offline P/I/D term calculator; active S3 zone loop is still hysteresis-based |
| Firing history | 12-entry persisted summary history with CSV export; full-resolution per-firing log storage remains pending |
| Scheduling | 8-entry persisted queue with due reminders; reminders never start a firing automatically |
| Simulation | Independent four-zone preview model and browser controls; no simulated values enter the S3 or outputs |
| Energy and firing cost | Estimate from configured zone wattage, output command history, and energy rate; not an electrical meter |
| Configurable alerts | Dashboard rules for CAN loss, stale status, controller fault, sensor validity, and optional temperature warning |
| PID tuning | Offline step-response recommendation calculator; no automatic live full-power heat test |
| Event history/export | Bounded alert ledger, firing summaries, and CSV download endpoints |

## Safety and correctness boundaries

- The S3 remains the only authority for sensor validation, firing state, local interlocks, and heater outputs.
- A schedule reaching its due time creates a reminder. It does not issue START and does not restart after boot or power loss.
- Simulation data is separate from live telemetry and cannot be used as a control input. Simulation must keep the CYD master-power permit dropped.
- Cost is an estimate based on operator-entered zone wattage and energy rate. It is not an electrical meter.
- PID diagnostics and tuning recommendations do not change the running controller's gains. The current S3 output algorithm is hysteresis, not PID.
- The offline tuner expects a recorded process step response. It never drives heaters to generate that response.
- Browser commands are authenticated requests to the S3 state machine. No browser endpoint can write a GPIO or force an output on.
- Dashboard warning settings cannot disable S3 safety faults or the physical high-limit.
- No feature in this document makes the controller certified or ready for live kiln operation.

## Validation

Portable tests cover telemetry rollover, history serialization, energy estimates, schedule reminder behavior, alert activation/acknowledgement, simulation isolation, PID terms, and offline tuning recommendation validation. Hardware display, CAN bus, thermocouple, relay, and safety-circuit tests remain outstanding.
