/*
 * AI Buddy — Audio Manager Implementation
 * =========================================
 * Time-multiplexes the single I2S peripheral between mic (RX) and speaker (TX).
 * Flow: setupMic → record → teardownMic → setupSpeaker → play → teardownSpeaker
 */

#include "audio_manager.h"

AudioManager::AudioManager()
    : _rxChan(NULL), _txChan(NULL),
      _recordBuffer(NULL), _recordLen(0),
      _recording(false), _playing(false), _currentRMS(0),
      _filterY(0), _lastSample(0) {}

bool AudioManager::begin() {
    // Allocate recording buffer in internal RAM
    _recordBuffer = (uint8_t*)malloc(MAX_RECORD_BYTES);
    
    if (!_recordBuffer) {
        Serial.println("[AUDIO] Failed to allocate recording buffer!");
        return false;
    }
    Serial.printf("[AUDIO] Recording buffer: %d bytes allocated\n", MAX_RECORD_BYTES);
    return true;
}

// ══════════════════════════════════════════
//  Microphone (I2S RX)
// ══════════════════════════════════════════

bool AudioManager::setupMicrophone() {
    i2s_chan_config_t chan_cfg = {
        .id = I2S_NUM_0,
        .role = I2S_ROLE_MASTER,
        .dma_desc_num = DMA_BUF_COUNT,
        .dma_frame_num = DMA_BUF_LEN,
        .auto_clear = true,
    };

    esp_err_t err = i2s_new_channel(&chan_cfg, NULL, &_rxChan);
    if (err != ESP_OK) {
        Serial.printf("[AUDIO] Mic channel create failed: %s\n", esp_err_to_name(err));
        return false;
    }

    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(SAMPLE_RATE),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_MONO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = (gpio_num_t)MIC_BCLK_PIN,
            .ws   = (gpio_num_t)MIC_WS_PIN,
            .dout = I2S_GPIO_UNUSED,
            .din  = (gpio_num_t)MIC_DATA_PIN,
            .invert_flags = { false, false, false },
        },
    };
    // INMP441 L/R=GND → LEFT channel
    std_cfg.slot_cfg.slot_mask = I2S_STD_SLOT_LEFT;

    err = i2s_channel_init_std_mode(_rxChan, &std_cfg);
    if (err != ESP_OK) {
        Serial.printf("[AUDIO] Mic init failed: %s\n", esp_err_to_name(err));
        i2s_del_channel(_rxChan);
        _rxChan = NULL;
        return false;
    }

    err = i2s_channel_enable(_rxChan);
    if (err != ESP_OK) {
        Serial.printf("[AUDIO] Mic enable failed: %s\n", esp_err_to_name(err));
        i2s_del_channel(_rxChan);
        _rxChan = NULL;
        return false;
    }

    Serial.println("[AUDIO] Microphone ready");
    return true;
}

void AudioManager::teardownMicrophone() {
    if (_rxChan) {
        i2s_channel_disable(_rxChan);
        i2s_del_channel(_rxChan);
        _rxChan = NULL;
    }
}

// ══════════════════════════════════════════
//  Speaker (I2S TX)
// ══════════════════════════════════════════

bool AudioManager::setupSpeaker() {
    i2s_chan_config_t chan_cfg = {
        .id = I2S_NUM_0,
        .role = I2S_ROLE_MASTER,
        .dma_desc_num = DMA_BUF_COUNT,
        .dma_frame_num = DMA_BUF_LEN,
        .auto_clear = true,
    };

    esp_err_t err = i2s_new_channel(&chan_cfg, &_txChan, NULL);
    if (err != ESP_OK) {
        Serial.printf("[AUDIO] Spk channel create failed: %s\n", esp_err_to_name(err));
        return false;
    }

    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(SAMPLE_RATE),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = (gpio_num_t)SPK_BCLK_PIN,
            .ws   = (gpio_num_t)SPK_LRC_PIN,
            .dout = (gpio_num_t)SPK_DOUT_PIN,
            .din  = I2S_GPIO_UNUSED,
            .invert_flags = { false, false, false },
        },
    };

    err = i2s_channel_init_std_mode(_txChan, &std_cfg);
    if (err != ESP_OK) {
        Serial.printf("[AUDIO] Spk init failed: %s\n", esp_err_to_name(err));
        i2s_del_channel(_txChan);
        _txChan = NULL;
        return false;
    }

    err = i2s_channel_enable(_txChan);
    if (err != ESP_OK) {
        Serial.printf("[AUDIO] Spk enable failed: %s\n", esp_err_to_name(err));
        i2s_del_channel(_txChan);
        _txChan = NULL;
        return false;
    }

    Serial.println("[AUDIO] Speaker ready");
    return true;
}

void AudioManager::teardownSpeaker() {
    if (_txChan) {
        i2s_channel_disable(_txChan);
        i2s_del_channel(_txChan);
        _txChan = NULL;
    }
}

// ══════════════════════════════════════════
//  Recording (with silence detection)
// ══════════════════════════════════════════

