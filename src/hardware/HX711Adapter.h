#ifndef HX711_ADAPTER_H
#define HX711_ADAPTER_H

#include "IScaleSensor.h"
#include "HX711.h"

class HX711Adapter : public IScaleSensor {
private:
    byte doutPin;
    byte sckPin;
    HX711 scale;
    float lastValidWeight;

public:
    HX711Adapter(byte dout, byte sck, float cal = 420.0f)
        : doutPin(dout), sckPin(sck), lastValidWeight(0.0f) {}

    void begin() override {
        pinMode(doutPin, INPUT);
        pinMode(sckPin, OUTPUT);
        digitalWrite(sckPin, LOW);

        scale.begin(doutPin, sckPin);
        delay(100);
        scale.set_scale(420.0f);
        scale.tare();
    }

    float readWeight() override {
        if (scale.is_ready()) {
            lastValidWeight = scale.get_units(1);
        }
        return lastValidWeight;
    }

    void tare() override {
        if (scale.is_ready()) {
            scale.tare();
        }
        lastValidWeight = 0.0f;
    }
};

#endif