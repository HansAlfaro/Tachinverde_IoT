#include "States.h"
#include "BinContext.h"
#include "SmartBinFacade.h"
#include "../network/PayloadFactory.h"

static void finishDeposit(BinContext* ctx) {
    int comp = ctx->activeCompartment;
    ctx->facade().addWasteHeight(comp, 5.0f);
    float currentHeight = ctx->facade().getAccumulatedHeight(comp);

    int idTacho = comp + 1;
    unsigned long currentEventId = ctx->facade().nextEventCounter(idTacho);

    String recyclePayload = PayloadFactory::createRecycleEvent(
        idTacho,
        currentEventId,
        ctx->studentId,
        ctx->wasteType,
        ctx->depositedWeight
    );
    if (ctx->facade().publishMessage("smartbin/recycle", recyclePayload)) {
        Serial.println("[MQTT TX -> smartbin/recycle]: " + recyclePayload);
    } else {
        Serial.println("[MQTT ERROR -> smartbin/recycle] No se pudo publicar el evento de reciclaje.");
    }

    Serial.println("------------------------------------------");
    Serial.println("[RESUMEN LOCAL DE TRANSACCION]");
    Serial.println(" - Evento: tacho-" + String(idTacho) + "-evento-" + String(currentEventId));
    Serial.println(" - Alumno: " + ctx->studentId);
    Serial.println(" - Tacho ID: " + String(idTacho) + " (" + ctx->wasteType + ")");
    Serial.println(" - Metodo interno: " + ctx->selectionMethod);
    Serial.println(" - Peso depositado: " + String(ctx->depositedWeight, 2) + " g");
    Serial.println(" - Altura acumulada: " + String(currentHeight, 1) + " cm / 110.0 cm");

    if (ctx->facade().isCompartmentFull(comp)) {
        Serial.println("[ALERTA] ¡TACHO LLENO! Capacidad: true");
        Serial.println("[BLOQUEO] Este tacho ha sido inhabilitado para futuras aperturas.");
    }
    Serial.println("==========================================");

    ctx->resetSession();
    ctx->timerStart = millis();
    ctx->transitionTo(new ThankYouState());
}

// --- IDLE STATE ---
void IdleState::handleInput(BinContext* ctx, String input) {
    if (input.startsWith("ID:") || input.length() >= 6) {
        String codUser = input;
        if (codUser.startsWith("ID:")) {
            codUser = codUser.substring(3);
        }
        codUser.trim();

        String payload = PayloadFactory::createStudentValidationRequest(codUser);
        if (!ctx->facade().publishMessage("smartbin/student/validate", payload)) {
            Serial.println("[MQTT ERROR -> smartbin/student/validate] No se pudo enviar el codigo. Reintente.");
            return;
        }

        ctx->studentId = codUser;
        ctx->studentValidationPending = true;
        ctx->timerStart = millis();
        Serial.println("[MQTT TX -> smartbin/student/validate]: " + payload);
        Serial.println("[VALIDANDO ALUMNO] Esperando respuesta del backend...");
        ctx->transitionTo(new WaitingForStudentValidationState());
    } else {
        Serial.println("[ERROR] Escanee un carnet valido de 8 digitos (ej: ID:00001001).");
    }
}

void IdleState::handleButton(BinContext* ctx, int btnIndex) {
    Serial.println("[ACCION BLOQUEADA] Primero debe escanear su carnet de alumno.");
}

void IdleState::update(BinContext* ctx) {}

// --- WAITING FOR STUDENT VALIDATION STATE ---
void WaitingForStudentValidationState::handleInput(BinContext* ctx, String input) {
    Serial.println("[VALIDANDO ALUMNO] Espere la respuesta antes de ingresar otro codigo.");
}

void WaitingForStudentValidationState::handleButton(BinContext* ctx, int btnIndex) {
    Serial.println("[ACCION BLOQUEADA] Espere la validacion del alumno.");
}

void WaitingForStudentValidationState::update(BinContext* ctx) {
    if (millis() - ctx->timerStart > 15000) {
        Serial.println("[TIMEOUT] No se recibio respuesta para validar al alumno. Intente nuevamente.");
        ctx->studentValidationPending = false;
        ctx->resetSession();
        ctx->transitionTo(new IdleState());
    }
}

// --- AWAITING SELECTION STATE ---
void AwaitingSelectionState::handleInput(BinContext* ctx, String input) {
    if (input.length() != 8) {
        Serial.println("[ERROR] El codigo QR del producto debe tener exactamente 8 digitos.");
        return;
    }

    for (unsigned int i = 0; i < input.length(); i++) {
        if (input.charAt(i) < '0' || input.charAt(i) > '9') {
            Serial.println("[ERROR] El codigo QR del producto debe contener solo digitos.");
            return;
        }
    }

    String payload = PayloadFactory::createProductValidationRequest(input);
    if (!ctx->facade().publishMessage("smartbin/product/validate", payload)) {
        Serial.println("[MQTT ERROR -> smartbin/product/validate] No se pudo enviar el codigo QR. Reintente.");
        return;
    }

    ctx->productCode = input;
    ctx->productValidationPending = true;
    ctx->timerStart = millis();
    Serial.println("[MQTT TX -> smartbin/product/validate]: " + payload);
    Serial.println("[VALIDANDO PRODUCTO] Esperando respuesta del backend...");
    ctx->transitionTo(new WaitingForProductValidationState());
}

