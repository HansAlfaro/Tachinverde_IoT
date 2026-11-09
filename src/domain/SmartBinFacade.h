#ifndef SMART_BIN_FACADE_H
#define SMART_BIN_FACADE_H

#include <Arduino.h>
#include "../hardware/IScaleSensor.h"
#include "../hardware/IDistanceSensor.h"
#include "../hardware/IActuator.h"
#include "../hardware/LcdStatusDisplay.h"
#include "../network/IMqttService.h"

class BinContext;

class SmartBinFacade {
public:
    IScaleSensor* scales[3];
    IDistanceSensor* distanceSensors[3];
    IActuator* servos[3];
    IMqttService* mqttService;
    LcdStatusDisplay statusDisplay;
    BinContext* context;
    const byte btnPins[3] = {25, 26, 27};

    float accumulatedHeight[3];
    const float FULL_LIMIT_CM = 110.0f;
    const unsigned long STATUS_PUBLISH_INTERVAL_MS = 30000UL;
    unsigned long lastStatusPublish;
    
    // Coordenadas geograficas del tacho
    double latitude;
    double longitude;

    unsigned long eventCounter;

    SmartBinFacade(IScaleSensor* sc[3], IDistanceSensor* ds[3], IActuator* sv[3], IMqttService* mqtt, double lat, double lon);
    ~SmartBinFacade();
    void begin();
    void checkInputs();
    void showProcessState(const String& state, const String& detail);
    bool isCompartmentFull(int index);
    void addWasteHeight(int index, float cm = 5.0f);
    void publishBinStatus(int index);
    void publishBinStatusesIfDue();
};

#endif