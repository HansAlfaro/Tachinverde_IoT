#include "BinContext.h"
#include "States.h"

BinContext::BinContext(SmartBinFacade* f) : facade(f), currentState(nullptr) {
    resetSession();
    transitionTo(new IdleState());
}

BinContext::~BinContext() {
    if (currentState != nullptr) {
        delete currentState;
    }
}

void BinContext::transitionTo(IBinState* newState) {
    if (currentState != nullptr) delete currentState;
    currentState = newState;
    Serial.print("[ESTADO] -> ");
    Serial.println(currentState->getStateName());
}

void BinContext::resetSession() {
    studentId = "";
    wasteType = "";
    selectionMethod = "";
    initialWeight = 0.0f;
    depositedWeight = 0.0f;
    activeCompartment = -1;
}

void BinContext::handleInput(String input) { currentState->handleInput(this, input); }
void BinContext::handleButton(int btnIndex) { currentState->handleButton(this, btnIndex); }
void BinContext::update() { currentState->update(this); }