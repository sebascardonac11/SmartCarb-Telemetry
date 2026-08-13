#ifndef LAMBDA_CANBUS_H
#define LAMBDA_CANBUS_H

#include <Arduino.h>

/**
 * Datos de la sonda lambda narrowband + estado del calentador, compartidos
 * entre el loop principal (lectura) y la tarea de envio CAN (otro nucleo).
 */
struct LambdaCanFrame
{
    float   voltage      = 0.0f;  // voltaje de la sonda tras el filtro RC + ADS1115 (V)
    uint8_t mixtureState = 0;     // AdsLambdaSensor::MixtureState (0=Rico,1=Estequiometrica,2=Pobre)
    uint8_t heaterDuty   = 0;     // % de potencia del calentador
    uint8_t heaterState  = 0;     // LambdaHeaterControl::State
    bool    sensorOk     = false; // true si el ADS1115 responde y la lectura es valida
};

/**
 * Envio por TWAI (CAN) de la trama de la sonda lambda narrowband.
 * Misma configuracion de driver que AimCanBus (arduino/ESP32/src/modules en
 * el repo Telemetria): TWAI_MODE_NORMAL, 500 kbit/s, acepta todos los filtros.
 *
 * El envio corre en su propia tarea FreeRTOS, fijada a un nucleo distinto del
 * loop principal, para no bloquear el muestreo de la sonda ni la maquina de
 * estados del calentador con la latencia de twai_transmit().
 *
 * Trama CAN (ID configurable, ver Config.h CAN_LAMBDA_MSG_ID):
 *   byte 0-1 : voltaje sonda x1000     (uint16, big-endian, mV)
 *   byte 2   : estado de mezcla         (uint8, 0=Rico 1=Estequiometrica 2=Pobre)
 *   byte 3   : duty calentador          (uint8, 0-100 %)
 *   byte 4   : estado calentador        (uint8, LambdaHeaterControl::State)
 *   byte 5   : sensorOk                 (uint8, 0/1)
 */
class LambdaCanBus
{
public:
    LambdaCanBus(int txPin, int rxPin, uint32_t canId);

    void begin();
    void sendFrame(const LambdaCanFrame &frame);

    // Actualiza (con proteccion por mutex) el ultimo dato disponible para la
    // tarea de envio periodico.
    void updateFrame(const LambdaCanFrame &frame);

    // Crea la tarea FreeRTOS que envia periodicamente por CAN el ultimo dato
    // actualizado via updateFrame(), fijada al nucleo indicado.
    void startTask(int core, uint32_t periodMs, UBaseType_t priority = 1, uint32_t stackSize = 4096);

private:
    int      _txPin;
    int      _rxPin;
    uint32_t _canId;
    uint32_t _periodMs = 100;

    LambdaCanFrame     _sharedFrame;
    SemaphoreHandle_t  _mutex;

    static void taskEntry(void *param);
    void taskLoop();
};

#endif
