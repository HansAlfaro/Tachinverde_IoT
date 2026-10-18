#include "States.h"
#include "BinContext.h"
#include "SmartBinFacade.h"
#include "../network/PayloadFactory.h"

// --- IDLE STATE ---
void IdleState::handleInput(BinContext* ctx, String input) {
    if (input.startsWith("ID:") || input.length() >= 6) {
        ctx->studentId = input;
        ctx->timerStart = millis();
        Serial.println("[ALUMNO IDENTIFICADO] Bienvenido: " + ctx->studentId);
        Serial.println("Seleccione residuo con boton o escanee QR de producto.");
        ctx->transitionTo(new AwaitingSelectionState());
    } else {
        Serial.println("[ERROR] Escanee un carnet valido de 8 digitos (ej: ID:00001001).");
    }
}

void IdleState::handleButton(BinContext* ctx, int btnIndex) {
    Serial.println("[ACCION BLOQUEADA] Primero debe escanear su carnet de alumno.");
}

void IdleState::update(BinContext* ctx) {}

// --- AWAITING SELECTION STATE ---
void AwaitingSelectionState::handleInput(BinContext* ctx, String input) {
    int targetComp = -1;
    String typeName = "";

    if (input.startsWith("QR:PET") || input.startsWith("QR:PLASTICO")) {
        targetComp = 0;
        typeName = "plastico";
    } else if (input.startsWith("QR:PAPEL") || input.startsWith("QR:CARTON")) {
        targetComp = 1;
        typeName = "papel/carton";
    } else if (input.startsWith("QR:ORGANICO")) {
        targetComp = 2;
        typeName = "organicos";
    } else {
        Serial.println("[QR DESCONOCIDO] Utilice un QR reconocido (QR:PLASTICO / QR:PAPEL / QR:ORGANICO).");
        return;
    }

    if (ctx->facade->isCompartmentFull(targetComp)) {
        Serial.print("[BLOQUEADO] El tacho de ");
        Serial.print(typeName);
        Serial.println(" esta LLENO (capacidad: true). Seleccione otro o solicite vaciado.");
        return;
    }

    ctx->wasteType = typeName;
    ctx->selectionMethod = "QR";
    ctx->activeCompartment = targetComp;
    Serial.println("[QR DETECTADO DIRECTO] Tipo: " + ctx->wasteType + " | Metodo: " + ctx->selectionMethod);
    ctx->initialWeight = ctx->facade->scales[ctx->activeCompartment]->readWeight();
    ctx->facade->servos[ctx->activeCompartment]->open();
    ctx->timerStart = millis();
    ctx->transitionTo(new CompartmentOpenState());
}

void AwaitingSelectionState::handleButton(BinContext* ctx, int btnIndex) {
    if (ctx->facade->isCompartmentFull(btnIndex)) {
        String names[3] = {"plastico", "papel/carton", "organicos"};
        Serial.print("[BLOQUEADO] El tacho ");
        Serial.print(names[btnIndex]);
        Serial.println(" esta LLENO (capacidad: true). No se puede abrir.");
        return;
    }

    if (btnIndex == 0) {
        ctx->wasteType = "plastico";
        ctx->activeCompartment = 0;
    } else if (btnIndex == 1) {
        ctx->wasteType = "papel/carton";
        ctx->activeCompartment = 1;
    } else if (btnIndex == 2) {
        ctx->wasteType = "organicos";
        ctx->activeCompartment = 2;
    }

    ctx->selectionMethod = "MANUAL";
    Serial.println("[BOTON PRESIONADO] Seleccionado: " + ctx->wasteType + " | Metodo: " + ctx->selectionMethod);
    ctx->initialWeight = ctx->facade->scales[ctx->activeCompartment]->readWeight();
    ctx->facade->servos[ctx->activeCompartment]->open();
    ctx->timerStart = millis();
    ctx->transitionTo(new CompartmentOpenState());
}

