/*
 * AI Buddy — Configuration
 * =========================
 * Specifically tuned for the ESP32-S3 Super Mini with PSRAM.
 */

#ifndef CONFIG_H
#define CONFIG_H

// ═══════════════════════════════════════
//  WiFi Settings
// ═══════════════════════════════════════
#define WIFI_SSID     "Addyyy"
#define WIFI_PASSWORD "123456789"

// ═══════════════════════════════════════
//  Backend Server
// ═══════════════════════════════════════
// IP address of the PC running the Python backend
#define WS_HOST       "192.168.1.4"
#define WS_PORT       8765

// ═══════════════════════════════════════
//  INMP441 Microphone (I2S RX)
// ═══════════════════════════════════════
#define MIC_BCLK_PIN  11
#define MIC_WS_PIN    12
#define MIC_DATA_PIN  13

// ═══════════════════════════════════════
//  MAX98357A Speaker (I2S TX)
// ═══════════════════════════════════════
#define SPK_BCLK_PIN  1
#define SPK_LRC_PIN   2
#define SPK_DOUT_PIN  4

// ═══════════════════════════════════════
//  SSD1306 / SH1106 OLED Display (I2C)
// ═══════════════════════════════════════
#define OLED_SDA_PIN  5
#define OLED_SCL_PIN  6
#define OLED_ADDR     0x3C
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64

// Set to true if using a 1.3" SH1106 OLED
// Set to false for 0.96" SSD1306
#define USE_SH1106    false

// ═══════════════════════════════════════
//  Voice Activation (No button needed!)
// ═══════════════════════════════════════
// The mic listens continuously. When volume exceeds VOICE_THRESHOLD,
// recording starts. It stops after SILENCE_DURATION ms of quiet.
#define VOICE_THRESHOLD   1000    // RMS above this = speech detected (start recording)
#define COOLDOWN_MS       3000    // Wait this long after playback before listening again

// ═══════════════════════════════════════
//  Audio Parameters (PSRAM optimized)
// ═══════════════════════════════════════
#define SAMPLE_RATE       16000   // 16kHz — optimal for speech
#define BITS_PER_SAMPLE   16
#define DMA_BUF_COUNT     8
#define DMA_BUF_LEN       1024    // Samples per DMA buffer

// Maximum recording duration
#define MAX_RECORD_SECS   4
#define MAX_RECORD_BYTES  (SAMPLE_RATE * 2 * MAX_RECORD_SECS)  // ~128KB, easily fits in Internal RAM

// Playback buffer needs to hold the entire generated response audio
#define MAX_PLAY_BYTES    150000 

// ═══════════════════════════════════════
//  Silence Detection (auto-stop recording)
// ═══════════════════════════════════════
#define SILENCE_THRESHOLD 600     // RMS below this = silence
#define SILENCE_DURATION  1500    // ms of silence before auto-stop

// ═══════════════════════════════════════
//  Timing
// ═══════════════════════════════════════
#define WIFI_TIMEOUT_MS   30000   // WiFi connection timeout (30 sec)

#endif // CONFIG_H
