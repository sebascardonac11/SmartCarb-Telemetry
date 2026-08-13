#include "AdsChtSensor.h"
#include "Config.h"

AdsChtSensor::AdsChtSensor(Ads1115Bus &bus, uint8_t channel)
    : _bus(bus), _channel(channel) {}

bool AdsChtSensor::probe()
{
    if (!_bus.isAvailable()) return false;

    bool ok = false;
    for (uint8_t attempt = 0; attempt < 3 && !ok; attempt++)
    {
        if (attempt > 0) delay(50);
        readTemperature();
        ok = (_status == 0);
    }
    return ok;
}

/**
 * Lee el divisor NTC con sobremuestreo + filtro de media movil (mismo
 * esquema que AdsLambdaSensor) y convierte el voltaje filtrado a °C con la
 * ecuacion Beta del NTC.
 */
float AdsChtSensor::readTemperature()
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
    float voltage = sum / _samplesFilled;

    float tempC = voltageToCelsius(voltage);

    // Rango fisico plausible para CHT (igual criterio que TemperatureNTC):
    // fuera de rango => dato descartado (NTC desconectado o en corto).
    if (tempC < -40.0f || tempC > 300.0f)
    {
        Serial.printf("[CHT] Dato fuera de rango descartado: %.1f C (V=%.3f)\n", tempC, voltage);
        _status = -3;
        return -1.0f;
    }

    _temperature          = tempC;
    _lastValidTemperature = tempC;
    _status                = 0;
    return tempC;
}

/**
 * Retorna la ultima temperatura filtrada. Si la ultima lectura fallo,
 * retorna la ultima valida conocida (igual que TemperatureNTC::getTemperature()).
 */
float AdsChtSensor::getTemperature()
{
    if (_status == 0) return _temperature;
    if (_lastValidTemperature > -9999.0f) return _lastValidTemperature;
    return (float)_status;
}

bool AdsChtSensor::isReady()
{
    return _status == 0;
}

/**
 * Ecuacion Beta para NTC en divisor resistivo (pull-up NTC_PULLUP_OHM a
 * NTC_VCC, NTC a GND, nodo intermedio leido por el ADS1115):
 *   R_ntc = R_pullup * V / (Vcc - V)
 *   1/T(K) = 1/T0 + (1/Beta) * ln(R_ntc / R25)
 * Constantes en Config.h — valores genericos de NTC automotriz (10k@25C,
 * Beta 3950) a calibrar con el sensor de culata real.
 */
float AdsChtSensor::voltageToCelsius(float voltage) const
{
    float denom = NTC_VCC - voltage;
    if (denom <= 0.0001f) denom = 0.0001f; // evita division por cero si el ADC satura

    float rNtc = NTC_PULLUP_OHM * (voltage / denom);

    const float t0Kelvin = 298.15f; // 25 °C
    float tempKelvin = 1.0f / ((1.0f / t0Kelvin) + (1.0f / NTC_BETA) * log(rNtc / NTC_R25_OHM));

    return tempKelvin - 273.15f;
}
