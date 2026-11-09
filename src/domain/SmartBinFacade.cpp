#include "SmartBinFacade.h"
#include "BinContext.h"
#include "../network/PayloadFactory.h"
#include <ArduinoJson.h>

SmartBinFacade::SmartBinFacade(IScaleSensor* sc[3], IDistanceSensor* ds[3], IActuator* sv[3], IMqttService* mqtt, double lat, double lon)
    : mqttService(mqtt), context(nullptr), lastStatusPublish(0), latitude(lat), longitude(lon), eventCounter(1) {
    for (int i = 0; i < 3; i++) {
        scales[i] = sc[i];
        distanceSensors[i] = ds[i];
        servos[i] = sv[i];
    }
    accumulatedHeight[0] = 20.0f;  // plastico
    accumulatedHeight[1] = 60.0f;  // papel/carton
    accumulatedHeight[2] = 106.0f; // vidrio (cerca del limite de 110 cm)
}

SmartBinFacade::~SmartBinFacade() {
    if (context != nullptr) delete context;
}

void SmartBinFacade::begin() {
    statusDisplay.begin();

    if (!mqttService->subscribe("smartbin/+/response")) {
        Serial.println("[MQTT ERROR] No se pudo configurar la suscripcion a smartbin/+/response.");
    }

    for (int i = 0; i < 3; i++) {
        scales[i]->begin();
        distanceSensors[i]->begin();
        servos[i]->begin();
        pinMode(btnPins[i], INPUT_PULLUP);
    }
    context = new BinContext(this);
}

void SmartBinFacade::showProcessState(const String& state, const String& detail) {
    if (state == "IDLE (Esperando Carnet)") {
        statusDisplay.show("Esperando ID", "ID de estudiante");
    } else if (state == "VALIDANDO_ALUMNO") {
        statusDisplay.show("Validando alumno", "Espere respuesta");
    } else if (state == "ESPERANDO_SELECCION") {
        statusDisplay.show("Alumno validado", "QR o boton");
    } else if (state == "VALIDANDO_PRODUCTO") {
        statusDisplay.show("Validando QR", "Espere respuesta");
    } else if (state == "PRODUCTO_DESCONOCIDO") {
        statusDisplay.show("Producto", "desconocido");
    } else if (state == "COMPARTIMIENTO_ABIERTO") {
        statusDisplay.show("Inserte material", detail);
    } else if (state == "GRACIAS_POR_RECICLAR") {
        statusDisplay.show("Gracias por", "reciclar");
    }
}

bool SmartBinFacade::isCompartmentFull(int index) {
    if (index >= 0 && index < 3) {
        return accumulatedHeight[index] >= FULL_LIMIT_CM;
    }
    return false;
}

void SmartBinFacade::addWasteHeight(int index, float cm) {
    if (index >= 0 && index < 3) {
        bool wasFull = isCompartmentFull(index);
        accumulatedHeight[index] += cm;
        if (!wasFull && isCompartmentFull(index)) {
            publishBinStatus(index);
        }
    }
}

void SmartBinFacade::publishBinStatus(int index) {
    if (index < 0 || index >= 3) {
        return;
    }

    const char* binTypes[3] = {"plastico", "papel/carton", "vidrio"};
    int idTacho = index + 1;
    int porcentaje = (int)((accumulatedHeight[index] * 100.0f / FULL_LIMIT_CM) + 0.5f);
    if (porcentaje > 100) {
        porcentaje = 100;
    }
    if (porcentaje < 0) {
        porcentaje = 0;
    }

    String payload = PayloadFactory::createBinStatus(
        idTacho,
        binTypes[index],
        latitude,
        longitude,
        isCompartmentFull(index),
        porcentaje
    );
    if (mqttService->publish("smartbin/status", payload)) {
        Serial.println("[MQTT TX -> smartbin/status]: " + payload);
    } else {
        Serial.println("[MQTT ERROR -> smartbin/status]: No se pudo publicar el estado del tacho " + String(idTacho));
    }
}

void SmartBinFacade::publishBinStatusesIfDue() {
    unsigned long now = millis();
    if (now - lastStatusPublish < STATUS_PUBLISH_INTERVAL_MS) {
        return;
    }

    lastStatusPublish = now;
    for (int i = 0; i < 3; i++) {
        publishBinStatus(i);
    }
}

void SmartBinFacade::checkInputs() {
    publishBinStatusesIfDue();

    String incomingTopic;
    String incomingPayload;
    if (mqttService->readMessage(incomingTopic, incomingPayload)) {
        if (incomingTopic == "smartbin/student/response") {
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, incomingPayload);
            if (error) {
                Serial.println("[MQTT ERROR] Respuesta de validacion JSON invalida: " + String(error.c_str()));
            } else if (!doc["codUser"].is<const char*>() || !doc["exists"].is<bool>()) {
                Serial.println("[MQTT ERROR] La respuesta debe incluir codUser y exists (booleano).");
            } else {
                context->handleStudentValidationResponse(
                    String(doc["codUser"].as<const char*>()),
                    doc["exists"].as<bool>()
                );
            }
        } else if (incomingTopic == "smartbin/product/response") {
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, incomingPayload);
            if (error) {
                Serial.println("[MQTT ERROR] Respuesta de producto JSON invalida: " + String(error.c_str()));
            } else if (!doc["codProducto"].is<const char*>() ||
                       !doc["tipo"].is<const char*>() ||
                       !doc["peso"].is<float>() ||
                       doc["peso"].as<float>() < 0.0f) {
                Serial.println("[MQTT ERROR] La respuesta de producto debe incluir codProducto, tipo (texto) y peso (numero >= 0, en gramos).");
            } else {
                context->handleProductValidationResponse(
                    String(doc["codProducto"].as<const char*>()),
                    String(doc["tipo"].as<const char*>()),
                    doc["peso"].as<float>()
                );
            }
        } else {
            Serial.println("[MQTT RX] Mensaje ignorado en topic no esperado: " + incomingTopic);
        }
    }

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