# Hardware Selection and Bring-up

Hardware selections must be recorded here before pin-level firmware is written.

## Required bill-of-material decisions

| Subsystem | Required decision |
|---|---|
| CYD ESP32 | Exact module/board revision, display size/controller, touch controller, flash/PSRAM |
| Pro Micro | Exact MCU, operating voltage, clock speed, bootloader |
| Temperature inputs | Sensor type, front-end IC, range, isolation, wiring, connector |
| Outputs | Four channel functions, driver topology, polarity, voltage/current ratings |
| UART | Pins, baud rate, logic-level conversion, grounding/isolation |
| Safety chain | Independent high-limit, contactor, disconnect, fusing, grounding |
| Power | Separate regulated logic supplies, brownout behavior, noise immunity |
| Enclosure | Temperature, dust, electrical and mechanical requirements |

## CYD selection notes

There are multiple CYD variants and clones. Do not assume all use identical GPIO assignments, display controllers, touch controllers, flash, or PSRAM. Capture the exact seller/manufacturer part number and board revision, then verify its schematic before selecting a firmware board definition.

## Temperature inputs

The requirement is up to four kiln temperature channels. The actual sensor technology has not yet been fixed. Select front ends based on the thermocouple/RTD type, required temperature range, accuracy, fault detection, noise environment, and wiring length. Include open-sensor and short/fault detection. Do not assume a sensor's disconnected state can be represented by a numeric temperature.

## Relay/output hardware

Four outputs are required, but the actual loads and switching strategy must be identified. MCU pins must only drive suitable low-voltage driver stages. Do not power kiln elements or other high-current loads through a hobby relay board unless the complete device is explicitly rated and approved for the load and application. Use appropriately rated contactors and independent safety devices designed for the kiln.

## Logic-level compatibility

The ESP32 is a 3.3 V logic device. Common Pro Micro variants may use different logic voltages. Verify the actual hardware before wiring UART TX/RX. Provide appropriate level conversion and electrical protection; do not assume 5 V tolerance on ESP32 pins.

## Bring-up checklist

- [ ] Record exact CYD model/revision and verify schematic.
- [ ] Record exact Pro Micro MCU, voltage, clock, and pinout.
- [ ] Select sensor technology and front-end modules.
- [ ] Select isolated/appropriate output drivers and feedback strategy.
- [ ] Design independent high-limit and power isolation.
- [ ] Document UART pins and voltage conversion.
- [ ] Verify power supplies, grounds, brownout behavior, and fusing.
- [ ] Test logic with all high-power loads disconnected.
- [ ] Test sensor failure cases and stale-data handling.
- [ ] Test all four outputs with low-voltage indicator loads.
- [ ] Verify watchdog and reset safe-state behavior.
