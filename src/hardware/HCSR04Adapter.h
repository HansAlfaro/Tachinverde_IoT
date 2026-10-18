#ifndef HCSR04_ADAPTER_H
#define HCSR04_ADAPTER_H

#include "IDistanceSensor.h"

class HCSR04Adapter : public IDistanceSensor {
private:
    byte trigPin;
    byte echoPin;

public:
    HCSR04Adapter(byte trig, byte echo) : trigPin(trig), echoPin(echo) {}

    void begin() override {
        pinMode(trigPin, OUTPUT);
        pinMode(echoPin, INPUT);
    }

    float getDistanceCm() override {
        digitalWrite(trigPin, LOW);
        delayMicroseconds(2);
        digitalWrite(trigPin, HIGH);
        delayMicroseconds(10);
        digitalWrite(trigPin, LOW);

        long duration = pulseIn(echoPin, HIGH, 25000);
        if (duration == 0) return 400.0f;
        return duration * 0.034f / 2.0f;
    }
};

#endif