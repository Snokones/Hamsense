# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project overview

HamSense is a two-node ESP32-C6 weather station:

- **`sensor_logger.txt`** (Arduino sketch source, normally `sensor_logger.ino`) — the **base station**. Reads outdoor temperature (MCP9808) and humidity/pressure (BME280); serves a live web page over `AsyncWebServer` + WebSocket; receives indoor readings from the satellite over ESP-NOW.
- **`satellite_sensor.txt`** (normally `satellite_sensor.ino`) — the **indoor satellite**. A XIAO ESP32-C6 + BME280 that wakes once a minute, takes one reading, sends it to the base over ESP-NOW, and deep-sleeps.

There is no build system, package manifest, or test suite in this directory — these are raw Arduino sketch files. Both currently have a `.txt` extension instead of `.ino`; if you're compiling them, each sketch needs its own folder named identically to its `.ino` file (Arduino IDE / arduino-cli requirement), e.g. `sensor_logger/sensor_logger.ino`.

### Branches

- **`main`** — the GPS build. An ATGM336H supplies UTC time (primary clock), position, altitude, and date.
- **`no-gps`** (this branch) — no GPS module at all. A ZIP code entered on the web page is resolved through AccuWeather to supply coordinates, elevation and timezone; NTP is the only clock source. **Do not port GPS code back into this branch**, and check which branch you're on before touching time, pressure, or location logic — they work differently on each.

### Files referenced but not present here

The sketches `#include` local headers that are gitignored because they hold secrets. `web_pages.h` **is** in this directory and is tracked.

