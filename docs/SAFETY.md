# Safety Requirements and Fault Policy

> **Critical:** This software project is not a safety certification. Kiln mains/high-current circuitry and independent over-temperature protection must be designed for the actual kiln by a qualified professional. Never rely on either ESP32, the browser, Wi-Fi, or a single software sensor reading as the only means of preventing overheating or fire.

## Mandatory design rules

1. All four heater-control requests default to inactive during reset, boot, brownout, watchdog timeout, malformed command, or loss of the dedicated controller's valid control state.
2. A reboot or restored connection must never cause an automatic transition back into firing.
3. All CYD/browser commands pass through the dedicated controller's validation and supervisory state machine before affecting outputs.
4. Browser clients cannot write GPIO pins or bypass interlocks.
5. Each of the four sensor values carries sensor type, validity, freshness, and front-end error information. Invalid, disconnected, wrong-type, implausible, or stale values are faults, not zero-degree readings.
6. A sensor fault in a zone must at minimum disable that zone's heater output and latch/report the fault. The project must define whether a single zone fault also stops all other zones; default commissioning behavior should stop all four zones until a kiln-specific safety analysis explicitly justifies another policy.
7. Loss of the CYD link must not disable local safety supervision. The dedicated controller must follow a documented local policy and must never accept stale/replayed start or resume commands.
8. Independent high-limit protection and final power isolation must remain effective if both microcontrollers fail.
9. Output devices, wiring, connectors, fuses, enclosures, isolation, and creepage/clearance must be rated for their actual loads and environment.
10. Firmware updates and configuration writes must not energize outputs.
11. A commanded output state is not proof of actual heater power being removed. SSR short-circuit failure must be considered; provide independent contactor/disconnect protection and appropriate feedback/current monitoring.
12. Four zone controllers must have independent software limits, but a shared hardware safety chain must be assessed for common-cause failures.

## Fault conditions to detect

At minimum, design for:
- Missing, disconnected, or wrong-type temperature sensor.
- Sensor open/short or front-end fault.
- Stale or implausible temperature data on any zone.
- Any zone exceeding configured operating limits or exceeding its ramp limit.
- Dedicated controller to CYD communication timeout.
- Invalid, corrupted, or out-of-sequence protocol frame.
- Unexpected reset, watchdog reset, or brownout.
- Output-driver fault or mismatch, where feedback is available.
- Heater current present when output is commanded off, where current sensing is installed.
- Invalid firing profile, invalid zone mapping, or impossible ramp/soak parameters.
- Storage/configuration integrity failure.
- Internal task stall or failure to service safety monitoring.

## Required fault response

For each fault, specify detection threshold, confirmation/debounce behavior, severity, latched status, operator acknowledgement requirements, output action, and recovery procedure. The default response for an unresolved safety-critical fault is to command all heater outputs to the safe state and latch the fault. Never clear a critical fault merely because the signal becomes valid again.

## Communications loss

- The dedicated ESP32-S3 supervises its own sensor acquisition and output control locally.
- The CYD sends heartbeat/status requests over the inter-controller link.
- If the CYD link is lost during firing, the dedicated controller must follow the explicitly selected policy. For initial commissioning, use all outputs inactive and latch a UI-link fault; do not assume a disconnected UI should leave heaters operating.
- On reconnection, exchange status and fault state first.
- Do not resend queued output/start/resume commands.
- Require deliberate operator acknowledgement and a new start sequence after fault review.

## Output-state terminology

The firmware and UI must distinguish:
- **Requested:** state requested by the profile/controller.
- **Commanded:** state sent to an output driver.
- **Reported:** state reported by the controller electronics.
- **Verified:** physical state confirmed by independent feedback/current sensing.

Never display a commanded state as physically verified unless feedback hardware supports that claim.

## Commissioning stages

1. Review schematics, ratings, isolation, earthing, fusing, enclosure, and high-limit design before powering the control electronics.
2. Test CYD and ESP32-S3 communications with heater outputs disconnected.
3. Test protocol framing and error handling with simulated data.
4. Test all four outputs using low-voltage indicator loads only.
5. Inject wrong sensor type, open/short, stale data, communications loss, resets, brownouts, and malformed frames.
6. Verify zone fault handling and the all-zones-safe default repeatedly with no kiln heating load connected.
7. Verify independent hardware high-limit and final power disconnect separately from firmware.
8. Have the completed power system inspected and commissioned by a qualified professional.
9. Only then perform controlled commissioning according to the kiln and component manufacturers' instructions.

## Open safety decisions

The project must not be considered ready to control a kiln until these are documented:
- Exact output driver and SSR/contactor topology.
- Electrically safe default state and polarity for each output.
- Independent high-limit device(s) and final power-disconnect design.
- Sensor types, operating ranges, fault modes, and installation locations for all four zones.
- Whether a single-zone fault shuts down all four zones (initial default: yes).
- UI-link loss policy (initial commissioning default: all outputs inactive and fault latched).
- Behavior for stop, pause, power failure, and interrupted profiles.
- Electrical enclosure and grounding/earthing design.
