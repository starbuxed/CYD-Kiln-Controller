# Implementation Roadmap

## Phase 0 — Hardware definition and safety review
- [ ] Select exact CYD model/revision.
- [ ] Select exact Pro Micro voltage/clock variant.
- [ ] Select sensor technology and four-channel front end.
- [ ] Define output loads, drivers, contactors, and feedback.
- [ ] Document independent high-limit and power isolation.
- [ ] Approve the electrical architecture before wiring high-power loads.

**Exit gate:** reviewed hardware BOM, schematics, pin map, and safety design.

## Phase 1 — Pro Micro I/O firmware
- [ ] UART framing, versioning, CRC, sequence validation.
- [ ] Four-channel sensor abstraction and fault reporting.
- [ ] Four output channels with safe startup behavior.
- [ ] Communications watchdog and fault latch.
- [ ] Unit tests for malformed frames and timeout cases.

**Exit gate:** bench-tested using simulated sensors and low-voltage indicator loads only.

## Phase 2 — ESP32 foundation
- [ ] Confirm CYD display/touch drivers.
- [ ] UART communication and Pro Micro discovery.
- [ ] Persistent configuration with schema version.
- [ ] Supervisory state machine and fault model.
- [ ] Local display of four sensor channels and four outputs.

**Exit gate:** repeatable reset, stale-sensor, and communications-loss testing.

## Phase 3 — Firing engine and profile management
- [ ] Profile schema and strict validation.
- [ ] Ramp/soak scheduler.
- [ ] Pause/stop behavior defined for selected kiln hardware.
- [ ] Event log and firing history.
- [ ] Simulation mode for profile tests.

**Exit gate:** deterministic simulated firing tests, including fault injection.

## Phase 4 — Self-hosted web application
- [ ] Local Wi-Fi/AP setup and mDNS.
- [ ] REST API and live event stream.
- [ ] Responsive dashboard.
- [ ] Profile editor and history views.
- [ ] Authentication, authorization, input validation, and audit events.

**Exit gate:** browser disconnection/reconnection tests and security review.

## Phase 5 — Integrated commissioning
- [ ] Test all fault scenarios with heating loads disconnected.
- [ ] Test independent hardware safety chain.
- [ ] Verify every output safe state and feedback claim.
- [ ] Validate temperature readings against calibrated reference equipment.
- [ ] Qualified electrical review and commissioning.

**Exit gate:** no live kiln operation until all hardware and safety requirements are satisfied.
