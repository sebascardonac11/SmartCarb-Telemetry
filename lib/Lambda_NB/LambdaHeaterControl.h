#ifndef LAMBDA_HEATER_CONTROL_H
#define LAMBDA_HEATER_CONTROL_H

#include <Arduino.h>

/**
 * Control del calentador de la sonda lambda narrowband Bosch LSF 4.2
 * (elemento ceramico ZrO2), gobernado por el MOSFET del esquematico
 * (Gate <- R100 <- GPIO7 PWM, Source -> GND, Drain -> calentador de la sonda).
 *
 * Sigue el criterio general de Bosch para el calentamiento de sondas lambda
 * calefactadas, pensado para evitar el "shock termico" del elemento ceramico:
 *
 *   1) COND_PROTECT — proteccion anti-condensacion: potencia reducida durante
 *      los primeros segundos para evaporar lentamente la humedad del escape
 *      antes de calentar a fondo (evita fisuras por choque termico).
 *   2) WARMUP        — rampa progresiva de potencia hasta el nivel objetivo,
 *      llevando el elemento ceramico a su temperatura de trabajo (~600C).
 *   3) READY          — regimen estable, manteniendo la potencia objetivo.
 *
 * IMPORTANTE: este hardware no mide corriente/resistencia del calentador, por
 * lo que el control es en lazo abierto (por tiempo), no en lazo cerrado por
 * temperatura real. Para una regulacion en lazo cerrado (como haria una ECU
 * con diagnóstico OBD) haria falta un sensor de corriente en la rama del
 * MOSFET para estimar la resistencia del calentador y, con ella, su
 * temperatura. Los tiempos/porcentajes por defecto son conservadores y deben
 * ajustarse con pruebas de banco sobre la sonda real.
 */
class LambdaHeaterControl
{
public:
    enum class State : uint8_t { OFF = 0, COND_PROTECT = 1, WARMUP = 2, READY = 3 };

    explicit LambdaHeaterControl(int pwmPin);

    void begin();     // configura el pin PWM (analogWrite) y deja el calentador apagado
    void start();     // dispara la secuencia de calentamiento (COND_PROTECT -> WARMUP -> READY)
    void stop();       // corta el calentador (duty 0) y vuelve a OFF
    void update();      // debe llamarse periodicamente desde loop(); avanza la maquina de estados

    State   getState() const { return _state; }
    uint8_t getDutyPercent() const { return _dutyPercent; }

private:
    // Tiempos y niveles de la rampa (ajustables segun calibracion de banco).
    static constexpr unsigned long COND_PROTECT_MS   = 8000;  // duracion fase anti-condensacion
    static constexpr uint8_t       COND_PROTECT_DUTY = 25;    // % de potencia durante la proteccion
    static constexpr unsigned long WARMUP_MS         = 12000; // duracion rampa hasta potencia objetivo
    static constexpr uint8_t       READY_DUTY        = 100;   // % de potencia en regimen estable

    static constexpr uint8_t PWM_MAX_DUTY = 255; // resolucion de analogWrite() en ESP32 (8 bits)

    int _pwmPin;

    State         _state = State::OFF;
    unsigned long _stateStartMs = 0;
    uint8_t       _dutyPercent = 0;

    void applyDuty(uint8_t percent);
};

#endif
