/* 
  ESP32 “Actuador” – Control Pan/Tilt con RTC y Seguimiento de Luz
  ----------------------------------------------------------------
  Proyecto: Heliostato – Actuador Pan/Tilt
  Archivo:  actuador_heliostato.ino
  Versión:  2.5
  Fecha:    2025-06-18
  Autor:    Tu Nombre / @TuUsuario
  Placa:    DFRobot FireBeetle ESP32 / Wemos Lolin32 Lite V3 / ESP32-DevKitC V4
  IDE:      Arduino IDE 2.x
  Librerías:
    • WiFi.h
    • WebServer.h
    • Wire.h
    • RTClib.h

  Pines:
    — Motores paso a paso 28BYJ-48 —
      PAN:
        IN1 → GPIO13
        IN2 → GPIO12
        IN3 → GPIO14
        IN4 → GPIO27
      TILT:
        IN1 → GPIO26
        IN2 → GPIO25
        IN3 → GPIO33
        IN4 → GPIO32
    — RTC DS3231 (I²C) —
      SDA → GPIO21
      SCL → GPIO22
    — Botón Stop/Start Auto (opcional):
      → any digital pin libre, p.ej. GPIO4

  Parámetros configurables:
    TH            = 80      // umbral luz
    COMM_TIMEOUT  = 1500    // ms sin /ldr → desactiva Auto
    α (filtro)    = 0.3     // suavizado exponencial
    DELAY_MIN     = 200     // µs velocidad máxima
    DELAY_MAX     = 2000    // µs velocidad mínima

  Descripción:
    • Crea AP “Heliostato_AP” (192.168.4.1) y servidor web.  
    • Página web con:
        ‣ Hora RTC (DS3231) y botón “Sincronizar RTC”.  
        ‣ Botón Start/Stop Auto y estado “Sensor Conectado/Desconectado”.  
        ‣ Controles manuales Pan/Tilt.  
    • Endpoint `/ldr?ul=&ur=&dl=&dr=`:
        ‣ Suaviza lecturas LDR (α).  
        ‣ Lógica invertida: iluminar mueve en sentido opuesto.  
        ‣ Velocidad proporcional a la diferencia de luz.  
    • Botones manuales siempre tienen prioridad.  
    • Timeout en Auto si no llegan datos LDR en COMM_TIMEOUT.  
    • Imprime en Serie (115200 bps) errores y eventos clave.

  Notas:
    • Versionado semántico: incrementa major/minor según cambios.  
    • Añade validación de parámetros y bloqueos antiparásitos en producción.  
    • Para futuras expansiones: incluye OTA, MQTT o watchdog.
*/

#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include "RTClib.h"

// ————— Motor PAN pins —————
const int Pin1 = 13, Pin2 = 12, Pin3 = 14, Pin4 = 27;
// ————— Motor TILT pins —————
const int Pin5 = 26, Pin6 = 25, Pin7 = 33, Pin8 = 32;

// Secuencia unipolar 8 pasos
const int pole1[] = {0,0,0,0, 0,1,1,1};
const int pole2[] = {0,0,0,1, 1,1,0,0};
const int pole3[] = {0,1,1,1, 0,0,0,0};
const int pole4[] = {1,1,0,0, 0,0,0,1};

int poleStep  = 0;   // índice PAN
int poleStep2 = 0;   // índice TILT

// Manual buttons state
int manualPan  = 0;  // 0=stop,1=left,2=right
int manualTilt = 0;  // 0=stop,1=up,  2=down

// Auto (LDR) state
int autoPan  = 0;
int autoTilt = 0;

bool autoEnabled = true;
bool commOk = false;
unsigned long lastLdr = 0;
const unsigned long COMM_TIMEOUT = 1500;  // ms

RTC_DS3231 rtc;
WebServer server(80);

const int TH = 80;  // umbral para LDR

// AP credentials
const char* ap_ssid = "Heliostato_AP";
const char* ap_pass = "12345678";

// ————— Bobinas —————
void driveStepper(int idx) {
  digitalWrite(Pin1, pole1[idx]);
  digitalWrite(Pin2, pole2[idx]);
  digitalWrite(Pin3, pole3[idx]);
  digitalWrite(Pin4, pole4[idx]);
}
void driveStepper2(int idx) {
  digitalWrite(Pin5, pole1[idx]);
  digitalWrite(Pin6, pole2[idx]);
  digitalWrite(Pin7, pole3[idx]);
  digitalWrite(Pin8, pole4[idx]);
}

