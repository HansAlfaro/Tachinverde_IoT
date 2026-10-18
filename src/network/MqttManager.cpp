#include "MqttManager.h"

MqttManager::MqttManager(const char* s, const char* p, const char* ip, int port)
    : ssid(s), password(p), brokerIp(ip), brokerPort(port), mqttClient(espClient), lastReconnectAttempt(0) {}

MqttManager& MqttManager::getInstance(const char* ssid, const char* pass, const char* ip, int port) {
    static MqttManager instance(ssid, pass, ip, port);
    return instance;
}

void MqttManager::begin() {
    Serial.print("[WIFI] Conectando a ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);

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

    mqttClient.setServer(brokerIp, brokerPort);
}

void MqttManager::loop() {
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
    if (mqttClient.connect(clientId.c_str())) {
        Serial.println(" CONECTADO.");
    } else {
        Serial.print(" Fallo, rc=");
        Serial.println(mqttClient.state());
    }
}

bool MqttManager::isConnected() {
    return mqttClient.connected();
}

bool MqttManager::publish(const char* topic, const String& payload) {
    if (mqttClient.connected()) {
        return mqttClient.publish(topic, payload.c_str());
    }
    return false;
}