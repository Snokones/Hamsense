# Hamsense

A two-node ESP32-C6 weather station. A **base station** reads outdoor temperature, humidity, pressure, and GPS, then serves a live web page over WiFi. A battery-powered **indoor satellite** wakes once a minute, takes a reading, radios it to the base over ESP-NOW, and goes back to deep sleep.

Both nodes run on the [Seeed XIAO ESP32-C6](https://wiki.seeedstudio.com/xiao_esp32c6_getting_started/).

---

## Nodes

### Base station — `sensor_logger.ino`

Reads sensors, keeps time, and serves the UI.

- **Outdoor temperature** — MCP9808 (high-precision I2C, ±0.25 °C typical)
- **Humidity + pressure** — BME280 (I2C)
- **GPS position, altitude, and time** — ATGM336H over UART1, parsed with TinyGPS++
- **Live web page** — `ESPAsyncWebServer` + WebSocket; readings push to all connected browsers every 10 s
- **7-day temperature history** — one sample every 15 min stored in a fixed-size ring on flash (LittleFS, `/temps.bin`), served as JSON at `GET /history` and drawn as a chart on the page
- **Indoor readings** — receives the satellite's `IndoorReport` over ESP-NOW and applies a sea-level-pressure correction using GPS altitude and outdoor temperature

**Time source:** GPS UTC is primary; NTP is the fallback when GPS has no fix and the station is on your WiFi.

**GPS duty-cycling:** the ATGM336H draws ~25–30 mA continuously, so the base keeps it awake ~30 s (extending to a 90 s cap until it gets a fix), then sleeps it for 10 minutes. Last known position/altitude/date are latched and reused while it sleeps, dropping average GPS draw to ~1–2 mA.

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
| ATGM336H | GPS TX → D7 (ESP RX), GPS RX → D6 (ESP TX), 9600 baud    |

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
| TinyGPSPlus (Mikal Hart)                    |  ✓   |           |
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
| `web_pages.h`                 | `index_html` for the live page — kept out of the `.ino` on purpose |
| `history_chart.snippet.html`  | Reference snippet for the 7-day history chart (not compiled)       |
| `secrets.h.example`           | Template for `secrets.h`                                            |
| `CLAUDE.md`                   | Guidance for AI coding assistants working in this repo             |

> **Why the HTML lives in `web_pages.h`, not inline:** the Arduino IDE's ctags-based auto-prototype generator mistakes JavaScript `function name(...) {}` inside a C++ raw string literal for real C++ functions and breaks the build with bogus errors. Keeping the HTML/JS in its own header sidesteps the bug. Don't move it back inline.
