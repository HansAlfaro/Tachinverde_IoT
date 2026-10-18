#ifndef IACTUATOR_H
#define IACTUATOR_H

#include <Arduino.h>

class IActuator {
public:
    virtual ~IActuator() {}
    virtual void begin() = 0;
    virtual void open() = 0;
    virtual void close() = 0;
};

#endif