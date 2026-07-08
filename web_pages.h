// web_pages.h
//
// The HTML page string literal (index_html) lives here instead of in the
// main .ino file. This is a deliberate workaround for a confirmed,
// longstanding Arduino IDE bug: the auto-prototype-generator (ctags)
// cannot correctly parse multi-line C++ raw string literals, so when this
// page's embedded JavaScript function declarations sit inside the main
// .ino file, ctags mistakes them for real C++ functions and injects
// bogus prototypes/definitions elsewhere in the file -- causing compile
// errors that point at lines that are actually plain JavaScript inside a
// string, not real C++.
//
// The Arduino IDE's automatic prototype generation only scans the
// primary .ino file's own content; a #include'd .h file's content is
// not subject to that same scan. Moving the raw string literal here
// is the fix recommended directly by Arduino's own developers in the
// upstream bug tracker (the bug itself is acknowledged as unlikely to
// ever be fixed in ctags). See the comment in the main .ino file, near
// the top, for the fuller explanation and links.
//
// IMPORTANT: this file is included exactly once, from the main .ino.
// Do not include it from anywhere else without adding include guards
// beyond #pragma once if you ever split it further -- duplicate
// PROGMEM definitions across translation units would fail to link.

#pragma once

#include <Arduino.h>  // for the PROGMEM macro -- defensive, in case include order ever changes

