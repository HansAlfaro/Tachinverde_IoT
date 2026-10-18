#ifndef IMQTT_SERVICE_H
#define IMQTT_SERVICE_H

#include <Arduino.h>

class IMqttService {
public:
    virtual ~IMqttService() {}
    virtual void begin() = 0;
    virtual void loop() = 0;
    virtual bool isConnected() = 0;
    virtual bool publish(const char* topic, const String& payload) = 0;
};

#endif