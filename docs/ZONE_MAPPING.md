# Configurable Sensor, Zone, and Output Mapping

The controller has four physical temperature inputs, four logical heating zones, and four relay command outputs. These are independent resources: their counts and assignments are not assumed to match.

## Per-sensor operating modes

Every physical sensor has one operating mode, selected independently of zone assignment:

| Mode | Acquire/display | Log | PID/control eligible | Kiln safety trip |
|---|---:|---:|---:|---:|
| **Off** | No | No | No | No |
| **Log only** | Yes | Yes | No | Yes |
| **Log and control** | Yes | Yes | Yes | Yes |
| **Log and safety** | Yes | Yes | No | Yes |

**Off is the only mode that disables monitoring and safety supervision.** Every other mode contributes to the kiln-wide safety check, regardless of zone assignment or PID use.

The safety check combines all enabled sensors using a worst-case rule: the highest valid temperature is the kiln safety temperature. If any enabled sensor reaches the configured limit, the kiln trips. A missing, stale, implausible, or electrically faulted enabled sensor also trips after the sensor-loss grace period. Safety is never averaged, because averaging could hide a hot spot.

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

Only sensors in **Log and control** mode may contribute to PID. For highest, lowest, and average, every selected control sensor must be valid. Independent kiln safety supervision checks every enabled sensor individually, including sensors not assigned to a zone; aggregation must never hide an overtemperature.

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

The hardware-independent `ZoneMapping` module validates mappings and calculates zone measurements. It is covered by native tests but is not yet integrated into the firing core. Per-sensor operating modes, UART configuration, status telemetry, PID eligibility, and independent safety-mask enforcement are implemented. The CYD touchscreen/browser selector is not yet implemented. The S3 controller remains non-operational for firing until hardware configuration and end-to-end safety validation are complete.
