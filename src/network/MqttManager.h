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
    const char* subscribedTopic;
    String incomingTopic;
    String incomingPayload;
    bool incomingMessageAvailable;

    MqttManager(const char* ssid, const char* pass, const char* ip, int port);
    static MqttManager* activeInstance;
    static void handleMessage(char* topic, uint8_t* payload, unsigned int length);

public:
    static MqttManager& getInstance(const char* ssid = "Wokwi-GUEST", const char* pass = "", const char* ip = "192.168.131.63", int port = 1883);

    void begin() override;
    void loop() override;
    bool isConnected() override;
    bool publish(const char* topic, const String& payload) override;
    bool subscribe(const char* topic) override;
    bool readMessage(String& topic, String& payload) override;
    void reconnect();
};

#endif