# CAN and Dual Shutdown Wiring Reference

> This pin map is for the project's current reference targets only: classic 2.8-inch ESP32-2432S028R CYD (single ESP32-WROOM) as main, and Espressif ESP32-S3-DevKitC-1 as secondary. CYD clones/revisions vary. Confirm board markings and continuity against the actual hardware before soldering or connecting anything.

## 1. CYD main board

The CYD has no CAN transceiver built in. Fit one external 3.3V-logic CAN transceiver module (for example, an SN65HVD230-based module after checking its exact schematic).

| CYD signal | GPIO / connector | Transceiver connection |
|---|---|---|
| TWAI TX | GPIO27, CN1 | TXD input |
| TWAI RX | GPIO22, CN1 | RXD output |
| 3.3V | CN1 | VCC (only if module explicitly supports 3.3V) |
| GND | CN1 | GND |

For the common CN1 header, GPIO27 and GPIO22 are broken out. Confirm the connector orientation and pin order on the exact CYD revision. Do not infer connector position from a board photo alone.

### CYD shutdown request

The classic 2.8-inch CYD's easily accessible header pins are already used by CAN. The reference build therefore repurposes **GPIO4**, the active-low red RGB LED control, as the main-board shutdown-request signal. The red LED then doubles as a shutdown indicator.

- Bring out GPIO4 from the exact board's LED/GPIO4 trace or suitable pad using a short, strain-relieved wire.
- Use only as a low-voltage logic request into an appropriately designed isolated/interface stage.
- Do not connect GPIO4 directly to a contactor coil, mains circuit, relay coil, or safety contact.
- Do not exceed the GPIO's electrical limits or load it with an optocoupler LED without a correctly engineered current-limiting/buffer stage.
- GPIO4 is configured as active-low open-drain in firmware. The external safety interface must define the safe condition for reset, high impedance, cable break, and loss of CYD power; firmware output alone does not provide that guarantee.

If the physical CYD revision does not expose a usable GPIO4 connection, keep `KILN_MAIN_SHUTDOWN_REQUEST_PIN=-1` and do not claim the main hardwired request is wired. The physical latching E-stop must still interrupt the safety-contactor circuit directly.

## 2. Secondary ESP32-S3 DevKitC-1

The selected reference PlatformIO target uses the Espressif ESP32-S3-DevKitC-1. Its J1 header exposes GPIO4, GPIO5, and GPIO6.

| ESP32-S3 signal | GPIO | Transceiver / safety interface |
|---|---:|---|
| TWAI TX | GPIO5 | CAN transceiver TXD input |
| TWAI RX | GPIO4 | CAN transceiver RXD output |
| 3.3V | 3V3 header | VCC (module must support 3.3V) |
| GND | GND header | GND |
| Secondary shutdown request | GPIO6 | Isolated low-voltage request interface |

GPIO4/5/6 are assigned only for this DevKitC-1 reference. If a different S3 board is used, re-verify every pin before changing build flags.

## 3. CAN bus between transceivers

| Main-side transceiver | Secondary-side transceiver |
|---|---|
| CANH | CANH |
| CANL | CANL |
| Reference GND, if non-isolated design requires it | Reference GND |

- Use twisted-pair cable for CANH/CANL.
- Use a linear bus topology; keep stubs short.
- Install 120-ohm termination at the two physical ends only. With two endpoints, one termination at each transceiver end.
- Never add a 120-ohm resistor at every node.
- Keep CAN wiring away from heater mains wiring, contactors, and high-current conductors.
- Choose a transceiver/module with a suitable common-mode range and environmental rating. Consider galvanic isolation and isolated power where ground offsets/noise warrant it.
- Firmware initially targets 500 kbit/s. Lower bitrate may be preferable for longer/noisier wiring after validation.

## 4. Independent shutdown circuit

The two GPIOs are **shutdown requests**, not power contacts:

- CYD GPIO4 -> main-board isolated request interface.
- ESP32-S3 GPIO6 -> secondary-board isolated request interface.
- Physical latching E-stop -> direct series interruption of the external safety-contactor control circuit.
- Independent kiln high-limit -> independent hardware interruption path.

The safety circuit must be engineered so either board request can de-energize its safety enable path. Do not connect the two GPIOs together. Do not use a GPIO as the only safety channel. Use an appropriate fail-safe energized-to-run architecture, hardware latching, and external monitoring so reset, loss of MCU power, or wire break cannot be mistaken for a healthy state.

Both firmware images assert their request when they detect/receive an emergency shutdown. The secondary also turns all controlled outputs inactive and latches the fault. The E-stop and high-limit must function when both ESP32s are unpowered or crashed.

## 5. First power-up checklist

1. Keep kiln heater power disconnected.
2. Verify CYD model and that CN1 GPIO27/GPIO22 and GPIO4 trace match this document.
3. Verify DevKitC-1 board revision and J1 GPIO4/GPIO5/GPIO6 positions.
4. Verify each transceiver supply voltage and TXD/RXD logic compatibility from its datasheet.
5. Verify CANH/CANL polarity and exactly two end terminations.
6. Power only the logic boards and test CAN heartbeat.
7. Test fragmented status messages, missing heartbeat, malformed fragments, and bus-off behavior.
8. Test each independent shutdown request with the isolated low-voltage safety-interface test fixture.
9. Test physical E-stop and high-limit action independently of firmware.
10. Test output channels with low-voltage indicator loads only.

Do not connect kiln heating power until the completed electrical design and independent safety circuit have been reviewed and tested by a qualified person.

## References

- [Espressif ESP32-S3-DevKitC-1 official hardware reference](https://documentation.espressif.com/api/resource/path/docs/projects/esp-dev-kits/en/latest/esp32s3/esp32-s3-devkitc-1/user_guide_v1.0.html)
- [CYD 2.8-inch connector and GPIO reference](https://github.com/khanhj/ESP32_2432S028/blob/main/PINS.md)
- [CYD board pin mapping and SD/display peripherals](https://esp3d.io/esp3d-tft/version_1x/hardware/esp32/sunton-28-2432/)
