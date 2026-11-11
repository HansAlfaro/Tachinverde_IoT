#ifndef SMART_BIN_FACADE_H
#define SMART_BIN_FACADE_H

#include <Arduino.h>
#include <memory>
#include "../hardware/IScaleSensor.h"
#include "../hardware/IDistanceSensor.h"
#include "../hardware/IActuator.h"
#include "../hardware/LcdStatusDisplay.h"
#include "../network/IMqttService.h"
#include "BinContext.h"

class SmartBinFacade {
private:
    IScaleSensor* scales[3];
    IDistanceSensor* distanceSensors[3];
    IActuator* servos[3];
    IMqttService* mqttService;
    LcdStatusDisplay statusDisplay;
    std::unique_ptr<BinContext> context;
    const byte btnPins[3] = {25, 26, 27};

    float accumulatedHeight[3];
    const float FULL_LIMIT_CM = 110.0f;
    const unsigned long STATUS_PUBLISH_INTERVAL_MS = 30000UL;
    const unsigned long EVENT_SYNC_RETRY_INTERVAL_MS = 5000UL;
    unsigned long lastStatusPublish;
    double latitude;
    double longitude;
    unsigned long eventCounters[3];
    bool mqttWasConnected;
    bool eventSyncReady;
    bool awaitingLastEvent;
    bool eventSyncAttempted;
    bool eventSyncFailed;
    int eventSyncTachoIndex;
    unsigned long lastEventSyncAttempt;

    void startEventSync();
    void pollEventSync();
    void handleLastEventResponse(const String& payload);

public:
    SmartBinFacade(IScaleSensor* sc[3], IDistanceSensor* ds[3], IActuator* sv[3], IMqttService* mqtt, double lat, double lon);
    SmartBinFacade(const SmartBinFacade&) = delete;
    SmartBinFacade& operator=(const SmartBinFacade&) = delete;

    void begin();
    void checkInputs();
    void showProcessState(const String& state, const String& detail);
    void showStatus(const String& line1, const String& line2);
    bool publishMessage(const char* topic, const String& payload);
    float readWeight(int index);
    void openCompartment(int index);
    void closeCompartment(int index);
    bool isCompartmentFull(int index);
    void addWasteHeight(int index, float cm = 5.0f);
    float getAccumulatedHeight(int index) const;
    unsigned long nextEventCounter(int idTacho);
    void publishBinStatus(int index);
    void publishBinStatusesIfDue();
};

#endif