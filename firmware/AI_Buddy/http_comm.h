/*
 * AI Buddy — HTTP Communication
 * ===============================
 * Simple HTTP POST to send audio and receive response.
 * No WebSocket library needed — uses built-in ESP32 HTTPClient.
 *
 * Flow:
 *   1. POST audio PCM to http://<host>:8765/talk
 *   2. Server responds with JSON: {"emotion": N, "text": "...", "audio_b64": "..."}
 */

#ifndef HTTP_COMM_H
#define HTTP_COMM_H

#include <Arduino.h>
#include "config.h"

// Response from server
struct BuddyResponse {
    int emotion;
    String text;
    uint8_t* audioData;
    size_t audioLen;
    int durationMs;
    bool valid;
};

class HttpComm {
public:
    HttpComm(String serverIp);
    ~HttpComm();

    // Send recorded audio, get back response
    BuddyResponse sendAudio(const uint8_t* data, size_t length);

    // Check if server is reachable
    bool ping();

private:
    String _serverUrl;
    uint8_t* _rxBuffer;
};

#endif // HTTP_COMM_H
