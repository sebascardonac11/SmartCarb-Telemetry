#include "LambdaCanBus.h"
#include "driver/twai.h"

LambdaCanBus::LambdaCanBus(int txPin, int rxPin, uint32_t lambdaCanId, uint32_t chtCanId)
    : _txPin(txPin), _rxPin(rxPin), _lambdaCanId(lambdaCanId), _chtCanId(chtCanId)
{
    _mutex = xSemaphoreCreateMutex();
}

void LambdaCanBus::begin()
{
    twai_general_config_t g_config =
        TWAI_GENERAL_CONFIG_DEFAULT((gpio_num_t)_txPin, (gpio_num_t)_rxPin, TWAI_MODE_NORMAL);
    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    if (twai_driver_install(&g_config, &t_config, &f_config) == ESP_OK)
    {
        Serial.println("[CAN] TWAI instalado");
    }
    if (twai_start() == ESP_OK)
    {
        Serial.println("[CAN] TWAI iniciado");
    }
}

void LambdaCanBus::updateFrame(const LambdaCanFrame &frame)
{
    if (xSemaphoreTake(_mutex, pdMS_TO_TICKS(20)) == pdTRUE)
    {
        _sharedFrame = frame;
        xSemaphoreGive(_mutex);
    }
}

void LambdaCanBus::sendFrame(const LambdaCanFrame &frame)
{
    uint16_t mv = (uint16_t)(frame.voltage * 1000.0f);

    twai_message_t message = {};
    message.identifier = _lambdaCanId;
    message.flags = TWAI_MSG_FLAG_NONE;
    message.data_length_code = 6;
    message.data[0] = (uint8_t)(mv >> 8);
    message.data[1] = (uint8_t)(mv & 0xFF);
    message.data[2] = frame.mixtureState;
    message.data[3] = frame.heaterDuty;
    message.data[4] = frame.heaterState;
    message.data[5] = frame.sensorOk ? 1 : 0;

    if (twai_transmit(&message, pdMS_TO_TICKS(10)) != ESP_OK)
    {
        Serial.println("[CAN] Error al enviar frame de lambda");
    }

    sendChtFrame(frame);
}

/**
 * Reproduce exactamente el formato de arduino/datalogger/src/main.cpp
 * (canSend()) en Telemetria, para que AimCanBus en el ESP32 principal la
 * reciba sin ningun cambio: RPM/Marcha/TPS en 0 (este nodo no los mide) y
 * CHT×10 en bytes 4-5.
 */
void LambdaCanBus::sendChtFrame(const LambdaCanFrame &frame)
{
    if (!frame.chtOk) return;

    int16_t chtRaw = (int16_t)(frame.cht * 10.0f);

    twai_message_t message = {};
    message.identifier = _chtCanId;
    message.flags = TWAI_MSG_FLAG_NONE;
    message.data_length_code = 6;
    message.data[0] = 0; // RPM bajo
    message.data[1] = 0; // RPM alto
    message.data[2] = 0; // Marcha
    message.data[3] = 0; // TPS
    message.data[4] = (uint8_t)(chtRaw & 0xFF);
    message.data[5] = (uint8_t)(chtRaw >> 8);

    if (twai_transmit(&message, pdMS_TO_TICKS(10)) != ESP_OK)
    {
        Serial.println("[CAN] Error al enviar frame de CHT");
    }
}

void LambdaCanBus::startTask(int core, uint32_t periodMs, UBaseType_t priority, uint32_t stackSize)
{
    _periodMs = periodMs;
    xTaskCreatePinnedToCore(taskEntry, "LambdaCanTask", stackSize, this, priority, nullptr, core);
}

void LambdaCanBus::taskEntry(void *param)
{
    static_cast<LambdaCanBus *>(param)->taskLoop();
}

void LambdaCanBus::taskLoop()
{
    for (;;)
    {
        LambdaCanFrame frame;
        if (xSemaphoreTake(_mutex, pdMS_TO_TICKS(20)) == pdTRUE)
        {
            frame = _sharedFrame;
            xSemaphoreGive(_mutex);
        }
        sendFrame(frame);
        vTaskDelay(pdMS_TO_TICKS(_periodMs));
    }
}
