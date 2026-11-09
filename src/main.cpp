#include <Arduino.h>
#include "hardware/HX711Adapter.h"
#include "hardware/HCSR04Adapter.h"
#include "hardware/CompartmentServo.h"
#include "network/MqttManager.h"
#include "domain/SmartBinFacade.h"

// --- CONFIGURACION DE RED ---
const char* WIFI_SSID = "Wokwi-GUEST";
const char* WIFI_PASS = "";

const char* MQTT_BROKER = "192.168.18.143";
const int   MQTT_PORT   = 1883;

// --- COORDENADAS GEOGRAFICAS DEL TACHO ---
const double LATITUD  = -12.0464; // Rango [-90, 90]
const double LONGITUD = -77.0428; // Rango [-180, 180]

// 1. Celdas de Carga HX711
HX711Adapter scalePlastic(19, 21);
HX711Adapter scalePaper(22, 23);
HX711Adapter scaleGeneral(32, 33);
IScaleSensor* scales[3] = { &scalePlastic, &scalePaper, &scaleGeneral };

// 2. Sensores Ultrasonicos
HCSR04Adapter usPlastic(4, 34);
HCSR04Adapter usPaper(5, 35);
HCSR04Adapter usGeneral(2, 15);
IDistanceSensor* distanceSensors[3] = { &usPlastic, &usPaper, &usGeneral };

// 3. Servomotores
CompartmentServo servoPlastic(13);
CompartmentServo servoPaper(12);
CompartmentServo servoGeneral(14);
IActuator* servos[3] = { &servoPlastic, &servoPaper, &servoGeneral };

// 4. Servicio MQTT
MqttManager& mqttService = MqttManager::getInstance(WIFI_SSID, WIFI_PASS, MQTT_BROKER, MQTT_PORT);

// Inyeccion de dependencias a la Fachada con las coordenadas
SmartBinFacade smartBin(scales, distanceSensors, servos, &mqttService, LATITUD, LONGITUD);

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("\n\n==========================================");
    Serial.println(" SISTEMA DE RECICLAJE INTELIGENTE INICIADO");
    Serial.println("==========================================");

    // Inicializar conectividad
    mqttService.begin();

    // Inicializar hardware y logica
    smartBin.begin();
}

void loop() {
    mqttService.loop();
    smartBin.checkInputs();
    delay(10);
}