void AwaitingSelectionState::handleButton(BinContext* ctx, int btnIndex) {
    if (ctx->facade().isCompartmentFull(btnIndex)) {
        String names[3] = {"plastico", "papel/carton", "vidrio"};
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
        ctx->wasteType = "vidrio";
        ctx->activeCompartment = 2;
    }

    ctx->selectionMethod = "MANUAL";
    Serial.println("[BOTON PRESIONADO] Seleccionado: " + ctx->wasteType + " | Metodo: " + ctx->selectionMethod);
    ctx->initialWeight = ctx->facade().readWeight(ctx->activeCompartment);
    ctx->facade().openCompartment(ctx->activeCompartment);
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

// --- WAITING FOR PRODUCT VALIDATION STATE ---
void WaitingForProductValidationState::handleInput(BinContext* ctx, String input) {
    Serial.println("[VALIDANDO PRODUCTO] Espere la respuesta antes de ingresar otro codigo o seleccionar con los botones.");
}

void WaitingForProductValidationState::handleButton(BinContext* ctx, int btnIndex) {
    Serial.println("[ACCION BLOQUEADA] Espere la validacion del producto.");
}

void WaitingForProductValidationState::update(BinContext* ctx) {
    if (millis() - ctx->timerStart > 15000) {
        Serial.println("[TIMEOUT] No se recibio respuesta para validar el producto. Puede intentar otro QR o usar los botones.");
        ctx->productValidationPending = false;
        ctx->productCode = "";
        ctx->timerStart = millis();
        ctx->transitionTo(new AwaitingSelectionState());
    }
}

// --- PRODUCT UNKNOWN STATE ---
void ProductUnknownState::handleInput(BinContext* ctx, String input) {
    if (input.length() != 8) {
        Serial.println("[ERROR] El codigo QR del producto debe tener exactamente 8 digitos.");
        return;
    }

    for (unsigned int i = 0; i < input.length(); i++) {
        if (input.charAt(i) < '0' || input.charAt(i) > '9') {
            Serial.println("[ERROR] El codigo QR del producto debe contener solo digitos.");
            return;
        }
    }

    String payload = PayloadFactory::createProductValidationRequest(input);
    if (!ctx->facade().publishMessage("smartbin/product/validate", payload)) {
        Serial.println("[MQTT ERROR -> smartbin/product/validate] No se pudo enviar el codigo QR. Reintente.");
        return;
    }

    ctx->productCode = input;
    ctx->productValidationPending = true;
    ctx->timerStart = millis();
    Serial.println("[MQTT TX -> smartbin/product/validate]: " + payload);
    Serial.println("[VALIDANDO PRODUCTO] Esperando respuesta del backend...");
    ctx->transitionTo(new WaitingForProductValidationState());
}

void ProductUnknownState::handleButton(BinContext* ctx, int btnIndex) {
    Serial.println("[ACCION BLOQUEADA] Producto desconocido. Ingrese otro codigo QR.");
}

void ProductUnknownState::update(BinContext* ctx) {
    unsigned long elapsed = millis() - ctx->timerStart;
    if (elapsed >= 3000 && !hasShownSecondMessage()) {
        ctx->facade().showStatus("Intente con otro", "Producto");
        markSecondMessageShown();
    }
    if (elapsed >= 10000) {
        Serial.println("[TIMEOUT] Producto desconocido. Se agotaron los 10 segundos. Volviendo a pedir el codigo del alumno.");
        ctx->productValidationPending = false;
        ctx->productCode = "";
        ctx->resetSession();
        ctx->transitionTo(new IdleState());
    }
}

// --- COMPARTMENT OPEN STATE ---
void CompartmentOpenState::handleInput(BinContext* ctx, String input) {}
void CompartmentOpenState::handleButton(BinContext* ctx, int btnIndex) {}

// --- THANK YOU STATE ---
void ThankYouState::handleInput(BinContext* ctx, String input) {}
void ThankYouState::handleButton(BinContext* ctx, int btnIndex) {}

void ThankYouState::update(BinContext* ctx) {
    if (millis() - ctx->timerStart >= 3000) {
        ctx->transitionTo(new IdleState());
    }
}

void CompartmentOpenState::update(BinContext* ctx) {
    int comp = ctx->activeCompartment;
    if (ctx->selectionMethod == "QR") {
        if (millis() - ctx->timerStart >= 5000) {
            ctx->facade().closeCompartment(comp);
            Serial.println("[DEPOSITO QR] Compartimiento cerrado despues de 5 segundos.");
            finishDeposit(ctx);
        }
        return;
    }

    float currentWeight = ctx->facade().readWeight(comp);
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

            ctx->facade().closeCompartment(comp);
            depositStarted = false;
            lastObservedWeight = 0.0f;
            stableStartTime = 0;
            finishDeposit(ctx);
            return;
        }
    } else {
        depositStarted = false;
        stableStartTime = 0;
    }

    if (millis() - ctx->timerStart > 20000) {
        Serial.println("[AVISO] Tiempo expirado sin deposito. Cerrando tacho.");
        ctx->facade().closeCompartment(comp);

        depositStarted = false;
        lastObservedWeight = 0.0f;
        stableStartTime = 0;

        ctx->resetSession();
        ctx->transitionTo(new IdleState());
    }
}