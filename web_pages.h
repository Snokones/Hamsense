// web_pages.h  (LITE variant)
//
// Condensed, mobile-first live-readings page for the "lite" build. Same data
// feed as the full page (WebSocket JSON: h/t/p/ts/sync plus the nested "in"
// indoor object), stripped down to readings + indoor + clock. Removed vs the
// full page: the day/night world map (and its d3/topojson CDN dependencies),
// the colour-theme picker, the 7-day high/low panel, the ZIP entry box and
// the 5-day forecast panel. The result loads with ZERO external network
// dependencies -- everything is inline, so it works on a phone that's only
// joined to the station's own fallback AP with no internet.
//
// NOTE: the firmware no longer sends a "gps" object, and "p" / "in"."p" are
// now RAW STATION PRESSURE. The full page gets its coordinates and elevation
// from a ZIP the user types; this page deliberately has no such box, so it
// takes them from the STATION constant at the top of the script instead.
// Set that once for your site -- it drives sunrise/sunset and the sea-level
// pressure correction. Leaving elevM at 0 simply reports station pressure.
//
// The raw string literal lives here (not inline in the .ino) for the same
// reason as the full page: the Arduino IDE's ctags auto-prototype generator
// mis-parses JavaScript function declarations inside a C++ raw string as
// real C++ functions and injects bogus prototypes, breaking the build. A
// #include'd header is not scanned that way. See the comment near the top
// of the main .ino for the fuller explanation.
//
// IMPORTANT: included exactly once, from the main .ino. Do not include it
// elsewhere without stronger include guards -- duplicate PROGMEM defs across
// translation units would fail to link.

#pragma once

#include <Arduino.h>  // for PROGMEM -- defensive, in case include order ever changes

// ------------------------------------------------------------
// HTML: live readings page (seven-segment digits, no map)
// ------------------------------------------------------------
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head>
<meta charset='utf-8'/>
<meta name='viewport' content='width=device-width,initial-scale=1,viewport-fit=cover'/>
<title>Hamsense Lite</title>
<style>
/* Palette matches main's brightened amber. On black, perceived brightness
   tracks relative luminance -- the old #ff8c00 sat at 0.40 and read flat.
   --theme-glow is deliberately BRIGHTER than --theme-main, not darker: a
   halo darker than the segment it surrounds muddies the edge instead of
   spreading light. There is no theme picker on the lite page, so these are
   the only colours in play. */
