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
- [x] Add first classic 2.8-inch ILI9341/XPT2046 touchscreen target and calibration screen.
- [x] Implement CAN heartbeat, status, profile, sensor-mode, zone-config, and command path foundation.
- [ ] Verify exact physical CYD revision, display/touch pin map, and S3 pin map.
- [ ] Persist and verify full sensor/zone/output mapping configuration.
- [ ] Display actual measured relay feedback and interlock state after feedback hardware exists.

## Phase 3 — Firing engine and profile management
- [x] Ramp/soak profile transfer and S3-side validation foundation.
- [x] Portable telemetry ring, firing-summary history, energy estimate, schedule reminder queue, alert ledger, simulation model, and PID diagnostic/tuning math.
- [x] Profile upload request from touchscreen and browser JSON profile upload.
- [ ] Full profile library with names, edit/read-back, backup, and import.
- [ ] Integrate mapping module into the S3 firing core.
- [ ] Complete continuous safety checks and test every fault transition.
- [ ] Persistent detailed telemetry per firing and export.
- [ ] Simulation and fault injection integrated into automated state-machine tests.
- [ ] No automatic resume after power loss or fault.

## Phase 4 — Self-hosted web application
- [x] CYD-hosted local WPA2 access point and HTTP server.
- [x] Local mDNS hostname where supported.
- [x] Authenticated REST API with CSRF check for state-changing requests.
- [x] Responsive browser dashboard with live status polling and temperature graph.
- [x] Profile upload, history summary, CSV export, alert ledger, schedule reminders, estimated energy/cost, isolated simulation, and offline PID tools.
- [ ] Wi-Fi station-mode onboarding and credentials UI.
- [ ] TLS/credential UX hardening for trusted-LAN deployment.
- [ ] WebSocket/SSE push and complete profile editor/library.
- [ ] Detailed telemetry retention and export for full-length firings.

## Phase 5 — Controller I/O commissioning
- [ ] Test four digital outputs with low-voltage indicator loads.
- [ ] Verify active-high/active-low configuration.
- [ ] Verify inactive outputs during reset, boot, brownout, and fault.
- [ ] Test all sensor fault scenarios.
- [ ] Verify optional feedback semantics.
- [ ] Verify independent kiln-side safety equipment separately before any heating operation.
