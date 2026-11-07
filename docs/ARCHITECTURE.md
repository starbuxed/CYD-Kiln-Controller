# System Architecture

## Selected controller architecture

- **CYD ESP32:** touchscreen, Wi-Fi, local web server, profiles, charts, configuration, and history.
- **Dedicated ESP32-S3:** four temperature channels, four independent zone-control loops, four digital ON/OFF control outputs, and local fault supervision.
- **External relay/SSR equipment:** selected and installed by the user. The controller sends four low-voltage logic commands; it does not design or switch kiln mains power directly.

The CYD is the operator interface, not the real-time controller. Loss of the display, web server, Wi-Fi, or CYD-to-S3 link must not disable local fault handling.

## System blocks

```text
 Phone / Tablet / PC
        |
        v
+-----------------------------+
| CYD ESP32                   |
| Touchscreen + local web app |
| Profiles, charts, history   |
+--------------+--------------+
               | Framed UART
               v
+-----------------------------+
| Dedicated ESP32-S3          |
| 4 temperature inputs        |
| 4 independent zone loops    |
| 4 digital ON/OFF commands   |
| Local safety state machine  |
+--------------+--------------+
               |
       4 protected low-voltage
       logic output interfaces
               |
       +-------+-------+-------+
       |       |       |       |
     Relay 1 Relay 2 Relay 3 Relay 4
       |       |       |       |
       +---- User-selected external kiln power equipment ----+

 Independent kiln-side safety protection remains separate.
```

This is a logical block diagram, not a mains wiring diagram. The user selects the external relay/SSR equipment. The controller project defines only the four compatible low-voltage control signals and their safe behavior.

## Dedicated ESP32-S3 responsibilities

- Acquire four independent temperature channels.
- Configure the thermocouple type independently for each channel.
- Validate sensor values, front-end faults, freshness, and plausible range.
- Run four independent zone-control loops, each mapped to a sensor and a digital output.
- Generate four independent ON/OFF commands using a configurable control strategy.
- Support active-high/active-low output polarity per channel.
- Keep outputs inactive during boot, reset, brownout, watchdog timeout, invalid sensor data, and latched safety faults.
- Continue local sensor and fault supervision if the CYD disconnects.
- Require an explicit new start after interruption; never automatically resume.
- Report commanded output states separately from physically verified states.

The output firmware must not assume relay coil voltage, SSR input voltage, or power-load ratings. It controls compatible low-voltage relay-module inputs through suitable output-interface circuitry.

## CYD responsibilities

- Own touchscreen and locally hosted browser application.
- Manage Wi-Fi and local web server.
- Store/edit firing profiles and configuration.
- Display four temperatures, four setpoints, four zone states, four output commands, alarms, and history.
- Send typed operator requests to the dedicated controller.
- Never directly write heater-control GPIOs or bypass the S3 safety state machine.

## Four-zone control model

| Zone | Temperature | Control loop | Digital command |
|---|---|---|---|
| 1 | Sensor 1 | Loop 1 | Output 1 ON/OFF |
| 2 | Sensor 2 | Loop 2 | Output 2 ON/OFF |
| 3 | Sensor 3 | Loop 3 | Output 3 ON/OFF |
| 4 | Sensor 4 | Loop 4 | Output 4 ON/OFF |

Default mapping is one-to-one, but the configuration layer should allow mapping changes while outputs are inactive. Each zone has independent setpoint, tuning, output limits, sensor type, and alarm thresholds.

## Sensor architecture

The current assumption is mixed thermocouple types across four channels. Use one compatible thermocouple interface per channel, with each channel independently configured. MAX31856 is a candidate for thermocouple channels. If "mixed" includes RTDs, those channels need a separate RTD-capable front end such as MAX31865.

## Output abstraction

Each output has a logical ON/OFF state and configurable active polarity. The physical output stage must be compatible with the user's relay-module input. The firmware must:
- Set the hardware inactive state before normal application startup.
- Never restore an ON state after reboot.
- Force all four outputs inactive on a latched critical fault.
- Reject output changes outside permitted controller states.
- Report commanded state and feedback state as separate fields.
- Treat missing feedback as "not verified", never as proof of OFF.

## Control and command flow

1. Operator selects a validated profile on the CYD or browser.
2. CYD sends the profile to the dedicated ESP32-S3.
3. Controller validates profile, four sensor configurations, zone mappings, and limits.
4. Operator explicitly arms and starts.
5. ESP32-S3 executes the four local loops and sets the four digital outputs.
6. Temperature, setpoint, output command, optional feedback, and faults are sent to the CYD.
7. CYD displays controller state and stores operator-facing history.

## Restart behavior

On reboot, outputs remain inactive, self-test runs, and a deliberate new operator start is required. Never resume an interrupted firing automatically.

## Hardware abstraction

Keep sensor front ends, output interface, optional feedback inputs, and board pin map behind interfaces. Do not assume a particular relay board or relay input voltage until the external module is selected.
