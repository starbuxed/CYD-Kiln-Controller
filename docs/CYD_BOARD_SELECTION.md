# CYD Board Selection Guide

The firmware provides selectable board profiles through PlatformIO environments. A profile selects the expected MCU family, panel dimensions, display-controller family, and touch-controller family.

**A model number is not always enough.** Vendor batches and clones may ship with different display-controller ICs or touch variants. Inspect the actual board, check its silkscreen and connectors, and verify the schematic/controller markings before enabling a driver or connecting external hardware.

## Included profiles

| PlatformIO environment | Common model / variant | MCU family | Display | Touch | Status |
|---|---|---|---|---|---|
| `cyd_28_ili9341_xpt2046` | ESP32-2432S028R classic | ESP32-WROOM-32 | 2.8", 240×320, ILI9341 SPI | XPT2046 resistive | Reference profile; confirm batch |
| `cyd_28_st7789_xpt2046` | 2.8" dual-USB / ST7789 revision | ESP32-WROOM-32 | 2.8", 240×320, ST7789 SPI | XPT2046 resistive | Controller and color/inversion need confirmation |
| `cyd_28_ili9341_cst820` | ESP32-2432S028C capacitive variant | ESP32-WROOM-32 | 2.8", 240×320, ILI9341 SPI | CST820 capacitive | Pin map not enabled |
| `cyd_24_family` | ESP32-2432S024 family | ESP32-WROOM-32 | 2.4\", ILI9341 or ST7789 SPI | N/R/C variants | Catalogued; pin map not verified |\n| `cyd_32_st7789` | ESP32-2432S032 family | ESP32-WROOM-32 | 3.2\", ST7789 SPI | Touch variant varies | Catalogued; pin map not verified |\n| `cyd_35_st7796_xpt2046` | ESP32-3248S035R | ESP32-WROOM-32 family | 3.5", 320×480, ST7796 SPI | Resistive touch | Verify touch IC and pin map |
| `cyd_43_s3_rgb_gt911` | ESP32-8048S043 | ESP32-S3 | 4.3", 800×480 RGB | GT911 capacitive | Separate S3/RGB pin map required |
| `cyd_70_s3_rgb_gt911` | ESP32-8048S070 | ESP32-S3 | 7.0", 800×480 RGB | GT911 capacitive | Separate S3/RGB pin map required |
| `custom_cyd` | Custom / clone | User-defined | User-defined | User-defined | Requires local profile definition |

The 4.3-inch and 7-inch S3 panels are not SPI drop-in replacements for the classic 2.8-inch CYD. Their RGB display buses require a different display driver, memory configuration, and GPIO map.

## Selecting a board

From `firmware/esp32`, run:

```sh
pio run -e cyd_28_ili9341_xpt2046
pio run -e cyd_28_st7789_xpt2046
pio run -e cyd_28_ili9341_cst820
pio run -e cyd_35_st7796_xpt2046
pio run -e cyd_43_s3_rgb_gt911
pio run -e cyd_70_s3_rgb_gt911
```

The current startup program only reports the selected profile over serial. These environments are build-selection scaffolding, not a claim that display/touch drivers have been validated on every hardware revision.

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

This is only the classic reference map. GPIO availability is limited because display, touch, SD, and onboard peripherals consume pins. Do not allocate a kiln sensor, relay, or UART signal to a GPIO based only on a generic online pinout. Check the exact board schematic and use an external I/O coprocessor for kiln I/O as designed.

## How to identify the common 2.8-inch variants

1. Read the board model printed on the PCB or product label.
2. Count USB connectors, but do not treat USB count as definitive proof of the display IC.
3. Check the display controller marking or identify it from a known-good low-risk display test.
4. Identify the touch controller: resistive XPT2046 and capacitive CST820 are not interchangeable.
5. Confirm display color order, inversion, orientation, touch calibration, and backlight polarity.
6. Record flash size, PSRAM presence, and exact ESP32 module.
7. Only then mark a profile's pin map as verified.

## Sources and evidence

- Atomic14 CYD family guide: https://www.atomic14.com/esp32/boards/cheap-yellow-display/
- Atomic14 classic 2.8-inch overview: https://www.atomic14.com/esp32/boards/esp32-2432s028r/
- Community pinout reference: https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display
- LCD Wizard board presets and caveats: https://lcdwizard.com/docs/boards

These references are useful starting points, not substitutes for the exact board schematic or inspection of the physical unit.
