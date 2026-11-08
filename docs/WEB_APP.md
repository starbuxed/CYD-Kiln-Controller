# Integrated Web Application and Local API

## Current implementation

The CYD starts its own WPA2 Wi-Fi access point and serves the browser dashboard directly from the ESP32. No cloud host, subscription, external server, CDN, or internet connection is required for local access.

At startup the serial monitor reports:
- Access-point SSID: \`KilnController-XXXXXX\`
- A unique generated password, stored in NVS
- Browser URL: \`http://192.168.4.1/\`
- mDNS URL where supported: \`http://kiln-controller.local/\`

The HTTP server uses Basic Authentication with user \`admin\` and the generated AP password. State-changing requests also require a per-boot CSRF token fetched from \`/api/v1/session\`. Keep the controller on its own local AP; this initial HTTP service is not intended to be port-forwarded or exposed to an untrusted network. TLS and configurable station-mode Wi-Fi onboarding remain future work.

The browser polls the status endpoint approximately every 1.5 seconds. It fetches a bounded telemetry window for chart rendering. No queued command is replayed after a reconnect.

## Browser pages

- **Dashboard:** four sensor readings, validity and fault states, controller state, CAN status, output command mask, active alerts, and temperature history graph.
- **Profiles:** JSON ramp/soak segment editor and upload to the S3. The S3 validates the profile and remains the control authority.
- **History:** firing summaries, peak temperatures, duration, estimated energy, and CSV export.
- **Alerts:** active/event ledger, acknowledgement, configurable dashboard-warning rules.
- **Schedules:** future firing reminders. A due schedule creates a notification only; it never starts or resumes a firing automatically.
- **Energy:** estimated consumption and cost from configured zone wattages, the commanded-output mask, and the entered energy rate. This is not an electrical meter.
- **Simulation:** isolated four-zone thermal preview. Simulated temperatures are separate from live controller telemetry and cannot command outputs.
- **PID diagnostics:** illustrative P/I/D term calculation and offline step-response recommendation. It does not write active gains or run a live full-power tuning test.

## REST API

Every endpoint requires HTTP Basic Authentication. Write endpoints additionally require the \`X-CSRF-Token\` header returned from \`GET /api/v1/session\`.

| Method | Endpoint | Purpose |
|---|---|---|
| GET | \`/api/v1/session\` | CSRF token and device identity |
| POST | \`/api/v1/time/sync\` | Set device clock from browser epoch |
| GET | \`/api/v1/status\` | Complete live status |
| GET | \`/api/v1/telemetry\` | Bounded recent graph samples |
| GET | \`/api/v1/telemetry.csv\` | Export recent telemetry |
| GET | \`/api/v1/history\` | Firing summaries |
| GET | \`/api/v1/history.csv\` | Export firing summaries |
| GET | \`/api/v1/alerts\` | Alert/event ledger |
| GET / POST | \`/api/v1/config/alerts\` | Read or update dashboard alert preferences |
| GET | \`/api/v1/schedules\` | List firing reminders |
| POST | \`/api/v1/schedules\` | Add a future reminder |
| DELETE | \`/api/v1/schedules/{id}\` | Cancel a reminder |
| POST | \`/api/v1/profiles\` | Validate and upload ramp/soak profile |
| POST | \`/api/v1/commands/start\` | Request start through S3 |
| POST | \`/api/v1/commands/pause\` | Request pause through S3 |
| POST | \`/api/v1/commands/resume\` | Request resume through S3 |
| POST | \`/api/v1/commands/stop\` | Request stop through S3 |
| POST | \`/api/v1/alerts/{id}/ack\` | Acknowledge an alert |
| POST | \`/api/v1/config/energy\` | Set estimated zone wattage and energy rate |
| POST | \`/api/v1/simulation/start\` | Start isolated preview model |
| POST | \`/api/v1/simulation/step\` | Set preview-model heater mask only |
| POST | \`/api/v1/simulation/stop\` | Stop preview model |
| POST | \`/api/v1/tuning/terms\` | Calculate illustrative PID terms |
| POST | \`/api/v1/tuning/advice\` | Calculate offline step-response suggestion |

## Safety boundaries

- Browser start/pause/resume/stop commands are ordinary requests to the ESP32-S3 state machine. No endpoint can write relay GPIO pins.
- The S3 validates firing commands locally. The CYD UI and HTTP API are not the control authority.
- Simulation never overrides live readings and never feeds the control algorithm.
- Schedules become due reminders only; they do not auto-start or auto-resume a kiln.
- Dashboard temperature warnings are informational and cannot modify the S3 safety limit or physical high-limit.
- PID recommendations are advisory. The currently integrated S3 zone controller uses hysteresis, not PID.
- The browser software E-stop is only an additional software request, not a physical E-stop.

## Remaining web work

- Wi-Fi station-mode onboarding and a touchscreen network-settings page.
- TLS or a stronger credential-management path for use on a trusted LAN.
- Full visual profile editor and named profile library.
- Persistent high-resolution per-firing telemetry storage beyond the bounded RAM graph buffer.
- WebSocket/SSE push instead of short-interval polling.
- More detailed authentication/session lifecycle and password-change UX.
