# Safety Requirements and Fault Policy

> **Critical:** This project is not a safety certification. The ESP32-S3 sends four low-voltage ON/OFF commands to external relay/SSR modules selected by the user. ESP32 GPIOs must not directly drive bare relay coils or heater loads. A commanded OFF state does not prove the external relay has opened or that heater power is absent.

## Mandatory controller rules

1. All four digital output commands default to inactive during reset, boot, brownout, watchdog timeout, malformed command, or loss of valid controller state.
2. Output polarity is configurable per channel, but the logical inactive state must be defined and externally biased safe during MCU reset/unpowered conditions.
3. A reboot or restored connection must never automatically resume a firing or restore an ON output.
4. All CYD/browser commands pass through the dedicated controller's validation and supervisory state machine.
5. Each temperature channel carries sensor type, validity, freshness, and front-end error information. Invalid, disconnected, wrong-type, implausible, or stale readings are faults.
6. A sensor fault must disable its associated output and latch/report the fault. Initial commissioning policy: a safety-critical sensor fault disables all four outputs.
7. Loss of the CYD link must not disable local fault supervision. Initial commissioning policy: all four outputs inactive and a latched link fault.
8. Firmware updates and configuration writes must not energize outputs.
9. The UI distinguishes logical command state from verified physical state. Without feedback inputs, show only commanded ON/OFF.
10. External relay/SSR hardware and kiln-side safety devices are outside this firmware's power-design scope, but must be installed with their own appropriate protection and independent high-limit arrangements.

## Output interface requirements

- Four independent logical ON/OFF channels.
- Per-channel active-high/active-low configuration.
- Defined inactive level established before normal firmware operation.
- External biasing or equivalent hardware keeps inputs inactive during MCU reset, boot, brownout, and power loss.
- Suitable protected interface circuitry between ESP32 GPIO and the user's relay module.
- Never connect a bare relay coil directly to a GPIO.
- Optional feedback may report an input/auxiliary-contact state, but only independent suitable feedback can support a verified physical-state claim.

## Fault conditions to detect

- Missing, disconnected, wrong-type, or faulty temperature sensor.
- Stale or implausible temperature data.
- Zone over-temperature or ramp-limit violation.
- CYD-to-S3 communication timeout.
- Invalid, corrupted, or out-of-sequence protocol frame.
- Unexpected reset, watchdog reset, or brownout.
- Output command/feedback mismatch, if feedback is provided.
- Invalid profile, zone mapping, or sensor configuration.
- Internal task stall or failure to service safety monitoring.

## Required fault response

The default response for an unresolved safety-critical fault is to command all four outputs inactive and latch the fault. Never clear a critical fault merely because the signal becomes valid again. Recovery requires review, acknowledgement, and a fresh explicit start sequence.

## Output-state terminology

- **Requested:** desired state from profile/control logic.
- **Commanded:** logical ON/OFF state applied to the output interface.
- **Feedback:** state observed by an optional feedback input.
- **Verified physical state:** only available when suitable independent hardware verifies the actual switched circuit.

Do not label a commanded state as verified.

## Commissioning

1. Verify the ESP32-S3 board pin map and output interface schematic.
2. Test all four outputs with low-voltage indicator loads only.
3. Verify active-high/active-low behavior for each channel.
4. Verify outputs remain inactive during reset, boot, brownout, and watchdog fault.
5. Inject sensor faults, stale data, communications loss, resets, and malformed frames.
6. Verify all critical faults force the configured inactive output state.
7. Connect the user's relay modules only after confirming their input voltage/current, polarity, and interface requirements.
8. Verify external kiln safety equipment independently of firmware before any heating operation.
