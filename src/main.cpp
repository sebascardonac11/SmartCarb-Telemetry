#include <Arduino.h>
#include <Wire.h>

#include "Config.h"
#include "AdsLambdaSensor.h"
#include "LambdaHeaterControl.h"
#include "LambdaCanBus.h"

AdsLambdaSensor lambdaSensor(ADS1115_I2C_ADDR);

LambdaHeaterControl heater(LAMBDA_HEATER_PWM_PIN);

LambdaCanBus canBus(TWAI_TX_PIN, TWAI_RX_PIN, CAN_LAMBDA_MSG_ID);

void setup()
{
    Serial.begin(115200);

    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    lambdaSensor.begin(Wire);

    heater.begin();
    heater.start();

    canBus.begin();
    // Tarea de envio CAN en su propio nucleo: loop() de Arduino corre en el
    // nucleo 1 por defecto (lectura de sonda + maquina de estados del
    // calentador); el envio CAN corre en el nucleo 0 (Config.h: CAN_TASK_CORE).
    canBus.startTask(CAN_TASK_CORE, CAN_SEND_PERIOD_MS);
}

void loop()
{
    lambdaSensor.readVoltage();
    heater.update();

    LambdaCanFrame frame;
    frame.voltage      = lambdaSensor.getVoltage();
    frame.mixtureState = (uint8_t)lambdaSensor.getMixtureState();
    frame.heaterDuty   = heater.getDutyPercent();
    frame.heaterState  = (uint8_t)heater.getState();
    frame.sensorOk     = lambdaSensor.isReady();

    canBus.updateFrame(frame);

    delay(20); // ~50 Hz de muestreo de la sonda lambda
}
