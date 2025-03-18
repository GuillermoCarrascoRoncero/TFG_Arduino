#include "ESP8266WiFi.h"
#include "WiFiHandler.h"

WiFiHandler::WiFiHandler(const char* APssid, const char* APPassword)
  : _APssid(APssid), _APPassword(APPassword), _server(80) {}

void WiFiHandler::begin() {
  if (!LittleFS.begin()) {
    Serial.println("Error al iniciar LittleFS");
    return;
  }
  Serial.println("Intentando conectar al WiFi...");
  loadCredentials();
}

void WiFiHandler::loadCredentials() {
  if (LittleFS.exists("/wifi.json")) {
    File wifiFile = LittleFS.open("/wifi.json", "r");
    if (wifiFile) {
      DynamicJsonDocument doc(512);
      DeserializationError error = deserializeJson(doc, wifiFile);
      wifiFile.close();

      if (error) {
        Serial.println("Error al deserializar el JSON");
        startAccessPoint();
        return;
      }

      if (!doc.containsKey("ssid") || !doc.containsKey("password")) {
        Serial.println("Error credenciales incompletas");
        startAccessPoint();
        return;
      }

      const char* ssid = doc["ssid"];
      const char* password = doc["password"];
      connectWiFi(ssid, password);
    }
  } else {
    Serial.println("Archivo no encontrado. Iniciando AP...");
    startAccessPoint();
  }
}

void WiFiHandler::connectWiFi(const char* WiFissid, const char* WiFiPassword) {
  WiFi.begin(WiFissid, WiFiPassword);
  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startTime < 10000) {
    Serial.println("Conectando al WiFi.....");
    delay(500);
  }
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Error al conectarse a la WiFi. Iniciando AP...");
    startAccessPoint();
  } else {
    Serial.println("Conectado a la WiFi");
    Serial.print("IP del ESP8266: ");
    Serial.println(WiFi.localIP());
    Serial.print("SSID WiFi: ");
    Serial.println(WiFi.SSID());
  }
  delay(3000);
}

String WiFiHandler::loadWebPage() {
  String page = "<html><head><title>Configuración WiFi</title>";
  page += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  page += "<style>";
  page += "body { font-family: 'Roboto', sans-serif; display: flex; text-align: center; align-items: center; justify-content: center; min-height: 100vh; margin: 0; background-color: #e0e0e0; color: #333; }";
  page += ".container { background-color: rgba(255, 255, 255, 0.6); border-radius: 25px; padding: 25px; max-width: 450px; width: 90%; backdrop-filter: blur(15px); box-shadow: 9px 9px 16px #b8b9be, -9px -9px 16px #ffffff; }";
  page += "h1 { text-align: center; color: #333; margin-top: 0px; font-size: 30px; }";
  page += "button { margin: 0 auto 20px; padding: 10px 20px; font-size: 16px; color: #fff; background: linear-gradient(145deg, #4a4a4a, #3a3a3a); border: none; border-radius: 12px; cursor: pointer; margin-top: 5px; margin-bottom: 0px; transition: all 0.3s; box-shadow: 4px 4px 10px #b8b9be, -4px -4px -10px #ffffff; }";
  page += "select { width: 100%; padding: 8px; font-size: 16px; margin-top: 10px; border-radius: 12px; border: 1px solid #ccc; background-color: #f5f5f5; }";
  page += "input[type='password'] { width: 100%; padding: 8px; font-size: 16px; margin-top: 10px; border-radius: 12px; border: 1px solid #ccc; background-color: #f5f5f5; }";
  page += "</style>";

  page += "</head><body>";
  page += "<div class='container'>";
  page += "<h1>Configuracion WiFi</h1>";
  page += "<form method='POST' action='/config' accept-charset='utf-8'>";
  page += "SSID: <select name='ssid'>";

  int n = WiFi.scanNetworks();
  for (int i = 0; i < n; ++i) {
    page += "<option value='" + WiFi.SSID(i) + "'>" + WiFi.SSID(i) + "</option>";
  }

  page += "</select><br>";
  page += "Password: <input type='password' name='password'><br>";
  page += "<button type='submit'>Guardar</button>";
  page += "</form>";
  page += "</div>";
  page += "</body></html>";

  return page;
}

void WiFiHandler::startAccessPoint() {
  Serial.println("Iniciando AP...");
  WiFi.softAP(_APssid, _APPassword);
  Serial.print("IP del AP: ");
  Serial.println(WiFi.softAPIP());

  _server.onNotFound([this]() {
    _server.sendHeader("Location", "http://192.168.4.1/", true);
    _server.send(302, "text/plain", "");
  });

  // Página principal del portal cautivo
  _server.on("/", HTTP_GET, [this]() {
    _server.send(200, "text/html", loadWebPage());
  });

  _server.on("/config", HTTP_POST, [this]() {
    String ssid = _server.arg("ssid");
    String password = _server.arg("password");

    if (ssid.length() > 0 && password.length() > 0) {
      saveCredentials(ssid.c_str(), password.c_str());
      _server.send(200, "text/html", "Guardado, reiniciando...");
      delay(2000);
      ESP.restart();
    } else {
      _server.send(400, "text/html", "Parámetros inválidos.");
    }
  });

  _server.begin();
}

void WiFiHandler::saveCredentials(const char* WiFissid, const char* WiFiPassword) {
  if (WiFissid == nullptr || WiFiPassword == nullptr) {
    Serial.println("Error SSID o contraseña nulos");
    return;
  }

  DynamicJsonDocument doc(512);
  doc["ssid"] = WiFissid;
  doc["password"] = WiFiPassword;

  File wifiFile = LittleFS.open("/wifi.json", "w");
  if (wifiFile) {
    serializeJson(doc, wifiFile);
    wifiFile.close();
  }
}

void WiFiHandler::handleClient() {
  _server.handleClient();
}

bool WiFiHandler::isWiFiConnected() {
  return WiFi.status() == WL_CONNECTED;
}
WiFiMode_t WiFiHandler::getmodeWiFi() {
  return WiFi.getMode();
}

ESP8266WebServer& WiFiHandler::getServer() {
  return _server;
}

WiFiClient& WiFiHandler::getWiFiClient() {
  return _wifiClient;
}