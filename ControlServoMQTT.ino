#include "WiFiHandler.h"
#include <Servo.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>

/* ================== CONFIGURACIÓN ================== */
//Numero de servomotores del brazo
#define NUM_SERVOS 6

//Posiciones de referencia para el inicio y reset
#define POS_REF_BASE 90
#define POS_REF_SHOULDER 40
#define POS_REF_ELBOW 30
#define POS_REF_WRISTTILT 180
#define POS_REF_WRISTROTATION 90
#define POS_REF_CLAW 80

//Maximo y minimo de ancho de pulso de la señal PWM
#define MIN_PULSE_WIDTH 500
#define MAX_PULSE_WIDTH 2500

#define SEND_INTERVAL 30000      // 30 segundos en milisegundos
unsigned long lastSendTime = 0;  // Tiempo del último envío de ángulos

// Configuración WiFi
const char* apName = "ConfigWiFiAP";
const char* apPassword = "TFG$Wifi";

WiFiHandler WiFiHandler(apName, apPassword);

//Configuracion MQTT
PubSubClient mqttClient(WiFiHandler.getWiFiClient());
const char* mqttServer = "192.168.1.77";
const int mqttPort = 1883;
const char* mqttClientName = "ESP8266";
const char* mqttTopics[] = { "esp8266/moveServo", "esp8266/predefinedMovement/request", "esp8266/servoAngles/request" };

//Configuracion de los servos
Servo servos[NUM_SERVOS];
int servoPins[NUM_SERVOS] = { 5, 4, 0, 2, 14, 12 };
int servoPosRef[NUM_SERVOS] = { POS_REF_BASE, POS_REF_SHOULDER, POS_REF_ELBOW, POS_REF_WRISTTILT, POS_REF_WRISTROTATION, POS_REF_CLAW };

/* ================== FUNCIONES SERVOMOTORES ================== */
// Función para mover servos gradualmente
void moveServoAngle(int servoIndex, int angle) {
  int targetAngle = constrain(angle, 0, 180);
  int currentAngle = servos[servoIndex].read();

  int step = targetAngle > currentAngle ? 1 : -1;

  for (int pos = currentAngle; pos != targetAngle + step; pos += step) {
    servos[servoIndex].write(pos);
    delay(20);
  }
}

// Función para resetear servos
void resetServosToRef() {
  Serial.println("Reseteando brazo");
  for (int servoIndex = 0; servoIndex < NUM_SERVOS; servoIndex++) {
    if (servos[servoIndex].read() != servoPosRef[servoIndex]) {
      moveServoAngle(servoIndex, servoPosRef[servoIndex]);
      delay(80);
    }
  }
}

// Función para bajar brazo
void bajarBrazo() {
  Serial.println("Bajando brazo");

  moveServoAngle(5, 30);
  moveServoAngle(3, 180);
  moveServoAngle(1, 0);
  moveServoAngle(2, 0);
}

// Función para agarrar objeto
void agarrarObjeto() {
  Serial.println("Agarrando objeto");
  moveServoAngle(5, 105);
  moveServoAngle(1, 40);
  moveServoAngle(2, 30);
}

// Función para cancelar bajada
void cancelarBajada() {
  Serial.println("Cancelando bajada");
  moveServoAngle(1, 40);
  moveServoAngle(2, 30);
  moveServoAngle(5, 100);
}

// Función para soltar objeto
void soltarObjeto() {
  Serial.println("Soltando objeto");
  moveServoAngle(5, 100);
  moveServoAngle(4, 90);
  moveServoAngle(3, 180);
  moveServoAngle(1, 0);
  moveServoAngle(2, 0);
  publishMQTT("esp8266/angles","anglesDB");
  moveServoAngle(5, 30);
  moveServoAngle(1, 40);
  moveServoAngle(2, 30);
  moveServoAngle(5, 105);
}

/* ================== FUNCIONES MQTT ================== */
//Suscripción a los topics MQTT
void subscribeMQTT() {
  for (String topic : mqttTopics) {
    if (mqttClient.subscribe(topic.c_str())) {
      Serial.print("Suscrito al topic: ");
      Serial.println(topic);
    } else {
      Serial.print("Error al suscribirse al topic: ");
      Serial.println(topic);
    }
  }
}

