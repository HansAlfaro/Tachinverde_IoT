#ifndef ISCALE_SENSOR_H
#define ISCALE_SENSOR_H

#include <Arduino.h>

class IScaleSensor {
public:
    virtual ~IScaleSensor() {}
    virtual void begin() = 0;
    virtual float readWeight() = 0;
    virtual void tare() = 0;
};

#endif