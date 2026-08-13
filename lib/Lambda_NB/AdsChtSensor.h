#ifndef ADS_CHT_SENSOR_H
#define ADS_CHT_SENSOR_H

#include <Arduino.h>
#include "Ads1115Bus.h"

/**
 * Lectura de temperatura de culata (CHT) mediante un termistor NTC leido en
 * el canal AIN1 del mismo ADS1115 usado para la sonda lambda (AIN0, ver
 * AdsLambdaSensor / Ads1115Bus).
 *
 * El repo Telemetria no usa ADS1115 para CHT: el ESP32 principal la lee via
 * un modulo NTC remoto por Modbus RS-485 (arduino/ESP32/src/modules/
 * TemperatureNTC.cpp) y el ESP32 "datalogger" secundario usa un termopar
 * MAX6675 (arduino/datalogger/src/main.cpp), enviando el resultado por CAN
 * con ID 0x200 en formato CHT×10 (int16). Esta clase reutiliza el mismo
 * patron no bloqueante que esos modulos (probe()/isReady()/ultimo valor
 * valido) y LambdaCanBus reusa ese mismo formato 0x200 para que esta placa
 * sea compatible con el receptor ya existente (AimCanBus) sin cambiar nada
 * del lado del ESP32 principal.
 *
 * Circuito supuesto (ver Config.h): NTC con pull-up a NTC_VCC, el nodo
 * intermedio del divisor va a AIN1. Los valores de NTC_BETA/NTC_R25_OHM/
 * NTC_PULLUP_OHM son un NTC generico automotriz (10k @ 25C, Beta 3950) —
 * ajustar en Config.h con los datos reales del sensor de culata usado.
 */
class AdsChtSensor
{
public:
    AdsChtSensor(Ads1115Bus &bus, uint8_t channel);

    bool  probe();          // intenta una lectura; retorna true si el ADS1115 responde
    float readTemperature(); // lectura con sobremuestreo + filtro de media movil (°C)
    float getTemperature();  // ultima temperatura filtrada, o la ultima valida si la lectura fallo
    bool  isReady();

private:
    static constexpr uint8_t OVERSAMPLE_COUNT  = 8; // lecturas ADC promediadas por muestra
    static constexpr uint8_t MOVING_AVG_WINDOW = 8; // muestras en el filtro de media movil

    Ads1115Bus &_bus;
    uint8_t _channel;
    int     _status = -1; // -1: sin leer, 0: OK, -2: ADS1115 no responde en I2C, -3: NTC fuera de rango

    float   _samples[MOVING_AVG_WINDOW] = {0};
    uint8_t _sampleIndex   = 0;
    uint8_t _samplesFilled = 0;

    float _temperature          = 0.0f;
    float _lastValidTemperature = -9999.0f;

    float voltageToCelsius(float voltage) const;
};

#endif
