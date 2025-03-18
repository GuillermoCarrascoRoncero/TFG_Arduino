#ifndef WIFI_HANDLER_H
#define WIFI_HANDLER_H

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

class WiFiHandler {
public:
  WiFiHandler(const char* APssid, const char* APPassword);
  void begin();
  void handleClient();
  bool isWiFiConnected();
  WiFiMode_t getmodeWiFi();
  ESP8266WebServer& getServer();
  WiFiClient& getWiFiClient();

private:
  const char* _APssid;
  const char* _APPassword;
  String loadWebPage();
  ESP8266WebServer _server;
  WiFiClient _wifiClient;

  void loadCredentials();
  void startAccessPoint();
  void saveCredentials(const char* WiFissid, const char* WiFiPassword);
  void connectWiFi(const char* WiFissid, const char* WiFiPassword);
};

#endif