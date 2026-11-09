# CYD Touchscreen Interface

## First supported target

The first implementation targets the default classic 2.8-inch CYD profile: ILI9341 240 x 320 display in landscape orientation and XPT2046 resistive touch.

- Display SPI: MOSI 13, MISO 12, SCLK 14, CS 15, DC 2, backlight 21.
- Touch SPI: SCLK 25, MISO 39, MOSI 32, CS 33, IRQ 36.

These are the existing classic CYD reference pins in the board profile. Confirm the exact board revision and schematic before wiring or relying on them. Other CYD profiles deliberately build with the touchscreen driver disabled until their display/touch hardware is verified.

## Screens

- Home dashboard: S3 state, CAN link, four thermocouple readings, sensor modes, faults, start/pause/resume/stop.
- Profile: up to four ramp/soak segments, with target temperature, ramp rate, soak duration, local NVS persistence, and upload to the S3.
- Sensors: four channels; Off / Log only / Log and control / Log and safety; thermocouple type and calibration offset.
- Zones: four zone hysteresis and setpoint-offset controls.
- Info: heartbeat age, sensor-valid mask, commanded-output mask, and fault flags.
- Touch calibration: three-point affine calibration stored in NVS.

## Start checks

The CYD only offers the START confirmation with fresh S3 status, healthy CAN, no reported fault, an uploaded profile, S3 state IDLE/ARMED, and valid readings for every sensor whose mode is not Off. The S3 remains authoritative and independently validates the start request.

The touchscreen E-STOP latches the CYD master-power permit off and sends an additional CAN E-stop command. It is a software request only, not a physical E-stop and not a replacement for the independent hardwired E-stop, high-limit, or qualified contactor safety circuit.

## Current limitations

- S3 acceptance remains authoritative; profile upload feedback is not yet a complete profile read-back/verification workflow.
- Thermocouple type and calibration settings are stored locally on the CYD and sent to the S3; S3-side nonvolatile persistence is not implemented.
- Zone minimum-on/minimum-off times are fixed at 10 seconds in this screen.
- Zone-to-sensor mapping is not exposed because the current S3 protocol does not define an independent sensor-to-zone assignment.
- Other CYD display/touch profiles remain disabled pending verified pin maps and drivers.
- Wi-Fi setup, local web application, SD asset browser, themes/icons, and firing history are separate unfinished work.

Use simulation and low-voltage indicator loads for bring-up. Do not test with live kiln heating power.