- `secrets.h` — `WIFI_SSID`, `WIFI_PASSWORD`, `FALLBACK_AP_SSID`, `FALLBACK_AP_PASSWORD`, `WEB_USER`, `WEB_PASS`, and (on `no-gps`) `ACCUWEATHER_API_KEY`. Copy from `secrets.h.example`. Missing `WEB_USER`/`WEB_PASS`/`ACCUWEATHER_API_KEY` is a deliberate compile error, not a runtime fallback.
- `satellite_secrets.h` — `WIFI_SSID`, `FALLBACK_AP_SSID`, `BASE_MAC` (satellite; copy from `satellite_secrets.h.example` and fill in the base's STA MAC, printed on its Serial Monitor at boot)
- `web_pages.h` — the `index_html` raw string for the live page. Deliberately kept **out of** `sensor_logger.ino` because the Arduino IDE's ctags-based auto-prototype generator mis-parses JavaScript `function name(...) {}` inside raw string literals as real C++ functions, breaking the build with bogus errors. Keep any HTML/JS payload in this header, not inline in the `.ino`.

## Build / flash

No CLI build has been set up in this repo; these are compiled via the Arduino IDE (or arduino-cli with equivalent settings) against board **XIAO ESP32-C6** on the `arduino-esp32` 3.x core (both sketches rely on 3.x callback signatures — see below).

Required libraries (Arduino Library Manager):
- Adafruit BME280 Library + Adafruit Unified Sensor
- Adafruit MCP9808 Library (base station only)
- TinyGPSPlus by Mikal Hart (base station, **`main` branch only** — not used on `no-gps`)
- ESPAsyncWebServer + AsyncTCP (base station only)
- ArduinoJson (base station only)
- ESP-NOW, WiFi, DNSServer, ESPmDNS are part of the ESP32 core.

## Architecture

### ESP-NOW contract between the two nodes

Both sketches define an identical `__attribute__((packed))` `IndoorReport` struct (`tempF`, `humidity`, `pressureHPa`, `seq` — 16 bytes, enforced by a `static_assert` in both files). **If you change this struct in one file, you must change it identically in the other**, or the base will silently receive garbled indoor readings. The satellite raw pressure is later sea-level-corrected on the base side using the base's GPS altitude and outdoor temperature (see `pushSensorData()` in `sensor_logger.txt`).

Both files use `arduino-esp32` 3.x ESP-NOW callback signatures (`wifi_tx_info_t` for send, `esp_now_recv_info_t` for receive) — older cores need different signatures; this is called out in comments at each callback.

### Satellite (`satellite_sensor.txt`) power/reliability model

- Deep-sleeps between 60s wake cycles; total awake time ~0.3–0.5s.
- Caches the base station's WiFi channel in `RTC_DATA_ATTR` memory (survives deep sleep, not power loss) so the ~2s network scan only runs on first boot or after a failed delivery, not every cycle.
- Sends via unicast ESP-NOW (gets a MAC-layer ACK) with up to 8 retries at 40ms spacing, to ride out the base's WiFi power-save beacon windows.
- On repeated delivery failure, clears the cached channel so the next wake rescans (handles the router changing channels).

### Base station (`sensor_logger.txt`) key subsystems

- **WiFi**: tries the home network 3× (~15s each), then falls back to its own AP (`PotaSense`) with a DNS server that redirects all lookups to `/`. A watchdog in `loop()` restarts the device after 5 minutes of continuous WiFi down (STA mode) or 10+ idle minutes in AP fallback, so it doesn't get stranded.
- **Time source** (`no-gps`): NTP only, via `configTzTime` in `setup()`, and only in STA mode. There is no other clock source — in AP fallback the clock never syncs, which also means **no temperature history is recorded** (history writes are gated on `getEpochSeconds()` returning true). This is a known, accepted regression from dropping the GPS; don't "fix" it by re-adding GPS.
- **Pressure** (`no-gps`): the firmware reports **raw station pressure** for both the BME280 and the forwarded satellite reading. The ICAO sea-level correction lives in `web_pages.h` (`toSeaLevel()`), because the altitude comes from the ZIP's elevation, which only the browser has. **The correction must exist in exactly one of those two places** — adding it back to the firmware without deleting it from the page applies it twice.
- **Location** (`no-gps`): the ZIP entered on the page resolves to `{lat, lon, elevM, tz, name}`, which `window.setStationLocation()` fans out to the map pin, sunrise/sunset, the clock timezone, and the pressure correction. The firmware knows nothing about it.
- **Shared state across tasks**: the ESP-NOW receive callback runs in the WiFi task; `indoorLast` is protected by a `portMUX_TYPE` spinlock (`portENTER_CRITICAL`/`portEXIT_CRITICAL`) since the 16-byte struct copy isn't atomic. Always use `snapshotIndoor()` to read it, never touch `indoorLast` directly outside the lock.
- **Sensor fallback pattern**: both MCP9808 and BME280 retry `begin()` from `loop()` if they failed at boot (or ever drop out), so a sensor connected after power-on (or reseated) recovers without a reboot.
- **Live data push**: `pushSensorData()` runs every 30s on `no-gps` (10s on `main`), serializes state to JSON via ArduinoJson, and broadcasts over the WebSocket (`ws.textAll`) to all connected clients.
- **Power** (`no-gps`): the node serves an always-on page, so it never sleeps. `setCpuFrequencyMhz(80)` is the floor with WiFi active. **Never add deep or light sleep to the base** — it would kill the web server and the USB CDC port used for flashing. Leave `WiFi.setSleep()` at its default; the satellite already retries 8× to ride through power-save windows.

## Communication style

- No pleasantries. No "Great question," "You're absolutely right," "I'll now...", or "In summary...". Answer directly.
- Disagree when you disagree. Flag a bad approach before implementing it, not after.
- No hedge words ("might," "could," "perhaps") when you have an actual opinion — state it.
- Give direct verdicts: "this is wrong because X," not "this could potentially be improved."
- State confidence level plainly when uncertain (e.g., "guessing," "fairly sure") instead of vague qualifiers.
- Don't pad responses to look thorough. Shorter correct answer beats a longer restatement of the question.
- Don't narrate what you're about to do beyond the required one-sentence heads-up; don't summarize at the end beyond what changed and what's next.

## Security

- `secrets.h` and `satellite_secrets.h` hold WiFi credentials and MAC addresses — never commit them, never print full credential values to Serial/logs, never inline them into a sketch file.
- ESP-NOW traffic between base and satellite is unencrypted (`peer.encrypt = false`) and unauthenticated — anyone in radio range can spoof `IndoorReport` packets or sniff readings. Don't treat this link as trusted; if encryption is ever added, it must be enabled identically on both ends or they stop communicating.
- The fallback AP (`PotaSense`) is a real WiFi network with a password in `secrets.h` — verify `FALLBACK_AP_PASSWORD` is never weakened to an open network without flagging it, since it's reachable by anyone nearby.
- The DNS-redirect-everything-to-`/` behavior only runs in AP fallback mode — don't extend it to STA mode, since it would silently swallow real 404s and mask misconfigured endpoints.
- Don't add remote/OTA update mechanisms, new open ports, or new web endpoints without flagging the exposure. Web auth is **per-handler** on `AsyncWebServer`, not global — every new `server.on(...)` needs its own `.setAuthentication(WEB_USER, WEB_PASS)` or it is silently public.
- `ACCUWEATHER_API_KEY` (on `no-gps`) is served to the browser by `GET /config`. That keeps it out of git but not out of reach of anyone who can log into the page — that's inherent to fetching a keyed API browser-side. Don't hardcode it into `web_pages.h`, which is committed.

## Working in this repo

- Any change to `IndoorReport` must be mirrored exactly in both files, including the `static_assert` byte count.
- Don't inline HTML/JS back into `sensor_logger.txt`/`.ino` — keep it in `web_pages.h` to avoid the ctags auto-prototype bug described above.
- On `no-gps`, the AccuWeather free tier is 50 calls/day **shared across every browser** that opens the page. The caching in the forecast IIFE (permanent ZIP→location cache, 3-hour forecast TTL) is what keeps it inside that budget. Raising the refresh rate is how you break it.
- Comments in these files frequently record hardware-specific gotchas (active-low LED pin, I2C address strapping, UART pin mapping, GPS checksum/wake timing) — treat them as load-bearing documentation, not boilerplate, when touching nearby code.
