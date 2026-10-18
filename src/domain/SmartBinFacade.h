#ifndef SMART_BIN_FACADE_H
#define SMART_BIN_FACADE_H

#include <Arduino.h>
#include "../hardware/IScaleSensor.h"
#include "../hardware/IDistanceSensor.h"
#include "../hardware/IActuator.h"
#include "../network/IMqttService.h"

class BinContext;

class SmartBinFacade {
public:
    IScaleSensor* scales[3];
    IDistanceSensor* distanceSensors[3];
    IActuator* servos[3];
    IMqttService* mqttService;
    BinContext* context;
    const byte btnPins[3] = {25, 26, 27};

    float accumulatedHeight[3];
    bool binFullNotified[3]; // Para evitar envios duplicados del evento de lleno
    const float FULL_LIMIT_CM = 110.0f;
    
    // Coordenadas geograficas del tacho
    double latitude;
    double longitude;

    unsigned long eventCounter;

    SmartBinFacade(IScaleSensor* sc[3], IDistanceSensor* ds[3], IActuator* sv[3], IMqttService* mqtt, double lat, double lon);
    ~SmartBinFacade();
    void begin();
    void checkInputs();
    bool isCompartmentFull(int index);
    void addWasteHeight(int index, float cm = 5.0f);
    void checkAndNotifyBinFull(int index);
};

#endif