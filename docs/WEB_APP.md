# Integrated Web Application and Local API

## Requirement
The ESP32 hosts the complete web application and API. It must work without cloud hosting, subscriptions, or an external server. Phones, tablets, laptops, and desktops connect using a modern browser over the local network or controller access point.

## Connectivity
- Wi-Fi station mode and configurable fallback access-point mode.
- Display connection mode, IP address, and signal status on the CYD.
- mDNS hostname where supported.
- No internet requirement. Do not expose the controller directly to the public internet.

## Main dashboard
- Current operating state and fault status.
- Four temperature channels with units, validity, and sample age.
- Active profile, segment, target temperature, ramp rate, and elapsed/remaining time.
- Four relay outputs with names, requested state, actual state, and interlock status.
- Start/pause/resume/stop actions subject to central state-machine checks.
- Live temperature/setpoint chart, alarms, event list, network status, and Pro Micro link status.
- Invalid or stale readings must be visually unmistakable and must never appear to be current valid readings.

## Pages
- Dashboard.
- Firing profile library and ramp/soak profile editor.
- Manual I/O diagnostics, interlocked and unavailable during unsafe states.
- Temperature sensor configuration and calibration.
- Relay mapping and diagnostics.
- Alarm/event log.
- Firing history, firing detail, and export.
- Network/security settings and system diagnostics.

## REST API
Suggested versioned endpoints (final schemas and authentication contract remain to be defined):

| Method | Endpoint | Purpose |
|---|---|---|
| GET | `/api/v1/status` | Complete current status |
| GET | `/api/v1/temperatures` | Values, validity, and age |
| GET | `/api/v1/outputs` | Output state and interlocks |
| GET | `/api/v1/profiles` | List profiles |
| GET | `/api/v1/profiles/{id}` | Read profile |
| PUT | `/api/v1/profiles/{id}` | Validate and save profile |
| POST | `/api/v1/commands/start` | Request start |
| POST | `/api/v1/commands/pause` | Request pause |
| POST | `/api/v1/commands/resume` | Request resume, if permitted |
| POST | `/api/v1/commands/stop` | Request stop |
| POST | `/api/v1/alarms/{id}/ack` | Acknowledge alarm |
| GET | `/api/v1/history` | List firing history |
| GET | `/api/v1/history/{id}` | Firing details |
| GET | `/api/v1/history/{id}/export` | Export firing data |

Use JSON with a versioned schema. State-changing methods require authentication, CSRF protection appropriate to the browser auth model, input validation, and audit logging. Never expose an endpoint that writes raw relay pins or bypasses the central state machine.

## Live updates
Use WebSocket or Server-Sent Events for live status, temperatures, state changes, alarms, and log events. REST remains available for snapshots and commands. On reconnect, clients must fetch authoritative status; they must never replay queued start, resume, or output commands automatically.

## Browser behavior
- Responsive phone and desktop layouts with touch-friendly controls.
- Clear confirmation for start and other consequential actions.
- Stop control should be easy to locate.
- Clearly indicate browser/controller connection loss.
- Browser state is display/cache only; the ESP32 owns authoritative state.
- Client retries may refresh status but must not blindly retry non-idempotent commands.
