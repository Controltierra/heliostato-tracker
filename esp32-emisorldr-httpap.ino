/* 
  ESP32 “Sensor” – Lectura y envío de LDR
  --------------------------------------
  Proyecto: Heliostato – Sensor de Luz
  Archivo:  sensor_ldr.ino
  Versión:  1.2
  Fecha:    2025-06-18
  Autor:    Tu Nombre / @TuUsuario
  Placa:    Wemos Lolin32 Lite V3 (TP4056 Li-Po) / ESP32-DevKitC V4 CP2102 USB-C
  IDE:      Arduino IDE 2.x
  Librerías:
    • WiFi.h
    • HTTPClient.h

  Pines ADC (GPIO1..39 en ADC1):
    UL (Up-Left)    → GPIO34 (ADC1_CH6)
    UR (Up-Right)   → GPIO33 (ADC1_CH5)
    DL (Down-Left)  → GPIO32 (ADC1_CH4)
    DR (Down-Right) → GPIO35 (ADC1_CH7)

  Constantes configurables:
    INTERVAL = 200  // ms entre envíos
    ssid      = "Heliostato_AP"
    password  = "12345678"

  Descripción:
    • Se conecta en modo STA al AP “Heliostato_AP” (192.168.4.1).  
    • Cada INTERVAL ms lee los 4 LDR y envía:
        GET /ldr?ul=<ul>&ur=<ur>&dl=<dl>&dr=<dr>
      al actuador.  
    • Reintenta conexión Wi-Fi automáticamente si se cae.  
    • Imprime en Serie (115200 bps) URL y código HTTP de respuesta.  

  Notas:
    • Ajusta INTERVAL si la red se satura.  
    • Para debug rápido, añade tests de conectividad o fallback a MQTT/OTA.
*/

#include <WiFi.h>
#include <HTTPClient.h>

// Credenciales del AP del actuador
const char* ssid     = "Heliostato_AP";
const char* password = "12345678";

// Pines ADC para tus LDR
const int pinUL = 34;
const int pinUR = 33;
const int pinDL = 32;
const int pinDR = 35;

// Intervalo envío ms
const unsigned long interval = 200;
unsigned long lastSend = 0;

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.print("Conectando a AP ");

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  WiFi.setAutoReconnect(true);

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
    Serial.print(".");
    delay(500);
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConectado! IP: " + WiFi.localIP().toString());
  } else {
    Serial.println("\nError conexión inicial");
  }
}

void loop() {
  // Reintento de WiFi si se pierde
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi desconectado, reconectando...");
    WiFi.reconnect();
    unsigned long t0 = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - t0 < 5000) {
      Serial.print(".");
      delay(500);
    }
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\nReconectado! IP: " + WiFi.localIP().toString());
    } else {
      Serial.println("\nFallo reconexión");
      delay(1000);
      return;  // no envía hasta reconectar
    }
  }

  unsigned long now = millis();
  if (now - lastSend < interval) return;
  lastSend = now;

  // Lectura LDR
  int ul = analogRead(pinUL);
  int ur = analogRead(pinUR);
  int dl = analogRead(pinDL);
  int dr = analogRead(pinDR);

  // Construye URL
  String url = String("http://") + WiFi.gatewayIP().toString()
             + "/ldr?ul=" + ul
             + "&ur=" + ur
             + "&dl=" + dl
             + "&dr=" + dr;

  HTTPClient http;
  http.begin(url);
  int code = http.GET();
  Serial.printf("GET %s -> %d\n", url.c_str(), code);
  http.end();
}
