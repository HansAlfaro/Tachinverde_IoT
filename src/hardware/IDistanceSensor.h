#ifndef IDISTANCE_SENSOR_H
#define IDISTANCE_SENSOR_H

#include <Arduino.h>

class IDistanceSensor {
public:
    virtual ~IDistanceSensor() {}
    virtual void begin() = 0;
    virtual float getDistanceCm() = 0;
};

#endif