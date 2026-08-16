# Hamsense — Lite UI

A two-node ESP32-C6 weather station. A **base station** reads outdoor temperature, humidity and pressure, then serves a live web page over WiFi. A battery-powered **indoor satellite** wakes once a minute, takes a reading, radios it to the base over ESP-NOW, and goes back to deep sleep.

Both nodes run on the [Seeed XIAO ESP32-C6](https://wiki.seeedstudio.com/xiao_esp32c6_getting_started/).

> **This is the `lite-ui` branch.** Same firmware as [`main`](https://github.com/Snokones/Hamsense/tree/main), but with a condensed, mobile-first page: no world map, no colour-theme picker, no 7-day panel, no ZIP box and no forecast. It pulls **zero external resources**, so it loads completely on a phone joined only to the station's own `PotaSense` AP with no internet — which is the whole point of it.
>
> The station's latitude, longitude and elevation are set from a collapsed **LOCATION** box on the page and persisted per browser. Elevation auto-fills from the entered coordinates where there's internet, and stays editable. With no internet the lookup is skipped and you type the elevation in — everything else still works. The `STATION` constant in `web_pages.h` is the built-in default for a browser that's never had one entered.
>
> **No GPS.** The ATGM336H and TinyGPS++ are gone. The older GPS build is preserved on the [`gps`](https://github.com/Snokones/Hamsense/tree/gps) branch.
>
> | GPS gave us | now comes from |
> |---|---|
> | UTC time (primary clock) | NTP only |
> | latitude / longitude | the `STATION` constant in `web_pages.h` |
> | altitude | the `STATION` constant, applied browser-side |
> | date | the browser's own clock |
>
> Two consequences worth knowing before you flash it:
>
> - **Pressure over the WebSocket is now raw station pressure.** The sea-level correction moved to the browser, because that's where the elevation is. Don't re-add it to the firmware without removing it from `web_pages.h`, or it gets applied twice.
> - **NTP is the only clock source.** In `PotaSense` AP-fallback mode there's no internet, so the clock never syncs — and since history writes are gated on a synced clock, **no history is recorded in AP mode**. GPS used to cover that case. A DS3231 RTC is the cheap fix if it matters; re-adding GPS is not.

---

## Nodes

### Base station — `sensor_logger.ino`

Reads sensors, keeps time, and serves the UI.

- **Outdoor temperature** — MCP9808 (high-precision I2C, ±0.25 °C typical)
- **Humidity + pressure** — BME280 (I2C), reported as raw station pressure
- **Live web page** — `ESPAsyncWebServer` + WebSocket; readings push to all connected browsers every 30 s
- **7-day temperature history** — one sample every 15 min stored in a fixed-size ring on flash (LittleFS, `/temps.bin`), served as JSON at `GET /history`. Still recorded on this branch, but the lite page draws no history panel; fetch the endpoint directly if you want the data
- **Indoor readings** — receives the satellite's `IndoorReport` over ESP-NOW and forwards it raw; the page applies the sea-level correction from `STATION.elevM`

**Time source:** NTP only. See the AP-mode caveat at the top.

**Power:** the base serves an always-on page, so it never sleeps — deep or light sleep would kill both the web server and the USB port you flash it through. The savings come from deleting the GPS (~25–30 mA) and running the CPU at 80 MHz (~15–25 mA), which is the floor with WiFi active. Measured **18 mA at 13.2 V** through a PFM buck to the XIAO's 5 V pin, roughly 45 % below the GPS build.

**WiFi with fallback:** tries your home network 3× (~15 s each). If it can't join, it broadcasts its own AP (`PotaSense`) with a DNS server that redirects every lookup to the live page, so the UI is reachable with no home WiFi around. A watchdog reboots the device if WiFi stays down for 5 minutes (station mode) or it sits idle 10+ minutes in AP fallback.

On the home network the base advertises itself over mDNS — browse to **http://hamsense.local** instead of hunting for its IP.

### Indoor satellite — `satellite_sensor.ino`

XIAO ESP32-C6 + BME280. Wakes every 60 s, takes one forced-mode reading, sends it to the base over ESP-NOW, and deep-sleeps. Total awake time per cycle is ~0.3–0.5 s, so it runs for months on a small LiPo.

To keep both nodes on the same WiFi channel (required for ESP-NOW), the satellite scans for the base's network to learn its channel, then caches that channel in RTC memory — surviving deep sleep, so the ~2 s scan runs only on first boot or after a delivery failure. Reports are sent by unicast (with a MAC-layer ACK) and retried up to 8× at 40 ms spacing to ride through the base's WiFi power-save windows.

---

## Wiring

Both sensors share the I2C bus on the base (SDA = D4, SCL = D5, 3V3 / GND).

**Base station (XIAO ESP32-C6):**

| Device   | Connection                                              |
|----------|---------------------------------------------------------|
| MCP9808  | I2C (SDA=D4, SCL=D5), address `0x18`                     |
| BME280   | I2C (SDA=D4, SCL=D5), address `0x76` or `0x77`           |

UART1 and pins D6/D7 are unused — nothing should be wired to them.

**Satellite (XIAO ESP32-C6):**

| Device | Connection                          |
|--------|-------------------------------------|
| BME280 | I2C (SDA=D4, SCL=D5), 3V3 / GND      |

---

## Build & flash

There's no CLI build wired up — these are compiled with the **Arduino IDE** (or `arduino-cli` with equivalent settings).

**Board:** XIAO ESP32-C6 on the `arduino-esp32` **3.x** core. Both sketches use the 3.x ESP-NOW callback signatures (`wifi_tx_info_t` / `esp_now_recv_info_t`); older cores need different signatures (noted in comments at each callback).

**Libraries (Arduino Library Manager):**

| Library                                     | Base | Satellite |
|---------------------------------------------|:----:|:---------:|
| Adafruit BME280 + Adafruit Unified Sensor   |  ✓   |     ✓     |
| Adafruit MCP9808                            |  ✓   |           |
| ESPAsyncWebServer + AsyncTCP                |  ✓   |           |
| ArduinoJson                                 |  ✓   |           |

`WiFi`, `esp_now`, `DNSServer`, `ESPmDNS`, and `LittleFS` ship with the ESP32 core.

> **Arduino folder layout:** the sketch files here have a `.txt` extension. The Arduino IDE requires each sketch to live in a folder named identically to its `.ino`, so rename before compiling:
>
> ```
> sensor_logger/sensor_logger.ino
> satellite_sensor/satellite_sensor.ino
> ```

---

## Configuration (secrets)

Credentials and MAC addresses live in local headers that are **git-ignored and must never be committed**.

### Base station — `secrets.h`

Copy the template and fill in your values:

```
cp secrets.h.example secrets.h
```

Defines it provides:

- `WIFI_SSID` / `WIFI_PASSWORD` — your home network
- `FALLBACK_AP_SSID` / `FALLBACK_AP_PASSWORD` — the `PotaSense` fallback AP
- `WEB_USER` / `WEB_PASS` — HTTP Basic Auth guarding the page, `/history`, and the WebSocket (**required** — the build fails without them)
- Optional static-IP block (commented out; DHCP by default)

No `ACCUWEATHER_API_KEY` on this branch, and no `GET /config` endpoint. Both exist on `main` solely to feed the forecast panel, so requiring a key here would mean registering for a service this build never contacts.

### Setting your station location

Open `web_pages.h` and edit the one constant near the top of the script:

```js
const STATION = { lat: 47.606, lon: -122.332, elevM: 0 };
```

- **`lat` / `lon`** — decimal degrees, south and west negative. Drives sunrise and sunset. Three decimal places (~100 m) is far more precision than sunrise times need.
- **`elevM`** — station elevation in **metres**. Drives the sea-level pressure correction. Left at `0`, the page reports raw station pressure — honest, but it won't match what your local airport reports.

Worth looking `elevM` up properly rather than guessing: at 1600 m, a 50 m error moves the reported pressure by roughly 6 hPa.

### Satellite — `satellite_secrets.h`

Provides `WIFI_SSID`, `FALLBACK_AP_SSID`, and `BASE_MAC`. `BASE_MAC` is the base station's **station-mode MAC**, printed on the base's Serial Monitor at boot — flash the base first, copy the MAC from its log, then set it here.

> **Web UI auth is Basic Auth over plain HTTP** — base64, not encryption. It keeps casual users off the page; it is not real secrecy. Use a passphrase you don't reuse.
>
> **The ESP-NOW link is unencrypted and unauthenticated** — anyone in radio range can sniff readings or spoof `IndoorReport` packets. Don't treat it as trusted.

---

## The ESP-NOW contract

Both sketches define an identical packed 16-byte struct:

```c
struct __attribute__((packed)) IndoorReport {
  float    tempF;
  float    humidity;
  float    pressureHPa;  // raw station pressure; base applies sea-level correction
  uint32_t seq;
};
static_assert(sizeof(IndoorReport) == 16, "...");
```

**If you change this struct, change it identically in both files** (including the `static_assert` byte count), or the base will silently receive garbled indoor readings.

---

## Repository layout

| File                          | Purpose                                                            |
|-------------------------------|--------------------------------------------------------------------|
| `sensor_logger.txt`           | Base station sketch (rename to `sensor_logger.ino` to build)       |
| `satellite_sensor.txt`        | Satellite sketch (rename to `satellite_sensor.ino` to build)       |
| `web_pages.h`                 | `index_html` for the **lite** page, and the `STATION` constant — kept out of the `.ino` on purpose |
| `secrets.h.example`           | Template for `secrets.h`                                            |
| `CLAUDE.md`                   | Guidance for AI coding assistants working in this repo             |

> **Why the HTML lives in `web_pages.h`, not inline:** the Arduino IDE's ctags-based auto-prototype generator mistakes JavaScript `function name(...) {}` inside a C++ raw string literal for real C++ functions and breaks the build with bogus errors. Keeping the HTML/JS in its own header sidesteps the bug. Don't move it back inline.
