#include "LambdaHeaterControl.h"

LambdaHeaterControl::LambdaHeaterControl(int pwmPin)
    : _pwmPin(pwmPin) {}

void LambdaHeaterControl::begin()
{
    pinMode(_pwmPin, OUTPUT);
    applyDuty(0);
    _state = State::OFF;
}

void LambdaHeaterControl::start()
{
    if (_state != State::OFF) return;
    _state = State::COND_PROTECT;
    _stateStartMs = millis();
    Serial.println("[Heater] Iniciando secuencia (proteccion anti-condensacion)");
}

void LambdaHeaterControl::stop()
{
    _state = State::OFF;
    applyDuty(0);
}

/**
 * Avanza la maquina de estados COND_PROTECT -> WARMUP -> READY en funcion del
 * tiempo transcurrido desde el inicio de cada fase. Debe llamarse en cada
 * iteracion del loop principal (no bloqueante).
 */
void LambdaHeaterControl::update()
{
    unsigned long elapsed = millis() - _stateStartMs;

    switch (_state)
    {
    case State::OFF:
        applyDuty(0);
        return;

    case State::COND_PROTECT:
        applyDuty(COND_PROTECT_DUTY);
        if (elapsed >= COND_PROTECT_MS)
        {
            _state = State::WARMUP;
            _stateStartMs = millis();
            Serial.println("[Heater] Rampa de calentamiento");
        }
        break;

    case State::WARMUP:
    {
        float progress = (float)elapsed / (float)WARMUP_MS;
        if (progress >= 1.0f)
        {
            _state = State::READY;
            _stateStartMs = millis();
            Serial.println("[Heater] Sonda en regimen estable");
        }
        else
        {
            uint8_t duty = COND_PROTECT_DUTY +
                           (uint8_t)(progress * (READY_DUTY - COND_PROTECT_DUTY));
            applyDuty(duty);
        }
        break;
    }

    case State::READY:
        applyDuty(READY_DUTY);
        break;
    }
}

void LambdaHeaterControl::applyDuty(uint8_t percent)
{
    _dutyPercent = percent;
    uint32_t duty = ((uint32_t)PWM_MAX_DUTY * percent) / 100;
    analogWrite(_pwmPin, duty);
}
