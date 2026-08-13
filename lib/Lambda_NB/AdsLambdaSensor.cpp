#include "AdsLambdaSensor.h"

AdsLambdaSensor::AdsLambdaSensor(uint8_t i2cAddress)
    : _i2cAddress(i2cAddress) {}

/**
 * Inicializa el ADS1115 en el bus I2C indicado (SDA=5 SCL=6, ver Config.h).
 * Ganancia GAIN_ONE => rango +-4.096V, ~0.125mV/bit, suficiente resolucion
 * para la señal 0-1V de la sonda narrowband ya amplificada/filtrada.
 */
bool AdsLambdaSensor::begin(TwoWire &wire)
{
    _available = _ads.begin(_i2cAddress, &wire);
    if (_available)
    {
        _ads.setGain(GAIN_ONE);
        _status = -1;
    }
    else
    {
        _status = -2;
        Serial.println("[Lambda] ADS1115 no responde en el bus I2C");
    }
    return _available;
}

/**
 * Intenta una lectura de verificacion. Igual que TemperatureNTC::probe(),
 * reintenta antes de declarar el sensor no disponible.
 */
bool AdsLambdaSensor::probe()
{
    if (!_available) return false;

    bool ok = false;
    for (uint8_t attempt = 0; attempt < 3 && !ok; attempt++)
    {
        if (attempt > 0) delay(50);
        readVoltage();
        ok = (_status == 0);
    }
    return ok;
}

/**
 * Lee la sonda con sobremuestreo (promedio de OVERSAMPLE_COUNT conversiones)
 * y aplica un filtro de media movil sobre las ultimas MOVING_AVG_WINDOW
 * muestras, tal como describe el README (oversampling + media movil) para
 * estabilizar la lectura analogica de la sonda lambda.
 */
float AdsLambdaSensor::readVoltage()
{
    if (!_available)
    {
        _status = -2;
        return -2.0f;
    }

    int32_t acc = 0;
    for (uint8_t i = 0; i < OVERSAMPLE_COUNT; i++)
    {
        acc += _ads.readADC_SingleEnded(0);
    }
    float rawAvg     = (float)acc / OVERSAMPLE_COUNT;
    float voltageNow = _ads.computeVolts((int16_t)rawAvg);

    _samples[_sampleIndex] = voltageNow;
    _sampleIndex = (_sampleIndex + 1) % MOVING_AVG_WINDOW;
    if (_samplesFilled < MOVING_AVG_WINDOW) _samplesFilled++;

    float sum = 0.0f;
    for (uint8_t i = 0; i < _samplesFilled; i++) sum += _samples[i];
    _voltage = sum / _samplesFilled;

    _lastValidVoltage = _voltage;
    _status = 0;
    return _voltage;
}

/**
 * Retorna el ultimo voltaje filtrado. Si la ultima lectura fallo, retorna el
 * ultimo valor valido conocido (igual que TemperatureNTC::getTemperature()).
 */
float AdsLambdaSensor::getVoltage()
{
    if (_status == 0) return _voltage;
    if (_lastValidVoltage >= 0.0f) return _lastValidVoltage;
    return (float)_status;
}

bool AdsLambdaSensor::isReady()
{
    return _status == 0;
}

/**
 * Clasifica la mezcla segun el voltaje de la sonda narrowband:
 * < 0.45V pobre, > 0.55V rica, entre ambos estequiometrica (ver README).
 */
AdsLambdaSensor::MixtureState AdsLambdaSensor::getMixtureState()
{
    float v = getVoltage();
    if (v > STOICH_HIGH_V) return MixtureState::RICH;
    if (v < STOICH_LOW_V)  return MixtureState::LEAN;
    return MixtureState::STOICH;
}
