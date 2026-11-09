#include "web_app.h"
#include "kiln_features.h"
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include <time.h>
#include <sys/time.h>
#include <esp_system.h>
#include <stdio.h>
#include <string.h>

namespace {
using namespace kiln::features;

WebServer server(80);
Preferences preferences;
ControllerLink* controller = nullptr;
TelemetryBuffer telemetry;
FiringHistory firingHistory;
ScheduleQueue scheduleQueue;
AlertLedger alertLedger;
EnergyCostTracker energyTracker;
KilnSimulator simulator;

char apName[32] = {};
char apPassword[20] = {};
char csrfToken[12] = {};
bool preferencesReady = false;
bool runActive = false;
uint32_t runStartMs = 0;
uint32_t runStartEpoch = 0;
uint32_t runId = 1;
uint32_t lastSampleMs = 0;
uint32_t simulationOutputMask = 0;
struct AlertPreferences {
  bool canLossEnabled;
  bool staleStatusEnabled;
  bool controllerFaultEnabled;
  bool sensorInvalidEnabled;
  bool temperatureWarningEnabled;
  int16_t temperatureWarningDeciC;
};
AlertPreferences alertPreferences = {true, true, true, true, false, 0};
int16_t peakTemperatures[4] = {0, 0, 0, 0};
EnergyConfig energyConfig = {{0, 0, 0, 0}, 0};
static constexpr uint8_t MAX_WEB_PROFILE_SEGMENTS = 32;

const char INDEX_HTML[] PROGMEM = R"HTML(
<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Kiln Controller</title>
<style>
:root{color-scheme:dark;--bg:#10151b;--panel:#1b242e;--line:#344250;--text:#edf3f7;--muted:#9eacb8;--accent:#39c4c8;--good:#49c78b;--warn:#e5b04e;--bad:#e46b6b}
*{box-sizing:border-box}body{margin:0;background:var(--bg);color:var(--text);font:15px system-ui,Segoe UI,sans-serif}
header{padding:16px 20px;border-bottom:1px solid var(--line);display:flex;justify-content:space-between;align-items:center;gap:12px;flex-wrap:wrap}
h1{font-size:20px;margin:0}small,.muted{color:var(--muted)}main{max-width:1250px;margin:auto;padding:18px}
nav{display:flex;gap:7px;flex-wrap:wrap;margin-bottom:15px}
button,.tab{background:var(--panel);border:1px solid var(--line);color:var(--text);padding:9px 13px;border-radius:8px;cursor:pointer}
button:hover,.tab.active{border-color:var(--accent);color:var(--accent)}
button.primary{background:#155e63;border-color:#258f94}button.danger{background:#702c35;border-color:#a44a54}
button:disabled{opacity:.35;cursor:not-allowed}
.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(220px,1fr));gap:12px}
.card{background:var(--panel);border:1px solid var(--line);border-radius:12px;padding:14px;margin-bottom:12px}
.kicker{color:var(--muted);font-size:12px;text-transform:uppercase;letter-spacing:.08em}
.value{font-size:28px;font-weight:650;margin-top:5px}.unit{font-size:14px;color:var(--muted)}
.status{display:inline-block;padding:5px 9px;border-radius:99px;background:#283440;color:var(--muted);font-size:12px}
.status.good{color:var(--good)}.status.warn{color:var(--warn)}.status.bad{color:var(--bad)}
.actions{display:flex;gap:8px;flex-wrap:wrap;margin-top:12px}
canvas{width:100%;height:300px;background:#121a22;border-radius:8px}
section{display:none}section.active{display:block}
table{border-collapse:collapse;width:100%;font-size:13px}td,th{text-align:left;padding:9px;border-bottom:1px solid var(--line)}
input,select,textarea{background:#111a23;color:var(--text);border:1px solid var(--line);border-radius:7px;padding:9px;max-width:100%;width:100%}
label{display:block;color:var(--muted);font-size:12px;margin:7px 0 4px}
.formgrid{display:grid;grid-template-columns:repeat(auto-fit,minmax(170px,1fr));gap:10px}
.notice{padding:10px;border-left:3px solid var(--warn);background:#342b1d;border-radius:5px;margin:10px 0}
hr{border:0;border-top:1px solid var(--line);margin:15px 0}
@media(max-width:600px){main{padding:10px}header{padding:12px}.value{font-size:23px}canvas{height:240px}}
</style>
</head>
<body>
<header><h1>CYD Kiln Controller</h1><div><span id="connection" class="status warn">Connecting…</span> <span id="state" class="status">UNKNOWN</span></div></header>
<main>
<nav>
<button class="tab active" data-page="dashboard">Dashboard</button>
<button class="tab" data-page="profiles">Profiles</button>
<button class="tab" data-page="history">History</button>
<button class="tab" data-page="alerts">Alerts</button>
<button class="tab" data-page="schedules">Schedules</button>
<button class="tab" data-page="cost">Energy</button>
<button class="tab" data-page="simulation">Simulation</button>
<button class="tab" data-page="tuning">PID diagnostics</button>
</nav>
<section id="dashboard" class="active">
<div id="simulationNotice" class="notice" hidden>SIMULATION PREVIEW ONLY — these values are not live kiln readings and cannot control heater outputs.</div>
<div class="grid" id="sensors"></div>
<div class="card"><div class="kicker">Temperature history</div><canvas id="chart" width="1100" height="300"></canvas><small>Invalid or stale sensor readings are omitted from the graph. Time axis is elapsed controller time.</small></div>
<div class="card"><div class="kicker">Controller and output diagnostics</div><div id="diagnostics" class="grid"></div>
<div class="actions">
<button id="start" class="primary" onclick="command('start')">Start</button>
<button onclick="command('pause')">Pause</button><button onclick="command('resume')">Resume</button>
<button class="danger" onclick="command('stop')">Stop</button>
</div><p class="muted">Commands are sent to the ESP32-S3 state machine. The browser cannot write heater GPIOs.</p></div>
</section>
<section id="profiles">
<div class="card"><h2>Ramp/soak profile upload</h2>
<p class="muted">Enter segments as JSON. The S3 validates the profile and remains the control authority.</p>
<textarea id="profileJson" rows="8">[{"targetC":200,"rampCPerHour":60,"holdSeconds":0},{"targetC":600,"rampCPerHour":100,"holdSeconds":1800}]</textarea>
<div class="actions"><button class="primary" onclick="uploadProfile()">Validate and send profile</button></div><div id="profileResult" class="muted"></div></div>
</section>
<section id="history"><div class="card"><h2>Firing history</h2><div id="historyTable"></div><div class="actions"><button onclick="loadHistory()">Refresh history</button><button onclick="location.href='/api/v1/history.csv'">Export CSV</button><button onclick="location.href='/api/v1/telemetry.csv'">Export recent telemetry CSV</button></div></div></section>
<section id="alerts"><div class="card"><h2>Alert rules</h2><p class="muted">These settings affect dashboard warnings only. They cannot disable or change the S3 safety limits or hardwired protection.</p><div class="formgrid">
<div><label><input id="alertCan" type="checkbox" checked> CAN link loss</label></div>
<div><label><input id="alertStale" type="checkbox" checked> Stale controller status</label></div>
<div><label><input id="alertFault" type="checkbox" checked> Controller fault</label></div>
<div><label><input id="alertSensor" type="checkbox" checked> Missing/invalid enabled sensor</label></div>
<div><label><input id="alertTemp" type="checkbox"> Temperature warning enabled</label></div>
<div><label>Temperature warning threshold °C</label><input id="alertTempLimit" type="number" min="0" max="1800" value="0"></div>
</div><div class="actions"><button class="primary" onclick="saveAlerts()">Save alert preferences</button></div></div><div class="card"><h2>Alerts and events</h2><div id="alertsTable"></div><button onclick="loadAlerts()">Refresh alerts</button></div></section>
<section id="schedules"><div class="card"><h2>Scheduled firing reminders</h2>
<div class="notice">Schedules notify you when a firing is due. They never start a kiln automatically, including after a reboot.</div>
<div class="formgrid"><div><label>Profile ID</label><input id="scheduleProfile" value="local-profile"></div><div><label>Start date/time (controller local time)</label><input id="scheduleTime" type="datetime-local"></div></div>
<div class="actions"><button class="primary" onclick="addSchedule()">Add reminder</button><button onclick="loadSchedules()">Refresh</button></div><div id="scheduleTable"></div></div></section>
<section id="cost"><div class="card"><h2>Estimated energy and cost</h2><p class="muted">Estimate uses operator-entered heater wattage and the recorded commanded output state. It is not a certified energy meter and does not measure actual current.</p>
<div class="formgrid">
<div><label>Zone 1 watts</label><input id="watts0" type="number" min="0" value="0"></div><div><label>Zone 2 watts</label><input id="watts1" type="number" min="0" value="0"></div><div><label>Zone 3 watts</label><input id="watts2" type="number" min="0" value="0"></div><div><label>Zone 4 watts</label><input id="watts3" type="number" min="0" value="0"></div><div><label>Energy price (minor units / kWh, e.g. cents)</label><input id="rate" type="number" min="0" value="0"></div></div>
<div class="actions"><button class="primary" onclick="saveEnergy()">Save energy settings</button></div><div id="energySummary" class="value">Not configured</div></div></section>
<section id="simulation"><div class="card"><h2>Isolated kiln simulation</h2>
<div class="notice">Simulation is a display/preview model only. It does not replace S3 sensor data, exercise heater relays, or change the master-power permit.</div>
<div class="formgrid"><div><label>Ambient °C</label><input id="simAmbient" type="number" value="20"></div><div><label>Initial °C</label><input id="simInitial" type="number" value="20"></div><div><label>Max simulated heat rise °C/hour</label><input id="simHeat" type="number" value="100"></div><div><label>Cooling °C/hour</label><input id="simCool" type="number" value="20"></div></div>
<div class="actions"><button class="primary" onclick="startSimulation()">Start preview</button><button class="danger" onclick="stopSimulation()">Stop preview</button><button onclick="simStep(0)">Cool all zones</button><button onclick="simStep(15)">Preview all zones heating</button></div>
<div id="simResult" class="muted"></div></div></section>
<section id="tuning"><div class="card"><h2>PID terms and offline tuning advisor</h2>
<p class="muted">The current S3 zone controller uses hysteresis, not PID. This page calculates diagnostic PID terms and an offline suggestion from an already-recorded step response. It does not change active gains or turn on heaters.</p>
<div class="formgrid"><div><label>Kp</label><input id="diagKp" type="number" value="1.0"></div><div><label>Ki</label><input id="diagKi" type="number" value="0.01"></div><div><label>Kd</label><input id="diagKd" type="number" value="0"></div><div><label>Setpoint °C</label><input id="diagSetpoint" type="number" value="200"></div><div><label>Measured °C</label><input id="diagMeasured" type="number" value="190"></div><div><label>Sample interval seconds</label><input id="diagDt" type="number" value="1"></div></div>
<div class="actions"><button onclick="pidTerms()">Calculate P / I / D example</button></div><div id="pidTermsResult" class="muted"></div><hr>
<h3>Recorded step-response analysis</h3>
<div class="formgrid"><div><label>Output before (%)</label><input id="stepU0" type="number" value="20"></div><div><label>Output after (%)</label><input id="stepU1" type="number" value="40"></div><div><label>Temperature before °C</label><input id="stepT0" type="number" value="100"></div><div><label>Temperature after °C</label><input id="stepT1" type="number" value="150"></div><div><label>Dead time seconds</label><input id="stepL" type="number" value="30"></div><div><label>Time to 63% response seconds</label><input id="stepTau" type="number" value="330"></div></div>
<div class="actions"><button class="primary" onclick="pidAdvice()">Calculate recommendation</button></div><div id="pidAdviceResult" class="muted"></div></div></section>
</main>
<script>
var csrf='';
var chartSamples=[];
var lastStatus=null;
document.querySelectorAll('.tab').forEach(function(b){b.addEventListener('click',function(){
 document.querySelectorAll('.tab').forEach(function(x){x.classList.remove('active')});
 document.querySelectorAll('section').forEach(function(x){x.classList.remove('active')});
 b.classList.add('active');document.getElementById(b.dataset.page).classList.add('active');
 if(b.dataset.page==='history')loadHistory();if(b.dataset.page==='alerts')loadAlerts();if(b.dataset.page==='schedules')loadSchedules();
});});
async function api(path,method,body){
 var options={method:method||'GET',headers:{}};
 if(body!==undefined){options.headers['Content-Type']='application/json';options.headers['X-CSRF-Token']=csrf;options.body=JSON.stringify(body);}
 var response=await fetch(path,options);
 if(!response.ok)throw new Error('HTTP '+response.status+' '+(await response.text()));
 var type=response.headers.get('content-type')||'';
 return type.indexOf('application/json')>=0?response.json():response.text();
}
async function init(){try{var session=await api('/api/v1/session');csrf=session.csrf;await api('/api/v1/time/sync','POST',{epoch:Math.floor(Date.now()/1000)});await refresh();setInterval(refresh,1500);}catch(e){setConnection(false,e.message);setTimeout(init,4000);}}
function setConnection(ok,msg){var e=document.getElementById('connection');e.textContent=ok?'CONNECTED':(msg||'OFFLINE');e.className='status '+(ok?'good':'bad');}
function fmtTemp(v,valid){return valid?(Number(v).toFixed(1)+' °C'):'INVALID / STALE';}
async function refresh(){try{var s=await api('/api/v1/status');lastStatus=s;setConnection(s.linkConnected&&s.statusFresh);document.getElementById('state').textContent=s.stateName;
 document.getElementById('state').className='status '+(s.faultFlags?'bad':(s.linkConnected?'good':'warn'));
 document.getElementById('simulationNotice').hidden=!s.simulationActive;
 var html='';s.sensors.forEach(function(t,i){html+='<div class="card"><div class="kicker">Thermocouple '+(i+1)+' · '+esc(t.modeName)+'</div><div class="value">'+fmtTemp(t.temperatureC,t.valid)+'</div><div class="muted">Setpoint '+Number(t.setpointC).toFixed(1)+' °C · '+esc(t.faultName)+'</div></div>';});
 document.getElementById('sensors').innerHTML=html;
 document.getElementById('diagnostics').innerHTML='<div class="card">Fault flags<br><b>0x'+Number(s.faultFlags).toString(16)+'</b></div><div class="card">Output command mask<br><b>0x'+Number(s.outputMask).toString(16)+'</b></div><div class="card">Heartbeat age<br><b>'+s.heartbeatAgeMs+' ms</b></div><div class="card">Active alerts<br><b>'+s.activeAlerts+'</b></div>';
 document.getElementById('energySummary').textContent=s.energyConfigured?(s.energyKWh.toFixed(3)+' kWh · estimated '+(s.estimatedCostMinor/100).toFixed(2)):'Enter zone watts and energy price to enable estimate';
 if(s.simulationActive)document.getElementById('simResult').textContent='Preview temperatures: '+s.simulationTemperaturesC.map(function(v){return v.toFixed(1)+' °C'}).join(' / ');
 await drawGraph();
 }catch(e){setConnection(false,e.message);}}
function esc(v){return String(v===undefined?'':v).replace(/[&<>"']/g,function(c){return {'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c];});}
function drawGraph(){return api('/api/v1/telemetry').then(function(r){chartSamples=r.samples||[];var c=document.getElementById('chart'),ctx=c.getContext('2d');ctx.clearRect(0,0,c.width,c.height);ctx.strokeStyle='#334250';ctx.lineWidth=1;ctx.beginPath();for(var y=20;y<c.height;y+=52){ctx.moveTo(40,y);ctx.lineTo(c.width-10,y);}ctx.stroke();
 if(chartSamples.length<2)return;
 var all=[];chartSamples.forEach(function(s){for(var i=0;i<4;i++)if(s.validMask&(1<<i))all.push(s.temp[i]);});
 if(!all.length)return;var min=Math.min.apply(null,all),max=Math.max.apply(null,all);if(max===min)max=min+1;
 ctx.fillStyle='#9eacb8';ctx.font='12px system-ui';ctx.fillText(max.toFixed(0)+' °C',4,20);ctx.fillText(min.toFixed(0)+' °C',4,c.height-8);
 var colors=['#40c7ce','#e5b04e','#74c78e','#dc8ce0'];
 for(var z=0;z<4;z++){ctx.strokeStyle=colors[z];ctx.lineWidth=2;ctx.beginPath();var started=false;
  chartSamples.forEach(function(s,i){if(!(s.validMask&(1<<z))){started=false;return;}var x=40+(c.width-55)*(i/(chartSamples.length-1));var y=20+(c.height-35)*(1-(s.temp[z]-min)/(max-min));if(!started){ctx.moveTo(x,y);started=true;}else ctx.lineTo(x,y);});ctx.stroke();}
 });}
async function command(name){if(name==='start'&&!confirm('Send START request to the S3 controller?'))return;if(name==='stop'&&!confirm('Stop the current firing?'))return;try{var r=await api('/api/v1/commands/'+name,'POST',{});alert(r.message||'Request sent to controller.');await refresh();}catch(e){alert(e.message);}}
async function uploadProfile(){try{var segments=JSON.parse(document.getElementById('profileJson').value);var r=await api('/api/v1/profiles','POST',{segments:segments});document.getElementById('profileResult').textContent=r.message;}catch(e){document.getElementById('profileResult').textContent='Profile error: '+e.message;}}
async function loadHistory(){try{var r=await api('/api/v1/history');var h='<table><tr><th>ID</th><th>Profile</th><th>Started</th><th>Duration</th><th>Peak temperatures °C</th><th>Energy</th></tr>';r.records.forEach(function(x){h+='<tr><td>'+x.id+'</td><td>'+esc(x.profileId)+'</td><td>'+x.startedEpoch+'</td><td>'+x.durationSeconds+' s</td><td>'+x.peaks.map(function(v){return (v/10).toFixed(1)}).join(' / ')+'</td><td>'+(x.energyMilliWh/1000000).toFixed(3)+' kWh</td></tr>';});document.getElementById('historyTable').innerHTML=h+'</table>';}catch(e){document.getElementById('historyTable').textContent=e.message;}}
async function loadAlerts(){try{var settings=await api('/api/v1/config/alerts');alertCan.checked=settings.canLossEnabled;alertStale.checked=settings.staleStatusEnabled;alertFault.checked=settings.controllerFaultEnabled;alertSensor.checked=settings.sensorInvalidEnabled;alertTemp.checked=settings.temperatureWarningEnabled;alertTempLimit.value=settings.temperatureWarningDeciC/10;var r=await api('/api/v1/alerts');var h='<table><tr><th>Code</th><th>Severity</th><th>Active</th><th>Acknowledged</th><th>Time</th><th></th></tr>';r.events.forEach(function(x){h+='<tr><td>'+esc(x.name)+'</td><td>'+esc(x.severity)+'</td><td>'+x.active+'</td><td>'+x.acknowledged+'</td><td>'+x.elapsedMs+' ms</td><td><button onclick="ackAlert('+x.id+')">Acknowledge</button></td></tr>';});document.getElementById('alertsTable').innerHTML=h+'</table>';}catch(e){document.getElementById('alertsTable').textContent=e.message;}}
async function saveAlerts(){try{var r=await api('/api/v1/config/alerts','POST',{canLossEnabled:alertCan.checked,staleStatusEnabled:alertStale.checked,controllerFaultEnabled:alertFault.checked,sensorInvalidEnabled:alertSensor.checked,temperatureWarningEnabled:alertTemp.checked,temperatureWarningDeciC:Math.round(Number(alertTempLimit.value)*10)});alert('Alert preferences saved. S3 safety limits were not changed.');loadAlerts();}catch(e){alert(e.message);}}
async function ackAlert(id){try{await api('/api/v1/alerts/'+id+'/ack','POST',{});loadAlerts();}catch(e){alert(e.message);}}
async function loadSchedules(){try{var r=await api('/api/v1/schedules');var h='<table><tr><th>Profile</th><th>Due epoch</th><th>Status</th><th></th></tr>';r.entries.forEach(function(x){h+='<tr><td>'+esc(x.profileId)+'</td><td>'+x.startEpoch+'</td><td>'+x.state+'</td><td><button onclick="cancelSchedule('+x.id+')">Cancel</button></td></tr>';});document.getElementById('scheduleTable').innerHTML=h+'</table>';}catch(e){document.getElementById('scheduleTable').textContent=e.message;}}
async function addSchedule(){try{var d=document.getElementById('scheduleTime').value;if(!d)throw new Error('Choose a date and time');var epoch=Math.floor(new Date(d).getTime()/1000);var r=await api('/api/v1/schedules','POST',{profileId:document.getElementById('scheduleProfile').value,startEpoch:epoch});alert(r.message);loadSchedules();}catch(e){alert(e.message);}}
async function cancelSchedule(id){try{await api('/api/v1/schedules/'+id,'DELETE',{});loadSchedules();}catch(e){alert(e.message);}}
async function saveEnergy(){try{var watts=[0,1,2,3].map(function(i){return Number(document.getElementById('watts'+i).value)});var rate=Number(document.getElementById('rate').value);var r=await api('/api/v1/config/energy','POST',{zoneWatts:watts,rateMinorPerKWh:rate});alert(r.message);refresh();}catch(e){alert(e.message);}}
async function startSimulation(){try{var cfg={ambientC:Number(simAmbient.value),initialTemperatureC:Number(simInitial.value),maximumHeatRateCPerHour:Number(simHeat.value),coolingRateCPerHour:Number(simCool.value)};await api('/api/v1/simulation/start','POST',cfg);refresh();}catch(e){alert(e.message);}}
async function stopSimulation(){try{await api('/api/v1/simulation/stop','POST',{});refresh();}catch(e){alert(e.message);}}
async function simStep(mask){try{await api('/api/v1/simulation/step','POST',{outputMask:mask});refresh();}catch(e){alert(e.message);}}
async function pidTerms(){try{var r=await api('/api/v1/tuning/terms','POST',{kp:Number(diagKp.value),ki:Number(diagKi.value),kd:Number(diagKd.value),setpointC:Number(diagSetpoint.value),measuredC:Number(diagMeasured.value),dtSeconds:Number(diagDt.value)});pidTermsResult.textContent='Error '+r.error.toFixed(2)+' · P '+r.p.toFixed(2)+' · I '+r.i.toFixed(2)+' · D '+r.d.toFixed(2)+' · illustrative output '+r.output.toFixed(1)+'%';}catch(e){pidTermsResult.textContent=e.message;}}
async function pidAdvice(){try{var r=await api('/api/v1/tuning/advice','POST',{initialOutputPercent:Number(stepU0.value),finalOutputPercent:Number(stepU1.value),initialTemperatureC:Number(stepT0.value),finalTemperatureC:Number(stepT1.value),deadTimeSeconds:Number(stepL.value),timeTo63PercentSeconds:Number(stepTau.value)});pidAdviceResult.textContent=r.valid?('Suggested starting point only: Kp '+r.kp.toFixed(4)+', Ki '+r.ki.toFixed(5)+', Kd '+r.kd.toFixed(4)+'; process gain '+r.processGainCPerFraction.toFixed(3)+' °C per 100% output.'):'Step response data was invalid or insufficient.';}catch(e){pidAdviceResult.textContent=e.message;}}
init();
</script>
</body>
</html>
)HTML";

bool authorized() {
  if (server.authenticate("admin", apPassword)) return true;
  server.requestAuthentication();
  return false;
}

bool csrfValid() {
  return server.header("X-CSRF-Token") == String(csrfToken);
}

bool writeAuthorized() {
  if (!authorized()) return false;
  if (!csrfValid()) {
    server.send(403, "application/json", "{\"error\":\"CSRF token missing or invalid\"}");
    return false;
  }
  return true;
}

void sendJson(JsonDocument& document, int status = 200) {
  String body;
  serializeJson(document, body);
  server.send(status, "application/json", body);
}

const char* stateName(uint8_t state) {
  switch (state) {
    case kiln::STATE_BOOT: return "BOOT";
    case kiln::STATE_SELF_TEST: return "SELF_TEST";
    case kiln::STATE_IDLE: return "IDLE";
    case kiln::STATE_ARMED: return "ARMED";
    case kiln::STATE_FIRING: return "FIRING";
    case kiln::STATE_PAUSED: return "PAUSED";
    case kiln::STATE_COMPLETE: return "COMPLETE";
    case kiln::STATE_FAULT: return "FAULT";
    default: return "UNKNOWN";
  }
}

const char* sensorModeName(uint8_t mode) {
  switch (mode) {
    case 0: return "OFF";
    case 1: return "LOG_ONLY";
    case 2: return "LOG_CONTROL";
    case 3: return "LOG_SAFETY";
    default: return "UNKNOWN";
  }
}

const char* sensorFaultName(uint8_t fault) {
  switch (fault) {
    case 0: return "OK";
    case 1: return "OPEN";
    case 2: return "SHORT";
    case 3: return "CONVERSION";
    case 4: return "STALE";
    case 5: return "OUT_OF_RANGE";
    case 6: return "RATE_IMPLAUSIBLE";
    case 7: return "NOT_CONFIGURED";
    default: return "UNKNOWN";
  }
}

const char* alertName(AlertCode code) {
  switch (code) {
    case ALERT_CAN_LOST: return "CAN_LOST";
    case ALERT_STATUS_STALE: return "STATUS_STALE";
    case ALERT_CONTROLLER_FAULT: return "CONTROLLER_FAULT";
    case ALERT_SENSOR_INVALID: return "SENSOR_INVALID";
    case ALERT_OVERTEMP: return "OVERTEMP";
    case ALERT_RUN_STARTED: return "RUN_STARTED";
    case ALERT_RUN_COMPLETED: return "RUN_COMPLETED";
    case ALERT_RUN_STOPPED: return "RUN_STOPPED";
    case ALERT_SCHEDULE_DUE: return "SCHEDULE_DUE";
    case ALERT_TEMP_WARNING: return "TEMPERATURE_WARNING";
    default: return "NONE";
  }
}

const char* severityName(AlertSeverity severity) {
  switch (severity) {
    case ALERT_INFO: return "INFO";
    case ALERT_WARNING: return "WARNING";
    case ALERT_CRITICAL: return "CRITICAL";
    default: return "UNKNOWN";
  }
}

void saveHistory() {
  FiringRecord records[FIRING_HISTORY_CAPACITY];
  uint8_t count = 0;
  firingHistory.exportRaw(records, count);
  preferences.putUChar("histCount", count);
  if (count) preferences.putBytes("history", records, count * sizeof(FiringRecord));
}

void loadHistory() {
  const uint8_t count = preferences.getUChar("histCount", 0);
  if (count == 0 || count > FIRING_HISTORY_CAPACITY) return;
  const size_t expected = count * sizeof(FiringRecord);
  if (preferences.getBytesLength("history") != expected) return;
  FiringRecord records[FIRING_HISTORY_CAPACITY];
  if (preferences.getBytes("history", records, expected) == expected) {
    firingHistory.importRaw(records, count);
  }
}

void saveSchedules() {
  ScheduledFiring entries[SCHEDULE_CAPACITY];
  uint8_t count = 0;
  scheduleQueue.exportRaw(entries, count);
  preferences.putUChar("schCount", count);
  if (count) preferences.putBytes("schedules", entries, count * sizeof(ScheduledFiring));
}

void loadSchedules() {
  const uint8_t count = preferences.getUChar("schCount", 0);
  if (count == 0 || count > SCHEDULE_CAPACITY) return;
  const size_t expected = count * sizeof(ScheduledFiring);
  if (preferences.getBytesLength("schedules") != expected) return;
  ScheduledFiring entries[SCHEDULE_CAPACITY];
  if (preferences.getBytes("schedules", entries, expected) == expected) {
    scheduleQueue.importRaw(entries, count);
  }
}

void saveEnergyConfig() {
  preferences.putBytes("energyCfg", &energyConfig, sizeof(energyConfig));
  energyTracker.configure(energyConfig);
}

void saveAlertPreferences() {
  preferences.putBytes("alertCfg", &alertPreferences, sizeof(alertPreferences));
}

void loadAlertPreferences() {
  if (preferences.getBytesLength("alertCfg") == sizeof(alertPreferences)) {
    preferences.getBytes("alertCfg", &alertPreferences, sizeof(alertPreferences));
  }
}

void loadEnergyConfig() {
  if (preferences.getBytesLength("energyCfg") == sizeof(energyConfig)) {
    preferences.getBytes("energyCfg", &energyConfig, sizeof(energyConfig));
  }
  energyTracker.configure(energyConfig);
}

void pulseAlert(AlertCode code, AlertSeverity severity, uint32_t nowMs) {
  alertLedger.setCondition(code, severity, true, nowMs);
  alertLedger.setCondition(code, severity, false, nowMs);
}

void finishRun(uint8_t finalState, uint32_t nowMs) {
  if (!runActive) return;
  FiringRecord record = {};
  record.id = runId++;
  record.startedEpoch = runStartEpoch;
  record.durationSeconds = static_cast<uint32_t>(nowMs - runStartMs) / 1000UL;
  strncpy(record.profileId, "active-profile", PROFILE_ID_LENGTH - 1);
  for (uint8_t i = 0; i < 4; ++i) record.peakTemperatureDeciC[i] = peakTemperatures[i];
  record.energyMilliWh = energyTracker.energyMilliWh();
  record.estimatedCostMinor = energyTracker.estimatedCostMinor();
  record.finalState = finalState;
  firingHistory.add(record);
  saveHistory();
  pulseAlert(finalState == kiln::STATE_COMPLETE ? ALERT_RUN_COMPLETED
                                                : ALERT_RUN_STOPPED,
             ALERT_INFO, nowMs);
  runActive = false;
}

void updateFeatures() {
  if (!controller) return;
  const uint32_t nowMs = millis();
  const bool connected = controller->connected();
  const bool fresh = connected && controller->hasStatus() &&
                     controller->statusFresh();
  energyTracker.update(nowMs, controller->outputCommandMask(), fresh);

  alertLedger.setCondition(ALERT_CAN_LOST, ALERT_CRITICAL,
                           alertPreferences.canLossEnabled && !connected, nowMs);
  alertLedger.setCondition(ALERT_STATUS_STALE, ALERT_WARNING,
                           alertPreferences.staleStatusEnabled &&
                               controller->hasStatus() && !fresh, nowMs);
  alertLedger.setCondition(ALERT_CONTROLLER_FAULT, ALERT_CRITICAL,
                           alertPreferences.controllerFaultEnabled &&
                               controller->hasStatus() && controller->faultFlags() != 0,
                           nowMs);

  uint8_t requiredSensors = 0;
  for (uint8_t i = 0; i < 4; ++i) {
    if (controller->sensorMode(i) != 0) {
      requiredSensors |= static_cast<uint8_t>(1U << i);
    }
  }
  const uint8_t missingSensors = static_cast<uint8_t>(
      requiredSensors & static_cast<uint8_t>(~controller->sensorValidMask()));
  alertLedger.setCondition(ALERT_SENSOR_INVALID, ALERT_CRITICAL,
                           alertPreferences.sensorInvalidEnabled &&
                               fresh && missingSensors != 0, nowMs);

  int16_t hottestDeciC = INT16_MIN;
  bool haveValidTemperature = false;
  if (fresh) {
    for (uint8_t i = 0; i < 4; ++i) {
      if (controller->sensorValidMask() & (1U << i)) {
        const int16_t value = controller->temperatureDeciC(i);
        if (!haveValidTemperature || value > hottestDeciC) hottestDeciC = value;
        haveValidTemperature = true;
      }
    }
  }
  const bool temperatureWarning =
      alertPreferences.temperatureWarningEnabled &&
      alertPreferences.temperatureWarningDeciC > 0 &&
      haveValidTemperature &&
      hottestDeciC >= alertPreferences.temperatureWarningDeciC;
  alertLedger.setCondition(ALERT_TEMP_WARNING, ALERT_WARNING,
                           temperatureWarning, nowMs, hottestDeciC);

  if (fresh) {
    if (static_cast<uint32_t>(nowMs - lastSampleMs) >= 10000UL) {
      TelemetrySample sample = {};
      sample.elapsedMs = runActive ? static_cast<uint32_t>(nowMs - runStartMs) : nowMs;
      sample.validMask = controller->sensorValidMask();
      sample.outputMask = controller->outputCommandMask();
      sample.state = controller->controllerState();
      for (uint8_t i = 0; i < 4; ++i) {
        sample.temperatureDeciC[i] = controller->temperatureDeciC(i);
        sample.setpointDeciC[i] = controller->setpointDeciC(i);
      }
      telemetry.push(sample, 10000);
      lastSampleMs = nowMs;
    }

    const uint8_t state = controller->controllerState();
    if (state == kiln::STATE_FIRING && !runActive) {
      runActive = true;
      runStartMs = nowMs;
      const time_t epoch = time(nullptr);
      runStartEpoch = epoch > 1700000000 ? static_cast<uint32_t>(epoch) : 0;
      for (uint8_t i = 0; i < 4; ++i) {
        peakTemperatures[i] = controller->temperatureDeciC(i);
      }
      energyTracker.reset(nowMs);
      pulseAlert(ALERT_RUN_STARTED, ALERT_INFO, nowMs);
    }
    if (runActive && (state == kiln::STATE_FIRING || state == kiln::STATE_PAUSED)) {
      for (uint8_t i = 0; i < 4; ++i) {
        if ((controller->sensorValidMask() & (1U << i)) &&
            controller->temperatureDeciC(i) > peakTemperatures[i]) {
          peakTemperatures[i] = controller->temperatureDeciC(i);
        }
      }
    } else if (runActive) {
      finishRun(state, nowMs);
    }
  }

  const time_t epoch = time(nullptr);
  if (epoch > 1700000000) {
    const uint8_t due = scheduleQueue.markDue(static_cast<uint32_t>(epoch));
    if (due) {
      pulseAlert(ALERT_SCHEDULE_DUE, ALERT_WARNING, nowMs);
      saveSchedules();
    }
  }
  if (simulator.active()) simulator.update(nowMs, static_cast<uint8_t>(simulationOutputMask));
}

void handleSession() {
  if (!authorized()) return;
  JsonDocument doc;
  doc["csrf"] = csrfToken;
  doc["device"] = apName;
  sendJson(doc);
}

void handleStatus() {
  if (!authorized()) return;
  JsonDocument doc;
  doc["state"] = controller ? controller->controllerState() : kiln::STATE_BOOT;
  doc["stateName"] = controller ? stateName(controller->controllerState()) : "BOOT";
  doc["faultFlags"] = controller ? controller->faultFlags() : 0;
  doc["outputMask"] = controller ? controller->outputCommandMask() : 0;
  doc["sensorValidMask"] = controller ? controller->sensorValidMask() : 0;
  doc["linkConnected"] = controller && controller->connected();
  doc["statusFresh"] = controller && controller->hasStatus() && controller->statusFresh();
  doc["heartbeatAgeMs"] = controller ? controller->heartbeatAgeMs() : 0xFFFFFFFFUL;
  doc["activeAlerts"] = alertLedger.activeCount();
  doc["telemetrySamples"] = telemetry.size();
  doc["energyConfigured"] = energyTracker.configured();
  doc["energyKWh"] = energyTracker.energyKWh();
  doc["estimatedCostMinor"] = energyTracker.estimatedCostMinor();
  doc["simulationActive"] = simulator.active();
  JsonArray sensors = doc["sensors"].to<JsonArray>();
  for (uint8_t i = 0; i < 4; ++i) {
    JsonObject sensor = sensors.add<JsonObject>();
    const uint8_t mode = controller ? controller->sensorMode(i) : 0;
    const uint8_t fault = controller ? controller->sensorFault(i) : 7;
    const bool valid = controller && controller->hasStatus() &&
        (controller->sensorValidMask() & (1U << i)) != 0 &&
        controller->statusFresh();
    sensor["mode"] = mode;
    sensor["modeName"] = sensorModeName(mode);
    sensor["valid"] = valid;
    sensor["temperatureC"] = valid ? controller->temperatureDeciC(i) / 10.0f : 0.0f;
    sensor["setpointC"] = controller ? controller->setpointDeciC(i) / 10.0f : 0.0f;
    sensor["fault"] = fault;
    sensor["faultName"] = sensorFaultName(fault);
  }
  doc["alertTemperatureWarningEnabled"] = alertPreferences.temperatureWarningEnabled;
  doc["alertTemperatureWarningDeciC"] = alertPreferences.temperatureWarningDeciC;
  JsonArray sim = doc["simulationTemperaturesC"].to<JsonArray>();
  for (uint8_t i = 0; i < 4; ++i) sim.add(simulator.temperatureC(i));
  sendJson(doc);
}

void handleTelemetry() {
  if (!authorized()) return;
  JsonDocument doc;
  JsonArray samples = doc["samples"].to<JsonArray>();
  TelemetrySample sample;
  const uint16_t sampleCount = telemetry.size();
  const uint16_t stride = sampleCount > 180
      ? static_cast<uint16_t>((sampleCount + 179) / 180) : 1;
  for (uint16_t i = 0; i < sampleCount; i = static_cast<uint16_t>(i + stride)) {
    if (!telemetry.getOldestFirst(i, sample)) continue;
    JsonObject item = samples.add<JsonObject>();
    item["elapsedMs"] = sample.elapsedMs;
    item["validMask"] = sample.validMask;
    item["outputMask"] = sample.outputMask;
    item["state"] = sample.state;
    JsonArray temp = item["temp"].to<JsonArray>();
    for (uint8_t z = 0; z < 4; ++z) temp.add(sample.temperatureDeciC[z] / 10.0f);
    JsonArray setpoint = item["setpoint"].to<JsonArray>();
    for (uint8_t z = 0; z < 4; ++z) setpoint.add(sample.setpointDeciC[z] / 10.0f);
  }
  sendJson(doc);
}

void handleHistory() {
  if (!authorized()) return;
  JsonDocument doc;
  JsonArray records = doc["records"].to<JsonArray>();
  FiringRecord record;
  for (uint8_t i = 0; i < firingHistory.size(); ++i) {
    if (!firingHistory.newestFirst(i, record)) continue;
    JsonObject item = records.add<JsonObject>();
    item["id"] = record.id;
    item["startedEpoch"] = record.startedEpoch;
    item["durationSeconds"] = record.durationSeconds;
    item["profileId"] = record.profileId;
    item["energyMilliWh"] = record.energyMilliWh;
    item["estimatedCostMinor"] = record.estimatedCostMinor;
    JsonArray peaks = item["peaks"].to<JsonArray>();
    for (uint8_t z = 0; z < 4; ++z) peaks.add(record.peakTemperatureDeciC[z]);
  }
  sendJson(doc);
}

void handleAlerts() {
  if (!authorized()) return;
  JsonDocument doc;
  JsonArray events = doc["events"].to<JsonArray>();
  AlertEvent event;
  for (uint8_t i = 0; i < alertLedger.size(); ++i) {
    if (!alertLedger.newestFirst(i, event)) continue;
    JsonObject item = events.add<JsonObject>();
    item["id"] = event.id;
    item["name"] = alertName(event.code);
    item["severity"] = severityName(event.severity);
    item["active"] = event.active;
    item["acknowledged"] = event.acknowledged;
    item["elapsedMs"] = event.elapsedMs;
    item["valueDeciC"] = event.valueDeciC;
  }
  sendJson(doc);
}

void handleSchedules() {
  if (!authorized()) return;
  JsonDocument doc;
  JsonArray entries = doc["entries"].to<JsonArray>();
  for (uint8_t i = 0; i < SCHEDULE_CAPACITY; ++i) {
    ScheduledFiring entry;
    if (!scheduleQueue.get(i, entry)) continue;
    JsonObject item = entries.add<JsonObject>();
    item["id"] = entry.id;
    item["profileId"] = entry.profileId;
    item["startEpoch"] = entry.startEpoch;
    item["state"] = entry.state == SCHEDULE_DUE ? "DUE" : "PENDING";
  }
  sendJson(doc);
}

bool parseBody(JsonDocument& doc) {
  const DeserializationError error = deserializeJson(doc, server.arg("plain"));
  if (error) {
    server.send(400, "application/json", "{\"error\":\"Invalid JSON body\"}");
    return false;
  }
  return true;
}

void handleCommand(uint8_t command) {
  if (!writeAuthorized()) return;
  if (!controller || !controller->connected() || !controller->statusFresh()) {
    server.send(503, "application/json", "{\"error\":\"S3 link or status is not fresh\"}");
    return;
  }
  const bool accepted = controller->sendCommand(command);
  JsonDocument doc;
  doc["sent"] = accepted;
  doc["message"] = accepted
      ? "Command request sent to S3; verify its reported state and acknowledgement."
      : "Command could not be sent.";
  sendJson(doc, accepted ? 202 : 503);
}

void handleProfileUpload() {
  if (!writeAuthorized()) return;
  if (!controller || !controller->connected() || !controller->statusFresh() ||
      controller->controllerState() != kiln::STATE_IDLE) {
    server.send(409, "application/json", "{\"error\":\"Profile changes require fresh link and S3 IDLE state\"}");
    return;
  }
  JsonDocument doc;
  if (!parseBody(doc)) return;
  JsonArray segments = doc["segments"].as<JsonArray>();
  if (segments.isNull() || segments.size() == 0 ||
      segments.size() > MAX_WEB_PROFILE_SEGMENTS) {
    server.send(400, "application/json", "{\"error\":\"Profile must contain 1 to 32 segments\"}");
    return;
  }
  const uint8_t count = static_cast<uint8_t>(segments.size());
  struct ParsedProfileSegment {
    float targetC;
    float rampCPerHour;
    uint32_t holdSeconds;
  };
  ParsedProfileSegment parsed[MAX_WEB_PROFILE_SEGMENTS];
  uint8_t index = 0;
  for (JsonVariant segment : segments) {
    const float target = segment["targetC"] | NAN;
    const float ramp = segment["rampCPerHour"] | NAN;
    const uint32_t hold = segment["holdSeconds"] | 0UL;
    if (!isfinite(target) || !isfinite(ramp) || target < 0.0f ||
        target > 1800.0f || ramp < 0.0f || ramp > 600.0f ||
        hold > 86400UL) {
      server.send(400, "application/json", "{\"error\":\"Segment contains invalid target, ramp, or hold value\"}");
      return;
    }
    parsed[index].targetC = target;
    parsed[index].rampCPerHour = ramp;
    parsed[index].holdSeconds = hold;
    ++index;
  }
  if (!controller->beginProfileConfig(count)) {
    server.send(503, "application/json", "{\"error\":\"Could not begin profile transfer\"}");
    return;
  }
  for (index = 0; index < count; ++index) {
    if (!controller->sendProfileSegment(index,
          static_cast<int16_t>(lroundf(parsed[index].targetC * 10.0f)),
          static_cast<int16_t>(lroundf(parsed[index].rampCPerHour * 10.0f)),
          parsed[index].holdSeconds)) {
      server.send(503, "application/json", "{\"error\":\"Segment transfer failed\"}");
      return;
    }
  }
  const bool sent = controller->commitProfileConfig();
  JsonDocument response;
  response["sent"] = sent;
  response["message"] = sent
      ? "Profile transfer submitted. Check S3 acknowledgement before starting."
      : "Profile commit could not be sent.";
  sendJson(response, sent ? 202 : 503);
}

void handleAddSchedule() {
  if (!writeAuthorized()) return;
  JsonDocument doc;
  if (!parseBody(doc)) return;
  const uint32_t startEpoch = doc["startEpoch"] | 0UL;
  const char* profileId = doc["profileId"] | "";
  const time_t now = time(nullptr);
  if (now < 1700000000 || startEpoch <= static_cast<uint32_t>(now)) {
    server.send(400, "application/json", "{\"error\":\"Clock must be synchronized and start time must be in the future\"}");
    return;
  }
  uint32_t id = 0;
  if (!scheduleQueue.add(startEpoch, profileId, id)) {
    server.send(400, "application/json", "{\"error\":\"Invalid schedule or queue is full\"}");
    return;
  }
  saveSchedules();
  JsonDocument response;
  response["id"] = id;
  response["message"] = "Reminder saved. It will never start the kiln automatically.";
  sendJson(response, 201);
}

void handleCancelSchedule() {
  if (!writeAuthorized()) return;
  const String path = server.uri();
  const int slash = path.lastIndexOf('/');
  const uint32_t id = static_cast<uint32_t>(path.substring(slash + 1).toInt());
  if (!id || !scheduleQueue.cancel(id)) {
    server.send(404, "application/json", "{\"error\":\"Schedule not found\"}");
    return;
  }
  saveSchedules();
  server.send(200, "application/json", "{\"cancelled\":true}");
}

void handleAlertConfigGet() {
  if (!authorized()) return;
  JsonDocument doc;
  doc["canLossEnabled"] = alertPreferences.canLossEnabled;
  doc["staleStatusEnabled"] = alertPreferences.staleStatusEnabled;
  doc["controllerFaultEnabled"] = alertPreferences.controllerFaultEnabled;
  doc["sensorInvalidEnabled"] = alertPreferences.sensorInvalidEnabled;
  doc["temperatureWarningEnabled"] = alertPreferences.temperatureWarningEnabled;
  doc["temperatureWarningDeciC"] = alertPreferences.temperatureWarningDeciC;
  sendJson(doc);
}

void handleAlertConfigPost() {
  if (!writeAuthorized()) return;
  JsonDocument doc;
  if (!parseBody(doc)) return;
  AlertPreferences candidate = alertPreferences;
  candidate.canLossEnabled = doc["canLossEnabled"] | candidate.canLossEnabled;
  candidate.staleStatusEnabled = doc["staleStatusEnabled"] | candidate.staleStatusEnabled;
  candidate.controllerFaultEnabled = doc["controllerFaultEnabled"] | candidate.controllerFaultEnabled;
  candidate.sensorInvalidEnabled = doc["sensorInvalidEnabled"] | candidate.sensorInvalidEnabled;
  candidate.temperatureWarningEnabled = doc["temperatureWarningEnabled"] | candidate.temperatureWarningEnabled;
  const int warning = doc["temperatureWarningDeciC"] | candidate.temperatureWarningDeciC;
  if (warning < 0 || warning > 18000) {
    server.send(400, "application/json", "{\"error\":\"Temperature warning threshold must be 0 to 1800 C\"}");
    return;
  }
  candidate.temperatureWarningDeciC = static_cast<int16_t>(warning);
  alertPreferences = candidate;
  saveAlertPreferences();
  server.send(200, "application/json", "{\"saved\":true,\"safetyLimitsChanged\":false}");
}

void handleClockSync() {
  if (!writeAuthorized()) return;
  JsonDocument doc;
  if (!parseBody(doc)) return;
  const uint32_t epoch = doc["epoch"] | 0UL;
  if (epoch < 1700000000UL || epoch > 4102444800UL) {
    server.send(400, "application/json", "{\"error\":\"Invalid clock epoch\"}");
    return;
  }
  struct timeval tv;
  tv.tv_sec = static_cast<time_t>(epoch);
  tv.tv_usec = 0;
  if (settimeofday(&tv, nullptr) != 0) {
    server.send(500, "application/json", "{\"error\":\"Could not set controller clock\"}");
    return;
  }
  server.send(200, "application/json", "{\"clockSynchronized\":true}");
}

void handleTelemetryCsv() {
  if (!authorized()) return;
  server.sendHeader("Content-Disposition", "attachment; filename=kiln-telemetry.csv");
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/csv", "elapsed_ms,valid_mask,output_mask,state,tc1_c,tc1_setpoint_c,tc2_c,tc2_setpoint_c,tc3_c,tc3_setpoint_c,tc4_c,tc4_setpoint_c\n");
  TelemetrySample sample;
  char line[220];
  for (uint16_t i = 0; i < telemetry.size(); ++i) {
    if (!telemetry.getOldestFirst(i, sample)) continue;
    snprintf(line, sizeof(line), "%lu,%u,%u,%u,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f\n",
      static_cast<unsigned long>(sample.elapsedMs), sample.validMask,
      sample.outputMask, sample.state,
      sample.temperatureDeciC[0] / 10.0f, sample.setpointDeciC[0] / 10.0f,
      sample.temperatureDeciC[1] / 10.0f, sample.setpointDeciC[1] / 10.0f,
      sample.temperatureDeciC[2] / 10.0f, sample.setpointDeciC[2] / 10.0f,
      sample.temperatureDeciC[3] / 10.0f, sample.setpointDeciC[3] / 10.0f);
    server.sendContent(line);
  }
  server.sendContent("");
}

void handleHistoryCsv() {
  if (!authorized()) return;
  server.sendHeader("Content-Disposition", "attachment; filename=kiln-history.csv");
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/csv", "id,started_epoch,duration_seconds,profile_id,tc1_peak_c,tc2_peak_c,tc3_peak_c,tc4_peak_c,energy_kwh,estimated_cost_minor,final_state\n");
  FiringRecord record;
  char line[220];
  for (uint8_t i = 0; i < firingHistory.size(); ++i) {
    if (!firingHistory.newestFirst(i, record)) continue;
    snprintf(line, sizeof(line), "%lu,%lu,%lu,%s,%.1f,%.1f,%.1f,%.1f,%.6f,%lu,%u\n",
      static_cast<unsigned long>(record.id),
      static_cast<unsigned long>(record.startedEpoch),
      static_cast<unsigned long>(record.durationSeconds),
      record.profileId,
      record.peakTemperatureDeciC[0] / 10.0f,
      record.peakTemperatureDeciC[1] / 10.0f,
      record.peakTemperatureDeciC[2] / 10.0f,
      record.peakTemperatureDeciC[3] / 10.0f,
      static_cast<double>(record.energyMilliWh) / 1000000.0,
      static_cast<unsigned long>(record.estimatedCostMinor),
      record.finalState);
    server.sendContent(line);
  }
  server.sendContent("");
}

void handleEnergyConfig() {
  if (!writeAuthorized()) return;
  JsonDocument doc;
  if (!parseBody(doc)) return;
  JsonArray watts = doc["zoneWatts"].as<JsonArray>();
  if (watts.size() != 4) {
    server.send(400, "application/json", "{\"error\":\"zoneWatts must contain four values\"}");
    return;
  }
  EnergyConfig candidate = {};
  for (uint8_t i = 0; i < 4; ++i) {
    const uint32_t value = watts[i] | 0UL;
    if (value > 50000UL) {
      server.send(400, "application/json", "{\"error\":\"Zone wattage is outside supported estimate range\"}");
      return;
    }
    candidate.zoneWatts[i] = static_cast<uint16_t>(value);
  }
  candidate.rateMinorPerKWh = doc["rateMinorPerKWh"] | 0UL;
  if (candidate.rateMinorPerKWh > 100000UL) {
    server.send(400, "application/json", "{\"error\":\"Energy rate is outside supported range\"}");
    return;
  }
  energyConfig = candidate;
  saveEnergyConfig();
  server.send(200, "application/json", "{\"saved\":true}");
}

void handleSimulationStart() {
  if (!writeAuthorized()) return;
  JsonDocument doc;
  if (!parseBody(doc)) return;
  SimulationConfig config;
  config.ambientC = doc["ambientC"] | 20.0f;
  config.initialTemperatureC = doc["initialTemperatureC"] | 20.0f;
  config.maximumHeatRateCPerHour = doc["maximumHeatRateCPerHour"] | 100.0f;
  config.coolingRateCPerHour = doc["coolingRateCPerHour"] | 20.0f;
  if (!simulator.begin(config, millis())) {
    server.send(400, "application/json", "{\"error\":\"Simulation settings are invalid\"}");
    return;
  }
  simulationOutputMask = 0;
  server.send(200, "application/json", "{\"simulationOnly\":true,\"message\":\"Isolated simulation preview started; live controller data is unchanged.\"}");
}

void handleSimulationStop() {
  if (!writeAuthorized()) return;
  simulator.stop();
  simulationOutputMask = 0;
  server.send(200, "application/json", "{\"simulationActive\":false}");
}

void handleSimulationStep() {
  if (!writeAuthorized()) return;
  if (!simulator.active()) {
    server.send(409, "application/json", "{\"error\":\"Start simulation preview first\"}");
    return;
  }
  JsonDocument doc;
  if (!parseBody(doc)) return;
  simulationOutputMask = static_cast<uint32_t>(doc["outputMask"] | 0UL) & 0x0F;
  simulator.update(millis(), static_cast<uint8_t>(simulationOutputMask));
  server.send(200, "application/json", "{\"simulationOnly\":true}");
}

void handlePidTerms() {
  if (!writeAuthorized()) return;
  JsonDocument doc;
  if (!parseBody(doc)) return;
  PidConfig config;
  config.kp = doc["kp"] | 0.0f;
  config.ki = doc["ki"] | 0.0f;
  config.kd = doc["kd"] | 0.0f;
  config.outputMinimum = 0.0f;
  config.outputMaximum = 100.0f;
  PidDiagnostics diagnostics;
  diagnostics.configure(config);
  const PidTerms terms = diagnostics.update(
      doc["setpointC"] | NAN, doc["measuredC"] | NAN, doc["dtSeconds"] | NAN);
  JsonDocument response;
  response["error"] = terms.error;
  response["p"] = terms.p;
  response["i"] = terms.i;
  response["d"] = terms.d;
  response["output"] = terms.output;
  response["diagnosticOnly"] = true;
  sendJson(response);
}

void handlePidAdvice() {
  if (!writeAuthorized()) return;
  JsonDocument doc;
  if (!parseBody(doc)) return;
  PidStepResponse response = {
    doc["initialOutputPercent"] | NAN,
    doc["finalOutputPercent"] | NAN,
    doc["initialTemperatureC"] | NAN,
    doc["finalTemperatureC"] | NAN,
    doc["deadTimeSeconds"] | NAN,
    doc["timeTo63PercentSeconds"] | NAN
  };
  const PidRecommendation recommendation = PidTuningAdvisor::fromStepResponse(response);
  JsonDocument output;
  output["valid"] = recommendation.valid;
  output["processGainCPerFraction"] = recommendation.processGainCPerFraction;
  output["kp"] = recommendation.kp;
  output["ki"] = recommendation.ki;
  output["kd"] = recommendation.kd;
  output["integralTimeSeconds"] = recommendation.integralTimeSeconds;
  output["derivativeTimeSeconds"] = recommendation.derivativeTimeSeconds;
  output["offlineRecommendationOnly"] = true;
  sendJson(output);
}

void handleAcknowledgeAlert() {
  if (!writeAuthorized()) return;
  const String path = server.uri();
  const int lastSlash = path.lastIndexOf('/');
  const int previousSlash = path.lastIndexOf('/', lastSlash - 1);
  if (previousSlash < 0 || lastSlash <= previousSlash) {
    server.send(404, "application/json", "{\"error\":\"Alert not found\"}");
    return;
  }
  const uint32_t id = static_cast<uint32_t>(
      path.substring(previousSlash + 1, lastSlash).toInt());
  if (!id || !alertLedger.acknowledge(id)) {
    server.send(404, "application/json", "{\"error\":\"Alert not found\"}");
    return;
  }
  server.send(200, "application/json", "{\"acknowledged\":true}");
}

void initializeNetwork() {
  if (!preferencesReady) preferencesReady = preferences.begin("kiln-web", false);
  const uint64_t mac = ESP.getEfuseMac();
  snprintf(apName, sizeof(apName), "KilnController-%06X",
           static_cast<uint32_t>(mac & 0xFFFFFFULL));
  if (preferencesReady &&
      preferences.getString("apPass", apPassword, sizeof(apPassword)) == 0) {
    const uint32_t randomA = esp_random();
    snprintf(apPassword, sizeof(apPassword), "K%08lx", static_cast<unsigned long>(randomA));
    preferences.putString("apPass", apPassword);
  }
  if (strlen(apPassword) < 8) {
    snprintf(apPassword, sizeof(apPassword), "K%08lx",
             static_cast<unsigned long>(esp_random()));
    if (preferencesReady) preferences.putString("apPass", apPassword);
  }
  snprintf(csrfToken, sizeof(csrfToken), "%08lx",
           static_cast<unsigned long>(esp_random()));

  WiFi.mode(WIFI_AP);
  const bool apStarted = WiFi.softAP(apName, apPassword);
  Serial.printf("Kiln web AP: %s\n", apStarted ? "started" : "FAILED");
  Serial.printf("SSID: %s\nPassword: %s\nURL: http://%s/\n",
                apName, apPassword, WiFi.softAPIP().toString().c_str());
  if (MDNS.begin("kiln-controller")) {
    MDNS.addService("http", "tcp", 80);
  }

  const char* headers[] = {"X-CSRF-Token"};
  server.collectHeaders(headers, 1);
  server.on("/", HTTP_GET, []() {
    if (!authorized()) return;
    server.send_P(200, "text/html; charset=utf-8", INDEX_HTML);
  });
  server.on("/api/v1/session", HTTP_GET, handleSession);
  server.on("/api/v1/time/sync", HTTP_POST, handleClockSync);
  server.on("/api/v1/status", HTTP_GET, handleStatus);
  server.on("/api/v1/telemetry", HTTP_GET, handleTelemetry);
  server.on("/api/v1/history", HTTP_GET, handleHistory);
  server.on("/api/v1/history.csv", HTTP_GET, handleHistoryCsv);
  server.on("/api/v1/telemetry.csv", HTTP_GET, handleTelemetryCsv);
  server.on("/api/v1/alerts", HTTP_GET, handleAlerts);
  server.on("/api/v1/config/alerts", HTTP_GET, handleAlertConfigGet);
  server.on("/api/v1/config/alerts", HTTP_POST, handleAlertConfigPost);
  server.on("/api/v1/schedules", HTTP_GET, handleSchedules);
  server.on("/api/v1/commands/start", HTTP_POST, [](){ handleCommand(kiln::CMD_START); });
  server.on("/api/v1/commands/pause", HTTP_POST, [](){ handleCommand(kiln::CMD_PAUSE); });
  server.on("/api/v1/commands/resume", HTTP_POST, [](){ handleCommand(kiln::CMD_RESUME); });
  server.on("/api/v1/commands/stop", HTTP_POST, [](){ handleCommand(kiln::CMD_STOP); });
  server.on("/api/v1/profiles", HTTP_POST, handleProfileUpload);
  server.on("/api/v1/schedules", HTTP_POST, handleAddSchedule);
  server.on("/api/v1/config/energy", HTTP_POST, handleEnergyConfig);
  server.on("/api/v1/simulation/start", HTTP_POST, handleSimulationStart);
  server.on("/api/v1/simulation/stop", HTTP_POST, handleSimulationStop);
  server.on("/api/v1/simulation/step", HTTP_POST, handleSimulationStep);
  server.on("/api/v1/tuning/terms", HTTP_POST, handlePidTerms);
  server.on("/api/v1/tuning/advice", HTTP_POST, handlePidAdvice);
  server.onNotFound([]() {
    if (server.method() == HTTP_DELETE && server.uri().startsWith("/api/v1/schedules/")) {
      handleCancelSchedule();
    } else if (server.method() == HTTP_POST &&
               server.uri().startsWith("/api/v1/alerts/") &&
               server.uri().endsWith("/ack")) {
      handleAcknowledgeAlert();
    } else {
      server.send(404, "application/json", "{\"error\":\"Not found\"}");
    }
  });
  server.begin();
}

}  // namespace

void KilnWebApp::begin(ControllerLink& link) {
  controller = &link;
  if (!preferencesReady) {
    preferencesReady = preferences.begin("kiln-web", false);
  }
  loadHistory();
  loadSchedules();
  loadEnergyConfig();
  loadAlertPreferences();
  initializeNetwork();
  Serial.println("Kiln web dashboard initialized. Browser session uses HTTP Basic authentication.");
}

void KilnWebApp::update() {
  updateFeatures();
  server.handleClient();
}
