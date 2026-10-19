#ifndef PAYLOAD_FACTORY_H
#define PAYLOAD_FACTORY_H

#include <Arduino.h>
#include <ArduinoJson.h>

class PayloadFactory {
public:
    static String createRecycleEvent(int idTacho, unsigned long eventCounter, const String& codUser, const String& tipo, float peso);
    static String createBinStatus(int idTacho, const String& tipo, double latitud, double longitud, bool capacidad, int porcentaje);
};

#endif