//Publicación mensaje MQTT
void publishMQTT(const char* topic, String action) {
  DynamicJsonDocument doc(512);
  doc["movement"] = action;
  JsonArray angles = doc.createNestedArray("angles");
  for (int index = 0; index < NUM_SERVOS; index++) {
    angles.add(servos[index].read());
  }
  String message;
  serializeJson(doc, message);
  if (mqttClient.publish(topic, message.c_str())) {
    Serial.println("Mensaje publicado correctamente:");
    Serial.println(message);
  } else {
    Serial.println("Error al publicar el mensaje");
  }
}

//Conexion al broker MQTT
void connectMQTT() {
  Serial.println("Iniciando conexion con el broker MQTT...");
  if (mqttClient.connect(mqttClientName)) {
    subscribeMQTT();
  } else {
    Serial.print("Error al conectarse a MQTT, rc=");
    Serial.print(mqttClient.state());
    Serial.println(" Intento en 5 segundos");
    delay(5000);
  }
}

//Callback de los mensajes MQTT
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String message;
  // Construir el mensaje recibido desde el payload
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  //Procesa el mensaje recibido
  DynamicJsonDocument doc(512);
  DeserializationError error = deserializeJson(doc, message);
  if (error) {
    Serial.println("Error al procesar el JSON");
    Serial.println(error.c_str());
    return;
  }

  //Actua segun el topic del mensaje
  if (strcmp(topic, mqttTopics[0]) == 0) {
    int servoIndex = doc["servo"];
    int targetAngle = doc["angle"];
    if (servoIndex >= 0 && servoIndex < NUM_SERVOS) {
      Serial.print("Moviendo ");
      Serial.print(servoIndex);
      Serial.print(" al angulo: ");
      Serial.println(targetAngle);

      moveServoAngle(servoIndex, targetAngle);
    } else {
      Serial.println("Indice del servo no valido");
    }
  } else if (strcmp(topic, mqttTopics[1]) == 0) {
    const char* movement = doc["movement"];
    if (strcmp(movement, "resetBrazo") == 0) {
      resetServosToRef();
    } else if (strcmp(movement, "bajarBrazo") == 0) {
      bajarBrazo();
    } else if (strcmp(movement, "agarrarBrazo") == 0) {
      agarrarObjeto();
    } else if (strcmp(movement, "cancelarBajada") == 0) {
      cancelarBajada();
    } else if (strcmp(movement, "soltarBrazo") == 0) {
      soltarObjeto();
    }
    publishMQTT("esp8266/predefinedMovement/response",movement);
    publishMQTT("esp8266/angles","anglesDB");
  } else if (strcmp(topic, mqttTopics[2]) == 0) {
    publishMQTT("esp8266/servoAngles/response","servoAngles");
  }
}

/* ================== FUNCIONES INICIALES ================== */
void setup() {
  Serial.begin(9600);

  // Inicializa WiFi usando WiFiHandler
  WiFiHandler.begin();
  delay(5000);

  // Comprueba que este conectado al WiFi
  if (WiFiHandler.isWiFiConnected()) {
    // Inicializa servos
    for (int servoIndex = 0; servoIndex < NUM_SERVOS; servoIndex++) {
      servos[servoIndex].attach(servoPins[servoIndex], MIN_PULSE_WIDTH, MAX_PULSE_WIDTH);
      delay(50);
      servos[servoIndex].write(servoPosRef[servoIndex]);
      delay(50);
    }
    // Inicia MQTT
    mqttClient.setServer(mqttServer, mqttPort);
    mqttClient.setCallback(mqttCallback);
    connectMQTT();
    //Envía mensaje indicando que ha sido iniciado
    publishMQTT("esp8266/startArm", "Arm Started");
  }
}

void loop() {
  // Maneja las peticiones HTTP del AP

  WiFiHandler.handleClient();
  //Reconexion a MQTT
  if (WiFiHandler.isWiFiConnected()) {
    if (!mqttClient.connected()) {
      connectMQTT();
    }
    // Mantener la conexion
    mqttClient.loop();
    //Envia angulos cada 30 segundos a la base de datos
    if (millis() - lastSendTime >= SEND_INTERVAL) {
      publishMQTT("esp8266/angles","anglesDB");
      lastSendTime = millis();
    }
  } else if (WiFiHandler.getmodeWiFi() == 1){
    ESP.restart();
  }
}
