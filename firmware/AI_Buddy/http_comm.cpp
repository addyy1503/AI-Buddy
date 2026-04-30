/*
 * AI Buddy — HTTP Communication Implementation
 *
 * Uses ESP32 built-in HTTPClient — no external WebSocket library!
 * Sends raw PCM audio via POST, receives JSON + base64 audio back.
 */

#include "http_comm.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "mbedtls/base64.h"

HttpComm::HttpComm() : _rxBuffer(nullptr) {
    _serverUrl = String("http://") + WS_HOST + ":" + String(WS_PORT) + "/talk";
    
    // Allocate small dummy buffer
    _rxBuffer = (uint8_t*)malloc(MAX_PLAY_BYTES);
}

HttpComm::~HttpComm() {
    if (_rxBuffer) free(_rxBuffer);
}

bool HttpComm::ping() {
    HTTPClient http;
    String pingUrl = String("http://") + WS_HOST + ":" + String(WS_PORT) + "/ping";
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

    int httpCode = http.POST((uint8_t*)data, length);

    if (httpCode != 200) {
        Serial.printf("[HTTP] Failed! Code: %d\n", httpCode);
        http.end();
        return resp;
    }

    // Parse JSON response
    String body = http.getString();
    http.end();

    Serial.printf("[HTTP] Response: %d bytes\n", body.length());

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, body);
    if (err) {
        Serial.printf("[HTTP] JSON error: %s\n", err.c_str());
        return resp;
    }

    resp.emotion = doc["emotion"] | 0;
    resp.text = doc["text"].as<String>();
    resp.durationMs = doc["duration_ms"] | 0;

    // Decode base64 audio
    const char* audioB64 = doc["audio_b64"];
    if (audioB64 && _rxBuffer) {
        size_t b64Len = strlen(audioB64);
        size_t decodedLen = 0;

        int ret = mbedtls_base64_decode(
            _rxBuffer, MAX_PLAY_BYTES, &decodedLen,
            (const unsigned char*)audioB64, b64Len
        );

        if (ret == 0 && decodedLen > 0) {
            resp.audioData = _rxBuffer;
            resp.audioLen = decodedLen;
            Serial.printf("[HTTP] Audio decoded: %d bytes\n", decodedLen);
        }
    }

    resp.valid = true;
    Serial.printf("[HTTP] OK! emotion=%d, text=\"%s\"\n", resp.emotion, resp.text.substring(0, 40).c_str());
    return resp;
}