void AwaitingSelectionState::update(BinContext* ctx) {
    if (millis() - ctx->timerStart > 15000) {
        Serial.println("[TIMEOUT] Sesion cancelada por inactividad.");
        ctx->resetSession();
        ctx->transitionTo(new IdleState());
    }
}

// --- COMPARTMENT OPEN STATE ---
void CompartmentOpenState::handleInput(BinContext* ctx, String input) {}
void CompartmentOpenState::handleButton(BinContext* ctx, int btnIndex) {}

void CompartmentOpenState::update(BinContext* ctx) {
    int comp = ctx->activeCompartment;
    float currentWeight = ctx->facade->scales[comp]->readWeight();
    float diff = currentWeight - ctx->initialWeight;

    static float lastObservedWeight = 0.0f;
    static unsigned long stableStartTime = 0;
    static bool depositStarted = false;

    if (diff > 0.05f) {
        if (abs(currentWeight - lastObservedWeight) > 0.02f) {
            lastObservedWeight = currentWeight;
            stableStartTime = millis();
            if (!depositStarted) {
                depositStarted = true;
                Serial.println("[DEPOSITANDO] Ajuste el peso deseado...");
            }
        } 
        else if (depositStarted && (millis() - stableStartTime >= 2500)) {
            ctx->depositedWeight = diff;
            Serial.print("[PESO ESTABILIZADO] Registrado: ");
            Serial.print(ctx->depositedWeight, 2);
            Serial.println(" g");

            ctx->facade->servos[comp]->close();
            ctx->facade->addWasteHeight(comp, 5.0f);
            float currentHeight = ctx->facade->accumulatedHeight[comp];

            int idTacho = comp + 1;
            unsigned long currentEventId = ctx->facade->eventCounter++;

            // 1. Enviar evento de reciclaje
            String recyclePayload = PayloadFactory::createRecycleEvent(
                idTacho,
                currentEventId,
                ctx->studentId, 
                ctx->wasteType,
                ctx->depositedWeight
            );
            ctx->facade->mqttService->publish("smartbin/recycle", recyclePayload);
            Serial.println("[MQTT TX -> smartbin/recycle]: " + recyclePayload);

            // 2. Verificar si este deposito lleno el tacho para emitir el estado
            ctx->facade->checkAndNotifyBinFull(comp);

            Serial.println("------------------------------------------");
            Serial.println("[RESUMEN LOCAL DE TRANSACCION]");
            Serial.println(" - Evento: tacho-" + String(idTacho) + "-evento-" + String(currentEventId));
            Serial.println(" - Alumno: " + ctx->studentId);
            Serial.println(" - Tacho ID: " + String(idTacho) + " (" + ctx->wasteType + ")");
            Serial.println(" - Metodo interno: " + ctx->selectionMethod);
            Serial.println(" - Peso depositado: " + String(ctx->depositedWeight, 2) + " g");
            Serial.println(" - Altura acumulada: " + String(currentHeight, 1) + " cm / 110.0 cm");

            if (ctx->facade->isCompartmentFull(comp)) {
                Serial.println("[ALERTA] ¡TACHO LLENO! Capacidad: true");
                Serial.println("[BLOQUEO] Este tacho ha sido inhabilitado para futuras aperturas.");
            }
            Serial.println("==========================================");

            depositStarted = false;
            lastObservedWeight = 0.0f;
            stableStartTime = 0;

            ctx->resetSession();
            ctx->transitionTo(new IdleState());
            return;
        }
    } else {
        depositStarted = false;
        stableStartTime = 0;
    }

    if (millis() - ctx->timerStart > 20000) {
        Serial.println("[AVISO] Tiempo expirado sin deposito. Cerrando tacho.");
        ctx->facade->servos[comp]->close();

        depositStarted = false;
        lastObservedWeight = 0.0f;
        stableStartTime = 0;

        ctx->resetSession();
        ctx->transitionTo(new IdleState());
    }
}