# Browser Application

The production web application is served directly from the ESP32. It must not require a cloud service, remote JavaScript CDN, or external API to perform local control and monitoring.

## Planned screens

- Dashboard: four temperature channels, four outputs, controller state, profile progress, alarms, and live chart.
- Profile library and ramp/soak editor.
- Firing history and export.
- Sensor/output configuration.
- Network and security settings.
- Diagnostics.

## Implementation constraints

- Responsive phone/tablet/desktop layout.
- Clearly display stale data and server disconnection.
- Fetch an authoritative status snapshot after every reconnect.
- Never replay queued start, resume, or output commands.
- Start/resume/stop actions are requests to the ESP32 state machine, never direct GPIO operations.
- Bundle all assets locally so the interface works without internet access.
- State-changing requests require authentication, CSRF protection as appropriate, validation, and audit events.
