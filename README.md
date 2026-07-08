# SmartCarb-Telemetry
ESP32-S3 based engine tuning telemetry system for real-time AFR (Lambda), EGT, CHT, and TPS monitoring. Designed for carburetor calibration and ice-cool engine diagnostics.

Es un sistema de telemetría de código abierto basado en el ESP32-S3 diseñado específicamente para la puesta a punto de motores carburados de alto rendimiento. 
Al cruzar los datos de apertura del acelerador con la riqueza de la mezcla y las temperaturas críticas de escape y culata, el dispositivo permite mapear con precisión quirúrgica el comportamiento del motor en cada circuito.

Variables Monitoreadas
Sensor  Parámetro  Propósito en la Carburación
Sonda Lambda Wideband  Mezcla Aire/Combustible (AFR)  Saber con exactitud si el motor va rico o pobre en tiempo real.
TPS (Potenciómetro)  Posición del Acelerador (%)Identificar qué circuito del carburador está actuando (bajas, medias o altas).
EGT (Termopar K)Temperatura de Gases de EscapeDetectar detonación inminente o exceso de avance/retraso de encendido.
CHT (Termopar/NTC)Temperatura de CulataMonitorear el estrés térmico general del cilindro.

Características Principales
Muestreo de Alta Velocidad: Lectura y correlación instantánea entre la posición del TPS y el valor Lambda.
Alertas Térmicas: LEDs de advertencia configurables si la EGT o la CHT superan los límites de seguridad.
Diseño Modular: Desarrollado en PlatformIO utilizando una arquitectura orientada a objetos para facilitar la adición de pantallas (OLED/TFT) o almacenamiento SD.
Conectividad: Preparado para transmitir datos vía Wi-Fi/Bluetooth a una interfaz gráfica o enlazarse con el Laptimer principal.
