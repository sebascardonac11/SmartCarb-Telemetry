#ifndef ADS1115_BUS_H
#define ADS1115_BUS_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_ADS1X15.h>

/**
 * Envoltorio del ADS1115 fisico compartido por los dos sensores de esta
 * placa: AIN0 = sonda lambda narrowband (AdsLambdaSensor), AIN1 = NTC de
 * culata (AdsChtSensor). El driver Adafruit solo debe inicializarse una vez
 * por chip, de ahi que ambos sensores tomen una referencia a este bus en
 * lugar de crear cada uno su propia instancia.
 */
class Ads1115Bus
{
public:
    explicit Ads1115Bus(uint8_t i2cAddress = 0x48);

    bool begin(TwoWire &wire = Wire);
    bool isAvailable() const { return _available; }
    Adafruit_ADS1115 &driver() { return _ads; }

private:
    Adafruit_ADS1115 _ads;
    uint8_t _i2cAddress;
    bool    _available = false;
};

#endif
