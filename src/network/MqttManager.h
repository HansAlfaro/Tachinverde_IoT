#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

#include <WiFi.h>
#include <PubSubClient.h>
#include "IMqttService.h"

class MqttManager : public IMqttService {
private:
    const char* ssid;
    const char* password;
    const char* brokerIp;
    int brokerPort;
    WiFiClient espClient;
    PubSubClient mqttClient;
    unsigned long lastReconnectAttempt;

    MqttManager(const char* ssid, const char* pass, const char* ip, int port);

public:
    static MqttManager& getInstance(const char* ssid = "Wokwi-GUEST", const char* pass = "", const char* ip = "192.168.1.50", int port = 1883);

    void begin() override;
    void loop() override;
    bool isConnected() override;
    bool publish(const char* topic, const String& payload) override;
    void reconnect();
};

#endif