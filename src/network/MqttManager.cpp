#include "MqttManager.h"

MqttManager* MqttManager::activeInstance = nullptr;

MqttManager::MqttManager(const char* s, const char* wifiPass, const char* ip, int port,
                         const char* mqttUser, const char* mqttPass)
    : ssid(s), wifiPassword(wifiPass), brokerIp(ip), brokerPort(port),
      mqttUsername(mqttUser), mqttPassword(mqttPass),
      lastReconnectAttempt(0),
      incomingMessageAvailable(false) {
    mqttClient.begin(brokerIp, brokerPort, espClient);
    mqttClient.onMessage(handleMessage);
}

MqttManager& MqttManager::getInstance(const char* ssid, const char* wifiPass,
                                      const char* ip, int port,
                                      const char* mqttUser, const char* mqttPass) {
    static MqttManager instance(ssid, wifiPass, ip, port, mqttUser, mqttPass);
    activeInstance = &instance;
    return instance;
}

void MqttManager::begin() {
    Serial.print("[WIFI] Conectando a ");
    Serial.println(ssid);
    WiFi.begin(ssid, wifiPassword);

    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
        delay(500);
        Serial.print(".");
    }
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n[WIFI] Conectado exitosamente.");
        Serial.print("[WIFI] IP asignada: ");
        Serial.println(WiFi.localIP());

    } else {
        Serial.println("\n[WIFI] No se pudo conectar a Wi-Fi (timeout).");
    }
}

void MqttManager::loop() {
    if (WiFi.status() != WL_CONNECTED) {
        return;
    }

    if (!mqttClient.connected()) {
        unsigned long now = millis();
        if (now - lastReconnectAttempt > 5000) {
            lastReconnectAttempt = now;
            reconnect();
        }
    } else {
        mqttClient.loop();
    }
}

void MqttManager::reconnect() {
    if (WiFi.status() != WL_CONNECTED) return;

    Serial.print("[MQTT] Intentando conexion al broker en ");
    Serial.print(brokerIp);
    Serial.print("...");

    String clientId = "ESP32_SmartBin_" + String(random(0xffff), HEX);
    if (mqttClient.connect(clientId.c_str(), mqttUsername, mqttPassword)) {
        Serial.println(" CONECTADO.");
        for (const String& topic : subscribedTopics) {
            if (!mqttClient.subscribe(topic.c_str(), 1)) {
                Serial.println("[MQTT ERROR] No se pudo suscribir a " + topic);
            } else {
                Serial.println("[MQTT SUB QoS 1] Suscrito a " + topic);
            }
        }
    } else {
        Serial.print(" Fallo, error=");
        Serial.println(static_cast<int>(mqttClient.lastError()));
    }
}

bool MqttManager::isConnected() {
    return mqttClient.connected();
}

bool MqttManager::publish(const char* topic, const String& payload) {
    if (mqttClient.connected()) {
        return mqttClient.publish(topic, payload, false, 1);
    }
    return false;
}

bool MqttManager::subscribe(const char* topic) {
    if (topic == nullptr) {
        return false;
    }

    for (const String& subscribedTopic : subscribedTopics) {
        if (subscribedTopic == topic) {
            return true;
        }
    }

    subscribedTopics.push_back(topic);
    if (!mqttClient.connected()) {
        return true;
    }
    return mqttClient.subscribe(subscribedTopics.back().c_str(), 1);
}

bool MqttManager::readMessage(String& topic, String& payload) {
    if (!incomingMessageAvailable) {
        return false;
    }

    topic = incomingTopic;
    payload = incomingPayload;
    incomingMessageAvailable = false;
    return true;
}

void MqttManager::handleMessage(String& topic, String& payload) {
    if (activeInstance == nullptr) {
        return;
    }

    activeInstance->incomingTopic = topic;
    activeInstance->incomingPayload = payload;
    activeInstance->incomingMessageAvailable = true;
}