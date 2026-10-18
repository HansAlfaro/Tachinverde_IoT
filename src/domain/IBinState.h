#ifndef IBIN_STATE_H
#define IBIN_STATE_H

#include <Arduino.h>

class BinContext; // Forward declaration

class IBinState {
public:
    virtual ~IBinState() {}
    virtual void handleInput(BinContext* ctx, String input) = 0;
    virtual void handleButton(BinContext* ctx, int btnIndex) = 0;
    virtual void update(BinContext* ctx) = 0;
    virtual String getStateName() = 0;
};

#endif