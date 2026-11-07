# Hardware Selection and Bring-up

## Current project decisions

| Subsystem | Current direction | Status |
|---|---|---|
| Operator interface | Existing CYD ESP32 | Family selected; exact revision still required |
| Dedicated kiln controller | ESP32-S3 development board, initially ESP32-S3-DevKitC-1 | Recommended; exact board and pin map to verify |
| Control topology | Four independent heater zones | User selected |
| Temperature inputs | Four individually configurable sensor channels | User selected mixed/configurable thermocouples; exact probes TBD |
| Thermocouple front ends | Candidate: four MAX31856-class interfaces, one per channel | Candidate only; verify module, isolation, range, and probe compatibility |
| RTD support | Add MAX31865-class front end for any RTD channel | Not included in current thermocouple-only assumption |
| Heater outputs | Four isolated low-voltage driver channels | Required; topology and ratings TBD |
| Power supply | Actual kiln supply not yet identified | Open; do not size power components |
| Independent safety | High-limit device(s), safety contactor, disconnect, overcurrent protection | Required; design and ratings TBD |

## Dedicated controller recommendation

Use a separate ESP32-S3 board for the kiln-control processor, rather than a Pro Micro or RP2040 for this iteration. The ESP32-S3 has sufficient processing and peripheral resources for four sensor interfaces, four local control loops, diagnostics, and a framed link to the CYD. It also keeps the project within one microcontroller toolchain.

A suitable development starting point is the Espressif ESP32-S3-DevKitC-1. For a final kiln installation, consider a custom carrier/PCB with screw terminals, protected power, isolated interfaces, clear separation of low-voltage and hazardous-voltage wiring, and serviceable connectors. The DevKit board itself is not an industrial safety controller.

## Four temperature channels and mixed thermocouples

The current assumption is that "mixed" means different thermocouple types can be used on different channels. Each channel must have its own configuration field, for example:

- `sensor_type`: K, J, N, R, S, T, E, or B (only enable types supported by the selected front-end and actual probe).
- `front_end`: exact converter/module model.
- `enabled`: channel enabled/disabled.
- `calibration_offset_c`: bounded calibration correction.
- `minimum_valid_c` / `maximum_valid_c`: application limits appropriate to the installed probe and kiln.
- `sample_interval_ms` and `stale_after_ms`.
- `zone_id`: zone controlled by this sensor, with one-to-one mapping as the initial default.

A MAX31856 is a thermocouple-to-digital converter, not an RTD interface. It is a candidate for each thermocouple channel because it supports multiple thermocouple types. Confirm the exact module's electrical design, cold-junction placement, isolation, wiring connector, noise immunity, and usable temperature range. Do not assume a low-cost breakout board provides galvanic isolation.

If any channel is to use an RTD (PT100/PT1000), it needs an RTD-capable front end, such as a MAX31865-class interface, and a separately validated sensor configuration. Do not connect an RTD to a MAX31856.

Four thermocouple channels should be electrically and physically arranged to reduce noise pickup from heater wiring. Use appropriate thermocouple extension wire and connectors of the matching type, correct polarity, strain relief, and separation from mains/heater conductors. Sensor wiring and grounding/shielding must follow the selected probe and front-end manufacturers' instructions.

## Four independent heater circuits

Each of the four control outputs is assigned to one independently controlled heater circuit/zone by default. The dedicated ESP32-S3 computes four separate control demands. The output hardware must be selected for the actual heater voltage, current, phase arrangement, duty-cycle strategy, fault behavior, and enclosure temperature.

Do not select SSR current ratings, contactor ratings, fuses, wire gauges, heat sinks, branch protection, or disconnect hardware until the actual kiln supply and each heater circuit's electrical ratings are known. Those choices require the kiln nameplate, wiring diagram, measured/known heater load, applicable electrical code, and qualified review.

The ESP32-S3 pins must only drive suitable low-voltage isolated driver inputs. They must never drive heater coils/elements directly. A SSR may fail shorted; software commanding its input low does not prove that heater power is off. Provide an appropriately rated independent safety contactor/final disconnect and suitable feedback/current monitoring where required by the safety analysis.

The control strategy may use zero-cross SSRs with time-proportioning control for compatible resistive heater loads, but the cycle time and switching scheme must be selected for the specific kiln and SSR. Do not assume phase-angle control or a generic relay module is suitable.

## Unknown kiln supply: required information before power-stage design

The user selected "other / not sure" for the supply. Keep all mains-power component choices open. Before any power wiring design, identify:

1. Kiln nameplate voltage and frequency.
2. Single-phase, split-phase, or three-phase supply and number of conductors.
3. Nameplate/current rating and total rated input power.
4. Each of the four heater circuit ratings and how elements are grouped.
5. Whether the kiln already has contactors, SSRs, fuses, a high-limit controller, or a factory control board.
6. Whether the existing kiln is being retrofitted or this is a new kiln build.

Do not infer a US 240 V single-phase supply from location or assume the heater circuits are equal. The word "other" is an unresolved electrical specification, not permission to guess.

## Logic power, communication, and isolation

- Use a properly regulated low-voltage supply for the ESP32-S3 and sensor front ends, with brownout behavior documented.
- Confirm the exact ESP32-S3 board logic voltage and pin availability.
- Use a robust, framed, CRC-protected UART link between the CYD and dedicated controller.
- Both are ESP32-family 3.3 V logic devices, but still verify board pin assignments, ground strategy, cable routing, and any required isolation.
- Keep sensor, logic, and heater wiring separated according to the electrical design.
- Consider isolated SPI/front ends or isolated sensor interfaces where the sensor wiring and kiln grounding/noise environment require it.
- The controller must boot with all heater driver requests inactive. Hardware pull-downs or equivalent circuitry should hold driver inputs in the safe state during reset and boot.

## Independent safety hardware

The safety chain must not depend on the ESP32-S3, CYD, firmware, Wi-Fi, browser, or serial link. At minimum, the design must include a suitable independent over-temperature/high-limit function and a final power-disconnect device. The number, placement, and ratings of high-limit sensors and switching devices must be determined from the kiln design and applicable requirements.

Provide appropriate overcurrent protection, protective earthing/grounding, enclosure, conductor sizing, isolation, creepage/clearance, and strain relief. Have mains/high-current design and commissioning reviewed by a qualified professional.

## Bring-up checklist

- [ ] Record exact CYD model/revision and verify schematic.
- [ ] Select exact ESP32-S3 board/module and verify pinout, flash, and PSRAM.
- [ ] Record kiln supply voltage, frequency, phase/conductor arrangement, and nameplate rating.
- [ ] Record ratings and wiring topology of all four heater circuits.
- [ ] Identify each probe type, temperature range, construction, and location.
- [ ] Select and verify one compatible sensor front end per channel.
- [ ] Decide whether any channel uses RTD rather than thermocouple.
- [ ] Define zone-to-sensor and zone-to-output mapping.
- [ ] Select isolated low-voltage output drivers and feedback strategy.
- [ ] Design independent high-limit and final power isolation.
- [ ] Document UART pins, grounding, cable, and protection.
- [ ] Verify power supplies, brownout behavior, fusing, and enclosure.
- [ ] Test logic with all high-power loads disconnected.
- [ ] Test sensor open/short, wrong-type, stale, and implausible readings.
- [ ] Test all four outputs with low-voltage indicator loads.
- [ ] Verify watchdog, reset, and fault-latch safe-state behavior.
