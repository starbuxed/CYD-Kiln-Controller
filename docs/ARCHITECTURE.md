# System Architecture

## System blocks

```text
 Phone / Tablet / PC browser
          | Wi-Fi / local LAN
          v
+-----------------------------------+
| CYD ESP32                          |
| - Local web server + REST API      |
| - Live status channel              |
| - Touchscreen UI                   |
| - Profile engine / scheduler       |
| - Supervisory safety state machine |
| - History and configuration        |
+----------------+------------------+
                 | UART (framed protocol)
                 | voltage-compatible interface
                 v
+-----------------------------------+
| Arduino Pro Micro                  |
| - Temperature I/O abstraction      |
| - Four configurable output states  |
| - Communication watchdog           |
| - Local output interlocks          |
| - Fault/status reporting           |
+----------------+------------------+
                 |
                 v
      Properly rated isolated
      drivers / contactors / safety
      devices / kiln power equipment

 Independent high-limit protection and
 power isolation must not depend on the
 ESP32, Pro Micro, Wi-Fi, or web browser.
```

This is a logical architecture. Final allocation of sensor acquisition and closed-loop control depends on the selected sensor front ends, output drivers, and safety analysis. Do not treat the diagram as a wiring diagram.

## ESP32/CYD responsibilities

- Own the touchscreen user experience and locally hosted browser application.
- Manage Wi-Fi station/access-point setup and mDNS where supported.
- Store configuration and firing profiles with versioning and validation.
- Run the firing profile scheduler and supervisory controller state machine.
- Accept local UI and authenticated browser commands through the same command-validation path.
- Publish authoritative system status to clients.
- Maintain firing history, alarms, and event logs.
- Monitor the Pro Micro heartbeat, data freshness, sensor validity, and reported output state.
- On a detected fault, transition to the specified safe state and report the cause.
- Never restart a firing automatically after reset, power loss, or communication recovery.

## Pro Micro responsibilities

- Perform the configured sensor and output I/O tasks.
- Report raw and converted readings with validity/error flags and timestamps or sequence counters.
- Accept only validated, bounded, explicit output-state commands.
- Enforce a communications watchdog. If valid host commands stop arriving within the configured timeout, de-energize controlled outputs and latch/report the communications fault.
- Start with all outputs in their defined safe state after power-up or reset.
- Reject malformed, stale, out-of-sequence, or unsupported commands.
- Report actual output-driver feedback if the hardware supports it; otherwise distinguish commanded state from verified physical state.

The Pro Micro watchdog is a secondary software protection, not independent high-limit protection. A stuck processor or failed output device must be addressed by hardware safety design.

## Control and command flow

1. The operator requests an action from the touchscreen or web application.
2. The request enters one shared ESP32 command handler.
3. The supervisory state machine checks current state, sensor validity, profile validity, permissions, and interlocks.
4. If allowed, the ESP32 sends a bounded command to the Pro Micro.
5. The Pro Micro validates the command and applies the requested output state only if its local conditions permit.
6. Status and faults propagate back to the ESP32 and browser clients.
7. Every state transition and consequential command is recorded.

The browser is never authoritative. It displays server state and requests actions; it does not directly control pins.

## State-machine baseline

Suggested supervisory states:

- `BOOT` — initialize; outputs remain safe.
- `IDLE` — ready, no active firing.
- `VALIDATING` — checking profile, sensors, configuration, and interlocks.
- `ARMED` — validated and awaiting explicit operator start.
- `FIRING` — profile execution active.
- `PAUSED` — controlled pause; output policy must be explicitly defined.
- `STOPPING` — executing defined safe shutdown.
- `COMPLETE` — profile completed; outputs safe.
- `FAULT` — fault latched; outputs safe.
- `COMMUNICATION_LOSS` — Pro Micro link lost; safe state required.

Do not assume that a generic pause is safe for every kiln or installation. Define and document pause/stop behavior with the kiln's electrical and thermal design.

## Persistence and restart behavior

Persist profiles and configuration with schema versions and integrity checks. Persist enough event data to diagnose interrupted firings. On any ESP32 reboot, begin in `BOOT`, verify hardware status, and require a deliberate operator action to begin a new firing. Never resume an interrupted firing automatically.

## Hardware abstraction

Keep sensor and output drivers behind interfaces. The application must not assume a particular thermocouple type, RTD interface, display controller, CYD pin map, relay-board polarity, or Pro Micro voltage/clock variant until these are chosen and documented.
