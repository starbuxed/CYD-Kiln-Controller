# CYD Board Selection Guide

The firmware provides selectable board profiles through PlatformIO environments. A profile selects the expected MCU family, panel dimensions, display-controller family, and touch-controller family.

**A model number is not always enough.** Vendor batches and clones may ship with different display-controller ICs or touch variants. Inspect the actual board, check its silkscreen and connectors, and verify the schematic/controller markings before enabling a driver or connecting external hardware.

## Included profiles

| PlatformIO environment | Common model / variant | MCU family | Display | Touch | Status |
|---|---|---|---|---|---|
| `cyd_28_ili9341_xpt2046` | ESP32-2432S028R classic | ESP32-WROOM-32 | 2.8-inch, ILI9341 SPI | XPT2046 resistive | Default reference profile; confirm batch |
| `cyd_28_st7789_xpt2046` | 2.8-inch ST7789 revision | ESP32-WROOM-32 | 2.8-inch, ST7789 SPI | XPT2046 resistive | Same common pin layout; panel settings need confirmation |
| `cyd_28_ili9341_cst820` | ESP32-2432S028C capacitive variant | ESP32-WROOM-32 | 2.8-inch, ILI9341-family SPI | CST820 capacitive | Catalogued; pin map not verified |
| `cyd_24_family` | ESP32-2432S024 family | ESP32-WROOM-32 | 2.4-inch, ILI9341 or ST7789 SPI | No-touch/resistive/capacitive variants | Catalogued; pin map not verified |
| `cyd_32_st7789` | ESP32-2432S032 family | ESP32 family | 3.2-inch, ST7789 SPI | Touch variant varies | Catalogued; pin map not verified |
| `cyd_35_st7796_xpt2046` | ESP32-3248S035R | ESP32 family | 3.5-inch, ST7796 SPI | Resistive variant; exact controller TBD | Catalogued; pin map not verified |
| `cyd_35_st7796_gt911` | ESP32-3248S035C | ESP32 family | 3.5-inch, ST7796 SPI | GT911 capacitive | Catalogued; pin map not verified |
| `cyd_43_s3_rgb_gt911` | ESP32-8048S043 | ESP32-S3 | 4.3-inch, 800x480 RGB | GT911 capacitive | Separate S3/RGB pin map required |
| `cyd_70_s3_rgb_gt911` | ESP32-8048S070 | ESP32-S3 | 7-inch, 800x480 RGB | GT911 capacitive | Separate S3/RGB pin map required |
| `custom_cyd` | Custom / clone | User-defined | User-defined | User-defined | Requires local profile definition |

The 4.3-inch and 7-inch S3 panels are not SPI drop-in replacements for the classic 2.8-inch CYD. Their RGB display buses require a different display driver, memory configuration, and GPIO map.

## Selecting a board

From `firmware/esp32`, run the matching environment:

```sh
pio run -e cyd_28_ili9341_xpt2046
pio run -e cyd_28_st7789_xpt2046
pio run -e cyd_28_ili9341_cst820
pio run -e cyd_24_family
pio run -e cyd_32_st7789
pio run -e cyd_35_st7796_xpt2046
pio run -e cyd_35_st7796_gt911
pio run -e cyd_43_s3_rgb_gt911
pio run -e cyd_70_s3_rgb_gt911
```

The current startup program reports the selected profile over serial. These environments are build-selection scaffolding, not a claim that display/touch drivers have been validated on every hardware revision.

## Recommended first board

For the first prototype, use the classic **Sunton ESP32-2432S028R, 2.8-inch, ILI9341 display, XPT2046 resistive touch**. It is the most widely documented CYD variant and has a large community example base. It has very limited free GPIO, which is why this project uses the Pro Micro as a dedicated I/O coprocessor.

When ordering, ask the seller to confirm the exact model, display controller, touch controller, ESP32 module, and flash size. USB connector count alone does not reliably identify the display controller.

## Classic 2.8-inch reference pin map

The common ESP32-2432S028R SPI layout is represented in `firmware/esp32/include/board_profiles.h`:

| Signal | GPIO |
|---|---:|
| TFT MISO | 12 |
| TFT MOSI | 13 |
| TFT SCLK | 14 |
| TFT CS | 15 |
| TFT DC | 2 |
| TFT reset | Board reset / -1 |
| TFT backlight | 21 |
| XPT2046 CLK | 25 |
| XPT2046 MOSI | 32 |
| XPT2046 MISO | 39 |
| XPT2046 CS | 33 |
| XPT2046 IRQ | 36 |
| Proposed Pro Micro UART2 RX | 22 |
| Proposed Pro Micro UART2 TX | 27 |

GPIO22/GPIO27 are proposed UART pins because they are exposed on common 2.8-inch boards. Verify the actual connector pin order and schematic before wiring. GPIO35 is input-only; GPIO21 is used for the TFT backlight. The display, touch, SD slot, and onboard peripherals consume many GPIOs.

Do not connect the Pro Micro until its logic voltage is known. A 5 V Pro Micro TX signal must not be connected directly to an ESP32 RX pin; use appropriate level conversion.

## How to identify the common variants

1. Read the board model printed on the PCB or product label.
2. Count USB connectors, but do not treat USB count as definitive proof of the display IC.
3. Check the display controller marking or identify it with a known-good low-risk display test.
4. Identify the touch controller: XPT2046, CST820, and GT911 are not interchangeable.
5. Confirm display color order, inversion, orientation, touch calibration, and backlight polarity.
6. Record flash size, PSRAM presence, and exact ESP32 module.
7. Only then mark a profile's pin map as verified.

## Sources and evidence

- [Atomic14 CYD family guide](https://www.atomic14.com/esp32/boards/cheap-yellow-display/)
- [Atomic14 classic 2.8-inch overview](https://www.atomic14.com/esp32/boards/esp32-2432s028r/)
- [Community 2.8-inch CYD board specifications](https://github.com/johnwcha/esp32-cyd-2.8/blob/main/BOARD_SPECS.md)
- [Community pinout reference](https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display)
- [LCD Wizard board presets and caveats](https://lcdwizard.com/docs/boards)

These references are useful starting points, not substitutes for the exact board schematic or inspection of the physical unit.
