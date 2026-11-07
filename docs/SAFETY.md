# Safety Requirements and Fault Policy

> **Critical:** This software project is not a safety certification. Kiln mains/high-current circuitry and independent over-temperature protection must be designed for the actual kiln by a qualified professional. Never rely on the ESP32, Pro Micro, browser, Wi-Fi, or a single software sensor reading as the only means of preventing overheating or fire.

## Mandatory design rules

1. All controlled outputs default to the documented safe state during reset, boot, brownout, watchdog timeout, malformed command, or loss of host communication.
2. A reboot or restored connection must never cause an automatic transition back into firing.
3. Every remote command passes through the same ESP32 supervisory state machine as a touchscreen command.
4. Browser clients cannot write GPIO pins or bypass interlocks.
5. Sensor values must carry validity and freshness information. Invalid, disconnected, implausible, or stale values are faults, not zero-degree readings.
6. Loss of the Pro Micro link is a fault. The ESP32 must not continue assuming the last output command succeeded.
7. The Pro Micro must have a hardware-appropriate communications watchdog behavior and an explicit output-safe startup sequence.
8. Independent high-limit protection and power isolation must remain effective if both microcontrollers fail.
9. Output devices, wiring, connectors, fuses, enclosures, isolation, and creepage/clearance must be rated for their actual loads and environment.
10. Firmware updates and configuration writes must not energize outputs.

## Fault conditions to detect

At minimum, design for:
- Missing or disconnected temperature sensor.
- Sensor open/short or front-end fault.
- Stale or implausible temperature data.
- Temperature exceeding configured operating limits.
- Pro Micro communication timeout.
- Invalid, corrupted, or out-of-sequence protocol frame.
- Unexpected reset or brownout.
- Output-driver fault or mismatch, where feedback is available.
- Invalid firing profile or impossible ramp/soak parameters.
- Storage/configuration integrity failure.
- Watchdog reset.
- Internal task stall or failure to service safety monitoring.

## Required fault response

For each fault, specify detection threshold, confirmation/debounce behavior, severity, latched status, operator acknowledgement requirements, output action, and recovery procedure. The default response for an unresolved safety-critical fault is to command outputs to the safe state and latch the fault. Never clear a critical fault merely because the signal becomes valid again.

## Communications loss

- The Pro Micro starts a watchdog timer that is refreshed only by valid, correctly sequenced host frames.
- On timeout, it de-energizes controlled outputs and latches a communications fault.
- The ESP32 marks output state as unknown until fresh status is received.
- On reconnection, exchange status and fault state first.
- Do not resend queued output/start/resume commands.
- Require deliberate operator acknowledgement and a new start sequence after fault review.

## Output-state terminology

The firmware and UI must distinguish:
- **Requested:** state requested by the supervisor.
- **Commanded:** state sent to the Pro Micro.
- **Reported:** state reported by the Pro Micro.
- **Verified:** physical state confirmed by independent feedback.

Never display a commanded state as physically verified unless feedback hardware supports that claim.

## Commissioning stages

1. Review schematics, ratings, isolation, earthing, fusing, enclosure, and high-limit design before powering the control electronics.
2. Test ESP32 display and web server with outputs disconnected.
3. Test UART framing and error handling with simulated data.
4. Test Pro Micro outputs using low-voltage indicator loads only.
5. Inject sensor faults, communication loss, resets, brownouts, and malformed frames.
6. Verify safe-state behavior repeatedly with no kiln heating load connected.
7. Have the completed power system inspected and commissioned by a qualified professional.
8. Only then perform controlled commissioning according to the kiln and component manufacturers' instructions.

## Open safety decisions

The project must not be considered ready to control a kiln until these are documented:
- Exact output driver and relay/contactor topology.
- Electrically safe default state and polarity for each output.
- Independent high-limit device and final power-disconnect design.
- Sensor types, operating ranges, fault modes, and installation locations.
- Communication watchdog timeout and recovery process.
- Behavior for stop, pause, power failure, and interrupted profiles.
- Electrical enclosure and grounding/earthing design.