// ------------------------------------------------------------
// HTML: live readings page (seven-segment digits + day/night map)
// ------------------------------------------------------------
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head>
<meta charset='utf-8'/>
<title>Hamsense</title>
<style>
:root{--theme-main:#ff8c00;--theme-glow:#cc7000;--theme-dim:#c98a2e;}
html,body{margin:0;height:100%;background:#000;}
body{color:var(--theme-main);font-family:Arial;text-align:center;}
#mapBg{position:fixed;top:0;left:0;width:100%;height:100%;z-index:0;overflow:hidden;}
#mapBg svg{width:100%;height:100%;}
.content{position:fixed;left:0;right:0;bottom:0;z-index:1;padding-bottom:28px;}
.wrap{display:flex;justify-content:center;gap:28px;align-items:flex-end;}
.reading{display:flex;align-items:flex-end;gap:3px;border:1px solid var(--theme-main);border-radius:8px;padding:8px 14px;background:rgba(0,0,0,0.25);}
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
.unit{font-size:20px;color:var(--theme-main);opacity:0.9;align-self:center;text-shadow:0 0 4px rgba(0,0,0,0.8);}
.unit-small{font-size:16px;}
.dot-container{width:10px;display:inline-block;height:47px;vertical-align:bottom;}
.dot{width:5px;height:5px;border-radius:50%;background:transparent;margin:0 auto;position:relative;top:37px;transition:0.2s;}
.dot.on{background:var(--theme-main);box-shadow:0 0 4px var(--theme-main),0 0 7px var(--theme-glow);}
a{color:var(--theme-main);text-decoration:none;margin-top:22px;display:block;text-shadow:0 0 4px rgba(0,0,0,0.8);}
.status{font-size:11px;color:#999;margin-top:14px;text-shadow:0 0 4px rgba(0,0,0,0.8);display:none;}
.status.synced{color:#5c5;}
.status.unsynced{color:#c55;}
.clock{font-size:15px;color:var(--theme-main);margin-top:12px;opacity:0.95;line-height:1.5;text-shadow:0 0 4px rgba(0,0,0,0.8);}
.clock .tz-label{color:var(--theme-dim);font-size:0.7em;margin-right:6px;}
.tz-select{background:transparent;color:var(--theme-dim);border:none;font-size:0.7em;font-family:Arial;margin-right:6px;text-shadow:0 0 4px rgba(0,0,0,0.8);}
.tz-select option{background:#111;color:#fff;}
.tz-select:focus{outline:1px solid var(--theme-main);}
.grid-sep{display:inline-block;width:14px;}
.theme-btn{position:fixed;top:14px;right:14px;z-index:2;width:32px;height:32px;border-radius:50%;background:rgba(0,0,0,0.5);border:1px solid var(--theme-main);display:flex;align-items:center;justify-content:center;cursor:pointer;padding:0;}
.theme-btn .swatch{width:16px;height:16px;border-radius:50%;background:var(--theme-main);box-shadow:0 0 4px var(--theme-main);}
.theme-menu{position:fixed;top:54px;right:14px;z-index:2;display:none;flex-direction:column;gap:8px;background:rgba(0,0,0,0.6);border:1px solid var(--theme-main);border-radius:8px;padding:8px;}
.theme-menu.open{display:flex;}
.theme-menu button{width:24px;height:24px;border-radius:50%;border:1px solid rgba(255,255,255,0.4);padding:0;cursor:pointer;}
.indoor-panel{position:fixed;bottom:14px;right:14px;z-index:1;border:1px solid var(--theme-main);border-radius:8px;background:rgba(0,0,0,0.25);padding:10px 14px;text-align:right;color:var(--theme-main);font-family:Arial;}
.indoor-title{font-size:12px;color:var(--theme-dim);letter-spacing:0.08em;margin-bottom:5px;}
.indoor-row{font-size:16px;line-height:1.55;}
.indoor-row .lbl{color:var(--theme-dim);font-size:0.75em;margin-right:6px;}
.indoor-row .iunit{color:var(--theme-dim);font-size:0.75em;margin-left:4px;}
</style>
</head><body>

<div id='mapBg'></div>

<div class='content'>
<div class='wrap'>
  <div class='reading'><div id='hum'></div><span class='unit'>%</span></div>
  <div class='reading'><div id='temp'></div><span class='unit'>F</span></div>
  <div class='reading'><div id='press'></div><span class='unit unit-small'>inHg</span></div>
</div>

<div class='clock'>
  <div><span class='tz-label'>UTC</span><span id='clockUtc'>--:--</span><span class='grid-sep'></span><span class='tz-label'>GRID</span><span id='gridSquare'>--</span><span class='grid-sep'></span><span class='tz-label'>↑</span><span id='sunrise'>--:--</span><span class='grid-sep'></span><span class='tz-label'>↓</span><span id='sunset'>--:--</span></div>
  <div>
    <select id='tzSelect' class='tz-select'></select>
    <span id='clockLocal'>--:--</span><span class='grid-sep'></span><span id='gpsDate' class='tz-label'>--/--/----</span>
  </div>
</div>

<div id='timeStatus' class='status'>connecting...</div>
</div>

<button class='theme-btn' id='themeBtn' aria-label='Change theme color'><span class='swatch'></span></button>
<div class='theme-menu' id='themeMenu'>
  <button data-theme='amber' style='background:#ff8c00' aria-label='Amber'></button>
  <button data-theme='red' style='background:#ff3b30' aria-label='Red'></button>
  <button data-theme='green' style='background:#34c759' aria-label='Green'></button>
  <button data-theme='yellow' style='background:#ffd60a' aria-label='Yellow'></button>
  <button data-theme='blue' style='background:#0a84ff' aria-label='Blue'></button>
  <button data-theme='pink' style='background:#ff2d78' aria-label='Pink'></button>
</div>

<div class='indoor-panel'>
  <div class='indoor-title'>INDOOR SENSOR</div>
  <div class='indoor-row'><span class='lbl'>HUM</span><span id='inHum'>--</span><span class='iunit'>%</span></div>
  <div class='indoor-row'><span class='lbl'>TEMP</span><span id='inTemp'>--</span><span class='iunit'>F</span></div>
  <div class='indoor-row'><span class='lbl'>PRES</span><span id='inPress'>--</span><span class='iunit'>inHg</span></div>
</div>

<script src="https://cdnjs.cloudflare.com/ajax/libs/d3/7.8.5/d3.min.js"></script>
<script src="https://cdnjs.cloudflare.com/ajax/libs/topojson/3.0.2/topojson.min.js"></script>
<script>
// GPS pin layer -- defined here (before the map IIFE) so window.initMapPin
// exists when the IIFE calls it during map setup. The WS handler in the
// later script block calls window.setGpsPin once a fix arrives.
let _pinLayer = null;
let _projectFn = null;
window.initMapPin = function(layer, projFn) {
  _pinLayer = layer;
  _projectFn = projFn;
};
window.setGpsPin = function(lat, lon) {
  if (!_pinLayer || !_projectFn) return;
  _pinLayer.selectAll('.gps-pin').remove();
  const pt = _projectFn([lon, lat]);
  if (!pt) return;
  _pinLayer.append('circle')
    .attr('class', 'gps-pin gps-pin-glow')
    .attr('cx', pt[0]).attr('cy', pt[1])
    .attr('r', 12)
    .attr('fill', 'none')
    .attr('stroke', getComputedStyle(document.documentElement).getPropertyValue('--theme-main').trim())
    .attr('stroke-width', 1.5)
    .attr('opacity', 0.4);
  _pinLayer.append('circle')
    .attr('class', 'gps-pin gps-pin-dot')
    .attr('cx', pt[0]).attr('cy', pt[1])
    .attr('r', 6)
    .attr('fill', getComputedStyle(document.documentElement).getPropertyValue('--theme-main').trim())
    .attr('opacity', 0.85);
};

function solarPosition(dtUtc) {
  const startOfYear = new Date(Date.UTC(dtUtc.getUTCFullYear(), 0, 1));
  const dayOfYear = (dtUtc - startOfYear) / 86400000;
  const gamma = 2 * Math.PI / 365.0 * dayOfYear;
  const eqtime = 229.18 * (0.000075 + 0.001868*Math.cos(gamma) - 0.032077*Math.sin(gamma)
              - 0.014615*Math.cos(2*gamma) - 0.040849*Math.sin(2*gamma));
  const decl = (0.006918 - 0.399912*Math.cos(gamma) + 0.070257*Math.sin(gamma)
              - 0.006758*Math.cos(2*gamma) + 0.000907*Math.sin(2*gamma)
              - 0.002697*Math.cos(3*gamma) + 0.00148*Math.sin(3*gamma));
  const utcHourDecimal = dtUtc.getUTCHours() + dtUtc.getUTCMinutes()/60 + dtUtc.getUTCSeconds()/3600;
  let subsolarLon = -15.0 * (utcHourDecimal - 12 + eqtime/60);
  subsolarLon = ((subsolarLon + 180) % 360 + 360) % 360 - 180;
  return { decl: decl * 180 / Math.PI, subsolarLon };
}

function terminatorLatLon(declDeg, subsolarLon) {
  const decl = declDeg * Math.PI / 180;
  const pts = [];
  for (let lon = -180; lon <= 180; lon += 2) {
    const dLon = (lon - subsolarLon) * Math.PI / 180;
    let lat;
    if (Math.abs(Math.tan(decl)) < 1e-6) {
      lat = (Math.cos(dLon) >= 0) ? -89.9 : 89.9;
    } else {
      lat = Math.atan(-Math.cos(dLon) / Math.tan(decl)) * 180 / Math.PI;
    }
    pts.push([lon, lat]);
  }
  return pts;
}

(function initMap() {
  const container = document.getElementById('mapBg');
  const W = window.innerWidth || 800;
  const H = window.innerHeight || 600;

  const landFill = 'rgba(245,242,235,0.45)';
  const oceanFill = '#000000';
  const nightFill = 'rgba(0,0,0,0.55)';
  let strokeColor = 'rgba(255,140,0,0.15)';
  let terminatorColor = 'rgba(255,140,0,0.4)';

  const scale = Math.max(W, H) / (2 * Math.PI) * 1.05 * 0.9; // 10% smaller
  const projection = d3.geoMercator()
    .scale(scale)
    .translate([W / 2, H / 2]);
  const path = d3.geoPath(projection);

  const svg = d3.select('#mapBg').append('svg')
    .attr('viewBox', `0 0 ${W} ${H}`)
    .attr('preserveAspectRatio', 'xMidYMid slice');

  svg.append('rect').attr('width', W).attr('height', H).attr('fill', oceanFill);
  const landLayer = svg.append('g');
  const nightLayer = svg.append('g');
  // Pin layer sits on top of everything so the location marker is always visible
  const pinLayer = svg.append('g');
  window.initMapPin(pinLayer, (coords) => projection(coords));

  function drawNightSide(decl, subsolarLon) {
    nightLayer.selectAll('*').remove();
    const poleLat = (decl >= 0) ? -89.9 : 89.9;
    const termPts = terminatorLatLon(decl, subsolarLon);
    const ring = [[-180, poleLat], ...termPts, [180, poleLat], [-180, poleLat]];
    nightLayer.append('path')
      .datum({ type: 'Polygon', coordinates: [ring] })
      .attr('d', path)
      .attr('fill', nightFill)
      .attr('stroke', 'none');
    nightLayer.append('path')
      .datum({ type: 'LineString', coordinates: termPts })
      .attr('d', path)
      .attr('fill', 'none')
      .attr('class', 'terminator-line')
      .attr('stroke', terminatorColor)
      .attr('stroke-width', 1);
  }

  function refreshMap() {
    const now = new Date();
    const { decl, subsolarLon } = solarPosition(now);
    drawNightSide(decl, subsolarLon);
  }

  d3.json('https://cdn.jsdelivr.net/npm/world-atlas@2/land-110m.json').then(world => {
    const land = topojson.feature(world, world.objects.land);
    landLayer.selectAll('path')
      .data(land.features ? land.features : [land])
      .join('path')
      .attr('d', path)
      .attr('fill', landFill)
      .attr('stroke', strokeColor)
      .attr('stroke-width', 0.5);
    refreshMap();
  }).catch(err => {
    console.error('Failed to load map data', err);
  });

  setInterval(refreshMap, 30 * 60 * 1000);

  // Called from the theme picker (a later script block) whenever the user
  // picks a new color. Converts the theme's main hex color into the same
  // translucent stroke/terminator shades used at startup, then re-paints
  // the already-rendered map elements directly (selectAll picks up
  // whatever's currently in the DOM, including across refreshMap() calls).
  window.setMapThemeColor = function(hexColor) {
    const r = parseInt(hexColor.slice(1, 3), 16);
    const g = parseInt(hexColor.slice(3, 5), 16);
    const b = parseInt(hexColor.slice(5, 7), 16);
    strokeColor = `rgba(${r},${g},${b},0.15)`;
    terminatorColor = `rgba(${r},${g},${b},0.4)`;
    landLayer.selectAll('path').attr('stroke', strokeColor);
    nightLayer.selectAll('path.terminator-line').attr('stroke', terminatorColor);
    // Repaint the GPS pin with the new theme color if one is showing
    if (_pinLayer) {
      _pinLayer.selectAll('.gps-pin-glow').attr('stroke', hexColor);
      _pinLayer.selectAll('.gps-pin-dot').attr('fill', hexColor);
    }
  };
})();
</script>
<script>
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
   if(c === '.'){
     out += makeDot(true);
     continue;
   }
   out += makeDigit(parseInt(c));
 }
 document.getElementById(id).innerHTML = out;
}

// 1 hPa = 0.0295299830714 inHg (exact conversion factor)
function hPaToInHg(hpa) {
  return hpa * 0.0295299830714;
}

// ---- GPS-derived display functions ----

// Maidenhead grid square locator (4-character), computed from GPS coordinates.
// The formula: divide adjusted lon/lat into field (A-R) and square (0-9) steps.
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

// Approximate IANA timezone from lat/lon for the US.
// Uses longitude bands with explicit exceptions for Arizona (no DST) and
// Alaska/Hawaii. Accurate for major US cities; known edge cases at state
// borders (e.g. parts of Indiana, western Kentucky) may be one zone off.
// The result is used to auto-select the existing timezone dropdown --
// the user can always manually override if the guess is wrong.
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
// Auto-selections can be overridden by the user; user selections persist and
// won't be overwritten by future GPS auto-selects in the same session.
let tzSetByUser = false;

// NOAA sunrise/sunset algorithm -- same solar position math used for the
// day/night terminator, applied here to find the exact rise/set times for
// the user's GPS coordinates and the current GPS date.
// Returns {sunrise, sunset} as Date objects in UTC, or null if polar day/night.
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

  const ha = Math.acos(cosHa) * 180 / Math.PI; // degrees
  const solarNoon = 720 - 4 * lonDeg - eqtime; // minutes from UTC midnight

  const srMin = solarNoon - 4 * ha;
  const ssMin = solarNoon + 4 * ha;

  // Convert minutes-from-midnight to Date objects on the given UTC date
  const baseDateMs = Date.UTC(year, month-1, day);
  return {
    sunrise: new Date(baseDateMs + srMin * 60000),
    sunset:  new Date(baseDateMs + ssMin * 60000)
  };
}

// Format a UTC Date as local time using the currently selected IANA timezone.
// Called whenever sunrise/sunset is (re)computed or the timezone changes.
function formatLocalTime(dateUtc) {
  const iana = document.getElementById('tzSelect').value || 'America/Los_Angeles';
  return new Intl.DateTimeFormat('en-US', {
    timeZone: iana, hour: 'numeric', minute: '2-digit', hour12: true
  }).format(dateUtc);
}

// Cache the last-known GPS position and date so sunrise/sunset can be
// recomputed if the user changes timezone without waiting for the next WS push.
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
    statusEl.className = 'status unsynced';
  }

  // Handle GPS data if present and a fix is active
  if (data.gps && data.gps.fix && data.gps.lat != null && data.gps.lon != null) {
    const lat = data.gps.lat;
    const lon = data.gps.lon;
    const year = data.gps.year;
    const mon  = data.gps.mon;
    const day  = data.gps.day;

    // Grid square
    document.getElementById('gridSquare').textContent = maidenhead4(lat, lon);

    // Cache for timezone-change recompute and update sunrise/sunset + date
    _lastGpsFix = { lat, lon, year, mon, day, ts: data.ts };
    updateSunriseSunset();
    updateDate();

    // Map pin
    if (window.setGpsPin) window.setGpsPin(lat, lon);

    // Auto-select timezone from GPS coordinates, but only if the user
    // hasn't already manually chosen a timezone in this session.
    if (!tzSetByUser) {
      const ianaGuess = approxTimezone(lat, lon);
      const sel = document.getElementById('tzSelect');
      if (Array.from(sel.options).some(o => o.value === ianaGuess)) {
        if (sel.value !== ianaGuess) {
          sel.value = ianaGuess;
          buildLocalFormatter(ianaGuess);
          tickClock();
          updateSunriseSunset(); // reformat with new zone
        }
      }
    }
  } else {
    // No fix yet -- show placeholder
    document.getElementById('gridSquare').textContent = '--';
    document.getElementById('sunrise').textContent = '--:--';
    document.getElementById('sunset').textContent  = '--:--';
    document.getElementById('gpsDate').textContent = '--/--/----';
  }

  // Indoor satellite panel. "ok" is false before the first report and
  // whenever the node has gone quiet for 3+ minutes -- show "--" then
  // rather than silently displaying stale data.
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

// Live UTC + selectable local clock, ticking off the browser's own clock
// (not the device's WS push) so it stays smooth and accurate between
// sensor updates. Intl.DateTimeFormat with an explicit IANA zone handles
// DST transitions automatically -- no manual DST logic needed for any zone.
const utcFormatter = new Intl.DateTimeFormat('en-US', {
  timeZone: 'UTC', hour: '2-digit', minute: '2-digit', hour12: false
});

// Standard US timezones. Arizona and Hawaii are listed separately from
// Mountain/Pacific rather than folded in, since neither observes DST --
// grouping them under the DST-observing zone would give wrong times for
// roughly half the year.
const US_TIMEZONES = [
  { iana: 'America/New_York',    label: 'Eastern' },
  { iana: 'America/Chicago',     label: 'Central' },
  { iana: 'America/Denver',      label: 'Mountain' },
  { iana: 'America/Phoenix',     label: 'Arizona' },
  { iana: 'America/Los_Angeles', label: 'Pacific' },
  { iana: 'America/Anchorage',   label: 'Alaska' },
  { iana: 'Pacific/Honolulu',    label: 'Hawaii' }
];
const DEFAULT_TZ = 'America/Los_Angeles'; // Pacific -- matches the original Seattle default

let localFormatter = null; // rebuilt whenever the selected timezone changes

function buildLocalFormatter(iana) {
  localFormatter = new Intl.DateTimeFormat('en-US', {
    timeZone: iana, hour: '2-digit', minute: '2-digit', hour12: true
  });
}

function loadSavedTimezone() {
  try {
    return localStorage.getItem('clockTimezone') || DEFAULT_TZ;
  } catch (e) {
    return DEFAULT_TZ; // localStorage can throw in some privacy modes -- fall back quietly
  }
}

function saveTimezone(iana) {
  try {
    localStorage.setItem('clockTimezone', iana);
  } catch (e) {
    // ignore -- selection just won't persist this session, not worth surfacing an error for
  }
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
// If localStorage had a saved value, treat it as a user choice so GPS auto-
// select doesn't silently override it on this session's first fix.
if (localStorage.getItem('clockTimezone')) tzSetByUser = true;

tzSelect.addEventListener('change', () => {
  tzSetByUser = true;
  buildLocalFormatter(tzSelect.value);
  saveTimezone(tzSelect.value);
  tickClock();
  updateSunriseSunset();
  updateDate(); // reformat date in new local timezone
});

function tickClock() {
  const now = new Date();
  document.getElementById('clockUtc').textContent = utcFormatter.format(now);
  document.getElementById('clockLocal').textContent = localFormatter.format(now);
}
tickClock();
setInterval(tickClock, 5000); // 5s is plenty now that only h:m is shown

// Theme color picker. Each entry: main (lit segments/text), glow (outer
// box-shadow, a deeper shade of main), dim (muted variant for the small
// UTC/timezone-dim labels). Persisted in localStorage so it survives reloads.
const THEMES = {
  amber:  { main: '#ff8c00', glow: '#cc7000', dim: '#c98a2e' },
  red:    { main: '#ff3b30', glow: '#b8241c', dim: '#c9504a' },
  green:  { main: '#34c759', glow: '#1f9f42', dim: '#3fae5e' },
  yellow: { main: '#ffd60a', glow: '#cc9f00', dim: '#cbaa2e' },
  blue:   { main: '#0a84ff', glow: '#0860c4', dim: '#3f8fd9' },
  pink:   { main: '#ff2d78', glow: '#c41f5c', dim: '#cf4d7e' }
};

function applyTheme(name) {
  const t = THEMES[name] || THEMES.amber;
  const root = document.documentElement.style;
  root.setProperty('--theme-main', t.main);
  root.setProperty('--theme-glow', t.glow);
  root.setProperty('--theme-dim', t.dim);
  // Map coastline/terminator strokes are drawn via JS (not CSS), so they
  // need to be updated directly here too -- otherwise they'd stay amber
  // regardless of the chosen theme.
  if (window.setMapThemeColor) window.setMapThemeColor(t.main);
}

function loadSavedTheme() {
  try {
    return localStorage.getItem('themeColor') || 'amber';
  } catch (e) {
    return 'amber'; // localStorage can throw in some privacy modes -- fall back quietly
  }
}

function saveTheme(name) {
  try {
    localStorage.setItem('themeColor', name);
  } catch (e) {
    // ignore -- theme just won't persist this session, not worth surfacing an error for
  }
}

const themeBtn = document.getElementById('themeBtn');
const themeMenu = document.getElementById('themeMenu');
themeBtn.addEventListener('click', () => {
  themeMenu.classList.toggle('open');
});
themeMenu.querySelectorAll('button[data-theme]').forEach(btn => {
  btn.addEventListener('click', () => {
    const name = btn.getAttribute('data-theme');
    applyTheme(name);
    saveTheme(name);
    themeMenu.classList.remove('open');
  });
});
// Close the menu if the user taps elsewhere on the page.
document.addEventListener('click', (e) => {
  if (!themeMenu.contains(e.target) && e.target !== themeBtn && !themeBtn.contains(e.target)) {
    themeMenu.classList.remove('open');
  }
});

applyTheme(loadSavedTheme());
</script>

</body></html>
)rawliteral";
