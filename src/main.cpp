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

LambdaCanBus canBus(TWAI_TX_PIN, TWAI_RX_PIN, CAN_LAMBDA_MSG_ID, CAN_CHT_MSG_ID, CAN_NO_ACK_MODE);

constexpr unsigned long SERIAL_PRINT_PERIOD_MS = 500; // 2 Hz, para no saturar el puerto
unsigned long lastSerialPrintMs = 0;

const char *mixtureStateToStr(AdsLambdaSensor::MixtureState state)
{
    switch (state)
    {
        case AdsLambdaSensor::MixtureState::RICH:   return "RICA";
        case AdsLambdaSensor::MixtureState::LEAN:   return "POBRE";
        case AdsLambdaSensor::MixtureState::STOICH: default: return "ESTEQUIOMETRICA";
    }
}

const char *heaterStateToStr(LambdaHeaterControl::State state)
{
    switch (state)
    {
        case LambdaHeaterControl::State::OFF:          return "OFF";
        case LambdaHeaterControl::State::COND_PROTECT: return "ANTICONDENSACION";
        case LambdaHeaterControl::State::WARMUP:       return "CALENTANDO";
        case LambdaHeaterControl::State::READY:        default: return "LISTO";
    }
}

void printSensorInfo()
{
    Serial.print("Lambda: ");
    Serial.print(lambdaSensor.getVoltage(), 3);
    Serial.print(" V (");
    Serial.print(mixtureStateToStr(lambdaSensor.getMixtureState()));
    Serial.print(") ");
    Serial.print(lambdaSensor.isReady() ? "OK" : "FALLO");

    Serial.print(" | CHT: ");
    Serial.print(chtSensor.getTemperature(), 1);
    Serial.print(" C ");
    Serial.print(chtSensor.isReady() ? "OK" : "FALLO");

    Serial.print(" | Calentador: ");
    Serial.print(heaterStateToStr(heater.getState()));
    Serial.print(" duty=");
    Serial.print(heater.getDutyPercent());
    Serial.println("%");
}

void scanI2CBus()
{
    Serial.println("[I2C] Escaneando bus...");
    uint8_t found = 0;
    for (uint8_t addr = 1; addr < 127; addr++)
    {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0)
        {
            Serial.print("[I2C] Dispositivo encontrado en 0x");
            Serial.println(addr, HEX);
            found++;
        }
    }
    if (found == 0) Serial.println("[I2C] Ningun dispositivo respondio");
}

void setup()
{
    Serial.begin(115200);

    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    scanI2CBus();
    adsBus.begin(Wire);

    heater.begin();
    heater.start();

    if (CAN_BUS_ENABLED)
    {
        canBus.begin();
        // Tarea de envio CAN en su propio nucleo: loop() de Arduino corre en el
        // nucleo 1 por defecto (lectura de sensores + maquina de estados del
        // calentador); el envio CAN corre en el nucleo 0 (Config.h: CAN_TASK_CORE).
        canBus.startTask(CAN_TASK_CORE, CAN_SEND_PERIOD_MS);
    }
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

    unsigned long now = millis();
    if (now - lastSerialPrintMs >= SERIAL_PRINT_PERIOD_MS)
    {
        lastSerialPrintMs = now;
        printSensorInfo();
    }

    delay(20); // ~50 Hz de muestreo (sonda lambda + NTC de culata)
}
