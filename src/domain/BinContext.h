#ifndef BIN_CONTEXT_H
#define BIN_CONTEXT_H

#include <Arduino.h>
#include "IBinState.h"

class SmartBinFacade;

class BinContext {
private:
    IBinState* currentState;
    SmartBinFacade* facade_;

public:
    String studentId;
    String wasteType;
    String selectionMethod; // "QR" o "MANUAL"
    float initialWeight;
    float depositedWeight;
    int activeCompartment;
    unsigned long timerStart;
    bool studentValidationPending;
    String productCode;
    bool productValidationPending;

    BinContext(SmartBinFacade* f);
    ~BinContext();

    SmartBinFacade& facade() const;
    void transitionTo(IBinState* newState);
    void resetSession();
    void handleInput(String input);
    void handleButton(int btnIndex);
    void update();
    void handleStudentValidationResponse(const String& codUser, bool exists);
    void handleProductValidationResponse(const String& codProducto, const String& tipo, float peso);
};

#endif