/*
 * ╔═══════════════════════════════════════╗
 * ║        AI Buddy — Main Sketch         ║
 * ║  ESP32-S3 Super Mini AI Desk Robot    ║
 * ╚═══════════════════════════════════════╝
 *
 * Uses WiFi + HTTP POST (no WebSocket library!)
 *
 * How it works:
 *   1. Connects to WiFi (2.4GHz)
 *   2. Mic listens for speech
 *   3. Voice detected → records to PSRAM → HTTP POST
 *   4. Backend: STT → Gemini LLM → TTS
 *   5. Response comes back with Base64 audio
 *   6. ESP32-S3 plays audio + shows matching face
 *
 * Board Settings:
 *   Board: ESP32S3 Dev Module
 *   USB CDC On Boot: Enabled
 *   PSRAM: OPI PSRAM (Crucial for Audio Buffers!)
 *   Flash Size: 4MB or 8MB
 */

#include <WiFi.h>
#include <WiFiUdp.h>
#include <esp_wifi.h>
#include "config.h"
#include "audio_manager.h"
#include "display_manager.h"
#include "http_comm.h"

// ── Global Objects ──
AudioManager   audio;
DisplayManager display;
HttpComm*      comm = nullptr;  // Created after WiFi connects

// ── State Machine ──
enum BuddyState {
    STATE_IDLE,
    STATE_SENDING,
    STATE_SPEAKING,
    STATE_COOLDOWN
};

BuddyState currentState = STATE_IDLE;
unsigned long cooldownStart = 0;
bool wifiConnected = false;

// ══════════════════════════════════════
//  WiFi Connection
// ══════════════════════════════════════

bool connectWiFiDirect() {
    Serial.printf("[WIFI] Connecting to %s\n", WIFI_SSID);

    WiFi.disconnect(true, true);
    WiFi.mode(WIFI_OFF);
    delay(1000);

    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    delay(100);

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 60) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("[WIFI] Connected! IP: %s\n", WiFi.localIP().toString().c_str());
        return true;
    } else {
        Serial.printf("[WIFI] FAILED (status: %d)\n", WiFi.status());
        return false;
    }
}

String discoverServer() {
    WiFiUDP udp;
    udp.begin(8766);
    Serial.println("[WIFI] Listening for server UDP broadcast on port 8766...");
    display.showStatus("Finding Server", "Listening...");
    
    char packetBuffer[255];
    unsigned long startWait = millis();
    
    while (true) {
        int packetSize = udp.parsePacket();
        if (packetSize) {
            int len = udp.read(packetBuffer, 255);
            if (len > 0) packetBuffer[len] = 0;
            String msg = String(packetBuffer);
            if (msg.startsWith("AI_BUDDY_SERVER")) {
                String ip = udp.remoteIP().toString();
                Serial.printf("[WIFI] Found Server at %s\n", ip.c_str());
                udp.stop();
                return ip;
            }
        }
        
        // Show animation
        if (millis() - startWait > 50) {
            display.update();
            startWait = millis();
        }
        delay(1);
    }
}

// ══════════════════════════════════════
//  Setup
// ══════════════════════════════════════

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n==============================");
    Serial.println("     AI Buddy Starting");
    Serial.println("==============================");

    // ── Step 1: WiFi FIRST (nothing else initialized) ──
    wifiConnected = connectWiFiDirect();
    if (wifiConnected) {
        esp_wifi_set_ps(WIFI_PS_NONE); // Disable Power Save to prevent hotspot drops
    }

    // ── Step 2: Display ──
    if (!display.begin()) {
        Serial.println("[MAIN] OLED failed");
    }

    if (wifiConnected) {
        display.showStatus("WiFi OK!", WiFi.localIP().toString().c_str());
    } else {
        display.showStatus("WiFi FAILED!", "Offline mode");
    }
    delay(1500);

    // ── Step 3: Audio ──
    display.showStatus("Init audio...", "");
    if (!audio.begin()) {
        Serial.println("[MAIN] Audio init failed!");
        display.showStatus("Audio FAILED!", "Check wiring");
        while (1) delay(1000);
    }

    // ── Step 4: HTTP Comm (only if WiFi connected) ──
    if (wifiConnected) {
        String serverIp = discoverServer();
        comm = new HttpComm(serverIp);

        // Quick server check
        display.showStatus("Checking server...", "");
        if (comm->ping()) {
            Serial.println("[MAIN] Server is reachable!");
            display.showStatus("Server OK!", "Talk to me!");
        } else {
            Serial.println("[MAIN] Server not reachable (will retry on speak)");
            display.showStatus("Server offline", "Will retry...");
        }
    }

    delay(1000);
    display.setEmotion(EMO_NEUTRAL);
    display.clearMessage();
    Serial.println("[MAIN] Ready! Start speaking.");
    Serial.printf("[MAIN] Voice threshold: %d RMS\n", VOICE_THRESHOLD);
    Serial.printf("[MAIN] Free heap: %d bytes\n", ESP.getFreeHeap());
    Serial.printf("[MAIN] Free PSRAM: %d bytes\n", ESP.getFreePsram());
}

