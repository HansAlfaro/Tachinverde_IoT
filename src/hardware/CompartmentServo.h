#ifndef COMPARTMENT_SERVO_H
#define COMPARTMENT_SERVO_H

#include "IActuator.h"
#include <ESP32Servo.h>

class CompartmentServo : public IActuator {
private:
    byte pin;
    Servo servo;
    int openAngle;
    int closeAngle;

public:
    CompartmentServo(byte p, int openA = 90, int closeA = 0)
        : pin(p), openAngle(openA), closeAngle(closeA) {}

    void begin() override {
        servo.attach(pin);
        close();
    }

    void open() override {
        servo.write(openAngle);
    }

    void close() override {
        servo.write(closeAngle);
    }
};

#endif