// ————— Página principal —————
void handleRoot() {
  String status = commOk ? "Sensor: Conectado" : "Sensor: Desconectado";
  String btnLabel = autoEnabled ? "Stop Auto" : "Start Auto";
  String btnClass = autoEnabled ? "on" : "";
  String html = R"rawliteral(
<!DOCTYPE html><html><head>
  <meta name="viewport" content="width=device-width">
  <title>Heliostato</title>
  <style>
    body{font-family:sans-serif;text-align:center;padding:20px;}
    .btn{width:60px;height:60px;font-size:24px;margin:10px;}
    #status{margin:10px;font-weight:bold;}
    #autoBtn{padding:8px 16px;border:none;border-radius:4px;cursor:pointer;}
    #autoBtn.on{background:#4a4;color:#fff;}
    #autoBtn{background:#f44;color:#fff;}
    #rtc{font-size:1.5em;margin-bottom:10px;}
  </style>
</head><body>
  <h2>Control Heliostato</h2>
  <div id="rtc">Hora RTC: --:--:--</div>
  <button onclick="syncRTC()">Sincronizar RTC</button><br>
  <div id="status">)rawliteral" + status + R"rawliteral(</div>
  <button id="autoBtn" class=")"rawliteral" + btnClass + R"rawliteral(" onclick="toggleAuto()">)"rawliteral" + btnLabel + R"rawliteral(</button>
  <p>
    <button class="btn" onclick="location='/panleft'">← Pan</button>
    <button class="btn" onclick="location='/panright'">Pan →</button>
  </p>
  <p>
    <button class="btn" onclick="location='/tiltup'">↑ Tilt</button>
    <button class="btn" onclick="location='/tiltdown'">Tilt ↓</button>
  </p>
  <script>
    function updateRTC() {
      fetch('/gettime').then(r=>r.text()).then(t=>{
        document.getElementById('rtc').textContent='Hora RTC: '+t;
      });
      fetch('/status').then(r=>r.text()).then(s=>{
        document.getElementById('status').textContent='Sensor: '+s;
      });
    }
    function syncRTC() {
      const d=new Date(), url=
        '/settime?y='+d.getFullYear()+
        '&m='+(d.getMonth()+1)+
        '&d='+d.getDate()+
        '&h='+d.getHours()+
        '&min='+d.getMinutes()+
        '&s='+d.getSeconds();
      fetch(url).then(r=>r.text()).then(alert);
    }
    function toggleAuto() {
      fetch('/toggleAuto').then(r=>r.text()).then(state=>{
        const btn=document.getElementById('autoBtn');
        if(state==='ON'){ btn.textContent='Stop Auto'; btn.classList.add('on'); }
        else           { btn.textContent='Start Auto';btn.classList.remove('on'); }
      });
    }
    setInterval(updateRTC,1000);
    updateRTC();
  </script>
</body></html>
)rawliteral";
  server.send(200, "text/html", html);
}

// ————— Manual movement —————
void panLeft()  { manualPan = (manualPan==1 ? 0 : 1); manualTilt = 0; handleRoot(); }
void panRight() { manualPan = (manualPan==2 ? 0 : 2); manualTilt = 0; handleRoot(); }
void tiltUp()   { manualTilt = (manualTilt==1? 0 : 1); manualPan = 0; handleRoot(); }
void tiltDown() { manualTilt = (manualTilt==2? 0 : 2); manualPan = 0; handleRoot(); }

// ————— RTC handlers —————
void handleGetTime() {
  DateTime now = rtc.now();
  char buf[9];
  snprintf(buf,sizeof(buf),"%02d:%02d:%02d", now.hour(), now.minute(), now.second());
  server.send(200, "text/plain", buf);
}
void handleSetTime() {
  if (server.hasArg("y")&&server.hasArg("m")&&server.hasArg("d")
   && server.hasArg("h")&&server.hasArg("min")&&server.hasArg("s")) {
    rtc.adjust(DateTime(
      server.arg("y").toInt(), server.arg("m").toInt(), server.arg("d").toInt(),
      server.arg("h").toInt(), server.arg("min").toInt(), server.arg("s").toInt()
    ));
    server.send(200, "text/plain", "RTC sincronizado");
  } else {
    server.send(400, "text/plain", "Faltan parámetros");
  }
}

// ————— Auto toggle & status —————
void handleToggleAuto() {
  autoEnabled = !autoEnabled;
  server.send(200, "text/plain", autoEnabled ? "ON" : "OFF");
}
void handleStatus() {
  server.send(200, "text/plain", commOk ? "Conectado" : "Desconectado");
}

// ————— LDR → Auto logic (inverted) —————
void handleLDR() {
  if (!(server.hasArg("ul")&&server.hasArg("ur")
     &&server.hasArg("dl")&&server.hasArg("dr"))) {
    server.send(400, "text/plain", "Missing params");
    return;
  }
  commOk = true;
  lastLdr = millis();

  int ul = server.arg("ul").toInt();
  int ur = server.arg("ur").toInt();
  int dl = server.arg("dl").toInt();
  int dr = server.arg("dr").toInt();

  // PAN inverted: if left side brighter → turn RIGHT
  int leftVal  = (ul + dl) / 2;
  int rightVal = (ur + dr) / 2;
  if      (leftVal  - rightVal > TH) autoPan = 2;
  else if (rightVal - leftVal   > TH) autoPan = 1;
  else                                 autoPan = 0;

  // TILT inverted: if top brighter → move DOWN
  int upVal   = (ul + ur) / 2;
  int downVal = (dl + dr) / 2;
  if      (upVal   - downVal   > TH) autoTilt = 2;
  else if (downVal - upVal     > TH) autoTilt = 1;
  else                                 autoTilt = 0;

  server.send(200, "text/plain", "OK");
}

void setup() {
  Serial.begin(115200);
  // motor pins
  pinMode(Pin1,OUTPUT);pinMode(Pin2,OUTPUT);
  pinMode(Pin3,OUTPUT);pinMode(Pin4,OUTPUT);
  pinMode(Pin5,OUTPUT);pinMode(Pin6,OUTPUT);
  pinMode(Pin7,OUTPUT);pinMode(Pin8,OUTPUT);

  // I2C & RTC
  Wire.begin(21,22);
  if (!rtc.begin()) { Serial.println("RTC no detectado"); while(1) delay(10); }
  if (rtc.lostPower()) rtc.adjust(DateTime(F(__DATE__),F(__TIME__)));

  // AP
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ap_ssid,ap_pass);
  Serial.printf("AP: %s  IP: %s\n",ap_ssid,WiFi.softAPIP().toString().c_str());

  // HTTP routes
  server.on("/",         HTTP_GET, handleRoot);
  server.on("/panleft",  HTTP_GET, panLeft);
  server.on("/panright", HTTP_GET, panRight);
  server.on("/tiltup",   HTTP_GET, tiltUp);
  server.on("/tiltdown", HTTP_GET, tiltDown);
  server.on("/gettime",  HTTP_GET, handleGetTime);
  server.on("/settime",  HTTP_GET, handleSetTime);
  server.on("/toggleAuto",HTTP_GET,handleToggleAuto);
  server.on("/status",   HTTP_GET, handleStatus);
  server.on("/ldr",      HTTP_GET, handleLDR);
  server.onNotFound([](){ server.send(404,"text/plain","Not found"); });
  server.begin();
  Serial.println("HTTP server iniciado");
}

void loop() {
  server.handleClient();

  // manage comm timeout
  if (millis() - lastLdr > COMM_TIMEOUT) commOk = false;

  // choose manual over auto
  int panState  = manualPan  != 0 ? manualPan  : (autoEnabled && commOk ? autoPan  : 0);
  int tiltState = manualTilt != 0 ? manualTilt : (autoEnabled && commOk ? autoTilt : 0);

  // PAN movement
  if     (panState == 1) { poleStep = (poleStep+1)%8; driveStepper(poleStep); }
  else if(panState == 2) { poleStep = (poleStep+7)%8; driveStepper(poleStep); }
  else                   { digitalWrite(Pin1,LOW);digitalWrite(Pin2,LOW);
                           digitalWrite(Pin3,LOW);digitalWrite(Pin4,LOW); }

  // TILT movement
  if     (tiltState == 1) { poleStep2 = (poleStep2+1)%8; driveStepper2(poleStep2); }
  else if(tiltState == 2) { poleStep2 = (poleStep2+7)%8; driveStepper2(poleStep2); }
  else                    { digitalWrite(Pin5,LOW);digitalWrite(Pin6,LOW);
                           digitalWrite(Pin7,LOW);digitalWrite(Pin8,LOW); }

  delay(5);
}
