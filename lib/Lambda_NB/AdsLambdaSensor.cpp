#include "AdsLambdaSensor.h"

AdsLambdaSensor::AdsLambdaSensor(Ads1115Bus &bus, uint8_t channel)
    : _bus(bus), _channel(channel) {}

/**
 * Intenta una lectura de verificacion. Igual que TemperatureNTC::probe(),
 * reintenta antes de declarar el sensor no disponible.
 */
bool AdsLambdaSensor::probe()
{
    if (!_bus.isAvailable()) return false;

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
    if (!_bus.isAvailable())
    {
        _status = -2;
        return -2.0f;
    }

    int32_t acc = 0;
    for (uint8_t i = 0; i < OVERSAMPLE_COUNT; i++)
    {
        acc += _bus.driver().readADC_SingleEnded(_channel);
    }
    float rawAvg     = (float)acc / OVERSAMPLE_COUNT;
    float voltageNow = _bus.driver().computeVolts((int16_t)rawAvg);

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
