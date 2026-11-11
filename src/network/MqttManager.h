#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

#include <WiFi.h>
#include <MQTT.h>
#include <vector>
#include "IMqttService.h"

class MqttManager : public IMqttService {
private:
    const char* ssid;
    const char* wifiPassword;
    const char* brokerIp;
    int brokerPort;
    const char* mqttUsername;
    const char* mqttPassword;
    WiFiClient espClient;
    MQTTClient mqttClient;
    unsigned long lastReconnectAttempt;
    std::vector<String> subscribedTopics;
    String incomingTopic;
    String incomingPayload;
    bool incomingMessageAvailable;

    MqttManager(const char* ssid, const char* wifiPass, const char* ip, int port,
                const char* mqttUser, const char* mqttPass);
    static MqttManager* activeInstance;
    static void handleMessage(String& topic, String& payload);

public:
    static MqttManager& getInstance(const char* ssid = "Wokwi-GUEST", const char* wifiPass = "",
                                    const char* ip = "192.168.131.63", int port = 1883,
                                    const char* mqttUser = "", const char* mqttPass = "");

    void begin() override;
    void loop() override;
    bool isConnected() override;
    bool publish(const char* topic, const String& payload) override;
    bool subscribe(const char* topic) override;
    bool readMessage(String& topic, String& payload) override;
    void reconnect();
};

#endif