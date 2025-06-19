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

# Calibración de Heliostato Pan/Tilt con ESP32

Este repositorio contiene el sketch para calibrar un heliostato de dos ejes (**Pan** y **Tilt**) usando un **ESP32**.

---

## 📋 Descripción

- **Homing automático**: cada eje gira lentamente hasta su final de carrera, rectifica para liberar el switch y añade un margen extra de **50 pasos**.
- **Calibración manual**: desde una sencilla **página web**, se desplazan bloques de **200 pasos** en sentido inverso al homing para medir el recorrido.
- **Límites lógicos**: impide superar **26 000 pasos** en Pan y **8 000 pasos** en Tilt.
- **Desenergizado de bobinas**: al detenerse, apaga los LEDs de los drivers ULN2003 para ahorrar energía y evitar calentamiento.

---

## 🛠️ Material

- **ESP32 DevKitC V4** (USB-C, CH340C o CP2102)  
- 2× **Drivers ULN2003** + motores **28BYJ-48**  
- 2× finales de carrera (switches de tope mecánico)  
- Cables y fuente de alimentación 5 V para los drivers

---

## 🔌 Conexiones (ESP32 GPIO)

| Función         | Pines ESP32                  |
|-----------------|------------------------------|
| **Pan Motor**   | coil A: GPIO13<br>coil B: 12<br>coil C: 14<br>coil D: 27 |
| **Tilt Motor**  | coil A: GPIO26<br>coil B: 25<br>coil C: 33<br>coil D: 32 |
| **Pan Endstop** | GPIO4 (INPUT_PULLUP, switch ↔ GND) |
| **Tilt Endstop**| GPIO5 (INPUT_PULLUP, switch ↔ GND) |

---

## 🚀 Uso

1. **Sube** el sketch al ESP32.
2. Conéctate a la red Wi-Fi **Heliostato_Calib** (sin contraseña adicional).
3. Abre en tu navegador `http://192.168.4.1`.
4. Pulsa **Pan +200** o **Tilt +200** para mover bloques de calibración.
5. Repite hasta alcanzar el tope mecánico y observa el contador.
6. Usa **Reset** para poner a cero y repetir.

---

## 🧠 Lógica de calibración

1. **Homing**  
   - Gira lentamente (delay 2 000 µs) hasta detectar el switch (nivel `LOW`).  
   - Rectifica hasta liberar (`HIGH`) y da **50 pasos** adicionales para margen.  
2. **Calibración manual**  
   - Cada pulsación en la web desplaza **200 pasos** en el sentido inverso al homing, con rápido pulso (delay 1 000 µs).  
   - Se detiene y desenergiza automáticamente al pulsar un nuevo switch o al llegar al límite lógico (26 000 / 8 000).  
3. **Desenergizado**  
   - Tras cada movimiento o al detenerse, todas las bobinas se apagan para apagar los LEDs del driver.

---


