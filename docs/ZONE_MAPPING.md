# Configurable Sensor, Zone, and Output Mapping

The controller has four physical temperature inputs, four logical heating zones, and four relay command outputs. These are independent resources: their counts and assignments are not assumed to match.

## Mapping capabilities

- A logical zone can use one or multiple temperature sensors.
- A physical sensor may be referenced by more than one logical zone.
- A logical zone can command one or multiple relay outputs together.
- A relay output can belong to only one logical zone. Sharing an output between zones is rejected to avoid conflicting commands.
- Unused sensors, zones, and outputs are allowed.
- Mappings may only be changed while the kiln is not firing or paused.

## Sensor combination

- **Designated:** one selected sensor is used for the zone control reading.
- **Highest:** the hottest assigned sensor controls the zone.
- **Lowest:** the coolest assigned sensor controls the zone.
- **Average:** the arithmetic mean controls the zone.

For highest, lowest, and average, every sensor assigned to that group must be valid. A missing sensor invalidates the zone measurement rather than silently changing the calculation. Independent safety supervision must still check individual sensor readings; aggregation must never hide an overtemperature.

## Examples

| Arrangement | Zone sensor mask | Zone output mask |
|---|---:|---:|
| One sensor, one heater circuit | `0001` | `0001` |
| Four sensors monitoring one heater circuit | `1111` | `0001` |
| Four sensors split across two heater circuits | `0011`, `1100` | `0001`, `1110` |
| One sensor controlling four outputs together | `0001` | `1111` |
| Four independent zones | `0001`, `0010`, `0100`, `1000` | `0001`, `0010`, `0100`, `1000` |

Masks are shown with bit 0 on the right (sensor/output 1).

## Implementation status

The hardware-independent `ZoneMapping` module validates mappings and calculates zone measurements. It is covered by native tests. The existing kiln control core still uses its earlier one-zone/one-sensor/one-output loop and must next be refactored to consume this mapping layer before this feature is active in firing control. The S3 controller remains non-operational for firing until that integration, hardware configuration, and safety validation are complete.


## Per-sensor PID/control enable

Each zone has two sensor selections:

- `sensor_channels`: sensors associated with the zone and retained for monitoring/safety supervision.
- `control_sensor_channels`: the subset enabled to contribute to the zone's PID/control temperature.

A sensor can therefore be turned OFF for PID control without being turned OFF as a safety sensor. For example, a kiln can use four sensors to monitor temperature but select only sensors 1 and 3 to calculate the zone control temperature.

The controller supports designated, highest, lowest, and average calculations over the control-enabled subset. Every sensor selected for a group calculation must be valid. A disabled-for-PID sensor remains subject to its own fault and overtemperature checks. The user interface should clearly label these controls as **Use for PID** rather than simply Sensor ON/OFF, to avoid implying that safety monitoring has been disabled.

This setting is per zone mapping. If one physical sensor is assigned to multiple zones, it can be enabled for PID in one zone and excluded from PID in another.
