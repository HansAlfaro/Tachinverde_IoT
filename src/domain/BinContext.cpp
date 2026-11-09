#include "BinContext.h"
#include "States.h"
#include "SmartBinFacade.h"

BinContext::BinContext(SmartBinFacade* f) : facade(f), currentState(nullptr) {
    studentValidationPending = false;
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
    facade->showProcessState(currentState->getStateName(), wasteType);
}

void BinContext::resetSession() {
    studentId = "";
    productCode = "";
    wasteType = "";
    selectionMethod = "";
    initialWeight = 0.0f;
    depositedWeight = 0.0f;
    activeCompartment = -1;
    studentValidationPending = false;
    productValidationPending = false;
}

void BinContext::handleInput(String input) { currentState->handleInput(this, input); }
void BinContext::handleButton(int btnIndex) { currentState->handleButton(this, btnIndex); }
void BinContext::update() { currentState->update(this); }

void BinContext::handleStudentValidationResponse(const String& codUser, bool exists) {
    if (!studentValidationPending || codUser != studentId) {
        Serial.println("[MQTT RX] Respuesta de validacion ignorada (sin solicitud coincidente).");
        return;
    }

    studentValidationPending = false;
    if (exists) {
        timerStart = millis();
        Serial.println("[ALUMNO IDENTIFICADO] Bienvenido: " + studentId);
        Serial.println("Seleccione residuo con boton o escanee QR de producto.");
        transitionTo(new AwaitingSelectionState());
    } else {
        Serial.println("[ALUMNO NO EXISTE] No se encontro el codigo " + studentId + ".");
        resetSession();
        transitionTo(new IdleState());
    }
}

void BinContext::handleProductValidationResponse(const String& codProducto, const String& tipo, float peso) {
    if (!productValidationPending || codProducto != productCode) {
        Serial.println("[MQTT RX] Respuesta de producto ignorada (sin solicitud coincidente).");
        return;
    }

    productValidationPending = false;
    String normalizedType = tipo;
    normalizedType.trim();
    normalizedType.toLowerCase();

    int targetCompartment = -1;
    String wasteName;
    if (normalizedType == "plastico" || normalizedType == "pet") {
        targetCompartment = 0;
        wasteName = "plastico";
    } else if (normalizedType == "papel" || normalizedType == "carton" || normalizedType == "papel/carton") {
        targetCompartment = 1;
        wasteName = "papel/carton";
    } else if (normalizedType == "vidrio" || normalizedType == "cristal") {
        targetCompartment = 2;
        wasteName = "vidrio";
    }

    if (targetCompartment < 0) {
        Serial.println("[PRODUCTO NO VALIDO] El backend devolvio un tipo de material desconocido: " + tipo);
        productCode = "";
        productValidationPending = false;
        timerStart = millis();
        transitionTo(new ProductUnknownState());
        return;
    }

    if (facade->isCompartmentFull(targetCompartment)) {
        Serial.println("[BLOQUEADO] El compartimiento de " + wasteName + " esta lleno. Puede elegir otro producto o usar los botones.");
        productCode = "";
        timerStart = millis();
        transitionTo(new AwaitingSelectionState());
        return;
    }

    wasteType = wasteName;
    selectionMethod = "QR";
    activeCompartment = targetCompartment;
    depositedWeight = peso;
    facade->servos[activeCompartment]->open();
    timerStart = millis();
    Serial.println("[PRODUCTO VALIDADO] Codigo: " + productCode + " | Material: " + wasteType +
                   " | Peso recibido: " + String(depositedWeight, 2) + " g");
    productCode = "";
    transitionTo(new CompartmentOpenState());
}