# Dual-Controller Master Power Interlock

## Intended behavior

There is one master-power permit relay controlled by the CYD main board and one controlled by the ESP32-S3 secondary board. These are not extra heating-zone outputs.

The two relay coils are controlled independently. Their normally-open (NO) contacts are wired in series in the master power-contactor control circuit. Both relays must be energized before the contactor can energize.

Conceptual control path:

    Separate low-voltage control supply
       -> Physical latching E-STOP (NC)
       -> Independent kiln HIGH-LIMIT (NC)
       -> CYD master permit relay K1 (NO contact)
       -> S3 master permit relay K2 (NO contact)
       -> Safety relay / contactor control input
       -> Control return

This is a functional concept only. Final topology, contactor/safety relay selection, protection, and feedback must be engineered for the actual kiln and jurisdiction by a qualified electrical professional.

## Relay behavior

| Condition | CYD K1 | S3 K2 | Main contactor request |
|---|---|---|---|
| Both controllers healthy and firing authorized | Energized | Energized | Can energize |
| CYD loses power, crashes, or drops permit | Released | Any state | Drops |
| Secondary loses power, crashes, or drops permit | Any state | Released | Drops |
| CAN communication lost | CYD releases | S3 releases on timeout | Drops |
| Sensor fault / overtemperature | CYD releases on reported fault | S3 releases locally | Drops |
| E-stop pressed | Irrelevant | Irrelevant | Hardware circuit opens |
| Independent high-limit trips | Irrelevant | Irrelevant | Hardware circuit opens |
| Boot/reboot/reset | Released | Released | Cannot energize |
| No active firing | Released | Released | Cannot energize |

## Firmware requirements

### CYD main board

- Configure one dedicated low-voltage output to drive the CYD master permit relay module.
- Keep output inactive at boot and whenever CAN is absent/stale.
- Energize K1 only when the S3 reports FIRING, the CAN controller is healthy, status is fresh, and no S3 fault is latched.
- On emergency-stop request or critical local failure, release K1 immediately and send CMD_ESTOP as supplementary CAN notification.
- Never restore K1 from saved state after reboot.

### Secondary ESP32-S3

- Configure one dedicated low-voltage output to drive the S3 master permit relay module.
- Keep output inactive at boot/reset and whenever local readiness is false.
- Energize K2 only when local controller is FIRING, local sensors/interlocks are valid, no fault is latched, and CYD heartbeat/CAN bus are healthy.
- On local fault, CAN timeout, CAN bus-off, emergency-stop, or STOP/abort, release K2 and force all four heating outputs inactive.
- Never restore K2 from saved state after reboot.

## Electrical implementation

- Use two separate relay modules or interposing relays, one driven by each MCU through a suitable protected interface.
- Wire the two NO dry contacts in series. Do not connect the ESP32 GPIO signals together.
- Each MCU drives only its own relay input. Verify logic voltage, input current, polarity, isolation, and reset behavior from the module datasheet.
- Use external biasing so each relay driver is guaranteed inactive while its MCU is reset or unpowered.
- Contacts must be rated for the safety-control circuit. They are not intended to switch kiln mains power directly.
- Prefer a properly selected safety relay/contactor with monitored feedback (EDM) and suitable force-guided contacts when the risk assessment requires a safety-rated architecture.
- A simple series pair of general-purpose relays is an independent permission interlock, but is not automatically a certified safety function. Welded contacts, wiring shorts, and common-cause failures must be considered.
- Physical E-stop and independent high-limit must interrupt the contactor/safety relay control path directly, without either ESP32.
- Do not use software or CAN status to claim mains power is definitely absent. Add appropriately designed auxiliary-contact or voltage feedback if the UI needs verified state.

## GPIO configuration

The exact CYD variant/revision remains unconfirmed. Its master permit GPIO remains disabled (KILN_MAIN_POWER_PERMIT_PIN=-1) until an exposed, safe, unused pin is confirmed. The ESP32-S3 DevKitC-1 reference build uses GPIO7 for the separate master permit output; verify the exact board/header before wiring. Relay outputs remain disabled until interfaces are checked.

## Commissioning

Test with kiln mains disconnected and a low-voltage indicator load:
1. K1 alone energized: contactor control path stays open.
2. K2 alone energized: contactor control path stays open.
3. Both energized: contactor control request may close.
4. Remove CYD power: contactor request opens.
5. Remove S3 power: contactor request opens.
6. Disconnect CAN during a firing simulation: both permits drop.
7. Trigger each software fault: both permits drop.
8. Operate physical E-stop and high-limit: circuit opens with both MCUs disconnected.
9. Verify reset/brownout and cable disconnection cause the de-energized state.

Only a qualified professional should approve the final connection to kiln power equipment.
