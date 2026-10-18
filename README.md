# Tacho inteligente

Firmware para un ESP32 que controla tres compartimentos de reciclaje, guía el depósito mediante botones o comandos por el monitor serial y publica eventos en un broker MQTT.

## Requisitos previos

### Visual Studio Code y PlatformIO

1. Instala [Visual Studio Code](https://code.visualstudio.com/).
2. En **Extensiones**, instala **PlatformIO IDE** (publisher: PlatformIO).
3. Abre en VS Code la carpeta raíz del proyecto, la que contiene `platformio.ini`.
4. PlatformIO descargará automáticamente la plataforma ESP32 y las librerías declaradas en `platformio.ini` al compilar. Las dependencias del proyecto son HX711, ESP32Servo, PubSubClient y ArduinoJson; no es necesario instalarlas manualmente.

### Wokwi Simulator (para simular)

Instala la extensión **Wokwi Simulator** en VS Code. El proyecto ya incluye `diagram.json` con el ESP32, sensores, servos y botones, y `wokwi.toml` apunta al firmware que genera PlatformIO. Compila una vez antes de iniciar la simulación para que existan los archivos `.pio/build/esp32doit-devkit-v1/firmware.elf` y `firmware.bin`.

Wokwi simula el hardware, pero no hace que una IP privada de tu computadora sea accesible desde la simulación. Para comunicar Wokwi con un Mosquitto local, configura un gateway de IoT compatible con Wokwi; como alternativa, usa un broker accesible desde Internet y protegido. No expongas un broker anónimo a Internet.

### Broker Mosquitto

Instala Mosquitto en el equipo que actuará como broker (por ejemplo, con el instalador oficial para Windows desde [mosquitto.org/download](https://mosquitto.org/download/)). Para probarlo localmente, inicia el servicio Mosquitto o abre una terminal y ejecútalo en primer plano:

```powershell
mosquitto.exe -v
```

Para permitir conexiones desde otros equipos de la red local, configura un listener que no esté limitado a `localhost`. Por ejemplo, crea un archivo `mosquitto-local.conf`:

```conf
listener 1883 0.0.0.0
allow_anonymous true
```

Inícialo desde PowerShell (ajusta la ruta si instalaste Mosquitto en otra ubicación):

```powershell
& "C:\Program Files\mosquitto\mosquitto.exe" -v -c "C:\ruta\mosquitto-local.conf"
```

`allow_anonymous true` es solo para pruebas en una red confiable. Para un despliegue real configura usuario/contraseña y restringe el acceso. Autoriza TCP entrante en el puerto `1883` en el firewall únicamente para la red necesaria.

### MQTTX (cliente para visualizar mensajes)

Instala [MQTTX](https://mqttx.app/) en el equipo desde el que quieras observar los mensajes. MQTTX es un cliente gráfico; no sustituye al broker Mosquitto, que debe estar ejecutándose.

1. En MQTTX, crea una conexión MQTT nueva.
2. Configura **Host** con `localhost` si MQTTX está en el mismo equipo que Mosquitto; si está en otro equipo, usa la IP del equipo donde corre el broker.
3. Configura **Port** como `1883` y el protocolo como `mqtt://` (TCP).
4. El firmware actual no envía credenciales MQTT. Si Mosquitto está configurado para permitir conexiones anónimas, deja usuario y contraseña vacíos.
5. Conecta y suscríbete a `smartbin/#` para recibir todos los mensajes, o por separado a `smartbin/recycle` y `smartbin/status`.

MQTTX debe poder alcanzar el broker por la red. Para Wokwi, una conexión al broker Mosquitto local puede requerir el gateway de IoT descrito arriba.

## Configuración para tu equipo

Edita las constantes de red en [`src/main.cpp`](./src/main.cpp):

```cpp
const char* WIFI_SSID = "Wokwi-GUEST";
const char* WIFI_PASS = "";
const char* MQTT_BROKER = "192.168.18.143";
const int MQTT_PORT = 1883;
```

- **ESP32 físico:** sustituye `WIFI_SSID` y `WIFI_PASS` por las credenciales de la red Wi-Fi. En `MQTT_BROKER`, pon la dirección IPv4 del equipo donde corre Mosquitto, obtenida con `ipconfig` (por ejemplo, `192.168.1.50`). El ESP32 y el broker deben tener conectividad entre sí; no pongas `localhost` ni `127.0.0.1`, porque para el ESP32 esas direcciones apuntarían al propio dispositivo.
- **Wokwi:** `Wokwi-GUEST` y la contraseña vacía son los valores de red de simulación que usa el proyecto. La IP privada de tu PC no suele ser enrutable desde el simulador: utiliza un gateway de IoT compatible con Wokwi o configura un broker accesible de forma segura desde la simulación.
- Cambia `LATITUD` y `LONGITUD` en el mismo archivo para publicar las coordenadas del tacho.
- Si cambias el nombre, contraseña o dirección del broker, vuelve a compilar y cargar el firmware.

No guardes credenciales reales en repositorios públicos. El firmware actual no implementa autenticación MQTT; si Mosquitto requiere usuario y contraseña, también habrá que añadir esas credenciales al cliente MQTT del código.

## Compilar, cargar y ejecutar

Desde la raíz del proyecto:

```powershell
pio run
pio run --target upload
pio device monitor
```

También puedes usar los botones de PlatformIO en VS Code: **Build**, **Upload** y **Monitor**. Selecciona el puerto serie correcto para la placa física; el monitor está configurado a **115200 baudios**.

Para Wokwi:

1. Ejecuta **Build** desde PlatformIO.
2. Abre `diagram.json` y usa **Start Simulator** desde la extensión Wokwi.
3. Revisa los mensajes de ejecución en el monitor serial de la simulación.

Para verificar Mosquitto desde una segunda terminal en el mismo equipo, suscríbete a los tópicos del firmware:

```powershell
mosquitto_sub.exe -h localhost -p 1883 -t "smartbin/#" -v
```

## Uso del sistema

1. Al arrancar, el firmware inicializa Wi-Fi/MQTT y los sensores, servos y botones. El cliente MQTT intenta reconectarse cada cinco segundos si pierde la conexión.
2. En el monitor serial, escribe un identificador (por ejemplo, `ID:00001001`) y presiona Enter. El sistema pasa de espera a selección.
3. Selecciona el compartimento mediante los botones o escribe una selección simulada por serial:
   - `QR:PLASTICO` o `QR:PET`
   - `QR:PAPEL` o `QR:CARTON`
   - `QR:ORGANICO`
4. Si el compartimento no está lleno, se abre su servo. El programa toma el peso inicial y espera un incremento mayor que `0.05 g` que permanezca estable durante 2.5 segundos. Al estabilizarse, cierra el servo, registra el evento, actualiza la altura estimada y vuelve al estado inicial.
5. Si no se registra depósito en 20 segundos, el compartimento se cierra y la sesión se cancela. Si no se selecciona un residuo en 15 segundos, se cancela la sesión de selección.
6. Las selecciones de un compartimento lleno se bloquean. Un depósito exitoso publica su evento en MQTT; al alcanzar el límite de capacidad, publica además un estado.

La lógica de ejecución usa tres estados: **Idle** (espera del identificador), **AwaitingSelection** (espera del tipo de residuo) y **CompartmentOpen** (supervisa el depósito). `BinContext` administra las transiciones y `SmartBinFacade` conecta esa lógica con hardware y MQTT.

### MQTT publicado

| Tópico | Cuándo | Contenido |
|---|---|---|
| `smartbin/recycle` | Después de un depósito válido | JSON con `idEvento`, `codUser`, `idTacho`, `tipo` y `peso`. |
| `smartbin/status` | Cuando un depósito hace que el tacho alcance el límite | JSON con `idTacho`, `latitud`, `longitud` y `capacidad`. |

En el estado actual, `capacidad: false` significa que está disponible y `capacidad: true` que está lleno/no disponible. El código no publica periódicamente los estados disponibles ni se suscribe a comandos MQTT.

## Hardware y simulación

`diagram.json` documenta las conexiones de la simulación. La asignación en el firmware es:

| Componente | Compartimento | Pines ESP32 |
|---|---|---|
| HX711 (DT, SCK) | Plástico | 19, 21 |
| HX711 (DT, SCK) | Papel/cartón | 22, 23 |
| HX711 (DT, SCK) | General/orgánicos | 32, 33 |
| HC-SR04 (TRIG, ECHO) | Plástico | 4, 16 |
| HC-SR04 (TRIG, ECHO) | Papel/cartón | 5, 18 |
| HC-SR04 (TRIG, ECHO) | General/orgánicos | 2, 15 |
| Servo | Plástico | 13 |
| Servo | Papel/cartón | 12 |
| Servo | General/orgánicos | 14 |
| Botón | Plástico | 25 |
| Botón | Papel/cartón | 26 |
| Botón | General/orgánicos | 27 |

### Alcance actual de la lógica de capacidad

Aunque los tres sensores ultrasónicos se inicializan, la detección de lleno de esta versión no utiliza sus mediciones: la capacidad se estima por altura acumulada en software. Los valores iniciales son 20 cm (plástico), 60 cm (papel/cartón) y 106 cm (general/orgánicos), el límite es 110 cm, y cada depósito añade 5 cm. Por ello, el compartimento de orgánicos parte cerca del límite. Estos valores son de demostración y deben adaptarse al recipiente real si se usa la lógica estimada.

La calibración HX711 se fija actualmente en `420.0` en `HX711Adapter.h`; para una báscula física se requiere calibrarla con la celda de carga instalada. La simulación sirve para probar el flujo y las conexiones representadas, no sustituye la calibración ni las comprobaciones eléctricas del hardware real.
