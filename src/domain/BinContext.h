#ifndef BIN_CONTEXT_H
#define BIN_CONTEXT_H

#include <Arduino.h>
#include "IBinState.h"

class SmartBinFacade;

class BinContext {
private:
    IBinState* currentState;

public:
    String studentId;
    String wasteType;
    String selectionMethod; // "QR" o "MANUAL"
    float initialWeight;
    float depositedWeight;
    int activeCompartment;
    unsigned long timerStart;
    SmartBinFacade* facade;

    BinContext(SmartBinFacade* f);
    ~BinContext();

    void transitionTo(IBinState* newState);
    void resetSession();
    void handleInput(String input);
    void handleButton(int btnIndex);
    void update();
};

#endif