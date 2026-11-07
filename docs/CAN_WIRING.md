# CAN and Master-Power Permit Wiring Reference

> This reference is for the classic 2.8-inch ESP32-2432S028R CYD and ESP32-S3-DevKitC-1 only. Board revisions/clones vary. Check the actual schematic and connector markings.

## 1. CYD CAN connection

The CYD has no CAN transceiver built in. Use one external transceiver compatible with ESP32 3.3V logic.

| CYD | GPIO | Transceiver |
|---|---:|---|
| TWAI TX | 27 | TXD input |
| TWAI RX | 22 | RXD output |
| 3.3V | 3V3 | VCC only if module supports 3.3V |
| GND | GND | GND |

GPIO27/22 are the project's reference pins for the classic CYD profile. Verify connector orientation and pin order on the exact board.

## 2. Secondary ESP32-S3 CAN connection

The current PlatformIO reference target is ESP32-S3-DevKitC-1.

| ESP32-S3 | GPIO | Transceiver |
|---|---:|---|
| TWAI TX | 5 | TXD input |
| TWAI RX | 4 | RXD output |
| 3.3V | 3V3 | VCC if module supports 3.3V |
| GND | GND | GND |

Each board requires its own external CAN transceiver. CANH connects to CANH and CANL to CANL. Use a twisted pair, short stubs, and exactly two 120-ohm terminators at the physical ends of a linear bus. Use a deliberate ground/reference or isolation scheme. Keep the CAN cable away from mains/heater wiring.

## 3. Two independent master-power permit relays

There is one master-power permit relay driven by each board. These are not heater-zone outputs.

| Board | GPIO output | Purpose | Initial config |
|---|---:|---|---|
| CYD main | Not assigned (-1) | Drives CYD permit relay K1 | Disabled until exact CYD header pin is verified |
| ESP32-S3 DevKitC-1 | GPIO7 | Drives S3 permit relay K2 | Reference pin; verify board and output-driver wiring |

Each GPIO drives only its own low-voltage relay-module input using a suitable protected interface. Each relay's normally-open dry contact is wired in series with the other relay's normally-open contact in the master power-contactor control circuit. Do not connect the two GPIOs together.

Conceptual control path:

    Low-voltage control supply
      -> Physical latching E-STOP (NC)
      -> Independent kiln HIGH-LIMIT (NC)
      -> CYD permit relay K1 (NO contact)
      -> S3 permit relay K2 (NO contact)
      -> Properly selected safety relay / contactor control input
      -> Return

Both K1 and K2 must energize before the main contactor control path can close. If either ESP32 loses power, crashes, loses permission, or drops its relay, the series path opens.

This is a functional concept, not a mains wiring plan. Select a suitable safety relay/contactor, protective components, monitoring/feedback, enclosure, and wiring under qualified electrical engineering review. Ordinary relays wired in series are not automatically a certified safety function; consider welded contacts, shorts, common-cause failures, and monitored force-guided contacts.

The physical E-stop and independent high-limit must directly interrupt the safety-control path without either ESP32. Never use the MCU relay outputs to switch kiln mains directly.

## 4. CYD permit pin

The exact CYD revision is not confirmed. The CYD master-permit GPIO remains disabled as -1 until an exposed, unused output pin is selected and verified against the actual board. GPIO27 and GPIO22 are assigned to CAN in this reference profile. Do not repurpose display, touch, SD, backlight, boot-strapping, or input-only pins. Update KILN_MAIN_POWER_PERMIT_PIN only after verification.

## 5. Startup and validation

All outputs start released. CYD K1 is energized only while the S3 reports active FIRING state over a healthy CAN link and no fault is reported. S3 K2 is energized only while locally FIRING, all configured safety sensors are valid, local faults are clear, the CAN bus and host heartbeat are healthy, and all four output channels are configured. Reboot never restores an energized permit.

Test with kiln mains disconnected:
1. K1 alone energized: contactor control path remains open.
2. K2 alone energized: contactor control path remains open.
3. Both energized: contactor control request may close.
4. Remove CYD power: path opens.
5. Remove S3 power: path opens.
6. Break CAN during firing simulation: both permits drop.
7. Trigger local sensor/control fault: S3 permit drops; CYD drops when fault status arrives.
8. Test physical E-stop and independent high-limit with both controllers disconnected.
9. Verify reset/brownout and a disconnected wire yield the de-energized condition.

## References

- [Espressif ESP32-S3-DevKitC-1 hardware reference](https://documentation.espressif.com/api/resource/path/docs/projects/esp-dev-kits/en/latest/esp32s3/esp32-s3-devkitc-1/user_guide_v1.0.html)
- [CYD 2.8-inch pin reference](https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display)
