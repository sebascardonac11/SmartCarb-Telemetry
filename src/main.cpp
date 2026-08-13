#include <Arduino.h>
#include <Wire.h>

#include "Config.h"
#include "Ads1115Bus.h"
#include "AdsLambdaSensor.h"
#include "AdsChtSensor.h"
#include "LambdaHeaterControl.h"
#include "LambdaCanBus.h"

Ads1115Bus adsBus(ADS1115_I2C_ADDR);
AdsLambdaSensor lambdaSensor(adsBus, ADS1115_CH_LAMBDA);
AdsChtSensor    chtSensor(adsBus, ADS1115_CH_CHT);

LambdaHeaterControl heater(LAMBDA_HEATER_PWM_PIN);

LambdaCanBus canBus(TWAI_TX_PIN, TWAI_RX_PIN, CAN_LAMBDA_MSG_ID, CAN_CHT_MSG_ID);

void setup()
{
    Serial.begin(115200);

    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    adsBus.begin(Wire);

    heater.begin();
    heater.start();

    canBus.begin();
    // Tarea de envio CAN en su propio nucleo: loop() de Arduino corre en el
    // nucleo 1 por defecto (lectura de sensores + maquina de estados del
    // calentador); el envio CAN corre en el nucleo 0 (Config.h: CAN_TASK_CORE).
    canBus.startTask(CAN_TASK_CORE, CAN_SEND_PERIOD_MS);
}

void loop()
{
    lambdaSensor.readVoltage();
    chtSensor.readTemperature();
    heater.update();

    LambdaCanFrame frame;
    frame.voltage      = lambdaSensor.getVoltage();
    frame.mixtureState = (uint8_t)lambdaSensor.getMixtureState();
    frame.heaterDuty   = heater.getDutyPercent();
    frame.heaterState  = (uint8_t)heater.getState();
    frame.sensorOk     = lambdaSensor.isReady();
    frame.cht          = chtSensor.getTemperature();
    frame.chtOk        = chtSensor.isReady();

    canBus.updateFrame(frame);

    delay(20); // ~50 Hz de muestreo (sonda lambda + NTC de culata)
}
