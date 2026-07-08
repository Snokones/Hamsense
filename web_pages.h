// web_pages.h  (LITE variant)
//
// Condensed, mobile-first live-readings page for the "lite" build. This is
// the same data feed as the full page (identical WebSocket JSON: h/t/p/ts/
// sync, the nested "in" indoor object, and the nested "gps" object), but
// stripped down to just the readings + clock + GPS-derived extras. Removed
// vs the full page: the day/night world map (and its d3/topojson CDN
// dependencies), the color-theme picker, and the 7-day high/low panel. The
// result loads with ZERO external network dependencies -- everything is
// inline, so it works on a phone that's only joined to the station's own
// fallback AP with no internet.
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
:root{--theme-main:#ff8c00;--theme-glow:#cc7000;--theme-dim:#c98a2e;}
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
.seg{position:absolute;background:transparent;border-radius:2px;transition:0.2s;}
.on{background:var(--theme-main);box-shadow:0 0 4px var(--theme-main),0 0 7px var(--theme-glow);}
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
.dot{width:5px;height:5px;border-radius:50%;background:transparent;margin:0 auto;position:relative;top:37px;transition:0.2s;}
.dot.on{background:var(--theme-main);box-shadow:0 0 4px var(--theme-main),0 0 7px var(--theme-glow);}

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
  <div><span class='tz-label'>UTC</span><span id='clockUtc'>--:--</span><span class='grid-sep'></span><span class='tz-label'>GRID</span><span id='gridSquare'>--</span></div>
  <div><span class='tz-label'>&#8593;</span><span id='sunrise'>--:--</span><span class='grid-sep'></span><span class='tz-label'>&#8595;</span><span id='sunset'>--:--</span></div>
  <div>
    <select id='tzSelect' class='tz-select'></select>
    <span id='clockLocal'>--:--</span><span class='grid-sep'></span><span id='gpsDate' class='tz-label'>--/--/----</span>
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

// ---- GPS-derived display helpers ----

// Maidenhead grid square locator (4-character) from GPS coordinates.
// Verified against known references: Seattle=CN87, Manhattan=FN30, London=IO91.
function maidenhead4(lat, lon) {
  const lonAdj = lon + 180.0;
  const latAdj = lat + 90.0;
  const fLon = Math.floor(lonAdj / 20);
  const fLat = Math.floor(latAdj / 10);
  const sLon = Math.floor((lonAdj % 20) / 2);
  const sLat = Math.floor(latAdj % 10);
  return String.fromCharCode(65+fLon) + String.fromCharCode(65+fLat) + sLon + sLat;
}

// Approximate IANA timezone from lat/lon for the US. Longitude bands with
// explicit exceptions for Arizona (no DST) and Alaska/Hawaii. Accurate for
// major US cities; used only to auto-select the dropdown -- the user can
// always override manually.
function approxTimezone(lat, lon) {
  if (lat < 25 && lon < -140)   return 'Pacific/Honolulu';
  if (lat > 54 || (lat > 50 && lon < -141)) return 'America/Anchorage';
  if (lon < -114)                return 'America/Los_Angeles';
  if (lon < -104) {
    if (lat > 31 && lat < 37.5 && lon > -115) return 'America/Phoenix';
    return 'America/Denver';
  }
  if (lon < -85.5)               return 'America/Chicago';
  return 'America/New_York';
}

// Track whether the timezone was set by GPS (auto) vs the user manually.
let tzSetByUser = false;

// NOAA sunrise/sunset algorithm for the GPS coordinates + current GPS date.
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

// Cache last-known GPS fix so sunrise/sunset + date can be recomputed on a
// timezone change without waiting for the next WS push.
let _lastGpsFix = null;

function updateSunriseSunset() {
  if (!_lastGpsFix) return;
  const { lat, lon, year, mon, day } = _lastGpsFix;
  const sun = calcSunriseSunset(lat, lon, year, mon, day);
  if (sun) {
    document.getElementById('sunrise').textContent = formatLocalTime(sun.sunrise);
    document.getElementById('sunset').textContent  = formatLocalTime(sun.sunset);
  } else {
    document.getElementById('sunrise').textContent = 'N/A';
    document.getElementById('sunset').textContent  = 'N/A';
  }
}

function updateDate() {
  if (!_lastGpsFix) return;
  const { year, mon, day, ts } = _lastGpsFix;
  if (!year || !mon || !day) return;
  const gpsUtcMs = Date.UTC(year, mon-1, day,
                            ts ? Math.floor((ts % 86400) / 3600) : 0,
                            ts ? Math.floor((ts % 3600) / 60) : 0,
                            ts ? (ts % 60) : 0);
  document.getElementById('gpsDate').textContent = new Intl.DateTimeFormat('en-US', {
    timeZone: document.getElementById('tzSelect').value || 'America/Los_Angeles',
    month: '2-digit', day: '2-digit', year: 'numeric'
  }).format(new Date(gpsUtcMs));
}

// ---- WebSocket handler ----
connectWS((event)=>{
  let data = JSON.parse(event.data);
  render('hum', data.h);
  render('temp', data.t);
  render('press', hPaToInHg(data.p), 2);

  const statusEl = document.getElementById('timeStatus');
  if (data.sync) {
    statusEl.textContent = 'clock synced';
    statusEl.className = 'status synced';
  } else {
    statusEl.textContent = 'clock NOT synced (relative time only)';
    statusEl.className = 'status';
  }

  // GPS-derived extras (grid square, sunrise/sunset, date) when a fix exists.
  if (data.gps && data.gps.fix && data.gps.lat != null && data.gps.lon != null) {
    const lat = data.gps.lat, lon = data.gps.lon;
    const year = data.gps.year, mon = data.gps.mon, day = data.gps.day;

    document.getElementById('gridSquare').textContent = maidenhead4(lat, lon);

    _lastGpsFix = { lat, lon, year, mon, day, ts: data.ts };
    updateSunriseSunset();
    updateDate();

    // Auto-select timezone from GPS, unless the user already chose one.
    if (!tzSetByUser) {
      const ianaGuess = approxTimezone(lat, lon);
      const sel = document.getElementById('tzSelect');
      if (Array.from(sel.options).some(o => o.value === ianaGuess)) {
        if (sel.value !== ianaGuess) {
          sel.value = ianaGuess;
          buildLocalFormatter(ianaGuess);
          tickClock();
          updateSunriseSunset();
        }
      }
    }
  } else {
    document.getElementById('gridSquare').textContent = '--';
    document.getElementById('sunrise').textContent = '--:--';
    document.getElementById('sunset').textContent  = '--:--';
    document.getElementById('gpsDate').textContent = '--/--/----';
  }

  // Indoor satellite panel. "ok" is false before the first report and after
  // the node goes quiet 3+ min -- show "--" then rather than stale data.
  if (data.in && data.in.ok) {
    document.getElementById('inHum').textContent   = data.in.h.toFixed(1);
    document.getElementById('inTemp').textContent  = data.in.t.toFixed(1);
    document.getElementById('inPress').textContent = hPaToInHg(data.in.p).toFixed(2);
  } else {
    document.getElementById('inHum').textContent   = '--';
    document.getElementById('inTemp').textContent  = '--';
    document.getElementById('inPress').textContent = '--';
  }
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
// A saved value counts as a user choice so GPS auto-select won't override it.
if (localStorage.getItem('clockTimezone')) tzSetByUser = true;

tzSelect.addEventListener('change', () => {
  tzSetByUser = true;
  buildLocalFormatter(tzSelect.value);
  saveTimezone(tzSelect.value);
  tickClock();
  updateSunriseSunset();
  updateDate();
});

function tickClock() {
  const now = new Date();
  document.getElementById('clockUtc').textContent = utcFormatter.format(now);
  document.getElementById('clockLocal').textContent = localFormatter.format(now);
}
tickClock();
setInterval(tickClock, 5000);
</script>

</body></html>
)rawliteral";
