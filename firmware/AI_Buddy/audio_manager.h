/*
 * AI Buddy — Audio Manager
 * =========================
 * Handles I2S microphone recording and speaker playback.
 * Time-multiplexes the single I2S peripheral on ESP32-C3.
 */

#ifndef AUDIO_MANAGER_H
#define AUDIO_MANAGER_H

#include <Arduino.h>
#include <driver/i2s_std.h>
#include "config.h"

class AudioManager {
public:
    AudioManager();

    // Initialize (no I2S channel active yet)
    bool begin();

    // ── Recording ──
    // Start recording into internal buffer. Returns when silence detected or max duration reached.
    bool startRecording();
    // Get pointer to recorded audio data
    const uint8_t* getRecordingData() const;
    // Get length of recorded audio in bytes
    size_t getRecordingLength() const;

    // ── Playback ──
    // Play raw PCM audio data through speaker
    bool playAudio(const uint8_t* data, size_t length);

    // ── Status ──
    bool isRecording() const { return _recording; }
    bool isPlaying() const { return _playing; }
    float getCurrentRMS() const { return _currentRMS; }

private:
    bool setupMicrophone();
    void teardownMicrophone();
    bool setupSpeaker();
    void teardownSpeaker();

    i2s_chan_handle_t _rxChan;
    i2s_chan_handle_t _txChan;

    uint8_t* _recordBuffer;
    size_t   _recordLen;
    bool     _recording;
    bool     _playing;
    float    _currentRMS;
    float    _filterY;     // High-pass filter state
    int16_t  _lastSample;  // High-pass filter state
};

#endif // AUDIO_MANAGER_H
