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

class WaitingForStudentValidationState : public IBinState {
public:
    String getStateName() override { return "VALIDANDO_ALUMNO"; }
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

class WaitingForProductValidationState : public IBinState {
public:
    String getStateName() override { return "VALIDANDO_PRODUCTO"; }
    void handleInput(BinContext* ctx, String input) override;
    void handleButton(BinContext* ctx, int btnIndex) override;
    void update(BinContext* ctx) override;
};

class ProductUnknownState : public IBinState {
private:
    bool secondMessageShown;

public:
    ProductUnknownState() : secondMessageShown(false) {}
    String getStateName() override { return "PRODUCTO_DESCONOCIDO"; }
    void handleInput(BinContext* ctx, String input) override;
    void handleButton(BinContext* ctx, int btnIndex) override;
    void update(BinContext* ctx) override;

    bool hasShownSecondMessage() const { return secondMessageShown; }
    void markSecondMessageShown() { secondMessageShown = true; }
};

class CompartmentOpenState : public IBinState {
public:
    String getStateName() override { return "COMPARTIMIENTO_ABIERTO"; }
    void handleInput(BinContext* ctx, String input) override;
    void handleButton(BinContext* ctx, int btnIndex) override;
    void update(BinContext* ctx) override;
};

class ThankYouState : public IBinState {
public:
    String getStateName() override { return "GRACIAS_POR_RECICLAR"; }
    void handleInput(BinContext* ctx, String input) override;
    void handleButton(BinContext* ctx, int btnIndex) override;
    void update(BinContext* ctx) override;
};

#endif