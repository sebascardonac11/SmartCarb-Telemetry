# SmartCarb-Telemetry
ESP32-S3 based engine tuning telemetry system for real-time AFR (Lambda), EGT, CHT, and TPS monitoring. Designed for carburetor calibration and ice-cool engine diagnostics.


# CarbTune-ESP32

![PlatformIO](https://img.shields.io/badge/PlatformIO-Compatible-orange.svg)
![Framework](https://img.shields.io/badge/Framework-Arduino__ESP32--S3-green.svg)
![License](https://img.shields.io/badge/license-MIT-blue.svg)

Sistema de telemetría y diagnóstico de bajo costo basado en el **ESP32-S3** para la puesta a punto y carburación de motores de alto rendimiento. Este dispositivo permite monitorear y correlacionar en tiempo real la apertura del acelerador (TPS) con el estado estequiométrico de la mezcla (Sonda Lambda Narrowband) y las temperaturas críticas de escape (EGT) y culata (CHT).

---

## 📊 Variables Monitoreadas y Sensores

| Parámetro | Sensor | Rango / Tipo | Propósito en la Carburación |
| :--- | :--- | :--- | :--- |
| **Mezcla (AFR)** | Sonda Lambda Narrowband | 0V - 1V (Analógico) | Identificar de forma cualitativa si el motor va **Rico** (>0.5V) o **Pobre** (<0.5V). |
| **Acelerador (TPS)** | Potenciómetro original o adaptado | 0% - 100% (Analógico) | Determinar exactamente qué circuito del carburador está actuando (bajas, medias o altas). |
| **Escape (EGT)** | Termopar Tipo K + MAX31855 | Hasta 1000 °C (SPI) | Detectar temperaturas excesivas por mezcla pobre o problemas de avance de encendido. |
| **Culata (CHT)** | Termopar Tipo K o Sensor NTC | Hasta 300 °C | Monitorear el estrés térmico general del cilindro y evitar agarrotamientos. |

---

## 🛠️ Características Principales

* **Filtraje Analógico:** Implementación de sobremuestreo (Oversampling) y filtros de media móvil en el ESP32-S3 para estabilizar las lecturas analógicas del TPS y la sonda Lambda.
* **Correlación TPS-Lambda:** Mapeo lógico para identificar si las fluctuaciones de la mezcla ocurren al abrir gas a fondo (circuito de alta/chiclé principal) o en ralentí/progresión.
* **Alertas Visuales:** Salidas digitales configurables para LEDs de advertencia en caso de picos peligrosos de EGT o CHT.
* **Arquitectura No Bloqueante:** Diseñado sobre PlatformIO estructurando el código de manera limpia para mantener un muestreo rápido sin interferir con la comunicación o pantallas.

---

## 📁 Estructura del Proyecto

```text
├── platformio.ini
├── include/
│   └── Config.h              # Definición de pines (GPIO) y parámetros de comunicación
├── lib/
│   └── Lambda_NB/
│       ├── AdsLambdaSensor.h/.cpp     # Lectura I2C de la sonda vía ADS1115 (oversampling + media móvil)
│       ├── LambdaHeaterControl.h/.cpp # Calentamiento de la sonda narrowband (protocolo Bosch)
│       └── LambdaCanBus.h/.cpp        # Envío de la trama por TWAI (CAN), en su propio núcleo
└── src/
    └── main.cpp             # Inicialización de periféricos, loop de lectura y arranque de la tarea CAN
```

> `Max31855_EGT/` y `TPS_Calibrate/` (EGT y TPS) siguen siendo trabajo futuro — no forman parte de esta versión, centrada en la sonda lambda narrowband, su calentador y el envío por CAN bus.

---

## 🔌 Pinout (ESP32-S3, v1.0)

| Función | Pin(es) | Notas |
| :--- | :--- | :--- |
| I2C SDA (ADS1115) | GPIO 5 | Lectura amplificada/filtrada (1kΩ + 100nF) de la sonda lambda narrowband |
| I2C SCL (ADS1115) | GPIO 6 | |
| TWAI CAN TX | GPIO 47 | Transceptor WCMCU230, 500 kbit/s |
| TWAI CAN RX | GPIO 48 | |
| Control calentador (PWM → Gate MOSFET) | GPIO 7 | Resistencia de gate 100Ω, MOSFET conmuta el calentador de la sonda a GND |

Trama CAN enviada (ID `0x210`, no colisiona con `0x100`/`0x200` usados en el repo `Telemetria`):

```
Byte 0-1 : Voltaje sonda × 1000   (uint16, big-endian, mV)
Byte 2   : Estado de mezcla       (uint8, 0=Rico 1=Estequiométrica 2=Pobre)
Byte 3   : Duty calentador        (uint8, 0-100 %)
Byte 4   : Estado calentador      (uint8, 0=OFF 1=Anticondensación 2=Rampa 3=Estable)
Byte 5   : Sensor OK              (uint8, 0/1)
```

### Arquitectura de tareas

- **`loop()` (núcleo 1, por defecto de Arduino):** lee el ADS1115 y avanza la máquina de estados del calentador.
- **Tarea `LambdaCanTask` (núcleo 0):** toma el último dato (protegido por mutex) y lo transmite por TWAI cada `CAN_SEND_PERIOD_MS`, sin bloquear el muestreo de la sonda.

### Calentamiento de la sonda (Bosch LSF 4.2)

Sigue el criterio general de Bosch para calentadores de sondas lambda: una fase de protección anti-condensación a baja potencia, seguida de una rampa progresiva hasta la potencia de régimen, para evitar el choque térmico del elemento cerámico. Al no contar este hardware con sensado de corriente/resistencia del calentador, el control es en lazo abierto (por tiempo); una regulación en lazo cerrado por temperatura real requeriría instrumentar esa rama con un sensor de corriente. Ver comentarios en `lib/Lambda_NB/LambdaHeaterControl.h`.
