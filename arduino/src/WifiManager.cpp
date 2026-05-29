#include "WifiManager.h"
#include <Arduino.h>
#include "index_html.h" // Hier zieht die Klasse dein Frontend her

// Konstruktor: Initialisiert Server (Port 80) und WebSocket (/ws)
WifiManager::WifiManager(const char* ssid, const char* password) 
    : _ssid(ssid), _password(password), _server(80), _ws("/ws") {}

void WifiManager::startAccessPoint() {
    WiFi.softAP(_ssid, _password);
    
    // WebSocket an den Server hängen
    _server.addHandler(&_ws);

    // Wenn jemand die IP aufruft, die index.html ausliefern
    _server.on("/", HTTP_GET, [this](AsyncWebServerRequest *request){
        request->send_P(200, "text/html", INDEX_HTML);
    });

    _server.begin();
    Serial.println("[WiFi] Server and WebSocket started.");
}

void WifiManager::sendLevelStatus(int level, bool success) {
    String statusText = success ? "true" : "false";
    
    // JSON-String bauen: {"level": "level1", "status": "true"}
    String jsonPayload = "{\"level\":\"level" + String(level) + "\",\"status\":\"" + statusText + "\"}";
    
    // Live an ALLE verbundenen Browser-Tabs pushen
    _ws.textAll(jsonPayload);
    Serial.println("[WiFi] Sent to Browser: " + jsonPayload);
}