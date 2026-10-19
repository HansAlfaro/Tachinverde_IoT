#include "SmartBinFacade.h"
#include "BinContext.h"
#include "../network/PayloadFactory.h"

SmartBinFacade::SmartBinFacade(IScaleSensor* sc[3], IDistanceSensor* ds[3], IActuator* sv[3], IMqttService* mqtt, double lat, double lon)
    : mqttService(mqtt), context(nullptr), lastStatusPublish(0), latitude(lat), longitude(lon), eventCounter(1) {
    for (int i = 0; i < 3; i++) {
        scales[i] = sc[i];
        distanceSensors[i] = ds[i];
        servos[i] = sv[i];
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

void SmartBinFacade::publishBinStatusesIfDue() {
    unsigned long now = millis();
    if (now - lastStatusPublish < STATUS_PUBLISH_INTERVAL_MS) {
        return;
    }

    lastStatusPublish = now;
    const char* binTypes[3] = {"plastico", "papel/carton", "organicos"};
    for (int i = 0; i < 3; i++) {
        int idTacho = i + 1;
        int porcentaje = (int)((accumulatedHeight[i] * 100.0f / FULL_LIMIT_CM) + 0.5f);
        if (porcentaje > 100) {
            porcentaje = 100;
        }
        if (porcentaje < 0) {
            porcentaje = 0;
        }

        String payload = PayloadFactory::createBinStatus(
            idTacho,
            binTypes[i],
            latitude,
            longitude,
            isCompartmentFull(i),
            porcentaje
        );
        if (mqttService->publish("smartbin/status", payload)) {
            Serial.println("[MQTT TX -> smartbin/status]: " + payload);
        } else {
            Serial.println("[MQTT ERROR -> smartbin/status]: No se pudo publicar el estado del tacho " + String(idTacho));
        }
    }
}

void SmartBinFacade::checkInputs() {
    publishBinStatusesIfDue();

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