// ══════════════════════════════════════
//  Main Loop
// ══════════════════════════════════════

unsigned long lastWifiRetry = 0;

void loop() {
    // Auto-retry WiFi if not connected (every 15 sec)
    if (!wifiConnected || WiFi.status() != WL_CONNECTED) {
        if (millis() - lastWifiRetry > 15000) {
            Serial.println("[WIFI] Retrying connection...");
            display.showMessage("Connecting...");
            display.update();
            wifiConnected = connectWiFiDirect();
            lastWifiRetry = millis();
            if (wifiConnected) {
                if (comm) { delete comm; }
                String serverIp = discoverServer();
                comm = new HttpComm(serverIp);
            }
            display.clearMessage();
        }
    }

    switch (currentState) {
        case STATE_IDLE:
            display.setEmotion(EMO_NEUTRAL);
            if (audio.startRecording()) {
                currentState = STATE_SENDING;
            }
            break;

        case STATE_SENDING: {
            display.setEmotion(EMO_THINKING);
            display.showMessage("Thinking...");
            display.update();

            if (!wifiConnected || !comm) {
                Serial.println("[MAIN] Not connected! Can't send audio.");
                display.setEmotion(EMO_SAD);
                display.showMessage("No WiFi!");
                display.update();
                delay(2000);
                display.clearMessage();
                currentState = STATE_IDLE;
                break;
            }

            // Send audio via HTTP and get response
            Serial.println("[MAIN] Sending audio to server...");
            BuddyResponse resp = comm->sendAudio(
                audio.getRecordingData(),
                audio.getRecordingLength()
            );

            if (!resp.valid) {
                Serial.println("[MAIN] Server error!");
                display.setEmotion(EMO_SAD);
                display.showMessage("Server error");
                display.update();
                delay(2000);
                display.clearMessage();
                currentState = STATE_IDLE;
                
                // IP might have changed or hotspot dropped the route
                Serial.println("[MAIN] Rediscovering server...");
                if (comm) { delete comm; }
                String serverIp = discoverServer();
                comm = new HttpComm(serverIp);
                break;
            }

            // Set emotion from response
            if (resp.emotion >= 0 && resp.emotion < EMO_COUNT) {
                display.setEmotion((Emotion)resp.emotion);
            }

            // Show response text
            if (resp.text.length() > 0) {
                Serial.printf("[MAIN] AI: %s\n", resp.text.c_str());
            }

            // Play audio if we got some from the server to play on the internal speaker
            if (resp.audioData && resp.audioLen > 0) {
                display.setEmotion(EMO_SPEAKING);
                display.showMessage("Speaking...");
                display.update();

                audio.playAudio(resp.audioData, resp.audioLen);
            } 
            // Otherwise, if the laptop is playing it, wait for the audio to finish!
            else if (resp.durationMs > 0) {
                display.setEmotion(EMO_SPEAKING);
                display.showMessage("Speaking...");
                
                Serial.printf("[MAIN] Waiting %d ms for laptop to finish speaking...\n", resp.durationMs);
                
                unsigned long startWait = millis();
                while (millis() - startWait < resp.durationMs) {
                    display.update();
                    delay(50); // Animation frame rate
                }
            }

            // Done — cooldown
            display.clearMessage();
            display.setEmotion(EMO_HAPPY);
            cooldownStart = millis();
            currentState = STATE_COOLDOWN;
            break;
        }

        case STATE_SPEAKING:
            // Not used in HTTP mode
            break;

        case STATE_COOLDOWN:
            if (millis() - cooldownStart > COOLDOWN_MS) {
                Serial.println("[MAIN] Listening again...");
                currentState = STATE_IDLE;
            }
            break;
    }

    display.update();
    delay(50);
}
