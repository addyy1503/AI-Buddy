/*
 * AI Buddy — HTTP Communication Implementation
 *
 * Uses ESP32 built-in HTTPClient — no external WebSocket library!
 * Sends raw PCM audio via POST, receives JSON + base64 audio back.
 */

#include "http_comm.h"
#include <HTTPClient.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>
#include "mbedtls/base64.h"

HttpComm::HttpComm(String serverIp) : _rxBuffer(nullptr) {
    _serverUrl = String("http://") + serverIp + ":" + String(WS_PORT) + "/talk";
    
    // Allocate large playback buffer in PSRAM
    _rxBuffer = (uint8_t*)heap_caps_malloc(MAX_PLAY_BYTES, MALLOC_CAP_SPIRAM);
}

HttpComm::~HttpComm() {
    if (_rxBuffer) free(_rxBuffer);
}

bool HttpComm::ping() {
    HTTPClient http;
    String pingUrl = _serverUrl;
    pingUrl.replace("/talk", "/ping");
    http.begin(pingUrl);
    http.setTimeout(3000);
    int code = http.GET();
    http.end();
    return (code == 200);
}

BuddyResponse HttpComm::sendAudio(const uint8_t* data, size_t length) {
    BuddyResponse resp = {0, "", nullptr, 0, false};

    HTTPClient http;
    http.begin(_serverUrl);
    http.addHeader("Content-Type", "application/octet-stream");
    http.setTimeout(30000);  // 30s timeout for AI processing

    Serial.printf("[HTTP] POST %d bytes to %s\n", length, _serverUrl.c_str());

    // Tell HTTPClient to collect our custom headers
    const char* headerKeys[] = {"X-Emotion", "X-Text", "X-Duration-Ms"};
    http.collectHeaders(headerKeys, 3);

    int httpCode = http.POST((uint8_t*)data, length);

    if (httpCode != 200) {
        Serial.printf("[HTTP] Failed! Code: %d\n", httpCode);
        http.end();
        return resp;
    }

    // Extract headers
    resp.emotion = http.header("X-Emotion").toInt();
    resp.text = http.header("X-Text");
    resp.durationMs = http.header("X-Duration-Ms").toInt();

    // Stream binary payload directly to PSRAM rxBuffer
    int len = http.getSize();
    Serial.printf("[HTTP] Binary response: %d bytes\n", len);

    if (len > 0 && len <= MAX_PLAY_BYTES && _rxBuffer) {
        WiFiClient* stream = http.getStreamPtr();
        size_t bytesRead = 0;
        
        while (http.connected() && bytesRead < len) {
            size_t available = stream->available();
            if (available) {
                // Read chunks directly into our PSRAM buffer
                size_t chunk = stream->readBytes(_rxBuffer + bytesRead, min(available, (size_t)(len - bytesRead)));
                bytesRead += chunk;
            }
            delay(1);
        }
        
        if (bytesRead > 0) {
            resp.audioData = _rxBuffer;
            resp.audioLen = bytesRead;
            Serial.printf("[HTTP] Audio streamed to PSRAM: %d bytes\n", bytesRead);
        }
    }

    http.end();
    resp.valid = true;
    Serial.printf("[HTTP] OK! emotion=%d, text=\"%s\"\n", resp.emotion, resp.text.substring(0, 40).c_str());
    return resp;
}
