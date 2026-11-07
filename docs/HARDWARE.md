# Hardware Selection and Bring-up

## Current project decisions

| Subsystem | Current direction | Status |
|---|---|---|
| Operator interface | Existing CYD ESP32 | Exact revision still required |
| Dedicated controller | ESP32-S3 development board, initially ESP32-S3-DevKitC-1 | Recommended; pin map to verify |
| Temperature inputs | Four independently configurable channels | User selected mixed thermocouples; exact probes TBD |
| Thermocouple front ends | Four MAX31856-class interfaces, one per channel | Candidate; verify modules and probe compatibility |
| Heater control commands | Four independent digital ON/OFF requests | User will select/provide external relay or SSR hardware |
| Logic interface | Four protected/isolated low-voltage output interfaces | Design to match the external relay-module input requirements |
| Kiln power switching | External relay/SSR/contactor equipment | Outside this controller firmware's design scope |
| Independent safety | Kiln-side high-limit and power protection | Must remain independent of the microcontroller |

## What this controller will do

The ESP32-S3 controller will read four temperatures and operate four independent digital control outputs. Each output has two logical states: ON and OFF. The control firmware will not need to know the mains voltage, heater current, phase arrangement, or relay load rating in order to issue those logical commands.

The user is responsible for selecting and installing the external relay/SSR/power-switching equipment appropriate for the kiln. The controller's low-voltage output interface must be electrically compatible with the selected relay module's control input.

The ESP32-S3 GPIO pins must not directly drive arbitrary relay coils or heater loads. Use a suitable interface stage (for example, a transistor or optically isolated driver board) matched to the relay module's input voltage, current, polarity, and isolation requirements. The firmware will expose configurable active-high/active-low logic and a defined inactive state. Hardware must hold every output inactive while the ESP32-S3 is reset, booting, unpowered, or in a fault state.

This project is implementing relay **control signals**, not designing the kiln's mains power circuit.

## Dedicated controller recommendation

Use a separate ESP32-S3 board for the kiln-control processor rather than a Pro Micro or RP2040 for this iteration. A suitable development starting point is the Espressif ESP32-S3-DevKitC-1. For a final installation, a carrier PCB can provide sensor connectors, four protected output interfaces, power regulation, and serviceable connectors.

## Four temperature channels and mixed thermocouples

The current assumption is that "mixed" means different thermocouple types can be used on different channels. Each channel must have its own configuration field:

- `sensor_type`: K, J, N, R, S, T, E, or B, limited to types supported by the selected front end and actual probe.
- `front_end`: exact converter/module model.
- `enabled`: channel enabled/disabled.
- `calibration_offset_c`: bounded calibration correction.
- `minimum_valid_c` / `maximum_valid_c`: application limits appropriate to the installed probe and kiln.
- `sample_interval_ms` and `stale_after_ms`.
- `zone_id`: zone controlled by this sensor; one-to-one mapping is the initial default.

A MAX31856 SPI driver is now included in the firmware using Adafruit's library. It is a thermocouple-to-digital converter, not an RTD interface. Each channel can be configured independently for K/J/N/R/S/T/E/B. The driver is present, but SPI and chip-select pins default to disabled. Configure build macros `KILN_TC_SPI_SCLK`, `KILN_TC_SPI_MISO`, `KILN_TC_SPI_MOSI`, and `KILN_TC1_CS_PIN` through `KILN_TC4_CS_PIN` only after verifying the selected S3 board pinout. Confirm each module's electrical design, cold-junction placement, isolation, connector, noise immunity, and usable temperature range. Do not assume a low-cost breakout board provides galvanic isolation.

If any channel is to use an RTD (PT100/PT1000), it needs an RTD-capable front end, such as a MAX31865-class interface. Do not connect an RTD to a MAX31856.

## Four independent digital outputs

The firmware will expose four independently controlled output channels, each with:
- Logical command: ON or OFF.
- Configurable active polarity (active-high or active-low).
- Configurable startup/inactive level.
- A hardware-defined inactive state during reset and boot.
- Fault override that forces all outputs inactive.
- Optional output feedback input, if the user's relay hardware provides one.

The output abstraction will not assume a particular relay board, SSR input voltage, coil voltage, or load. Those are external hardware choices. Before connecting the controller to a relay module, verify its control-input voltage/current, active polarity, isolation, and whether it is a logic input or a bare coil. Do not connect a bare relay coil directly to an ESP32 GPIO.

The four channels are independent commands; the firmware does not infer the physical heater state from the commanded logic level. Without feedback, the UI must label them as commanded ON/OFF, not verified heater power.

## Logic power and communication

- Use a properly regulated low-voltage supply for the ESP32-S3 and sensor front ends.
- Confirm the exact ESP32-S3 board logic voltage and pin availability.
- Use a framed, CRC-protected UART link between the CYD and dedicated controller.
- Both boards use 3.3 V logic, but verify exact pin assignments, cable routing, and grounding.
- Keep sensor wiring separated from relay/heater power wiring.
- Use appropriate interface isolation where required by the actual installation.
- Provide external biasing so all four relay-control inputs remain inactive during MCU reset, boot, brownout, or unpowered conditions.

## Safety boundary

The ESP32-S3 firmware will force its four control requests inactive on boot, reset, watchdog timeout, invalid sensor data, or a latched safety fault. These are software control behaviors, not a certified safety function. The kiln installation still needs its own suitable independent high-limit protection and power-disconnect arrangements. A relay or SSR can fail mechanically or electrically; commanding OFF does not prove that heater power has stopped.

## Bring-up checklist

- [ ] Record exact CYD model/revision and verify schematic.
- [ ] Select exact ESP32-S3 board/module and verify pinout.
- [ ] Identify each probe type, temperature range, construction, and location.
- [ ] Select and verify one compatible sensor front end per channel.
- [ ] Assign and verify the MAX31856 SPI bus and each enabled chip-select pin.
- [ ] Decide whether any channel uses RTD rather than thermocouple.
- [ ] Define zone-to-sensor and zone-to-output mapping.
- [ ] Select external relay/SSR modules (user responsibility).
- [ ] Confirm each relay module's logic input voltage/current and active polarity.
- [ ] Implement four protected output interfaces compatible with those inputs.
- [ ] Decide whether relay-state feedback is available.
- [ ] Verify outputs remain inactive during reset, boot, brownout, and fault.
- [ ] Test all four outputs with low-voltage indicator loads before connecting relay modules.
- [ ] Test sensor open/short, wrong-type, stale, and implausible readings.
- [ ] Verify watchdog and fault-latch behavior.
