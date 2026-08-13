#pragma once

#include <Arduino.h>

// ── I2C — ADS1115 (lectura amplificada de la sonda lambda narrowband) ──────
constexpr int     I2C_SDA_PIN      = 5;
constexpr int     I2C_SCL_PIN      = 6;
constexpr uint8_t ADS1115_I2C_ADDR = 0x48; // ADDR a GND (direccion por defecto)

// ── TWAI / CAN bus ──────────────────────────────────────────────────────────
constexpr int      TWAI_TX_PIN        = 47;
constexpr int      TWAI_RX_PIN        = 48;
// ID propio para este nodo (sonda lambda narrowband). No colisiona con los IDs
// ya usados en el repo Telemetria: 0x100 (CHT hacia AIM) y 0x200 (RPM/marcha/TPS/CHT).
constexpr uint32_t CAN_LAMBDA_MSG_ID  = 0x210;
constexpr uint32_t CAN_SEND_PERIOD_MS = 100; // 10 Hz

// ── Control del calentador de la sonda lambda narrowband (Bosch LSF 4.2) ───
// PWM via analogWrite() (abstrae LEDC internamente, resolucion 8 bits fija).
constexpr int LAMBDA_HEATER_PWM_PIN = 7;

// ── Nucleos FreeRTOS ─────────────────────────────────────────────────────────
// loop() de Arduino corre por defecto en el nucleo 1 (lectura de sensores /
// maquina de estados del calentador). El envio por CAN corre en su propia
// tarea, fijada al nucleo 0, para no bloquear el muestreo de la sonda.
constexpr int CAN_TASK_CORE = 0;
