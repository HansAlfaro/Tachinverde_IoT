#include "SmartBinFacade.h"
#include "BinContext.h"
#include "../network/PayloadFactory.h"

SmartBinFacade::SmartBinFacade(IScaleSensor* sc[3], IDistanceSensor* ds[3], IActuator* sv[3], IMqttService* mqtt, double lat, double lon)
    : mqttService(mqtt), context(nullptr), eventCounter(1), latitude(lat), longitude(lon) {
    for (int i = 0; i < 3; i++) {
        scales[i] = sc[i];
        distanceSensors[i] = ds[i];
        servos[i] = sv[i];
        binFullNotified[i] = false;
    }
    accumulatedHeight[0] = 20.0f;  // plastico
    accumulatedHeight[1] = 60.0f;  // papel/carton
    accumulatedHeight[2] = 106.0f; // organicos (cerca del limite de 110 cm)
}

SmartBinFacade::~SmartBinFacade() {
    if (context != nullptr) delete context;
}

void SmartBinFacade::begin() {
    for (int i = 0; i < 3; i++) {
        scales[i]->begin();
        distanceSensors[i]->begin();
        servos[i]->begin();
        pinMode(btnPins[i], INPUT_PULLUP);
    }
    context = new BinContext(this);
}

bool SmartBinFacade::isCompartmentFull(int index) {
    if (index >= 0 && index < 3) {
        return accumulatedHeight[index] >= FULL_LIMIT_CM;
    }
    return false;
}

void SmartBinFacade::addWasteHeight(int index, float cm) {
    if (index >= 0 && index < 3) {
        accumulatedHeight[index] += cm;
    }
}

void SmartBinFacade::checkAndNotifyBinFull(int index) {
    // Si cruza el limite y todavia no se ha notificado que se lleno
    if (isCompartmentFull(index) && !binFullNotified[index]) {
        binFullNotified[index] = true;
        int idTacho = index + 1;
        bool capacidad = true; // Lleno/no disponible -> capacidad: true

        String payload = PayloadFactory::createBinStatus(idTacho, latitude, longitude, capacidad);
        mqttService->publish("smartbin/status", payload);
        Serial.println("[MQTT TX -> smartbin/status]: " + payload);
    }
}

void SmartBinFacade::checkInputs() {
    if (Serial.available() > 0) {
        String input = Serial.readStringUntil('\n');
        input.trim();
        if (input.length() > 0) {
            context->handleInput(input);
        }
    }

    for (int i = 0; i < 3; i++) {
        if (digitalRead(btnPins[i]) == LOW) {
            delay(30);
            if (digitalRead(btnPins[i]) == LOW) {
                context->handleButton(i);
                while (digitalRead(btnPins[i]) == LOW);
            }
        }
    }

    context->update();
}