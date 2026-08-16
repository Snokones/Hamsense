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
.reading{display:flex;align-items:flex-end;gap:3px;border:1px solid var(--theme-main);border-radius:8px;padding:8px 14px;background:rgba(0,0,0,0.25);box-shadow:0 0 10px var(--theme-glow),inset 0 0 12px rgba(0,0,0,0.6);}
.digit{position:relative;width:28px;height:47px;display:inline-block;margin:0 2px;}
/* Transition the two properties that actually change, NOT `all`. With
   `transition:all`, a change to --theme-main does not invalidate already-
   painted segments in Chromium: the custom property updates, var() resolves
   to the new colour, and the element keeps rendering the old one until it is
   recreated. That made theme switching appear to do nothing until the next
   WebSocket push rebuilt the digits. applyTheme() also forces a re-render as
   a belt-and-braces fix -- see the theme picker script. */
.seg{position:absolute;background:transparent;border-radius:2px;transition:background-color 0.2s,box-shadow 0.2s;}
/* Lit segment bloom. Three layers instead of two: a tight core at the segment
   edge, a mid halo, and a wide soft spill. The wide layer is what actually
   reads as "glowing" on black -- the old 4px/7px pair was too tight to spread
   past the segment itself, so low-luminance themes (blue, red, pink) looked
   flat no matter what color they were. */
.on{background:var(--theme-main);box-shadow:0 0 5px var(--theme-main),0 0 13px var(--theme-glow),0 0 26px var(--theme-glow);}
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
.dot{width:5px;height:5px;border-radius:50%;background:transparent;margin:0 auto;position:relative;top:37px;transition:background-color 0.2s,box-shadow 0.2s;}
.dot.on{background:var(--theme-main);box-shadow:0 0 5px var(--theme-main),0 0 13px var(--theme-glow),0 0 26px var(--theme-glow);}
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
/* 7-day high/low panel -- mirror of .indoor-panel, pinned bottom-left */
.history-panel{position:fixed;bottom:14px;left:14px;z-index:1;border:1px solid var(--theme-main);border-radius:8px;background:rgba(0,0,0,0.25);padding:10px 14px;text-align:left;color:var(--theme-main);font-family:Arial;}
.history-title{font-size:12px;color:var(--theme-dim);letter-spacing:0.08em;margin-bottom:5px;}
.history-row{font-size:16px;line-height:1.4;white-space:nowrap;}
.history-row .lbl{color:var(--theme-dim);font-size:0.75em;margin-right:8px;display:inline-block;min-width:2.6em;}
.history-row .sep{color:var(--theme-dim);margin:0 4px;}
.history-row .hunit{color:var(--theme-dim);font-size:0.75em;margin-left:4px;}
/* ZIP entry + 5-day forecast -- same idiom as the other two panels, pinned
   top-left (bottom corners are taken by history/indoor, top-right by the
   theme button). */
.forecast-panel{position:fixed;top:14px;left:14px;z-index:1;border:1px solid var(--theme-main);border-radius:8px;background:rgba(0,0,0,0.25);padding:10px 14px;text-align:left;color:var(--theme-main);font-family:Arial;min-width:200px;}
.forecast-title{font-size:12px;color:var(--theme-dim);letter-spacing:0.08em;margin-bottom:6px;}
.loc-row{display:flex;align-items:center;gap:6px;margin-bottom:6px;}
.coord-input{width:5.2em;background:transparent;color:var(--theme-main);border:1px solid var(--theme-dim);border-radius:4px;font-family:Arial;font-size:13px;padding:2px 5px;}
.coord-input:focus{outline:1px solid var(--theme-main);}
.loc-clear{background:transparent;color:var(--theme-dim);border:1px solid var(--theme-dim);border-radius:4px;font-size:14px;line-height:1;padding:2px 7px;cursor:pointer;font-family:Arial;}
.loc-clear:hover{color:var(--theme-main);border-color:var(--theme-main);}
.elev-row{display:flex;align-items:center;gap:6px;margin-bottom:6px;font-size:11px;color:var(--theme-dim);letter-spacing:0.06em;}
.elev-input{width:4.8em;background:transparent;color:var(--theme-main);border:1px solid var(--theme-dim);border-radius:4px;font-family:Arial;font-size:13px;padding:2px 5px;}
.elev-input:focus{outline:1px solid var(--theme-main);}
/* Dimmed while the value came from the automatic lookup rather than the user,
   so it's obvious at a glance which elevation you're actually looking at. */
