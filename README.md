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
│       ├── Ads1115Bus.h/.cpp          # Driver ADS1115 compartido (un solo begin() para ambos canales)
│       ├── AdsLambdaSensor.h/.cpp     # Lectura I2C de la sonda vía ADS1115 AIN0 (oversampling + media móvil)
│       ├── AdsChtSensor.h/.cpp        # Lectura de CHT vía NTC en ADS1115 AIN1 (oversampling + media móvil)
│       ├── LambdaHeaterControl.h/.cpp # Calentamiento de la sonda narrowband (protocolo Bosch)
│       └── LambdaCanBus.h/.cpp        # Envío de las tramas por TWAI (CAN), en su propio núcleo
└── src/
    └── main.cpp             # Inicialización de periféricos, loop de lectura y arranque de la tarea CAN
```

> `Max31855_EGT/` y `TPS_Calibrate/` (EGT y TPS) siguen siendo trabajo futuro — no forman parte de esta versión, centrada en la sonda lambda narrowband, su calentador, la CHT y el envío por CAN bus.

---

## 🔌 Pinout (ESP32-S3, v1.0)

| Función | Pin(es) | Notas |
| :--- | :--- | :--- |
| I2C SDA (ADS1115) | GPIO 5 | AIN0 = sonda lambda narrowband (filtro 1kΩ + 100nF); AIN1 = divisor NTC de culata (CHT) |
| I2C SCL (ADS1115) | GPIO 6 | |
| TWAI CAN TX | GPIO 47 | Transceptor WCMCU230, 500 kbit/s |
| TWAI CAN RX | GPIO 48 | |
| Control calentador (PWM → Gate MOSFET) | GPIO 7 | Resistencia de gate 100Ω, MOSFET conmuta el calentador de la sonda a GND |

Esta placa envía **dos** tramas CAN por ciclo:

**1) Sonda lambda + calentador** (ID `0x210`, propia de esta placa — no colisiona con `0x100`/`0x200` de `Telemetria`):

```
Byte 0-1 : Voltaje sonda × 1000   (uint16, big-endian, mV)
Byte 2   : Estado de mezcla       (uint8, 0=Rico 1=Estequiométrica 2=Pobre)
Byte 3   : Duty calentador        (uint8, 0-100 %)
Byte 4   : Estado calentador      (uint8, 0=OFF 1=Anticondensación 2=Rampa 3=Estable)
Byte 5   : Sensor OK              (uint8, 0/1)
```

**2) CHT** (ID `0x200`, **mismo formato que `arduino/datalogger`** en el repo `Telemetria` — así el ESP32 principal la recibe con `AimCanBus` sin ningún cambio):

```
Byte 0-1 : RPM     (0, este nodo no lo mide)
Byte 2   : Marcha  (0, este nodo no lo mide)
Byte 3   : TPS     (0, este nodo no lo mide)
Byte 4-5 : CHT × 10 (int16, little-endian)
```

### Arquitectura de tareas

- **`loop()` (núcleo 1, por defecto de Arduino):** lee el ADS1115 (sonda lambda + NTC de culata) y avanza la máquina de estados del calentador.
- **Tarea `LambdaCanTask` (núcleo 0):** toma el último dato (protegido por mutex) y transmite ambas tramas por TWAI cada `CAN_SEND_PERIOD_MS`, sin bloquear el muestreo de los sensores.

### Calentamiento de la sonda (Bosch LSF 4.2)

Sigue el criterio general de Bosch para calentadores de sondas lambda: una fase de protección anti-condensación a baja potencia, seguida de una rampa progresiva hasta la potencia de régimen, para evitar el choque térmico del elemento cerámico. Al no contar este hardware con sensado de corriente/resistencia del calentador, el control es en lazo abierto (por tiempo); una regulación en lazo cerrado por temperatura real requeriría instrumentar esa rama con un sensor de corriente. Ver comentarios en `lib/Lambda_NB/LambdaHeaterControl.h`.

### Medición de CHT vía ADS1115

El repo `Telemetria` **no** usa ADS1115 para CHT: el ESP32 principal la lee de un módulo NTC remoto por Modbus RS-485, y el ESP32 `datalogger` secundario usa un termopar MAX6675. Esta placa la mide con un NTC en un divisor resistivo conectado a `AIN1` del mismo ADS1115 (constantes `NTC_*` en `include/Config.h`: 10kΩ@25°C, Beta 3950 — valores genéricos que hay que calibrar con el sensor real) y la publica en el formato CAN `0x200` ya existente para que sea una fuente compatible/alternativa al MAX6675. Ver `lib/Lambda_NB/AdsChtSensor.h`.
