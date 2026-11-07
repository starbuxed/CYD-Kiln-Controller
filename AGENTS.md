# Instructions for Coding Agents

You are working on the CYD Kiln Controller repository. Read `README.md` and all relevant documents in `docs/` before changing firmware or hardware assumptions.

## Project intent

Build a ceramic kiln controller around a CYD ESP32 touchscreen and an Arduino Pro Micro I/O coprocessor. The system must support up to four kiln temperature channels, four configurable output channels, ramp/soak profiles, local history/logging, a touchscreen UI, and a self-hosted browser application served by the ESP32.

## Non-negotiable safety constraints

- Treat this as safety-critical control software. Do not claim certification or that software alone makes a kiln safe.
- Never create a raw network-to-GPIO or browser-to-relay control path.
- All local and remote commands must pass through the same supervisory state machine and validation.
- Outputs must default to their documented safe state at boot/reset/brownout/watchdog timeout/communications loss.
- Pro Micro communications timeout must de-energize controlled outputs and latch/report a fault.
- Never automatically resume or replay start/resume/output commands after reset or reconnection.
- Invalid, stale, disconnected, or implausible temperature readings are faults, not zero-valued temperatures.
- Preserve independent hardware high-limit protection and power isolation. Do not replace them with firmware logic.
- Do not represent an output as physically verified unless feedback hardware exists.
- Do not test early firmware with live kiln heating elements. Use simulation and low-voltage indicator loads.

## Do not invent hardware details

The exact CYD variant/revision, display/touch controllers, ESP32 pin map, Pro Micro MCU/voltage/clock, sensor technology/front-end, relay drivers, output polarity, and power-stage topology are not yet finalized.

Do not guess these values. Introduce board-specific configuration layers and leave unresolved configuration explicit. Before writing pin-level implementation, update `docs/DECISIONS.md` and `docs/HARDWARE.md` with verified details and sources.

## Engineering approach

1. Implement portable, host-testable logic first: protocol parser, profile validation, state machine, fault model, and simulation.
2. Keep ESP32 and Pro Micro responsibilities separated according to `docs/ARCHITECTURE.md`.
3. Use a versioned UART frame with length, sequence number, and CRC. Reject malformed, stale, duplicate, and unsupported messages.
4. Keep protocol constants and schemas in `shared/`; avoid duplicated magic numbers.
5. Make configuration and profile storage versioned and validate both on load and before firing.
6. Keep the web assets local to the ESP32. The browser must not need a CDN or internet connection.
7. Use versioned JSON REST endpoints and a live event channel as described in `docs/WEB_APP.md`.
8. Keep authentication, authorization, CSRF protection as appropriate, input validation, and audit logging in the design for state-changing API calls.
9. Add tests for every safety-state transition, fault case, timeout, and protocol parser change.
10. Document decisions, assumptions, limitations, and test coverage in the same change.

## Delivery expectations

For each implementation phase:
- State exactly what was implemented and what remains stubbed.
- Include build/test commands and the results actually observed.
- Do not claim firmware compiles or hardware works unless it was built/tested.
- Keep commits focused and update the roadmap checkboxes only when the relevant exit criteria are met.
- Prefer small, reviewable changes over a large untested rewrite.