:root{--theme-main:#ffb04d;--theme-glow:#ff8c00;--theme-dim:#e8a962;}
*{box-sizing:border-box;}
html,body{margin:0;min-height:100%;background:#000;}
body{color:var(--theme-main);font-family:Arial,Helvetica,sans-serif;text-align:center;
 padding:20px 14px calc(20px + env(safe-area-inset-bottom));
 display:flex;flex-direction:column;align-items:center;gap:18px;}
.title{font-size:13px;letter-spacing:0.22em;color:var(--theme-dim);text-transform:uppercase;}

/* --- outdoor readings: seven-segment digits, stacked so they always fit --- */
.readings{display:flex;flex-direction:column;gap:12px;width:100%;max-width:340px;}
.reading{display:flex;align-items:flex-end;justify-content:center;gap:4px;
 border:1px solid var(--theme-main);border-radius:8px;padding:8px 12px;
 background:rgba(0,0,0,0.25);}
.reading .rlbl{align-self:center;font-size:12px;color:var(--theme-dim);
 letter-spacing:0.08em;min-width:3.2em;text-align:left;}
.digit{position:relative;width:28px;height:47px;display:inline-block;margin:0 2px;}
/* Name the transitioned properties rather than using `all`: with `all`, a
   change to --theme-main does not invalidate already-painted segments in
   Chromium. Moot here (no theme picker) but wrong is wrong, and it matches main. */
.seg{position:absolute;background:transparent;border-radius:2px;transition:background-color 0.2s,box-shadow 0.2s;}
/* Three bloom layers: tight core, mid halo, wide spill. The wide layer is
   what actually reads as glowing on black. */
.on{background:var(--theme-main);box-shadow:0 0 5px var(--theme-main),0 0 13px var(--theme-glow),0 0 26px var(--theme-glow);}
.A{top:0;left:5px;width:18px;height:5px;}
.B{top:5px;right:0;width:5px;height:18px;}
.C{bottom:5px;right:0;width:5px;height:18px;}
.D{bottom:0;left:5px;width:18px;height:5px;}
.E{bottom:5px;left:0;width:5px;height:18px;}
.F{top:5px;left:0;width:5px;height:18px;}
.G{top:21px;left:5px;width:18px;height:5px;}
.unit{font-size:18px;color:var(--theme-main);opacity:0.9;align-self:center;min-width:2.6em;text-align:left;}
.unit-small{font-size:15px;}
.dot-container{width:10px;display:inline-block;height:47px;vertical-align:bottom;}
.dot{width:5px;height:5px;border-radius:50%;background:transparent;margin:0 auto;position:relative;top:37px;transition:background-color 0.2s,box-shadow 0.2s;}
.dot.on{background:var(--theme-main);box-shadow:0 0 5px var(--theme-main),0 0 13px var(--theme-glow),0 0 26px var(--theme-glow);}

/* --- indoor: plain numerics, compact --- */
.indoor{border:1px solid var(--theme-main);border-radius:8px;background:rgba(0,0,0,0.25);
 padding:10px 16px;width:100%;max-width:340px;}
.indoor-title{font-size:12px;color:var(--theme-dim);letter-spacing:0.08em;margin-bottom:6px;}
.indoor-row{font-size:17px;line-height:1.6;display:flex;justify-content:space-between;}
.indoor-row .lbl{color:var(--theme-dim);font-size:0.8em;}
.indoor-row .iunit{color:var(--theme-dim);font-size:0.8em;margin-left:4px;}

/* --- clock + GPS extras --- */
.clock{font-size:15px;color:var(--theme-main);line-height:1.7;opacity:0.95;}
.clock .tz-label{color:var(--theme-dim);font-size:0.72em;margin-right:5px;}
.grid-sep{display:inline-block;width:12px;}
.tz-select{background:transparent;color:var(--theme-dim);border:none;font-size:0.8em;
 font-family:inherit;margin-right:6px;}
.tz-select option{background:#111;color:#fff;}
.tz-select:focus{outline:1px solid var(--theme-main);}
.status{font-size:11px;color:#c55;min-height:14px;}
.status.synced{color:#5c5;display:none;} /* hide the noise once synced */
</style>
</head><body>

<div class='title'>Hamsense &middot; Lite</div>

<div class='readings'>
  <div class='reading'><span class='rlbl'>HUM</span><div id='hum'></div><span class='unit'>%</span></div>
  <div class='reading'><span class='rlbl'>TEMP</span><div id='temp'></div><span class='unit'>F</span></div>
  <div class='reading'><span class='rlbl'>PRES</span><div id='press'></div><span class='unit unit-small'>inHg</span></div>
</div>

<div class='indoor'>
  <div class='indoor-title'>INDOOR SENSOR</div>
  <div class='indoor-row'><span class='lbl'>HUM</span><span><span id='inHum'>--</span><span class='iunit'>%</span></span></div>
  <div class='indoor-row'><span class='lbl'>TEMP</span><span><span id='inTemp'>--</span><span class='iunit'>F</span></span></div>
  <div class='indoor-row'><span class='lbl'>PRES</span><span><span id='inPress'>--</span><span class='iunit'>inHg</span></span></div>
</div>

<div class='clock'>
  <div><span class='tz-label'>UTC</span><span id='clockUtc'>--:--</span></div>
  <div><span class='tz-label'>&#8593;</span><span id='sunrise'>--:--</span><span class='grid-sep'></span><span class='tz-label'>&#8595;</span><span id='sunset'>--:--</span></div>
  <div>
    <select id='tzSelect' class='tz-select'></select>
    <span id='clockLocal'>--:--</span><span class='grid-sep'></span><span id='localDate' class='tz-label'>--/--/----</span>
  </div>
</div>

<div id='timeStatus' class='status'>connecting...</div>

<script>
// Reconnecting WebSocket with capped exponential backoff.
function connectWS(onMessage) {
  let socket = null;
  let backoffMs = 1000;
  const MAX_BACKOFF = 10000;
  function connect() {
    socket = new WebSocket(`ws://${location.host}/ws`);
    socket.onmessage = onMessage;
    socket.onopen = () => { backoffMs = 1000; };
    socket.onclose = scheduleReconnect;
    socket.onerror = () => { socket.close(); };
  }
  function scheduleReconnect() {
    setTimeout(connect, backoffMs);
    backoffMs = Math.min(backoffMs * 2, MAX_BACKOFF);
  }
  connect();
  return { send: (msg) => { if (socket && socket.readyState === WebSocket.OPEN) socket.send(msg); } };
}
</script>
<script>
// ---- seven-segment digit rendering ----
const map={
0:['A','B','C','D','E','F'],
1:['B','C'],
2:['A','B','G','E','D'],
3:['A','B','G','C','D'],
4:['F','G','B','C'],
5:['A','F','G','C','D'],
6:['A','F','G','C','D','E'],
7:['A','B','C'],
8:['A','B','C','D','E','F','G'],
9:['A','B','C','D','F','G']
};

function makeDigit(n){
 let s='';
 ['A','B','C','D','E','F','G'].forEach(seg=>{
   const on = map[n].includes(seg) ? ' on' : '';
   s+=`<div class='seg ${seg}${on}'></div>`;
 });
 return `<div class='digit'>${s}</div>`;
}

function makeDot(on){
 return `<div class='dot-container'><div class='dot ${on?'on':''}'></div></div>`;
}

function render(id, val, decimals=1){
 val = Number(val).toFixed(decimals);
 let out='';
 for(let i=0;i<val.length;i++){
   const c = val[i];
   if(c === '.'){ out += makeDot(true); continue; }
   out += makeDigit(parseInt(c));
 }
 document.getElementById(id).innerHTML = out;
}

// 1 hPa = 0.0295299830714 inHg (exact conversion factor)
function hPaToInHg(hpa){ return hpa * 0.0295299830714; }

// ============================================================
// STATION -- set these three numbers once for your site.
// ============================================================
// The full build resolves these from a ZIP code the user types. The lite
// page has no such box by design, so they are constants here.
//
//   lat / lon : decimal degrees, south and west negative. Drives sunrise
//               and sunset. Get them from any map -- three decimal places
//               (~100 m) is far more precision than sunrise times need.
//   elevM     : station elevation in METRES above sea level. Drives the
//               sea-level pressure correction below. Leave it at 0 and the
//               page simply reports raw station pressure, which is honest
//               but won't match what your local airport reports.
//
// Getting elevM wrong skews pressure noticeably: at 1600 m an error of 50 m
// moves the result by roughly 6 hPa, so it's worth looking up properly
// rather than guessing.
const STATION = { lat: 47.606, lon: -122.332, elevM: 0 };

// Sea-level pressure reduction, ICAO standard barometric formula:
//   SLP = P_station * (1 + (0.0065 * alt_m) / T_K) ^ 5.2561
// The firmware sends RAW station pressure and does no correction, so this is
// the only place it happens -- if you ever move it back into the sketch,
// delete it here or it gets applied twice.
//
// Uses the OUTDOOR temperature: the air column being modelled is the one
// outside, between the station and sea level.
function toSeaLevel(stationHPa, outdoorTempF) {
  if (!STATION.elevM) return stationHPa;   // elevM 0 -> no correction
  const tK = (outdoorTempF - 32) * 5 / 9 + 273.15;
  if (!isFinite(tK) || tK <= 0) return stationHPa;
  return stationHPa * Math.pow(1 + (0.0065 * STATION.elevM) / tK, 5.2561);
}

// NOAA sunrise/sunset algorithm for the station coordinates on today's date.
// Returns {sunrise, sunset} as UTC Date objects, or null on polar day/night.
function calcSunriseSunset(latDeg, lonDeg, year, month, day) {
  const lat = latDeg * Math.PI / 180;
  const startOfYear = Date.UTC(year, 0, 1);
  const dayOfYear = Math.floor((Date.UTC(year, month-1, day) - startOfYear) / 86400000) + 1;
  const gamma = 2 * Math.PI / 365 * (dayOfYear - 1);

  const eqtime = 229.18 * (0.000075 + 0.001868*Math.cos(gamma) - 0.032077*Math.sin(gamma)
               - 0.014615*Math.cos(2*gamma) - 0.040849*Math.sin(2*gamma));
  const decl = 0.006918 - 0.399912*Math.cos(gamma) + 0.070257*Math.sin(gamma)
             - 0.006758*Math.cos(2*gamma) + 0.000907*Math.sin(2*gamma)
             - 0.002697*Math.cos(3*gamma) + 0.00148*Math.sin(3*gamma);

  const cosHa = Math.cos(90.833 * Math.PI / 180) /
                (Math.cos(lat) * Math.cos(decl)) - Math.tan(lat) * Math.tan(decl);
  if (cosHa < -1 || cosHa > 1) return null; // polar day or polar night

  const ha = Math.acos(cosHa) * 180 / Math.PI;
  const solarNoon = 720 - 4 * lonDeg - eqtime;
  const srMin = solarNoon - 4 * ha;
  const ssMin = solarNoon + 4 * ha;
  const baseDateMs = Date.UTC(year, month-1, day);
  return {
    sunrise: new Date(baseDateMs + srMin * 60000),
    sunset:  new Date(baseDateMs + ssMin * 60000)
  };
}

// Format a UTC Date as local time in the currently selected IANA timezone.
function formatLocalTime(dateUtc) {
  const iana = document.getElementById('tzSelect').value || 'America/Los_Angeles';
  return new Intl.DateTimeFormat('en-US', {
    timeZone: iana, hour: 'numeric', minute: '2-digit', hour12: true
  }).format(dateUtc);
}

// Last WS payload, so the readouts can be re-rendered on demand.
let _lastWs = null;

function updateSunriseSunset() {
  // Today's UTC date: the NOAA algorithm works in UTC and returns UTC
  // instants, which formatLocalTime() then renders in the selected zone.
  const now = new Date();
  const sun = calcSunriseSunset(STATION.lat, STATION.lon,
                                now.getUTCFullYear(), now.getUTCMonth() + 1, now.getUTCDate());
  if (sun) {
    document.getElementById('sunrise').textContent = formatLocalTime(sun.sunrise);
    document.getElementById('sunset').textContent  = formatLocalTime(sun.sunset);
  } else {
    document.getElementById('sunrise').textContent = 'N/A'; // polar day or night
    document.getElementById('sunset').textContent  = 'N/A';
  }
}

// Date straight off the browser clock, rendered in the selected zone. The
// firmware no longer supplies a date, and there is no reason to reconstruct
// one when the browser already has it.
function updateDate() {
  document.getElementById('localDate').textContent = new Intl.DateTimeFormat('en-US', {
    timeZone: document.getElementById('tzSelect').value || 'America/Los_Angeles',
    month: '2-digit', day: '2-digit', year: 'numeric'
  }).format(new Date());
}

// ---- WebSocket handler ----
// Split out of the callback so a timezone change can re-render without
// waiting up to 30s for the next push.
function renderReadings(data) {
  render('hum', data.h);
  render('temp', data.t);
  // data.p is RAW station pressure from the firmware; reduce it here.
  render('press', hPaToInHg(toSeaLevel(data.p, data.t)), 2);

  const statusEl = document.getElementById('timeStatus');
  if (data.sync) {
    statusEl.textContent = 'clock synced';
    statusEl.className = 'status synced';
  } else {
    statusEl.textContent = 'clock NOT synced (relative time only)';
    statusEl.className = 'status';
  }

  // Indoor satellite panel. "ok" is false before the first report and after
  // the node goes quiet 3+ min -- show "--" then rather than stale data.
  // Its pressure gets the same reduction, deliberately using the OUTDOOR
  // temperature: the air column between here and sea level is outside.
  if (data.in && data.in.ok) {
    document.getElementById('inHum').textContent   = data.in.h.toFixed(1);
    document.getElementById('inTemp').textContent  = data.in.t.toFixed(1);
    document.getElementById('inPress').textContent =
      hPaToInHg(toSeaLevel(data.in.p, data.t)).toFixed(2);
  } else {
    document.getElementById('inHum').textContent   = '--';
    document.getElementById('inTemp').textContent  = '--';
    document.getElementById('inPress').textContent = '--';
  }
}

connectWS((event)=>{
  const data = JSON.parse(event.data);
  _lastWs = data;
  renderReadings(data);
});

// ---- Live clock (ticks off the browser clock, smooth between WS pushes) ----
const utcFormatter = new Intl.DateTimeFormat('en-US', {
  timeZone: 'UTC', hour: '2-digit', minute: '2-digit', hour12: false
});

// US timezones. Arizona and Hawaii are listed separately (neither observes
// DST), so folding them into Mountain/Pacific would be wrong half the year.
const US_TIMEZONES = [
  { iana: 'America/New_York',    label: 'Eastern' },
  { iana: 'America/Chicago',     label: 'Central' },
  { iana: 'America/Denver',      label: 'Mountain' },
  { iana: 'America/Phoenix',     label: 'Arizona' },
  { iana: 'America/Los_Angeles', label: 'Pacific' },
  { iana: 'America/Anchorage',   label: 'Alaska' },
  { iana: 'Pacific/Honolulu',    label: 'Hawaii' }
];
const DEFAULT_TZ = 'America/Los_Angeles';

let localFormatter = null; // rebuilt whenever the selected timezone changes

function buildLocalFormatter(iana) {
  localFormatter = new Intl.DateTimeFormat('en-US', {
    timeZone: iana, hour: '2-digit', minute: '2-digit', hour12: true
  });
}

function loadSavedTimezone() {
  try { return localStorage.getItem('clockTimezone') || DEFAULT_TZ; }
  catch (e) { return DEFAULT_TZ; } // localStorage can throw in privacy modes
}
function saveTimezone(iana) {
  try { localStorage.setItem('clockTimezone', iana); } catch (e) { /* ignore */ }
}

const tzSelect = document.getElementById('tzSelect');
US_TIMEZONES.forEach(tz => {
  const opt = document.createElement('option');
  opt.value = tz.iana;
  opt.textContent = tz.label;
  tzSelect.appendChild(opt);
});

const savedTz = loadSavedTimezone();
tzSelect.value = savedTz;
buildLocalFormatter(savedTz);

tzSelect.addEventListener('change', () => {
  buildLocalFormatter(tzSelect.value);
  saveTimezone(tzSelect.value);
  tickClock();
  updateSunriseSunset();
});

function tickClock() {
  const now = new Date();
  document.getElementById('clockUtc').textContent = utcFormatter.format(now);
  document.getElementById('clockLocal').textContent = localFormatter.format(now);
  // The date comes from this same browser clock now, so refresh it on the
  // tick -- otherwise it would sit on yesterday's date until a tz change.
  updateDate();
}
tickClock();
setInterval(tickClock, 5000);

// Sunrise/sunset depend only on STATION and today's date, so compute them
// once at load rather than waiting for a WebSocket push. They are also
// recomputed on a timezone change, and the 5s tick rolls the date over.
updateSunriseSunset();
</script>

</body></html>
)rawliteral";
