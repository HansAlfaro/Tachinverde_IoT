#include "PayloadFactory.h"

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

String PayloadFactory::createBinStatus(int idTacho, double latitud, double longitud, bool capacidad) {
    JsonDocument doc;

    doc["idTacho"]   = idTacho;
    doc["latitud"]   = latitud;
    doc["longitud"]  = longitud;
    doc["capacidad"] = capacidad;

    String output;
    serializeJson(doc, output);
    return output;
}