.elev-input.auto{color:var(--theme-dim);}
.loc-place{display:block;font-size:11px;color:var(--theme-dim);white-space:nowrap;overflow:hidden;text-overflow:ellipsis;max-width:13em;margin-bottom:6px;}
.forecast-row{font-size:15px;line-height:1.45;white-space:nowrap;}
.forecast-row .lbl{color:var(--theme-dim);font-size:0.75em;margin-right:8px;display:inline-block;min-width:2.6em;}
.forecast-row .sep{color:var(--theme-dim);margin:0 4px;}
.forecast-row .funit{color:var(--theme-dim);font-size:0.75em;margin-left:4px;}
.forecast-row .cond{color:var(--theme-dim);font-size:0.72em;margin-left:8px;}
.forecast-note{font-size:10px;color:#999;margin-top:6px;max-width:15em;white-space:normal;line-height:1.35;}
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
  <div><span class='tz-label'>UTC</span><span id='clockUtc'>--:--</span><span class='grid-sep'></span><span class='tz-label'>↑</span><span id='sunrise'>--:--</span><span class='grid-sep'></span><span class='tz-label'>↓</span><span id='sunset'>--:--</span></div>
  <div>
    <select id='tzSelect' class='tz-select'></select>
    <span id='clockLocal'>--:--</span><span class='grid-sep'></span><span id='localDate' class='tz-label'>--/--/----</span>
  </div>
</div>

<div id='timeStatus' class='status'>connecting...</div>
</div>

<button class='theme-btn' id='themeBtn' aria-label='Change theme color'><span class='swatch'></span></button>
<div class='theme-menu' id='themeMenu'>
  <!-- Swatch colors must track THEMES.<name>.main in the theme script below. -->
  <button data-theme='amber' style='background:#ffb04d' aria-label='Amber'></button>
  <button data-theme='red' style='background:#ff5347' aria-label='Red'></button>
  <button data-theme='green' style='background:#4ade80' aria-label='Green'></button>
  <button data-theme='yellow' style='background:#ffd60a' aria-label='Yellow'></button>
  <button data-theme='blue' style='background:#58b6ff' aria-label='Blue'></button>
  <button data-theme='pink' style='background:#ff5fa2' aria-label='Pink'></button>
</div>

<div class='indoor-panel'>
  <div class='indoor-title'>INDOOR SENSOR</div>
  <div class='indoor-row'><span class='lbl'>HUM</span><span id='inHum'>--</span><span class='iunit'>%</span></div>
  <div class='indoor-row'><span class='lbl'>TEMP</span><span id='inTemp'>--</span><span class='iunit'>F</span></div>
  <div class='indoor-row'><span class='lbl'>PRES</span><span id='inPress'>--</span><span class='iunit'>inHg</span></div>
</div>

<div class='history-panel'>
  <div class='history-title'>7-DAY HIGH / LOW</div>
  <div id='histRows'>
    <div class='history-row'><span class='lbl'>--</span><span class='hi'>--</span><span class='sep'>/</span><span class='lo'>--</span><span class='hunit'>F</span></div>
  </div>
</div>

<div class='forecast-panel'>
  <div class='forecast-title'>5-DAY FORECAST</div>
  <div class='loc-row'>
    <input id='latInput' class='coord-input' type='text' inputmode='decimal' placeholder='lat' aria-label='Latitude'/>
    <input id='lonInput' class='coord-input' type='text' inputmode='decimal' placeholder='lon' aria-label='Longitude'/>
    <button id='locClear' class='loc-clear' title='Clear location' aria-label='Clear location'>&#215;</button>
  </div>
  <div class='elev-row'>
    <span>ELEV</span>
    <input id='elevInput' class='elev-input' type='text' inputmode='decimal' placeholder='auto' aria-label='Elevation in metres'/>
    <span>m</span>
  </div>
  <span id='locPlace' class='loc-place'>enter coordinates</span>
  <div id='fcRows'></div>
  <div id='fcNote' class='forecast-note'></div>
</div>

<script src="https://cdnjs.cloudflare.com/ajax/libs/d3/7.8.5/d3.min.js"></script>
<script src="https://cdnjs.cloudflare.com/ajax/libs/topojson/3.0.2/topojson.min.js"></script>
<script>
// Location pin layer -- defined here (before the map IIFE) so window.initMapPin
// exists when the IIFE calls it during map setup. In the GPS build this was
// driven by a satellite fix; now the forecast module calls setLocationPin
// once the entered ZIP resolves to coordinates.
let _pinLayer = null;
let _projectFn = null;
window.initMapPin = function(layer, projFn) {
  _pinLayer = layer;
  _projectFn = projFn;
};
window.clearLocationPin = function() {
  if (_pinLayer) _pinLayer.selectAll('.loc-pin').remove();
};
window.setLocationPin = function(lat, lon) {
  if (!_pinLayer || !_projectFn) return;
  _pinLayer.selectAll('.loc-pin').remove();
  const pt = _projectFn([lon, lat]);
  if (!pt) return;
  _pinLayer.append('circle')
    .attr('class', 'loc-pin loc-pin-glow')
    .attr('cx', pt[0]).attr('cy', pt[1])
    .attr('r', 12)
    .attr('fill', 'none')
    .attr('stroke', getComputedStyle(document.documentElement).getPropertyValue('--theme-main').trim())
    .attr('stroke-width', 1.5)
    .attr('opacity', 0.4);
  _pinLayer.append('circle')
    .attr('class', 'loc-pin loc-pin-dot')
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
  // Initial values only -- setMapThemeColor() overwrites both as soon as the
  // saved theme is applied. Alphas raised (0.15 -> 0.30, 0.40 -> 0.65) so the
  // theme color actually registers on the map instead of being a hint.
  let strokeColor = 'rgba(255,176,77,0.30)';
  let terminatorColor = 'rgba(255,176,77,0.65)';

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
    strokeColor = `rgba(${r},${g},${b},0.30)`;
    terminatorColor = `rgba(${r},${g},${b},0.65)`;
    landLayer.selectAll('path').attr('stroke', strokeColor);
    nightLayer.selectAll('path.terminator-line').attr('stroke', terminatorColor);
    // Repaint the location pin with the new theme color if one is showing
    if (_pinLayer) {
      _pinLayer.selectAll('.loc-pin-glow').attr('stroke', hexColor);
      _pinLayer.selectAll('.loc-pin-dot').attr('fill', hexColor);
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

// ---- Location-derived display functions ----
// Everything here used to be fed by a GPS fix. It is now fed by the ZIP code
// the user types into the forecast panel, which the last script block on this
// page resolves into coordinates, elevation and a timezone.

// Station location: { lat, lon, elevM, tz, name }. Null until a ZIP resolves,
// which is the normal state on a first visit -- every consumer below has to
// tolerate it.
let _loc = null;

// Most recent WebSocket payload. Kept so the readouts can be re-rendered the
// moment a location arrives, instead of showing uncorrected pressure until
// the next push up to 30s later.
let _lastWs = null;

// Track whether the timezone was set automatically (from the ZIP) or chosen
// by the user. A user's explicit choice wins and is never overwritten.
let tzSetByUser = false;

// Sea-level pressure reduction -- ICAO standard barometric formula:
//   SLP = P_station * (1 + (0.0065 * alt_m) / T_K) ^ 5.2561
// This used to run on the ESP32 against GPS altitude. The altitude now comes
// from the ZIP's elevation, which only the browser knows, so the firmware
// sends RAW station pressure and the reduction happens here. Do not re-add it
// to the firmware without deleting it here, or it applies twice.
//
// With no location yet, alt = 0 makes the factor exactly 1, so station
// pressure passes through untouched -- the honest fallback, and the same
// behaviour the firmware had before its first GPS fix.
//
// Uses the OUTDOOR temperature: the air column being modelled is the one
// outside, between the station and sea level. If the MCP9808 is down the
// firmware reports 0 F and this skews -- but a dead temperature sensor is
// already obvious on the page, and at low elevations the error is a fraction
// of a hPa.
function toSeaLevel(stationHPa, outdoorTempF) {
  if (!_loc || !isFinite(_loc.elevM) || _loc.elevM === 0) return stationHPa;
  const tK = (outdoorTempF - 32) * 5 / 9 + 273.15;
  if (!isFinite(tK) || tK <= 0) return stationHPa;
  return stationHPa * Math.pow(1 + (0.0065 * _loc.elevM) / tK, 5.2561);
}

// NOAA sunrise/sunset algorithm -- same solar position math used for the
// day/night terminator, applied here to find the exact rise/set times for
// the ZIP's coordinates on today's date.
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

function updateSunriseSunset() {
  const srEl = document.getElementById('sunrise');
  const ssEl = document.getElementById('sunset');
  if (!_loc) { srEl.textContent = '--:--'; ssEl.textContent = '--:--'; return; }
  // Today's UTC date. The NOAA algorithm works in UTC and returns UTC
  // instants, which formatLocalTime() then renders in the selected zone --
  // same convention the GPS build used with the fix's own UTC date.
  const now = new Date();
  const sun = calcSunriseSunset(_loc.lat, _loc.lon,
                                now.getUTCFullYear(), now.getUTCMonth() + 1, now.getUTCDate());
  if (sun) {
    srEl.textContent = formatLocalTime(sun.sunrise);
    ssEl.textContent = formatLocalTime(sun.sunset);
  } else {
    srEl.textContent = 'N/A'; // polar day or polar night
    ssEl.textContent = 'N/A';
  }
}

// Date straight off the browser clock, rendered in the selected zone. The GPS
// build reconstructed this from the fix's date fields; with no other date
// source there is no reason not to use the browser's.
function updateDate() {
  document.getElementById('localDate').textContent = new Intl.DateTimeFormat('en-US', {
    timeZone: document.getElementById('tzSelect').value || 'America/Los_Angeles',
    month: '2-digit', day: '2-digit', year: 'numeric'
  }).format(new Date());
}

// Called by the forecast module once a ZIP resolves. Mirrors the
// window.initMapPin / window.setLocationPin handoff used between script
// blocks elsewhere on this page.
window.setStationLocation = function(loc) {
  _loc = loc;
  if (window.setLocationPin) window.setLocationPin(loc.lat, loc.lon);
  if (loc.tz && !tzSetByUser) selectTimezone(loc.tz, loc.name);
  updateSunriseSunset();
  updateDate();
  // Re-render immediately so pressure picks up the elevation correction
  // instead of waiting for the next 30s push.
  if (_lastWs) renderReadings(_lastWs);
};

// Called when the user clears the location. Everything that keyed off it goes
// back to its no-location state: the pin disappears, sunrise/sunset blank, and
// pressure reverts to raw station pressure via toSeaLevel()'s !_loc guard.
// The timezone selection is deliberately left alone -- it's a display
// preference the user may have set by hand, and silently resetting it would
// be surprising.
window.clearStationLocation = function() {
  _loc = null;
  if (window.clearLocationPin) window.clearLocationPin();
  updateSunriseSunset();
  if (_lastWs) renderReadings(_lastWs);
};

// ---- WebSocket handler ----
// Split out of the socket callback so setStationLocation() can re-run it the
// moment a ZIP resolves -- both pressure readouts depend on the elevation.
function renderReadings(data) {
  render('hum', data.h);
  render('temp', data.t);
  // data.p is RAW station pressure from the BME280; reduce it here.
  render('press', hPaToInHg(toSeaLevel(data.p, data.t)), 2);

  const statusEl = document.getElementById('timeStatus');
  if (data.sync) {
    statusEl.textContent = 'clock synced';
    statusEl.className = 'status synced';
  } else {
    statusEl.textContent = 'clock NOT synced (relative time only)';
    statusEl.className = 'status unsynced';
  }

  // Indoor satellite panel. "ok" is false before the first report and
  // whenever the node has gone quiet for 3+ minutes -- show "--" then rather
  // than silently displaying stale data. Its pressure gets the same
  // reduction, deliberately using the OUTDOOR temperature: the air column
  // between the station and sea level is outside, and the indoor thermostat
  // says nothing about it.
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

// Point the clock at an arbitrary IANA zone, adding it to the dropdown if it
// isn't one of the seven standard entries above. This matters: AccuWeather
// returns the precise zone for a ZIP, which is often outside that list
// (America/Detroit, America/Boise, America/Indiana/Indianapolis, ...).
// Intl accepts any IANA zone, so the rest of the clock machinery is unchanged.
// Deliberately does NOT persist the choice -- saving it would make the page
// treat it as a user selection on the next load and stop honouring the ZIP.
function selectTimezone(iana, label) {
  const sel = document.getElementById('tzSelect');
  // Drop any option left over from a previously entered ZIP.
  Array.from(sel.options).forEach(o => { if (o.dataset.fromZip === '1') o.remove(); });
  if (!Array.from(sel.options).some(o => o.value === iana)) {
    const opt = document.createElement('option');
    opt.value = iana;
    opt.textContent = label || iana.split('/').pop().replace(/_/g, ' ');
    opt.dataset.fromZip = '1';
    sel.insertBefore(opt, sel.firstChild);
  }
  sel.value = iana;
  buildLocalFormatter(iana);
  tickClock();
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
  // The date now comes from this same browser clock rather than a GPS fix, so
  // refresh it on the tick -- otherwise it would stay on yesterday's date
  // until the next ZIP resolve or timezone change.
  updateDate();
}
tickClock();
setInterval(tickClock, 5000); // 5s is plenty now that only h:m is shown

// Theme color picker. Each entry: main (lit segments/text), glow (outer
// box-shadow, a deeper shade of main), dim (muted variant for the small
// UTC/timezone-dim labels). Persisted in localStorage so it survives reloads.
// Theme colors. Three roles per theme:
//   main - lit segments and primary text
//   glow - the bloom halo behind lit elements. NOTE this is now a *brighter,
//          more saturated* companion to main, not a darker shade. The old
//          darker glow muddied the halo instead of spreading it.
//   dim  - small labels (UTC/GRID/unit captions, panel titles)
//
// Everything here is tuned for a pure black background, where perceived
// brightness tracks relative luminance. Yellow was the reference point: at
// ~0.69 it was roughly 3x the old blue (~0.24), which is exactly why it was
// the only one that read as "popping". The others were raised toward it as
// far as their hue allows -- red and pink physically cannot reach yellow's
// luminance without turning into salmon and bubblegum, so they lean on the
// wider bloom below instead. Yellow's `main` is deliberately unchanged.
const THEMES = {
  amber:  { main: '#ffb04d', glow: '#ff8c00', dim: '#e8a962' },
  red:    { main: '#ff5347', glow: '#ff2a1c', dim: '#f08279' },
  green:  { main: '#4ade80', glow: '#22c55e', dim: '#7eeaa8' },
  yellow: { main: '#ffd60a', glow: '#ffc400', dim: '#edcb52' },
  blue:   { main: '#58b6ff', glow: '#0a84ff', dim: '#8ecdff' },
  pink:   { main: '#ff5fa2', glow: '#ff1e73', dim: '#ff96c2' }
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
  // Repaint the seven-segment digits. Already-painted .seg elements do not
  // reliably pick up a --theme-main change on their own (see the .seg
  // transition comment in the stylesheet), so without this the readouts keep
  // the previous theme's colour until the next WebSocket push rebuilds them
  // -- up to 30s of the picker apparently doing nothing. Guarded because
  // applyTheme() also runs at load, before any reading has arrived.
  if (typeof _lastWs !== 'undefined' && _lastWs) renderReadings(_lastWs);
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
<script>
// 7-day high/low panel (bottom-left). Pulls the temperature history from
// /history on the base station, groups samples into local calendar days using
// the currently selected timezone, and shows each of the last 7 days' high and
// low. Days with no data show "--". Note: /history stores 15-minute means, so
// these are the warmest/coolest 15-min average, not true instantaneous extremes.
(function(){
  const HIST_REFRESH_MS = 5 * 60 * 1000; // /history only changes every 15 min; 5 min is plenty

  function selectedTz(){
    const sel = document.getElementById('tzSelect');
    return (sel && sel.value) || 'America/Los_Angeles';
  }

  // Stable YYYY-MM-DD key for an epoch (seconds) in the given IANA zone.
  function dayKey(epochSec, tz){
    return new Intl.DateTimeFormat('en-CA', {
      timeZone: tz, year:'numeric', month:'2-digit', day:'2-digit'
    }).format(new Date(epochSec * 1000));
  }
  // Short weekday label for a day key (built at local noon to dodge tz edges).
  function dayLabel(key, tz){
    const p = key.split('-').map(Number);
    const dt = new Date(Date.UTC(p[0], p[1]-1, p[2], 12));
    return new Intl.DateTimeFormat('en-US', { timeZone: tz, weekday:'short' }).format(dt);
  }

  function renderHistory(payload){
    const tz = selectedTz();
    const byDay = {};
    const samples = (payload && payload.samples) ? payload.samples : [];
    samples.forEach(function(s){
      const k = dayKey(s[0], tz), v = s[1];
      if (!byDay[k]) byDay[k] = { hi: v, lo: v };
      else { if (v > byDay[k].hi) byDay[k].hi = v; if (v < byDay[k].lo) byDay[k].lo = v; }
    });

    // Last 7 local days, oldest first.
    const nowSec = Math.floor(Date.now() / 1000);
    let html = '';
    for (let i = 6; i >= 0; i--){
      const k = dayKey(nowSec - i * 86400, tz);
      const rec = byDay[k];
      const hi = rec ? Math.round(rec.hi) : null;
      const lo = rec ? Math.round(rec.lo) : null;
      html += "<div class='history-row'><span class='lbl'>" + dayLabel(k, tz) + "</span>"
            + "<span class='hi'>"  + (hi == null ? '--' : hi) + "</span>"
            + "<span class='sep'>/</span>"
            + "<span class='lo'>"  + (lo == null ? '--' : lo) + "</span>"
            + "<span class='hunit'>F</span></div>";
    }
    document.getElementById('histRows').innerHTML = html;
  }

  function loadHistory(){
    fetch('/history').then(function(r){ return r.json(); })
      .then(renderHistory)
      .catch(function(){ /* device offline or endpoint absent -- keep last render */ });
  }

  loadHistory();
  setInterval(loadHistory, HIST_REFRESH_MS);
  // Re-render on timezone change so day grouping and labels stay consistent.
  const sel = document.getElementById('tzSelect');
  if (sel) sel.addEventListener('change', loadHistory);
})();
</script>
<script>
// ---- Location entry + 5-day forecast (top-left panel) ----
//
// This runs in the BROWSER, not on the ESP32. The firmware has no HTTP client
// and no TLS stack; adding one to a C6 already running AsyncWebServer +
// LittleFS + ESP-NOW + ArduinoJson would cost heap and radio power for no
// benefit, since the device viewing this page already has internet.
//
// The user enters latitude and longitude directly. This replaced a ZIP box:
// a ZIP resolved to AccuWeather's coarse city-level elevation, which fed the
// sea-level pressure correction and made the reported pressure wrong by more
// than the correction was worth. Coordinates give an exact point, and the
// elevation for that point comes from Open-Meteo's terrain model -- keyless
// and outside the AccuWeather quota -- with the field left editable so a bad
// DEM value or an unusual site can be corrected by hand.
//
// TWO SEPARATE SERVICES, deliberately:
//   * Open-Meteo  -> elevation. Free, no key, no quota. Runs FIRST and
//     independently, because pressure depends on it and it must keep working
//     when the forecast doesn't.
//   * AccuWeather -> forecast + timezone + place name, via the geoposition
//     endpoint (not postal codes, so it works anywhere, not just US ZIPs).
//
// QUOTA IS THE DESIGN CONSTRAINT for the AccuWeather half. Free tier is 50
// calls/day for the whole key, shared by every browser that opens this page:
//   * the coordinate -> location lookup is cached FOREVER (location keys are
//     stable), keyed on coordinates rounded to 3 decimals (~100 m) so small
//     edits don't each cost a call;
//   * the forecast is cached for 3 hours, and a reload renders from that
//     cache with ZERO calls.
// That works out to ~8 calls/device/day. For scale: refreshing every 5 minutes
// the way the history panel does would be 288 calls/day from a single tab and
// would exhaust the quota before breakfast. Do not lower FORECAST_TTL_MS.
//
// The free tier also caps the daily forecast at 5 days (10- and 15-day are
// paid), which is why this panel shows 5 and not 7.
(function(){
  const FORECAST_TTL_MS = 3 * 60 * 60 * 1000; // see quota note above
  const LAT_KEY  = 'stationLat';
  const LON_KEY  = 'stationLon';
  const ELEV_KEY = 'stationElevM';    // the value in use (auto or manual)
  const ELEV_MANUAL_KEY = 'stationElevManual'; // '1' = user typed it; don't overwrite
  const AW = 'https://dataservice.accuweather.com';
  const OPEN_METEO_ELEV = 'https://api.open-meteo.com/v1/elevation';

  // Cache keys are rounded to 3 decimals (~100 m). Without that, nudging a
  // coordinate by a metre would look like a brand new location and burn an
  // AccuWeather call from a 50/day budget every time.
  function coordKey(lat, lon){ return lat.toFixed(3) + ',' + lon.toFixed(3); }
  function locKeyFor(ck){ return 'awLoc:' + ck; }
  function fcKeyFor(ck){ return 'awFc:' + ck; }
  function elevKeyFor(ck){ return 'elev:' + ck; }

  const latInput  = document.getElementById('latInput');
  const lonInput  = document.getElementById('lonInput');
  const elevInput = document.getElementById('elevInput');
  const locClear  = document.getElementById('locClear');
  const locPlace  = document.getElementById('locPlace');
  const fcRows    = document.getElementById('fcRows');
  const fcNote    = document.getElementById('fcNote');

  let apiKey = null;
  let busy = false;

  // localStorage can throw in some privacy modes -- same defensive pattern the
  // theme and timezone code already uses.
  function lsGet(k){ try { return localStorage.getItem(k); } catch(e){ return null; } }
  function lsSet(k,v){ try { localStorage.setItem(k,v); } catch(e){} }
  function readJson(k){
    const raw = lsGet(k);
    if (!raw) return null;
    try { return JSON.parse(raw); } catch(e){ return null; }
  }
  function note(msg){ fcNote.textContent = msg || ''; }

  // IconPhrase is third-party text going into innerHTML -- escape it.
  function escapeHtml(s){
    return String(s == null ? '' : s).replace(/[&<>"']/g, function(c){
      return { '&':'&amp;', '<':'&lt;', '>':'&gt;', '"':'&quot;', "'":'&#39;' }[c];
    });
  }

  // The key lives in secrets.h and is handed over by the auth-gated /config
  // endpoint, so it never enters this committed file. Fetched once per load.
  function getApiKey(){
    if (apiKey) return Promise.resolve(apiKey);
    return fetch('/config')
      .then(function(r){ return r.json(); })
      .then(function(c){ apiKey = c && c.awkey; return apiKey; });
  }

  function checkStatus(r){
    if (r.status === 401 || r.status === 403) throw new Error('bad-key');
    if (r.status === 503) throw new Error('quota');
    if (!r.ok) throw new Error('http-' + r.status);
    return r.json();
  }

  // Coordinates -> { key, name, tz }, cached permanently. AccuWeather location
  // keys are stable, so re-fetching one for the same point is a wasted call.
  // Uses the geoposition endpoint rather than the postal-code one: the user
  // now supplies a point directly, and this accepts any point on Earth rather
  // than just US ZIPs.
  //
  // NOTE: lat/lon and elevation deliberately do NOT come from this response.
  // The user's own coordinates are more precise than the city AccuWeather
  // snaps to, and its elevation is a coarse value for that city -- which is
  // exactly the inaccuracy this whole change exists to fix. Only the location
  // key (for the forecast), the display name, and the timezone are taken.
  function resolveLocation(lat, lon){
    const ck = coordKey(lat, lon);
    const cached = readJson(locKeyFor(ck));
    if (cached && cached.key) return Promise.resolve(cached);
    return getApiKey().then(function(k){
      if (!k) throw new Error('no-key');
      return fetch(AW + '/locations/v1/cities/geoposition/search?apikey=' + encodeURIComponent(k)
                   + '&q=' + encodeURIComponent(lat + ',' + lon)).then(checkStatus);
    }).then(function(L){
      if (!L || !L.Key) throw new Error('no-location');
      const loc = {
        key:  L.Key,
        name: (L.LocalizedName || '') +
              (L.AdministrativeArea && L.AdministrativeArea.ID ? ', ' + L.AdministrativeArea.ID : ''),
        tz:   (L.TimeZone && L.TimeZone.Name) || null
      };
      lsSet(locKeyFor(ck), JSON.stringify(loc));
      return loc;
    });
  }

  // Elevation for a point, from Open-Meteo's terrain model. Keyless, no quota,
  // and CORS-enabled, so it costs nothing against the AccuWeather budget.
  //
  // It's a ~90 m digital elevation model, so it's the terrain height at the
  // point, not your antenna mast or your floor of the building -- spot checks:
  // Denver 1599 (true ~1609), Seattle 59. Good to a few metres at a normal
  // site, which is well under a hPa of pressure error. Sharp peaks get
  // smoothed and read low. That's precisely why the field stays editable.
  //
  // Cached permanently per rounded coordinate: terrain does not move.
  function lookupElevation(lat, lon){
    const ck = coordKey(lat, lon);
    const cached = lsGet(elevKeyFor(ck));
    if (cached !== null && cached !== '') return Promise.resolve(parseFloat(cached));
    return fetch(OPEN_METEO_ELEV + '?latitude=' + encodeURIComponent(lat)
                 + '&longitude=' + encodeURIComponent(lon))
      .then(function(r){ if (!r.ok) throw new Error('elev-http-' + r.status); return r.json(); })
      .then(function(j){
        const v = (j && j.elevation && j.elevation.length) ? j.elevation[0] : null;
        if (typeof v !== 'number') throw new Error('elev-bad-response');
        lsSet(elevKeyFor(ck), String(v));
        return v;
      });
  }

  // metric=false so temperatures arrive in F and match the rest of the page.
  function loadForecast(ck, locationKey){
    const cached = readJson(fcKeyFor(ck));
    if (cached && cached.t && (Date.now() - cached.t) < FORECAST_TTL_MS) {
      return Promise.resolve(cached.days); // cache hit -- no API call
    }
    return getApiKey().then(function(k){
      if (!k) throw new Error('no-key');
      return fetch(AW + '/forecasts/v1/daily/5day/' + encodeURIComponent(locationKey)
                   + '?apikey=' + encodeURIComponent(k) + '&metric=false').then(checkStatus);
    }).then(function(j){
      const days = (j.DailyForecasts || []).map(function(d){
        return {
          epoch: d.EpochDate,
          hi: (d.Temperature && d.Temperature.Maximum) ? d.Temperature.Maximum.Value : null,
          lo: (d.Temperature && d.Temperature.Minimum) ? d.Temperature.Minimum.Value : null,
          cond: (d.Day && d.Day.IconPhrase) || ''
        };
      });
      lsSet(fcKeyFor(ck), JSON.stringify({ t: Date.now(), days: days }));
      return days;
    }).catch(function(err){
      // Show stale data rather than blanking the panel on a transient failure.
      if (cached && cached.days) { note('forecast stale (' + err.message + ')'); return cached.days; }
      throw err;
    });
  }

  function renderForecast(days){
    const sel = document.getElementById('tzSelect');
    const tz = (sel && sel.value) || 'America/Los_Angeles';
    let html = '';
    days.slice(0, 5).forEach(function(d){
      const label = new Intl.DateTimeFormat('en-US', { timeZone: tz, weekday: 'short' })
                      .format(new Date(d.epoch * 1000));
      html += "<div class='forecast-row'><span class='lbl'>" + label + "</span>"
            + "<span class='hi'>" + (d.hi == null ? '--' : Math.round(d.hi)) + "</span>"
            + "<span class='sep'>/</span>"
            + "<span class='lo'>" + (d.lo == null ? '--' : Math.round(d.lo)) + "</span>"
            + "<span class='funit'>F</span>"
            + "<span class='cond'>" + escapeHtml(d.cond) + "</span></div>";
    });
    fcRows.innerHTML = html;
  }

  const MESSAGES = {
    'no-key':      'no API key -- set ACCUWEATHER_API_KEY in secrets.h',
    'bad-key':     'API key rejected -- check ACCUWEATHER_API_KEY',
    'quota':       'AccuWeather daily quota used up -- resets tomorrow',
    'no-location': 'no forecast location near those coordinates'
  };

  // Stored coordinates, or null if none/invalid. Validated on read as well as
  // on write, so a hand-edited localStorage can't feed NaN into the pressure
  // maths or the sunrise solver.
  function storedCoords(){
    const lat = parseFloat(lsGet(LAT_KEY));
    const lon = parseFloat(lsGet(LON_KEY));
    if (!isFinite(lat) || !isFinite(lon)) return null;
    if (lat < -90 || lat > 90 || lon < -180 || lon > 180) return null;
    return { lat: lat, lon: lon };
  }

  function storedElev(){
    const e = parseFloat(lsGet(ELEV_KEY));
    return isFinite(e) ? e : 0;
  }
  function elevIsManual(){ return lsGet(ELEV_MANUAL_KEY) === '1'; }

  // Push the current location out to the rest of the page: map pin,
  // sunrise/sunset, clock timezone and the sea-level pressure correction all
  // key off this. Called on every change, including elevation-only edits.
  function publish(tz, name){
    const c = storedCoords();
    if (!c || !window.setStationLocation) return;
    window.setStationLocation({
      lat: c.lat, lon: c.lon, elevM: storedElev(),
      tz: tz || null, name: name || null
    });
  }

  function showElev(v, manual){
    elevInput.value = (v === null || v === undefined || !isFinite(v)) ? '' : String(Math.round(v));
    elevInput.classList.toggle('auto', !manual);
  }

  function refresh(){
    const c = storedCoords();
    if (!c) {
      locPlace.textContent = 'enter coordinates';
      fcRows.innerHTML = '';
      note('');
      return;
    }
    if (busy) return;
    busy = true;
    note('');

    // Elevation first and independently of AccuWeather: it's the thing the
    // pressure reading depends on, it's keyless, and it must still work if
    // the forecast is unavailable (bad key, quota, CORS, no internet).
    const elevStep = elevIsManual()
      ? Promise.resolve(storedElev())
      : lookupElevation(c.lat, c.lon).then(function(v){
          lsSet(ELEV_KEY, String(v));
          showElev(v, false);
          return v;
        }).catch(function(){
          note('elevation lookup failed -- type it in metres');
          return storedElev();
        });

    elevStep.then(function(){
      publish(null, null);              // pressure + pin correct even if the rest fails
      return resolveLocation(c.lat, c.lon);
    }).then(function(loc){
      locPlace.textContent = loc.name || (c.lat.toFixed(3) + ', ' + c.lon.toFixed(3));
      publish(loc.tz, loc.name);        // re-publish with the timezone
      return loadForecast(coordKey(c.lat, c.lon), loc.key);
    }).then(function(days){
      renderForecast(days);
    }).catch(function(err){
      if (MESSAGES[err.message]) {
        note(MESSAGES[err.message]);
      } else if (err instanceof TypeError) {
        // fetch() rejects with TypeError when the browser blocks the response.
        // For a cross-origin API that means CORS, which is worth naming: it
        // cannot be fixed from this page.
        note('forecast request blocked (CORS or offline)');
      } else {
        note('forecast unavailable (' + err.message + ')');
      }
      // The readings stay correct regardless -- publish() already ran.
      locPlace.textContent = c.lat.toFixed(3) + ', ' + c.lon.toFixed(3);
    }).then(function(){ busy = false; });
  }

  // --- wiring ---
  (function initFields(){
    const c = storedCoords();
    if (c) { latInput.value = c.lat; lonInput.value = c.lon; }
    const e = parseFloat(lsGet(ELEV_KEY));
    if (isFinite(e)) showElev(e, elevIsManual());
  })();

  function commitCoords(){
    const lat = parseFloat(latInput.value.trim());
    const lon = parseFloat(lonInput.value.trim());
    if (!isFinite(lat) || !isFinite(lon)) { note('latitude and longitude must be numbers'); return; }
    if (lat < -90 || lat > 90)   { note('latitude must be between -90 and 90'); return; }
    if (lon < -180 || lon > 180) { note('longitude must be between -180 and 180'); return; }
    const prev = storedCoords();
    if (prev && prev.lat === lat && prev.lon === lon) return; // no-op, saves a call
    lsSet(LAT_KEY, String(lat));
    lsSet(LON_KEY, String(lon));
    // New point invalidates a manual elevation -- it was for somewhere else.
    lsSet(ELEV_MANUAL_KEY, '0');
    showElev(null, false);
    fcRows.innerHTML = '';
    refresh();
  }

  function commitElev(){
    const raw = elevInput.value.trim();
    if (raw === '') {                     // blank -> revert to the automatic value
      lsSet(ELEV_MANUAL_KEY, '0');
      refresh();
      return;
    }
    const v = parseFloat(raw);
    if (!isFinite(v))            { note('elevation must be a number, in metres'); return; }
    if (v < -500 || v > 9000)    { note('elevation must be between -500 and 9000 m'); return; }
    lsSet(ELEV_KEY, String(v));
    lsSet(ELEV_MANUAL_KEY, '1');
    showElev(v, true);
    note('');
    publish(null, null);                  // re-render pressure immediately
  }

  [latInput, lonInput].forEach(function(el){
    el.addEventListener('change', commitCoords);
    el.addEventListener('keydown', function(e){ if (e.key === 'Enter') commitCoords(); });
  });
  elevInput.addEventListener('change', commitElev);
  elevInput.addEventListener('keydown', function(e){ if (e.key === 'Enter') commitElev(); });

  locClear.addEventListener('click', function(){
    [LAT_KEY, LON_KEY, ELEV_KEY, ELEV_MANUAL_KEY].forEach(function(k){
      try { localStorage.removeItem(k); } catch(e){}
    });
    latInput.value = ''; lonInput.value = ''; showElev(null, false);
    locPlace.textContent = 'enter coordinates';
    fcRows.innerHTML = '';
    note('');
    // Deliberately leaves the awLoc:/awFc:/elev: caches alone: they're keyed
    // by coordinate, so re-entering the same point costs zero API calls.
    if (window.clearStationLocation) window.clearStationLocation();
  });

  // Re-label the day columns if the user switches timezone (no API call).
  const tzSel = document.getElementById('tzSelect');
  if (tzSel) tzSel.addEventListener('change', function(){
    const c = storedCoords();
    const cache = c ? readJson(fcKeyFor(coordKey(c.lat, c.lon))) : null;
    if (cache && cache.days) renderForecast(cache.days);
  });

  refresh();
  // Re-check on the cache TTL. Every tick before expiry is a cache hit, so
  // this costs one call per 3 hours, not one per tick.
  setInterval(refresh, FORECAST_TTL_MS);
})();
</script>

</body></html>
)rawliteral";
