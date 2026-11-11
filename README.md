# Tacho inteligente

Firmware para un ESP32 que controla tres compartimentos de reciclaje, guía el depósito mediante botones o comandos por el monitor serial y publica eventos en un broker MQTT.

## Requisitos previos

### Visual Studio Code y PlatformIO

1. Instala [Visual Studio Code](https://code.visualstudio.com/).
2. En **Extensiones**, instala **PlatformIO IDE** (publisher: PlatformIO).
3. Abre en VS Code la carpeta raíz del proyecto, la que contiene `platformio.ini`.
4. PlatformIO descargará automáticamente la plataforma ESP32 y las librerías declaradas en `platformio.ini` al compilar. Las dependencias del proyecto son HX711, ESP32Servo, MQTT, ArduinoJson y LiquidCrystal_I2C; no es necesario instalarlas manualmente.

### Wokwi Simulator (para simular)

Instala la extensión **Wokwi Simulator** en VS Code. El proyecto ya incluye `diagram.json` con el ESP32, sensores, servos y botones, y `wokwi.toml` apunta al firmware que genera PlatformIO. Compila una vez antes de iniciar la simulación para que existan los archivos `.pio/build/esp32doit-devkit-v1/firmware.elf` y `firmware.bin`.

Wokwi simula el hardware, pero no hace que una IP privada de tu computadora sea accesible desde la simulación. Para comunicar Wokwi con un Mosquitto local, configura un gateway de IoT compatible con Wokwi; como alternativa, usa un broker accesible desde Internet y protegido. No expongas un broker anónimo a Internet.

### Broker Mosquitto

Instala Mosquitto en el equipo que actuará como broker (por ejemplo, con el instalador oficial para Windows desde [mosquitto.org/download](https://mosquitto.org/download/)). Para probarlo localmente, inicia el servicio Mosquitto o abre una terminal y ejecútalo en primer plano:

```powershell
mosquitto.exe -v
```

Para usar MQTT sin cifrar durante las pruebas, configura Mosquitto con un listener en el puerto `1883` y conexiones anónimas:

```conf
listener 1883 0.0.0.0
allow_anonymous true
```

El listener debe aceptar conexiones desde la interfaz de red que usa el simulador. Permite el puerto `1883` en el firewall solo para la red de pruebas. Esta configuración sin TLS ni autenticación es solo para una red confiable; no la expongas a Internet.

Inicia Mosquitto desde PowerShell (ajusta las rutas):

```powershell
& "C:\Program Files\mosquitto\mosquitto.exe" -v -c "C:\ruta\mosquitto-local.conf"
```

### MQTTX (cliente para visualizar mensajes)

Instala [MQTTX](https://mqttx.app/) en el equipo desde el que quieras observar los mensajes. MQTTX es un cliente gráfico; no sustituye al broker Mosquitto, que debe estar ejecutándose.

1. En MQTTX, crea una conexión MQTT nueva con TLS deshabilitado.
2. Configura **Host** con `localhost` si MQTTX está en el mismo equipo que Mosquitto; si está en otro equipo, usa la IP del equipo donde corre el broker.
3. Configura **Port** como `1883` y el protocolo como `mqtt://`.
4. El firmware actual no envía credenciales MQTT. Si Mosquitto está configurado para permitir conexiones anónimas, deja usuario y contraseña vacíos.
5. Conecta y suscríbete a `smartbin/#` para recibir todos los mensajes, o por separado a `smartbin/recycle` y `smartbin/status`.

## Configuración para tu equipo

Edita las constantes de red en [`src/main.cpp`](./src/main.cpp):

```cpp
const char* WIFI_SSID = "Wokwi-GUEST";
const char* WIFI_PASS = "";
const char* MQTT_BROKER = "192.168.131.63";
const int MQTT_PORT = 1883;
const char* MQTT_USER = "tu_usuario_mqtt";
const char* MQTT_PASS = "tu_contrasena_mqtt";
```

- **Wokwi:** usa `Wokwi-GUEST` y contraseña vacía. `MQTT_BROKER` debe ser la IP privada actual del equipo donde corre Mosquitto (no `localhost`) y `MQTT_PORT` debe coincidir con el listener MQTT sin TLS. El simulador debe tener ruta de red hasta esa IP y el firewall debe permitir el puerto `1883`.
- Reemplaza `MQTT_USER` y `MQTT_PASS` en `src/main.cpp` con el usuario y la contraseña configurados en Mosquitto.
- Esta configuración es MQTT sin cifrado ni autenticación, destinada únicamente a pruebas en una red confiable. No expongas el puerto a Internet.
- Cambia `LATITUD` y `LONGITUD` en el mismo archivo para publicar las coordenadas del tacho.
- Si cambias el nombre, contraseña o dirección del broker, vuelve a compilar y cargar el firmware.

El firmware actual no implementa autenticación MQTT; Mosquitto debe permitir conexiones anónimas para esta prueba.

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

1. Al arrancar, el firmware inicializa Wi-Fi/MQTT y los sensores, servos y botones. El cliente MQTT intenta reconectarse cada cinco segundos si pierde la conexión. Al conectarse, consulta al backend el último evento de cada tacho y bloquea nuevas operaciones hasta sincronizar las tres secuencias; hace lo mismo después de cada reconexión.
2. En el monitor serial, escribe un código (por ejemplo, `ID:00001001`) y presiona Enter. El firmware publica `{"codUser":"00001001"}` en `smartbin/student/validate` y espera la respuesta del backend en `smartbin/student/response`, con el formato `{"codUser":"00001001","exists":true}` o `{"codUser":"00001001","exists":false}`. Solo si `exists` es `true` pasa a selección; si es `false`, informa que el alumno no existe. La respuesta debe incluir el mismo `codUser` solicitado.
3. Selecciona el compartimento con uno de los tres botones, o escribe por serial el código QR del producto como exactamente 8 dígitos (por ejemplo, `12345678`). El firmware envía el código al backend y espera la respuesta con el material y el peso antes de abrir el compartimento. Los códigos con letras, prefijos o una longitud distinta se rechazan.
4. Si el compartimento no está lleno, la selección manual abre el servo y espera a que el peso de la balanza se estabilice. Para la selección por QR, el peso informado por el backend se usa directamente y el servo permanece abierto durante 5 segundos; después se cierra y se registra el depósito automáticamente. Tras completar el reciclaje, la pantalla muestra “Gracias por reciclar” durante 3 segundos y luego vuelve a pedir el código del alumno.
5. Si no se registra un depósito manual en 20 segundos, el compartimento se cierra y la sesión se cancela. Si un tipo recibido por QR no es reconocido, la pantalla muestra “Producto desconocido” y, a los 3 segundos, “Intente con otro Producto”; se puede volver a escanear durante 10 segundos. Si no se escanea otro código en ese plazo, vuelve a pedir el código de alumno. Si no se selecciona un residuo en 15 segundos, se cancela la sesión de selección.
6. Las selecciones de un compartimento lleno se bloquean. Un depósito exitoso publica su evento en MQTT; el estado de los tres tachos se publica periódicamente cada 30 segundos.

La lógica de ejecución usa estados para la espera del identificador, validaciones, selección, depósito y confirmación de reciclaje. `BinContext` administra las transiciones y `SmartBinFacade` conecta esa lógica con hardware y MQTT.

### Topics MQTT

El firmware publica y se suscribe con QoS 1. El broker y los clientes MQTT deben admitir QoS 1; este nivel confirma la recepción por el broker, pero puede entregar mensajes duplicados, por lo que el backend debe deduplicar eventos usando `idEvento`.

| Tópico | Cuándo | Contenido |
|---|---|---|
| `smartbin/recycle` | Después de un depósito válido | JSON con `idEvento`, `codUser`, `idTacho`, `tipo` y `peso`. |
| `smartbin/status` | Cada 30 segundos, para cada tacho | JSON con `idTacho`, `tipo`, `latitud`, `longitud`, `capacidad` y `porcentaje`. |
| `smartbin/student/validate` | Al ingresar un codigo de alumno por serial | JSON con `codUser`. |
| `smartbin/product/validate` | Al ingresar un QR de producto válido por serial | JSON con `codProducto`, código de exactamente 8 dígitos. |

El firmware se suscribe a `smartbin/+/response` y `smartbin/events/last/response` para recibir respuestas. El backend debe responder a las consultas de secuencia en `smartbin/events/last/request` con `{"idTacho":1,"lastEvent":42}` en `smartbin/events/last/response`. `lastEvent` es el último número usado por ese tacho, o `0` si aún no tiene eventos; la secuencia de cada tacho empieza en `1`. El firmware consulta los tachos 1, 2 y 3 en orden, reintenta cada cinco segundos si no recibe respuesta y no acepta nuevos ingresos hasta completar la sincronización. En caso de reconexión, conserva además el siguiente número local si es mayor que el reportado por el backend. El backend debe seguir deduplicando por `idEvento`: la consulta al historial no elimina la posibilidad de reenvíos QoS 1.

El backend también debe publicar en `smartbin/student/response` un JSON con `codUser` y `exists` (booleano), y responder en `smartbin/product/response` con un JSON como `{"codProducto":"12345678","tipo":"plastico","peso":25.5}`. `codProducto` debe coincidir con la solicitud pendiente, `tipo` debe ser exactamente `plastico`, `papel/carton` o `vidrio` (se ignoran diferencias de mayúsculas y espacios al inicio o al final) y `peso` debe ser un número no negativo expresado en gramos. Si el producto no existe o tiene otro tipo de material, el backend puede responder con `tipo:"desconocido"` (incluyendo también `peso`); el firmware no abrirá ningún compartimento. Ambas validaciones vencen tras 15 segundos sin respuesta y se ignoran respuestas con códigos que no coinciden con la solicitud pendiente.

En el estado actual, `tipo` identifica el compartimento (`plastico`, `papel/carton` o `vidrio`), `capacidad: false` significa que está disponible y `capacidad: true` que está lleno/no disponible. `porcentaje` es el porcentaje de llenado estimado según la altura acumulada, redondeado a un entero y limitado al rango de 0 a 100. El primer envío se realiza 30 segundos después del arranque.

## Hardware y simulación

`diagram.json` documenta las conexiones de la simulación. La asignación en el firmware es:

| Componente | Compartimento | Pines ESP32 |
|---|---|---|
| HX711 (DT, SCK) | Plástico | 19, 21 |
| HX711 (DT, SCK) | Papel/cartón | 22, 23 |
| HX711 (DT, SCK) | Vidrio | 32, 33 |
| HC-SR04 (TRIG, ECHO) | Plástico | 4, 34 |
| HC-SR04 (TRIG, ECHO) | Papel/cartón | 5, 35 |
| HC-SR04 (TRIG, ECHO) | Vidrio | 2, 15 |
| LCD 16×2 I2C | Estados del proceso | SDA 16, SCL 18 |
| Servo | Plástico | 13 |
| Servo | Papel/cartón | 12 |
| Servo | Vidrio | 14 |
| Botón | Plástico | 25 |
| Botón | Papel/cartón | 26 |
| Botón | Vidrio | 27 |

### Alcance actual de la lógica de capacidad

Aunque los tres sensores ultrasónicos se inicializan, la detección de lleno de esta versión no utiliza sus mediciones: la capacidad se estima por altura acumulada en software. Los valores iniciales son 20 cm (plástico), 60 cm (papel/cartón) y 106 cm (vidrio), el límite es 110 cm, y cada depósito añade 5 cm. Por ello, el compartimento de vidrio parte cerca del límite. Estos valores son de demostración y deben adaptarse al recipiente real si se usa la lógica estimada.

La LCD usa la dirección I2C `0x27`. Sus mensajes indican espera del alumno, validación, selección de material y depósito abierto. Para liberar los pines I2C 16 y 18, el ECHO del sensor ultrasónico de plástico se conecta al GPIO 34 y el de papel/cartón al GPIO 35; actualiza también esos pines si cableas hardware físico. La calibración HX711 se fija actualmente en `420.0` en `HX711Adapter.h`; para una báscula física se requiere calibrarla con la celda de carga instalada. La simulación sirve para probar el flujo y las conexiones representadas, no sustituye la calibración ni las comprobaciones eléctricas del hardware real.
