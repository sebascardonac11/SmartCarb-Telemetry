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
    bool    sensorOk     = false; // true si el ADS1115 responde y la lectura de lambda es valida
    float   cht          = 0.0f;  // temperatura de culata (°C), via NTC en AIN1
    bool    chtOk        = false; // true si la lectura de CHT es valida
};

/**
 * Envio por TWAI (CAN) de la sonda lambda narrowband + CHT.
 * Misma configuracion de driver que AimCanBus (arduino/ESP32/src/modules en
 * el repo Telemetria): TWAI_MODE_NORMAL, 500 kbit/s, acepta todos los filtros.
 *
 * El envio corre en su propia tarea FreeRTOS, fijada a un nucleo distinto del
 * loop principal, para no bloquear el muestreo de los sensores ni la maquina
 * de estados del calentador con la latencia de twai_transmit(). Cada ciclo
 * envia DOS frames:
 *
 * 1) Trama propia de esta placa (ID lambdaCanId, ver Config.h CAN_LAMBDA_MSG_ID):
 *   byte 0-1 : voltaje sonda x1000     (uint16, big-endian, mV)
 *   byte 2   : estado de mezcla         (uint8, 0=Rico 1=Estequiometrica 2=Pobre)
 *   byte 3   : duty calentador          (uint8, 0-100 %)
 *   byte 4   : estado calentador        (uint8, LambdaHeaterControl::State)
 *   byte 5   : sensorOk                 (uint8, 0/1)
 *
 * 2) Trama de CHT compatible con arduino/datalogger de Telemetria (ID chtCanId,
 *    ver Config.h CAN_CHT_MSG_ID = 0x200), MISMO formato que ya recibe AimCanBus
 *    en el ESP32 principal sin modificarlo:
 *   byte 0-1 : RPM     (0 en este nodo)
 *   byte 2   : Marcha  (0 en este nodo)
 *   byte 3   : TPS     (0 en este nodo)
 *   byte 4-5 : CHT×10  (int16, little-endian — igual que arduino/datalogger)
 */
class LambdaCanBus
{
public:
    LambdaCanBus(int txPin, int rxPin, uint32_t lambdaCanId, uint32_t chtCanId);

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
    uint32_t _lambdaCanId;
    uint32_t _chtCanId;
    uint32_t _periodMs = 100;

    LambdaCanFrame     _sharedFrame;
    SemaphoreHandle_t  _mutex;

    void sendChtFrame(const LambdaCanFrame &frame);
    static void taskEntry(void *param);
    void taskLoop();
};

#endif