bool AudioManager::startRecording() {
    if (_playing) {
        return false;
    }

    if (!setupMicrophone()) return false;

    _recordLen = 0;
    unsigned long silenceStart = 0;
    _filterY = 0;
    _lastSample = 0;

    // Discard first 100ms (I2S warmup produces noise)
    uint8_t discard[1024];
    size_t discardRead;
    for (int i = 0; i < 5; i++) {
        i2s_channel_read(_rxChan, discard, sizeof(discard), &discardRead, 100);
    }

    // ── Phase 1: Monitor for voice activity ──
    bool voiceFound = false;

    while (!voiceFound) {
        // Read 32-bit samples from I2S
        int32_t rawChunk[256];
        size_t bytesRead = 0;

        esp_err_t err = i2s_channel_read(_rxChan, rawChunk, sizeof(rawChunk), &bytesRead, 100);
        if (err != ESP_OK || bytesRead == 0) continue;

        int numSamples = bytesRead / sizeof(int32_t);

        // Convert 32-bit → 16-bit
        int16_t converted[256];
        long long sumSq = 0;
        for (int i = 0; i < numSamples; i++) {
            converted[i] = (int16_t)(rawChunk[i] >> 16);
            
            // High-pass filter (remove low freq hums)
            float alpha = 0.9f; 
            _filterY = alpha * (_filterY + converted[i] - _lastSample);
            _lastSample = converted[i];
            
            int16_t filtered = (int16_t)_filterY;
            sumSq += (long long)filtered * filtered;
        }
        _currentRMS = sqrtf((float)sumSq / numSamples);

        if (_currentRMS > VOICE_THRESHOLD) {
            voiceFound = true;
            size_t copyBytes = numSamples * sizeof(int16_t);
            memcpy(_recordBuffer, converted, copyBytes);
            _recordLen = copyBytes;
            break;
        }
        
        // Feed watchdog to prevent crashes while waiting
        delay(1);
    }

    // ── Phase 2: Speech detected! Record until silence ──
    _recording = true;
    Serial.println("[AUDIO] Voice detected! Recording...");

    while (_recording && _recordLen < MAX_RECORD_BYTES) {
        int32_t rawChunk[256];
        size_t bytesRead = 0;

        esp_err_t err = i2s_channel_read(_rxChan, rawChunk, sizeof(rawChunk), &bytesRead, 100);
        if (err != ESP_OK || bytesRead == 0) continue;

        int numSamples = bytesRead / sizeof(int32_t);

        // Convert 32→16 bit and calculate RMS
        int16_t converted[256];
        long long sumSq = 0;
        for (int i = 0; i < numSamples; i++) {
            converted[i] = (int16_t)(rawChunk[i] >> 16);
            
            // High-pass filter (remove low freq hums)
            float alpha = 0.9f; 
            _filterY = alpha * (_filterY + converted[i] - _lastSample);
            _lastSample = converted[i];
            
            int16_t filtered = (int16_t)_filterY;
            sumSq += (long long)filtered * filtered;
        }
        _currentRMS = sqrtf((float)sumSq / numSamples);

        // Silence detection
        if (_currentRMS > SILENCE_THRESHOLD) {
            silenceStart = 0;
        } else {
            if (silenceStart == 0) {
                silenceStart = millis();
            } else if (millis() - silenceStart > SILENCE_DURATION) {
                Serial.println("[AUDIO] Silence detected — stopping");
                break;
            }
        }

        // Copy 16-bit data to record buffer
        size_t copyBytes = numSamples * sizeof(int16_t);
        copyBytes = min(copyBytes, (size_t)(MAX_RECORD_BYTES - _recordLen));
        memcpy(_recordBuffer + _recordLen, converted, copyBytes);
        _recordLen += copyBytes;
    }

    _recording = false;
    teardownMicrophone();

    Serial.printf("[AUDIO] Recorded %d bytes (%.1f sec)\n",
                  _recordLen, (float)_recordLen / (SAMPLE_RATE * 2));
    return _recordLen > 0;
}

const uint8_t* AudioManager::getRecordingData() const {
    return _recordBuffer;
}

size_t AudioManager::getRecordingLength() const {
    return _recordLen;
}

// ══════════════════════════════════════════
//  Playback
// ══════════════════════════════════════════

bool AudioManager::playAudio(const uint8_t* data, size_t length) {
    if (_recording) {
        Serial.println("[AUDIO] Can't play while recording!");
        return false;
    }

    if (!setupSpeaker()) return false;

    _playing = true;
    Serial.printf("[AUDIO] 🔊 Playing %d bytes...\n", length);

    size_t offset = 0;
    while (offset < length) {
        size_t chunkSize = min((size_t)2048, length - offset);
        size_t bytesWritten = 0;

        esp_err_t err = i2s_channel_write(_txChan, data + offset, chunkSize, &bytesWritten, portMAX_DELAY);
        if (err != ESP_OK) {
            Serial.printf("[AUDIO] Write error: %s\n", esp_err_to_name(err));
            break;
        }
        offset += bytesWritten;
    }

    // Flush with silence to avoid clicks
    uint8_t silence[512] = {0};
    size_t written;
    for (int i = 0; i < 4; i++) {
        i2s_channel_write(_txChan, silence, sizeof(silence), &written, 100);
    }

    _playing = false;
    teardownSpeaker();

    Serial.println("[AUDIO] Playback complete");
    return true;
}
