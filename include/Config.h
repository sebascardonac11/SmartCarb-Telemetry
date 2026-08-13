#pragma once

#include <Arduino.h>

// ── I2C — ADS1115 compartido (sonda lambda + NTC de culata) ────────────────
constexpr int     I2C_SDA_PIN      = 5;
constexpr int     I2C_SCL_PIN      = 6;
constexpr uint8_t ADS1115_I2C_ADDR = 0x48; // ADDR a GND (direccion por defecto)

// Canales del ADS1115 (single-ended)
constexpr uint8_t ADS1115_CH_LAMBDA = 0; // AIN0: sonda lambda narrowband (filtro RC 1k+100nF)
constexpr uint8_t ADS1115_CH_CHT    = 1; // AIN1: divisor NTC de culata

// NTC de culata (divisor: NTC_PULLUP_OHM a NTC_VCC, NTC a GND, nodo -> AIN1).
// Valores genericos de NTC automotriz — CALIBRAR con el sensor real, el repo
// Telemetria no usa ADS1115 para CHT (ver AdsChtSensor.h para el detalle).
constexpr float NTC_VCC         = 3.3f;
constexpr float NTC_PULLUP_OHM  = 10000.0f;
constexpr float NTC_R25_OHM     = 10000.0f; // resistencia nominal a 25 °C
constexpr float NTC_BETA        = 3950.0f;  // coeficiente Beta (25/85 °C)

// ── TWAI / CAN bus ──────────────────────────────────────────────────────────
constexpr int      TWAI_TX_PIN        = 47;
constexpr int      TWAI_RX_PIN        = 48;
// ID propio para este nodo (sonda lambda narrowband + calentador). No colisiona
// con los IDs ya usados en el repo Telemetria: 0x100 (CHT hacia AIM) y 0x200
// (RPM/marcha/TPS/CHT, recibido por AimCanBus en el ESP32 principal).
constexpr uint32_t CAN_LAMBDA_MSG_ID  = 0x210;
// Frame de CHT en el MISMO formato que ya usa arduino/datalogger en Telemetria
// (RPM=0, Marcha=0, TPS=0, CHT×10 en bytes 4-5) para que AimCanBus lo reciba
// sin cambios, con esta placa como fuente de CHT via ADS1115+NTC.
constexpr uint32_t CAN_CHT_MSG_ID     = 0x200;
constexpr uint32_t CAN_SEND_PERIOD_MS = 100; // 10 Hz

// ── Control del calentador de la sonda lambda narrowband (Bosch LSF 4.2) ───
// PWM via analogWrite() (abstrae LEDC internamente, resolucion 8 bits fija).
constexpr int LAMBDA_HEATER_PWM_PIN = 7;

// ── Nucleos FreeRTOS ─────────────────────────────────────────────────────────
// loop() de Arduino corre por defecto en el nucleo 1 (lectura de sensores /
// maquina de estados del calentador). El envio por CAN corre en su propia
// tarea, fijada al nucleo 0, para no bloquear el muestreo de la sonda.
constexpr int CAN_TASK_CORE = 0;
