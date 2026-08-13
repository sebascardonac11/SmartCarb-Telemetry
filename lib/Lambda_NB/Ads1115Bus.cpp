#include "Ads1115Bus.h"

Ads1115Bus::Ads1115Bus(uint8_t i2cAddress)
    : _i2cAddress(i2cAddress) {}

/**
 * Inicializa el ADS1115 en el bus I2C indicado (SDA=5 SCL=6, ver Config.h).
 * Ganancia GAIN_ONE => rango +-4.096V, ~0.125mV/bit: suficiente resolucion
 * tanto para la señal 0-1V de la sonda lambda como para el divisor del NTC
 * de culata (0-3.3V).
 */
bool Ads1115Bus::begin(TwoWire &wire)
{
    _available = _ads.begin(_i2cAddress, &wire);
    if (_available)
    {
        _ads.setGain(GAIN_ONE);
    }
    else
    {
        Serial.println("[ADS1115] No responde en el bus I2C");
    }
    return _available;
}
