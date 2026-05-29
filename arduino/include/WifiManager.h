#pragma once
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>

class WifiManager {
private:
    const char* _ssid;
    const char* _password;
    AsyncWebServer _server; // Neuer Webserver auf dem ESP
    AsyncWebSocket _ws;     // Neuer WebSocket-Kanal

public:
    WifiManager(const char* ssid, const char* password);
    void startAccessPoint();
    
    // NEUE METHODE: Sagt der Webseite live Bescheid!
    void sendLevelStatus(int level, bool success);
};