# Configurable Sensor, Zone, and Output Mapping

The controller has four physical temperature inputs, four logical heating zones, and four relay command outputs. These are independent resources: their counts and assignments are not assumed to match.

## Per-sensor operating modes

Every physical sensor has one operating mode, selected independently of zone assignment:

| Mode | Acquire/display | Log | PID/control eligible | Kiln safety trip |
|---|---:|---:|---:|---:|
| **Off** | No | No | No | No |
| **Log only** | Yes | Yes | No | No |
| **Log and control** | Yes | Yes | Yes | No |
| **Log and safety** | Yes | Yes | No | Yes |

**Off is the only mode that disables monitoring.** Every other mode continues acquiring and logging the sensor's readings and faults.

A sensor in **Log and safety** mode is independently checked for sensor faults, stale readings, implausible readings, and overtemperature, regardless of whether it is assigned to a zone. A sensor in **Log and control** mode is eligible for PID only in zones where it is selected, but it does not independently trip kiln safety. These are intentionally separate responsibilities.

Newly configured sensors default to **Log and safety**. Changing a sensor mode invalidates its previous sample; a fresh valid sample is required before it can be used again.

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

Only sensors in **Log and control** mode may contribute to PID. For highest, lowest, and average, every selected control sensor must be valid. Independent safety supervision checks sensors in **Log and safety** mode individually; aggregation must never hide an overtemperature.

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

The hardware-independent `ZoneMapping` module validates mappings and calculates zone measurements. It is covered by native tests. The existing kiln control core still uses its earlier one-zone/one-sensor/one-output loop and must next be refactored to consume this mapping layer before this feature is active in firing control. Per-sensor operating modes and their independent monitoring/control/safety masks are implemented in `SensorManager`; the S3 safety and control integration and configuration-protocol/UI wiring remain to be completed. The S3 controller remains non-operational for firing until those integrations, hardware configuration, and safety validation are complete.
