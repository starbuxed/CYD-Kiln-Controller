# Implementation Roadmap

## Phase 0 — Controller I/O definition
- [ ] Select exact CYD model/revision.
- [ ] Select ESP32-S3 board and verify pin map.
- [ ] Identify four temperature probes and their individual thermocouple types.
- [ ] Select/verify four compatible sensor front ends.
- [ ] Decide whether any sensor channel is RTD and provide its dedicated front end.
- [ ] Define four zone-to-sensor-to-output mappings.
- [ ] Define generic four-channel digital output abstraction and per-channel active polarity.
- [ ] When available, record the external relay module's control-input voltage/current and polarity to finalize the interface stage.

**Exit gate:** verified low-voltage controller I/O design; external kiln power equipment is user-selected and outside this firmware scope.

## Phase 1 — ESP32-S3 controller foundation
- [ ] Implement safe startup and four inactive output defaults.
- [ ] Implement four sensor-channel abstractions and per-channel thermocouple type configuration.
- [ ] Implement sensor fault, stale-data, and plausibility checks.
- [ ] Implement four independent digital output channels with configurable polarity.
- [ ] Implement local watchdog and latched fault manager.
- [ ] Implement four independent zone-control loops.
- [ ] Unit-test state transitions, sensor faults, output commands, and safe-state behavior.

**Exit gate:** bench-tested with simulated sensors and low-voltage indicator loads only.

## Phase 2 — CYD operator interface and controller link
- [ ] Confirm CYD display/touch drivers.
- [ ] Implement framed, versioned, CRC-protected CYD-to-S3 communications.
- [ ] Implement controller discovery, heartbeat, and reconnect status exchange.
- [ ] Persist versioned zone and sensor configuration.
- [ ] Display four temperatures, four setpoints, four zone states, four output commands, and alarms.

## Phase 3 — Firing engine and profile management
- [ ] Profile schema supports common ramp/soak schedules and optional zone offsets.
- [ ] Validate setpoints, ramp rates, output limits, and sensor mappings for all zones.
- [ ] Define stop/pause behavior.
- [ ] Event log and firing history.
- [ ] Simulation mode and fault injection.
- [ ] No automatic resume after power loss or fault.

## Phase 4 — Self-hosted web application
- [ ] Local Wi-Fi/AP setup and mDNS.
- [ ] REST API and live event stream.
- [ ] Responsive four-zone dashboard.
- [ ] Four-zone profile editor and history.
- [ ] Authentication, authorization, validation, and audit events.

## Phase 5 — Controller I/O commissioning
- [ ] Test four digital outputs with low-voltage indicator loads.
- [ ] Verify active-high/active-low configuration.
- [ ] Verify inactive outputs during reset, boot, brownout, and fault.
- [ ] Test all sensor fault scenarios.
- [ ] Verify optional feedback semantics.
- [ ] Verify independent kiln-side safety equipment separately before any heating operation.
