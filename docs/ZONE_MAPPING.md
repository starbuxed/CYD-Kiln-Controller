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
