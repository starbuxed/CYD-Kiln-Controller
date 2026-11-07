# Safety Requirements and Fault Policy

> This is experimental development firmware, not a certified kiln safety controller. Neither ESP32 nor CAN is a substitute for independent kiln-side overtemperature protection, correctly rated power switching, protective devices, and a physical emergency-stop circuit.

## Mandatory safety architecture

1. CYD main and secondary ESP32-S3 each have an independent hardwired shutdown-request interface.
2. A physical latching emergency-stop directly interrupts the safety-contactor control circuit without either MCU, CAN, Wi-Fi, or browser.
3. An independent kiln-side high-limit device is mandatory.
4. The engineered safety circuit must allow either MCU request to de-energize the safety enable path.
5. Do not connect MCU GPIO outputs together; use separate isolated/interface stages.
6. MCU requests supplement hardware safety; GPIO is not the sole safety contact.
7. Neither MCU, CAN software, nor web UI may clear an active physical E-stop or high-limit trip.
8. Emergency shutdown is software-latched and requires inspection/manual reset after the physical cause is cleared.
9. Both boards boot with heater-control outputs inactive.
10. Never auto-restart/resume after reset, brownout, communications loss, or fault.

## Secondary local safety

- Monitor every enabled temperature channel, independent of zone mapping.
- Invalid, stale, disconnected, wrong-type, implausible, or out-of-range data is a fault, not a zero reading.
- Detect converter faults, excessive rate, sensor disagreement, and local overtemperature.
- Monitor CAN heartbeat and controller bus-off/error state.
- Force all four outputs inactive on critical faults and latch.
- Continue local safety monitoring without CYD.
- Reject START/RESUME unless all configured interlocks, sensor readiness, outputs, profile, and communication checks pass.

## CYD safety

- Main-board watchdog and CAN heartbeat.
- Emergency shutdown user command and optional local physical input.
- Dedicated hardwired shutdown request independent of CAN.
- Display fault state and distinguish local request from confirmed hardware status.
- Stale CAN data means link fault and no normal firing controls.
- Do not present software-requested OFF as proof heater power is disconnected.

## CAN faults

During firing, treat heartbeat timeout, bus-off, persistent bus errors, malformed safety payloads, stale/duplicate command sequence, and session mismatch as critical. Secondary forces controlled outputs inactive and latches fault. CAN recovery does not authorize restart.

## Output interface

- Four independent ON/OFF command channels.
- Configurable polarity only after electrical verification.
- Hardware must keep relay input inactive during MCU reset, boot, brownout, power loss, or disconnected harness.
- Protected interface matched to external relay/SSR input.
- Never drive bare relay coils directly from ESP32 GPIO.
- UI distinguishes command from feedback; absent feedback means unverified.
- Do not claim firmware OFF can interrupt a failed-closed SSR.

## Bring-up rule

Initial testing must use simulation and low-voltage indicator loads only. Keep kiln heating power disconnected until wiring, physical E-stop, independent high-limit, contactor behavior, isolation, grounding, and output interface have been reviewed and tested by a qualified person.
