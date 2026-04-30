/*
 * WiFi Connection Test v2 — ESP32-C3
 * Clean connection without scan interference
 */

#include <WiFi.h>

#define WIFI_SSID     "Addyyy"
#define WIFI_PASSWORD "123456789"

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n=== WiFi Test v2 ===\n");

    // Clean slate
    WiFi.disconnect(true, true);
    WiFi.mode(WIFI_OFF);
    delay(1000);

    // Connect
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    delay(100);

    Serial.printf("Connecting to '%s' with password '%s'...\n", WIFI_SSID, WIFI_PASSWORD);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 60) {
        delay(500);
        int s = WiFi.status();
        Serial.printf("  [%d/60] status=%d", attempts + 1, s);
        if (s == 0) Serial.print(" (IDLE)");
        if (s == 1) Serial.print(" (SSID_NOT_FOUND)");
        if (s == 4) Serial.print(" (WRONG_PASSWORD)");
        if (s == 5) Serial.print(" (CONNECTION_LOST)");
        if (s == 6) Serial.print(" (DISCONNECTED)");
        if (s == 3) Serial.print(" (CONNECTED!)");
        Serial.println();
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("\n*** CONNECTED! IP: %s ***\n", WiFi.localIP().toString().c_str());
    } else {
        Serial.println("\n*** FAILED ***");
        Serial.println("Try: Change hotspot password on phone, then update here");
    }
}

void loop() {
    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("Connected! IP: %s\n", WiFi.localIP().toString().c_str());
    } else {
        Serial.printf("Not connected. Status: %d. Retrying...\n", WiFi.status());
        WiFi.disconnect(true);
        delay(1000);
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    }
    delay(10000);
}
