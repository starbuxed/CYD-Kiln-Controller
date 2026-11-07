# Implementation Roadmap

## Phase 0 — Hardware definition and safety review
- [ ] Select exact CYD model/revision.
- [ ] Select exact ESP32-S3 controller board and verify pin map.
- [ ] Identify kiln nameplate voltage, frequency, phase, current, and existing wiring.
- [ ] Identify the ratings and wiring of all four heater circuits.
- [ ] Identify the four temperature probes and their individual thermocouple types.
- [ ] Decide whether any sensor channel is RTD and select its distinct front end if required.
- [ ] Select and verify four sensor front ends and isolation strategy.
- [ ] Define four zone-to-sensor-to-output mappings.
- [ ] Define output drivers, SSR/contactor topology, feedback, and independent high-limit chain.
- [ ] Approve electrical architecture before wiring high-power loads.

**Exit gate:** reviewed hardware BOM, schematics, pin map, sensor compatibility matrix, and safety design.

## Phase 1 — Dedicated ESP32-S3 controller foundation
- [ ] Create controller firmware target for the selected ESP32-S3 board.
- [ ] Implement safe startup and hardware output-inactive defaults.
- [ ] Implement four sensor-channel abstractions and per-channel thermocouple type configuration.
- [ ] Implement sensor fault, stale-data, and plausibility checks.
- [ ] Implement four output abstractions with feedback support.
- [ ] Implement local watchdog and latched fault manager.
- [ ] Implement four independent zone-control tasks/loops in a deterministic scheduler.
- [ ] Unit-test state transitions, sensor faults, and output safe-state behavior.

**Exit gate:** bench-tested with simulated sensors and low-voltage indicator loads only.

## Phase 2 — CYD operator interface and controller link
- [ ] Confirm CYD display/touch drivers.
- [ ] Implement framed, versioned, CRC-protected CYD-to-S3 communications.
- [ ] Implement controller discovery, heartbeat, and reconnect status exchange.
- [ ] Persist versioned zone and sensor configuration.
- [ ] Display four temperatures, four setpoints, four zone states, four outputs, and alarms.
- [ ] Ensure UI loss cannot bypass local safety behavior.

**Exit gate:** repeatable reset, stale-sensor, and communications-loss testing.

## Phase 3 — Firing engine and profile management
- [ ] Profile schema supports common ramp/soak schedules and optional zone offsets.
- [ ] Strict validation for every zone's setpoint, ramp, output limits, and sensor mapping.
- [ ] Define kiln-specific pause/stop behavior.
- [ ] Event log and firing history.
- [ ] Simulation mode for profile tests.
- [ ] No automatic resume after power loss or fault.

**Exit gate:** deterministic simulated four-zone firing tests, including fault injection.

## Phase 4 — Self-hosted web application
- [ ] Local Wi-Fi/AP setup and mDNS.
- [ ] REST API and live event stream.
- [ ] Responsive dashboard with four zones and sensor health.
- [ ] Four-zone profile editor and history views.
- [ ] Authentication, authorization, input validation, and audit events.

**Exit gate:** browser disconnection/reconnection tests and security review.

## Phase 5 — Integrated commissioning
- [ ] Test all fault scenarios with heating loads disconnected.
- [ ] Test independent hardware safety chain.
- [ ] Verify each output's safe state and feedback claim.
- [ ] Validate each temperature channel against calibrated reference equipment.
- [ ] Verify mixed thermocouple types and cold-junction compensation per channel.
- [ ] Qualified electrical review and commissioning.

**Exit gate:** no live kiln operation until all hardware and safety requirements are satisfied.
