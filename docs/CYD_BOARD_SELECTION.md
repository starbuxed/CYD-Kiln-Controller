# CYD Board Selection and CAN Pin Verification

The CYD main board provides touchscreen, local web app, Wi-Fi, profiles, history, and SD-card assets. CAN needs two accessible GPIO signals (TWAI TX/RX) plus an external transceiver. A separate shutdown-request interface may require another GPIO or connector pin.

For every physical board:
1. Record exact model/revision, ESP32 module, display IC, touch IC, flash/PSRAM, SD interface, and exposed headers.
2. Obtain or verify the schematic.
3. Confirm CAN TX/RX are exposed and suitable; avoid input-only, boot-strapping, flash, reserved, or peripheral-conflicting pins.
4. Confirm no conflict with TFT, touch, SD, backlight, or onboard peripherals.
5. Confirm separate shutdown-request interface wiring.
6. Only then define macros and mark the board profile verified.

Both boards require a CAN transceiver compatible with MCU logic voltage. TWAI is not a transceiver. Select by datasheet and provide CANH, CANL, reference-ground/isolation strategy, and exactly two end-of-line 120-ohm terminators.

Keep CAN TX, CAN RX, and shutdown pins disabled until exact hardware is known. Build targets may compile with transport disabled, but firmware must report CAN unavailable and refuse to arm/start.

The exact CYD variant is not yet confirmed. Do not publish a live wiring pinout until the physical board and schematic are verified.
