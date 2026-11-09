#include "PayloadFactory.h"

String PayloadFactory::createStudentValidationRequest(const String& codUser) {
    JsonDocument doc;
    doc["codUser"] = codUser;

    String output;
    serializeJson(doc, output);
    return output;
}

String PayloadFactory::createProductValidationRequest(const String& codProducto) {
    JsonDocument doc;
    doc["codProducto"] = codProducto;

    String output;
    serializeJson(doc, output);
    return output;
}

String PayloadFactory::createRecycleEvent(int idTacho, unsigned long eventCounter, const String& codUser, const String& tipo, float peso) {
    JsonDocument doc;

    String cleanUser = codUser;
    if (cleanUser.startsWith("ID:")) {
        cleanUser = cleanUser.substring(3);
    }
    cleanUser.trim();

    String idEvento = "tacho-" + String(idTacho) + "-evento-" + String(eventCounter);

    doc["idEvento"] = idEvento;
    doc["codUser"]  = cleanUser;
    doc["idTacho"]  = idTacho;
    doc["tipo"]     = tipo;
    doc["peso"]     = serialized(String(peso, 2));

    String output;
    serializeJson(doc, output);
    return output;
}

String PayloadFactory::createBinStatus(int idTacho, const String& tipo, double latitud, double longitud, bool capacidad, int porcentaje) {
    JsonDocument doc;

    doc["idTacho"]   = idTacho;
    doc["tipo"]      = tipo;
    doc["latitud"]   = latitud;
    doc["longitud"]  = longitud;
    doc["capacidad"] = capacidad;
    doc["porcentaje"] = porcentaje;

    String output;
    serializeJson(doc, output);
    return output;
}