#ifndef ADS_LAMBDA_SENSOR_H
#define ADS_LAMBDA_SENSOR_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_ADS1X15.h>

/**
 * Lectura de la sonda lambda narrowband (Bosch LSF 4.2) a traves del ADS1115.
 * La señal de la sonda pasa antes por un filtro RC (1k + 100nF) hacia la
 * entrada AIN0 del ADS1115.
 *
 * Reutiliza el mismo patron que TemperatureNTC (arduino/ESP32/src/modules en
 * el repo Telemetria): probe()/isReady()/getters no bloqueantes que devuelven
 * el ultimo valor valido si la lectura actual falla. Ademas aplica el
 * sobremuestreo + filtro de media movil descrito en el README de este repo.
 */
class AdsLambdaSensor
{
public:
    enum class MixtureState : uint8_t { RICH = 0, STOICH = 1, LEAN = 2 };

    explicit AdsLambdaSensor(uint8_t i2cAddress = 0x48);

    bool  begin(TwoWire &wire = Wire);
    bool  probe();          // intenta una lectura; retorna true si el ADS1115 responde
    float readVoltage();    // lectura con sobremuestreo + filtro de media movil (V)
    float getVoltage();     // ultimo voltaje filtrado, o el ultimo valido si la lectura fallo
    bool  isReady();
    bool  isAvailable() { return _available; }

    MixtureState getMixtureState();

private:
    static constexpr uint8_t OVERSAMPLE_COUNT   = 8; // lecturas ADC promediadas por muestra
    static constexpr uint8_t MOVING_AVG_WINDOW  = 8; // muestras en el filtro de media movil
    static constexpr float   STOICH_LOW_V       = 0.45f; // < 0.45V => mezcla pobre
    static constexpr float   STOICH_HIGH_V      = 0.55f; // > 0.55V => mezcla rica

    Adafruit_ADS1115 _ads;
    uint8_t _i2cAddress;
    bool    _available = false;
    int     _status = -1; // -1: sin leer, 0: OK, -2: ADS1115 no responde en I2C

    float   _samples[MOVING_AVG_WINDOW] = {0};
    uint8_t _sampleIndex   = 0;
    uint8_t _samplesFilled = 0;

    float _voltage          = 0.0f;
    float _lastValidVoltage = -1.0f;
};

#endif
