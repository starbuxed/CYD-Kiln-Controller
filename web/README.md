# Browser Application

The CYD now starts a local WPA2 access point and serves a self-contained dashboard from the ESP32. The current UI is embedded in \`firmware/esp32/src/web_app.cpp\`; it has no remote JavaScript, CSS, chart, or API dependencies.

## Current pages

- Dashboard with four sensor readings, validity/fault state, CAN status, output command mask, alerts, and live temperature graph.
- JSON ramp/soak profile upload to the S3.
- Firing-summary history and CSV export.
- Alert/event ledger and configurable dashboard-warning rules.
- Scheduled firing reminders (due notification only; never automatic start).
- Estimated energy/cost using user-configured zone watts and tariff.
- Isolated four-zone simulation preview.
- PID-term diagnostics and offline step-response recommendation.

## Access and security

At boot the serial monitor reports the generated SSID/password and \`http://192.168.4.1/\`. The mDNS alias \`kiln-controller.local\` is enabled where supported. HTTP Basic Authentication uses username \`admin\` and the unique AP password. Write endpoints require a CSRF header.

The initial web service is intended for its local WPA2 AP, not public-network exposure. TLS and station-mode Wi-Fi onboarding are not yet implemented.

## Implementation constraints

- The browser is display/cache only; the ESP32-S3 owns firing state, safety, and output switching.
- On reconnect, the browser fetches authoritative status and never replays a start/resume command.
- Simulated temperatures are not mixed into live controller samples and do not feed the S3.
- Scheduling creates reminders only. No reboot or schedule event can auto-resume a firing.
- Energy is an estimate, not a meter; configure actual zone wattage and energy rate.
- PID diagnostics/tuning recommendations do not update active S3 control gains. The current zone controller is hysteresis-based.
- Continue using simulated sensors and low-voltage indicator loads only until hardware and independent safety review are complete.
