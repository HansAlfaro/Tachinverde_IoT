#include "SmartBinFacade.h"
#include "BinContext.h"
#include "../network/PayloadFactory.h"
#include <ArduinoJson.h>
#include <limits.h>

SmartBinFacade::SmartBinFacade(IScaleSensor* sc[3], IDistanceSensor* ds[3], IActuator* sv[3], IMqttService* mqtt, double lat, double lon)
    : mqttService(mqtt), lastStatusPublish(0), latitude(lat), longitude(lon),
      eventCounters{1, 1, 1}, mqttWasConnected(false), eventSyncReady(false),
      awaitingLastEvent(false), eventSyncAttempted(false), eventSyncFailed(false),
      eventSyncTachoIndex(0),
      lastEventSyncAttempt(0) {
    for (int i = 0; i < 3; i++) {
        scales[i] = sc[i];
        distanceSensors[i] = ds[i];
        servos[i] = sv[i];
    }
    accumulatedHeight[0] = 20.0f;  // plastico
    accumulatedHeight[1] = 60.0f;  // papel/carton
    accumulatedHeight[2] = 106.0f; // vidrio (cerca del limite de 110 cm)
}

void SmartBinFacade::begin() {
    statusDisplay.begin();

    if (!mqttService->subscribe("smartbin/+/response") ||
        !mqttService->subscribe("smartbin/events/last/response")) {
        Serial.println("[MQTT ERROR] No se pudieron configurar las suscripciones de respuesta MQTT.");
    }

    for (int i = 0; i < 3; i++) {
        scales[i]->begin();
        distanceSensors[i]->begin();
        servos[i]->begin();
        pinMode(btnPins[i], INPUT_PULLUP);
    }
    context.reset(new BinContext(this));
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

void SmartBinFacade::showStatus(const String& line1, const String& line2) {
    statusDisplay.show(line1, line2);
}

bool SmartBinFacade::publishMessage(const char* topic, const String& payload) {
    return mqttService->publish(topic, payload);
}

float SmartBinFacade::readWeight(int index) {
    if (index < 0 || index >= 3) {
        Serial.println("[ERROR] Indice de compartimiento invalido al leer el peso.");
        return 0.0f;
    }
    return scales[index]->readWeight();
}

void SmartBinFacade::openCompartment(int index) {
    if (index < 0 || index >= 3) {
        Serial.println("[ERROR] Indice de compartimiento invalido al abrir.");
        return;
    }
    servos[index]->open();
}

void SmartBinFacade::closeCompartment(int index) {
    if (index < 0 || index >= 3) {
        Serial.println("[ERROR] Indice de compartimiento invalido al cerrar.");
        return;
    }
    servos[index]->close();
}

bool SmartBinFacade::isCompartmentFull(int index) {
    if (index >= 0 && index < 3) {
        return accumulatedHeight[index] >= FULL_LIMIT_CM;
    }
    return false;
}

float SmartBinFacade::getAccumulatedHeight(int index) const {
    if (index < 0 || index >= 3) {
        Serial.println("[ERROR] Indice de compartimiento invalido al consultar la altura.");
        return 0.0f;
    }
    return accumulatedHeight[index];
}

unsigned long SmartBinFacade::nextEventCounter(int idTacho) {
    if (idTacho < 1 || idTacho > 3) {
        Serial.println("[ERROR] Indice de tacho invalido al generar idEvento.");
        return 0;
    }
    return eventCounters[idTacho - 1]++;
}

void SmartBinFacade::startEventSync() {
    eventSyncReady = false;
    awaitingLastEvent = false;
    eventSyncAttempted = false;
    eventSyncFailed = false;
    eventSyncTachoIndex = 0;
    Serial.println("[MQTT] Sincronizando la secuencia de eventos con el backend.");
}

void SmartBinFacade::pollEventSync() {
    if (eventSyncReady || eventSyncFailed || !mqttService->isConnected()) {
        return;
    }

    if (eventSyncTachoIndex >= 3) {
        eventSyncReady = true;
        Serial.println("[MQTT] Secuencias sincronizadas. Sistema habilitado.");
        return;
    }

    const unsigned long now = millis();
    if (awaitingLastEvent) {
        if (now - lastEventSyncAttempt < EVENT_SYNC_RETRY_INTERVAL_MS) {
            return;
        }
        Serial.println("[MQTT TIMEOUT] Sin respuesta al consultar el ultimo evento; se reintentara.");
        awaitingLastEvent = false;
    } else if (eventSyncAttempted && now - lastEventSyncAttempt < EVENT_SYNC_RETRY_INTERVAL_MS) {
        return;
    }

    const int idTacho = eventSyncTachoIndex + 1;
    const String payload = PayloadFactory::createLastEventRequest(idTacho);
    lastEventSyncAttempt = now;
    eventSyncAttempted = true;
    if (mqttService->publish("smartbin/events/last/request", payload)) {
        awaitingLastEvent = true;
        Serial.println("[MQTT TX -> smartbin/events/last/request]: " + payload);
    } else {
        Serial.println("[MQTT ERROR] No se pudo consultar la secuencia del tacho " + String(idTacho) + ".");
    }
}

void SmartBinFacade::handleLastEventResponse(const String& payload) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload);
    if (error) {
        Serial.println("[MQTT ERROR] Respuesta del ultimo evento JSON invalida: " + String(error.c_str()));
        return;
    }
    if (!doc["idTacho"].is<int>() || !doc["lastEvent"].is<unsigned long>()) {
        Serial.println("[MQTT ERROR] La respuesta debe incluir idTacho y lastEvent (enteros no negativos).");
        return;
    }

    const int idTacho = doc["idTacho"].as<int>();
    if (!awaitingLastEvent || idTacho != eventSyncTachoIndex + 1) {
        Serial.println("[MQTT RX] Respuesta de secuencia ignorada (tacho no solicitado).");
        return;
    }

    const unsigned long lastEvent = doc["lastEvent"].as<unsigned long>();
    if (lastEvent == ULONG_MAX) {
        Serial.println("[MQTT ERROR] La secuencia de eventos del backend alcanzo el maximo.");
        awaitingLastEvent = false;
        eventSyncFailed = true;
        return;
    }

    const unsigned long nextEvent = lastEvent + 1;
    if (eventCounters[idTacho - 1] < nextEvent) {
        eventCounters[idTacho - 1] = nextEvent;
    }
    Serial.println("[MQTT RX] Tacho " + String(idTacho) + ", ultimo evento: " +
                   String(lastEvent) + ", siguiente: " + String(eventCounters[idTacho - 1]));
    awaitingLastEvent = false;
    eventSyncAttempted = false;
    eventSyncTachoIndex++;
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
    if (!context) {
        Serial.println("[ERROR] SmartBinFacade::begin() debe ejecutarse antes de checkInputs().");
        return;
    }

    const bool connected = mqttService->isConnected();
    if (connected && !mqttWasConnected) {
        startEventSync();
    } else if (!connected && mqttWasConnected) {
        eventSyncReady = false;
        awaitingLastEvent = false;
        eventSyncAttempted = false;
        eventSyncFailed = false;
        eventSyncTachoIndex = 0;
        Serial.println("[MQTT] Conexion perdida; se bloquearan nuevas operaciones hasta sincronizar.");
    }
    mqttWasConnected = connected;
    pollEventSync();

    publishBinStatusesIfDue();

    String incomingTopic;
    String incomingPayload;
    if (mqttService->readMessage(incomingTopic, incomingPayload)) {
        if (incomingTopic == "smartbin/events/last/response") {
            handleLastEventResponse(incomingPayload);
        } else if (incomingTopic == "smartbin/student/response") {
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

    if (!eventSyncReady) {
        if (Serial.available() > 0) {
            Serial.readStringUntil('\n');
            Serial.println("[MQTT] Operacion ignorada: esperando sincronizacion de eventos.");
        }
        context->update();
        return;
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