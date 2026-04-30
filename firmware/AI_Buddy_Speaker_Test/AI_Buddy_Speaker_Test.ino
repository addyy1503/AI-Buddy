/*
 * AI Buddy — Speaker Test (MAX98357A)
 * ====================================
 * Plays a 440Hz sine wave tone through the MAX98357A amplifier.
 * Use this to verify your speaker wiring before building the full project.
 *
 * Wiring (ESP32-C3 SuperMini):
 *   MAX98357A VIN  → 5V
 *   MAX98357A GND  → GND
 *   MAX98357A BCLK → GPIO 3
 *   MAX98357A LRC  → GPIO 4
 *   MAX98357A DIN  → GPIO 2
 *   MAX98357A GAIN → Leave floating (9dB) or connect to GND (12dB)
 *
 * Expected: You should hear a clear 440Hz tone from the speaker.
 * If silent: Check wiring, ensure 5V power to MAX98357A, check serial output.
 */

#include <driver/i2s_std.h>
#include <math.h>

// ── Pin Definitions ──
#define SPK_BCLK  3
#define SPK_LRC   4
#define SPK_DOUT  2

// ── Audio Settings ──
#define SAMPLE_RATE   16000
#define TONE_FREQ     440
#define AMPLITUDE     8000  // Volume (max ~32767 for 16-bit)

static i2s_chan_handle_t tx_chan = NULL;

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n=============================");
    Serial.println("  AI Buddy — Speaker Test");
    Serial.println("=============================");
    Serial.println("Playing 440Hz sine wave...");

    // Configure I2S TX channel
    i2s_chan_config_t chan_cfg = {
        .id = I2S_NUM_0,
        .role = I2S_ROLE_MASTER,
        .dma_desc_num = 8,
        .dma_frame_num = 256,
        .auto_clear = true,
    };
    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, &tx_chan, NULL));

    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(SAMPLE_RATE),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = (gpio_num_t)SPK_BCLK,
            .ws   = (gpio_num_t)SPK_LRC,
            .dout = (gpio_num_t)SPK_DOUT,
            .din  = I2S_GPIO_UNUSED,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv   = false,
            },
        },
    };
    ESP_ERROR_CHECK(i2s_channel_init_std_mode(tx_chan, &std_cfg));
    ESP_ERROR_CHECK(i2s_channel_enable(tx_chan));

    Serial.println("I2S TX initialized. You should hear a tone!");
}

void loop() {
    static float phase = 0.0f;
    const float phase_inc = 2.0f * PI * TONE_FREQ / SAMPLE_RATE;

    int16_t samples[256];
    for (int i = 0; i < 256; i++) {
        samples[i] = (int16_t)(AMPLITUDE * sinf(phase));
        phase += phase_inc;
        if (phase >= 2.0f * PI) phase -= 2.0f * PI;
    }

    size_t bytes_written = 0;
    i2s_channel_write(tx_chan, samples, sizeof(samples), &bytes_written, portMAX_DELAY);
}
