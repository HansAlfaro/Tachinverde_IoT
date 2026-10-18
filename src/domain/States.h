#ifndef STATES_H
#define STATES_H

#include "IBinState.h"

class IdleState : public IBinState {
public:
    String getStateName() override { return "IDLE (Esperando Carnet)"; }
    void handleInput(BinContext* ctx, String input) override;
    void handleButton(BinContext* ctx, int btnIndex) override;
    void update(BinContext* ctx) override;
};

class AwaitingSelectionState : public IBinState {
public:
    String getStateName() override { return "ESPERANDO_SELECCION"; }
    void handleInput(BinContext* ctx, String input) override;
    void handleButton(BinContext* ctx, int btnIndex) override;
    void update(BinContext* ctx) override;
};

class CompartmentOpenState : public IBinState {
public:
    String getStateName() override { return "COMPARTIMIENTO_ABIERTO"; }
    void handleInput(BinContext* ctx, String input) override;
    void handleButton(BinContext* ctx, int btnIndex) override;
    void update(BinContext* ctx) override;
};

#endif