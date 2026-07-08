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
├── include/
│   ├── Config.h         # Definición de pines (GPIO), calibración analógica y umbrales
│   └── Filters.h        # Algoritmos de filtrado para señales analógicas ruidosas
├── lib/
│   ├── Max31855_EGT/    # Librería/Clase para la lectura del termopar de escape vía SPI
│   ├── Lambda_NB/       # Lógica de conversión de Voltaje a Estado (Rico/Estequiométrico/Pobre)
│   └── TPS_Calibrate/   # Rutina de calibración para el mapeo de 0% a 100% del acelerador
└── src/
    └── main.cpp         # Inicialización de periféricos y ciclo principal de telemetría

