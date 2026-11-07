# System Architecture

## Selected controller architecture

The project is moving from a CYD + Pro Micro design to a dual-ESP32 design:

- **CYD ESP32:** operator interface, touchscreen, Wi-Fi, local web server, profile editor, charts, configuration, and history.
- **Dedicated ESP32-S3 controller:** four temperature acquisition channels, four independent heater-control channels, zone PID/time-proportioning logic, local safety supervision, and hardware I/O.
- **Independent safety hardware:** high-limit protection and final heater power disconnect that do not depend on either ESP32, firmware, Wi-Fi, or the browser.

The CYD is not the sole kiln controller. Loss of the display, web server, Wi-Fi, or inter-processor link must not disable the dedicated controller's local fault handling.

## System blocks

```text
 Phone / Tablet / PC browser
          | Local Wi-Fi / LAN
          v
+-------------------------------------+
| CYD ESP32 - Operator Interface      |
| - Touchscreen UI                   |
| - Local web server / REST / events  |
| - Profiles and firing history       |
| - Configuration and user commands  |
| - Remote display of status/alarms   |
+------------------+------------------+
                   | UART, framed and CRC protected
                   | 3.3 V logic or verified isolation
                   v
+-------------------------------------+
| Dedicated ESP32-S3 Kiln Controller  |
| - Four sensor front-end channels    |
| - Four independent zone controllers |
| - Four isolated low-voltage outputs |
| - Local safety state machine        |
| - Sensor freshness/fault monitoring |
| - Watchdogs and event reporting     |
+----------+------------------+-------+
           | SPI              | isolated driver inputs
           v                  v
   +---------------+   +----------------------+
   | 4 x sensor    |   | 4 x properly rated   |
   | front ends    |   | SSR/driver channels  |
   +---------------+   +----------+-----------+
                                 |
                                 v
                       Heater power equipment
                       and independent contactor

 Independent high-limit device(s), final contactor,
 disconnect, overcurrent protection, and grounding
 remain effective if both ESP32s fail.
```

This is a logical architecture, not a wiring diagram or approved power design.

## Dedicated ESP32-S3 responsibilities

The dedicated controller is authoritative for local kiln control. It must:

- Acquire four independent temperature channels and retain per-channel sensor identity/configuration.
- Support a different supported thermocouple type on each channel, subject to the selected front-end and probe compatibility.
- Validate sensor values, sensor fault flags, data freshness, plausible range, and channel configuration.
- Run up to four independent zone-control loops, each associated with a configured temperature channel and heater output.
- Enforce output limits, minimum switching intervals, ramp limits, and validated firing-state transitions.
- Apply a documented safe output state during boot, reset, brownout, watchdog timeout, invalid sensor data, or safety-critical fault.
- Continue local safety supervision if the CYD disconnects.
- Require a fresh explicit start sequence after interruption; never automatically resume a firing.
- Report commanded and physically verified states separately. Physical verification requires suitable feedback hardware.

The dedicated ESP32-S3 must not depend on the CYD to execute a fast local control loop or to turn off outputs on a detected local fault.

## CYD ESP32 responsibilities

- Own the touchscreen user experience and locally hosted browser application.
- Manage Wi-Fi station/access-point setup and mDNS.
- Store and edit versioned firing profiles and configuration.
- Display four temperatures, four setpoints, four zone states, four outputs, alarms, and firing history.
- Send operator requests to the dedicated controller through a single authenticated/validated command path.
- Display controller heartbeat, sensor freshness, fault latches, and communication status.
- Never directly control heater GPIOs or bypass the dedicated controller's interlocks.
- Never automatically resend start, resume, or output commands after a disconnect.

The CYD may orchestrate profile selection and user interaction, but the dedicated controller must independently enforce the active control limits and safe state.

## Four-zone control model

There are four independently controlled heater circuits. The initial logical mapping is:

| Zone | Temperature input | Control loop | Heater output |
|---|---|---|---|
| 1 | Sensor channel 1 | PID/time-proportioning 1 | Output 1 |
| 2 | Sensor channel 2 | PID/time-proportioning 2 | Output 2 |
| 3 | Sensor channel 3 | PID/time-proportioning 3 | Output 3 |
| 4 | Sensor channel 4 | PID/time-proportioning 4 | Output 4 |

Allow a configuration layer to map sensor channels to zones and outputs, but default to one-to-one mapping. Each zone has independent setpoint, tuning parameters, output limits, sensor type, and alarm thresholds. The overall firing profile may provide a common ramp/soak target or zone-specific offsets, but all four loops must enforce their own local limits.

A four-output system does not imply that four outputs can safely be switched at arbitrary rates. The final switching strategy, cycle time, SSR type, and power distribution must be defined for the actual kiln and heater circuits.

## Temperature sensor architecture

The current design assumption is **mixed thermocouple types across the four channels**, not a mixture of thermocouples and RTDs. Use one suitable thermocouple interface per channel, with each channel configured independently for a supported thermocouple type. The MAX31856 family is a candidate because it supports multiple thermocouple types; confirm the required temperature range, accuracy, isolation, and probe construction before selecting the exact module.

If "mixed" is intended to include RTDs (such as PT100/PT1000), add an RTD-capable front end such as a MAX31865 on the relevant channel. A MAX31856 is not an RTD interface. Do not silently treat RTD inputs as thermocouples.

## Control and command flow

1. Operator selects a validated profile on the CYD touchscreen or browser.
2. CYD sends a profile/configuration request to the dedicated ESP32-S3.
3. Dedicated controller validates the complete profile, all four zone mappings, sensor configuration, limits, and interlocks.
4. Operator explicitly arms and starts the firing.
5. Dedicated controller executes four local control loops and enforces local safety constraints.
6. Temperature, setpoint, output request, output feedback, and faults are reported to the CYD.
7. CYD displays authoritative controller state and records operator-facing history.

The browser is never authoritative. It displays controller state and requests actions; it does not directly control pins.

## State-machine baseline

Suggested dedicated-controller states:

- `BOOT` — initialize; outputs remain safe.
- `SELF_TEST` — verify configuration, sensors, front ends, and safety inputs.
- `IDLE` — ready, no active firing.
- `VALIDATING` — validate profile, zones, sensors, and interlocks.
- `ARMED` — validated and awaiting explicit operator start.
- `FIRING` — four zone loops active under validated limits.
- `PAUSED` — only if the kiln-specific output behavior is explicitly defined.
- `STOPPING` — execute defined safe shutdown.
- `COMPLETE` — profile complete; heater outputs safe.
- `FAULT` — safety fault latched; heater outputs safe.
- `UI_LINK_LOSS` — CYD link lost; local control follows the explicitly approved policy, while safety monitoring remains local.

Do not assume a generic pause is safe for every kiln or installation. Define and document pause/stop behavior with the kiln's electrical and thermal design.

## Restart behavior

Persist profiles and configuration with schema versions and integrity checks. On any dedicated-controller reboot, begin in `BOOT`, force outputs safe, run self-test, and require a deliberate new operator start. Never resume an interrupted firing automatically. The CYD reconnecting must not restart a firing.

## Hardware abstraction

Keep each sensor front end, output driver, feedback input, and board pin map behind an interface. The application must not assume a particular thermocouple type, RTD interface, CYD pin map, ESP32-S3 carrier-board pinout, SSR polarity, or supply voltage until selected and documented.
