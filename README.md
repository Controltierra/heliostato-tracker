# Heliostato Pan/Tilt con ESP32

Control de un espejo heliostato sobre dos ejes (Pan/Tilt) usando dos ESP32:

- **ESP32 Sensor**: lee 4 LDR (esquinas UL/UR/DL/DR) y envía sus valores via HTTP.
- **ESP32 Actuador**: recibe lecturas, calcula seguimiento de luz, maneja dos motores paso-a-paso y un RTC DS3231, y expone una interfaz web para control manual y auto.

---

## 📋 Características

- **Seguimiento automático de luz**  
  • Lectura de 4 LDR (arriba-izq, arriba-der, abajo-izq, abajo-der).  
  • Lógica invertida: iluminar mueve en sentido opuesto para agarrar el máximo de luz.  
  • Suavizado exponencial y velocidad proporcional a la diferencia de iluminación.  
  • Timeout si no llegan datos en >1.5 s.  

- **Control manual**  
  • Botones en la web para mover Pan (← →) y Tilt (↑ ↓).  
  • Prioridad manual sobre seguimiento.

- **RTC DS3231**  
  • Lectura continua de hora.  
  • Botón “Sincronizar RTC” ajusta el reloj a la hora del navegador.  

- **Interfaz web**  
  • Modo AP Wi-Fi (`Heliostato_AP`, IP `192.168.4.1`).  
  • Página responsive para móvil y desktop.  
  • Estado “Sensor conectado/desconectado”, botones Start/Stop Auto, sincronización de RTC y control manual.

---

## 🛠️ Requisitos de hardware

### ESP32 Sensor

- **Placa**: Wemos Lolin32 Lite V3 / ESP32-DevKitC V4 CP2102 USB-C  
- **LDR x4**  
- **Resistencias** 10 kΩ (pull-down)  
- **Conexión**:
  | LDR       | GPIO  |
  |-----------|-------|
  | UL (arr-izq)  | 34    |
  | UR (arr-der)  | 33    |
  | DL (ab-izq)   | 32    |
  | DR (ab-der)   | 35    |

### ESP32 Actuador

- **Placa**: DFRobot FireBeetle ESP32 / Wemos Lolin32 Lite V3 / ESP32-DevKitC V4  
- **Motores**: 2× 28BYJ-48 + 2× ULN2003 driver  
- **RTC**: DS3231 (I²C)  
- **Cables y alimentación**  
- **Conexiones**:

  **Motores PAN**  
  | IN1 | IN2 | IN3 | IN4 |
  |-----|-----|-----|-----|
  | 13  | 12  | 14  | 27  |

  **Motores TILT**  
  | IN1 | IN2 | IN3 | IN4 |
  |-----|-----|-----|-----|
  | 26  | 25  | 33  | 32  |

  **RTC DS3231**  
  | SDA | SCL |
  |-----|-----|
  | 21  | 22  |

---

## 💾 Software

- **Arduino IDE** 1.8.x o 2.x  
- **Librerías**:
  - WiFi.h  
  - HTTPClient.h  
  - WebServer.h  
  - Wire.h  
  - RTClib.h  

---

## 🚀 Instalación

1. Clona este repositorio:
   ```bash
   git clone https://github.com/tuusuario/heliostato-esp32.git
   cd heliostato-esp32
