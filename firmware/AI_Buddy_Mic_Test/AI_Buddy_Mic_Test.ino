/*
 * AI Buddy — OLED Face Test
 * ==========================
 * Displays animated robot eyes on the OLED and cycles through
 * different emotion states every 3 seconds.
 *
 * Wiring (ESP32-C3 SuperMini):
 *   OLED VCC → 3.3V
 *   OLED GND → GND
 *   OLED SDA → GPIO 0
 *   OLED SCL → GPIO 1
 *
 * Supports both 0.96" SSD1306 and 1.3" SH1106 (change USE_SH1106 below)
 *
 * Required Libraries (install via Library Manager):
 *   - Adafruit SSD1306
 *   - Adafruit GFX Library
 *   (If using 1.3" SH1106, also install: Adafruit SH110X)
 */

#include <Wire.h>

// ── Display Configuration ──
// Set to true if you have a 1.3" SH1106 OLED, false for 0.96" SSD1306
#define USE_SH1106 false

#if USE_SH1106
  #include <Adafruit_SH110X.h>
  Adafruit_SH1106G display(128, 64, &Wire, -1);
#else
  #include <Adafruit_SSD1306.h>
  Adafruit_SSD1306 display(128, 64, &Wire, -1);
#endif

#include <Adafruit_GFX.h>

// ── Pin Definitions ──
#define OLED_SDA  0
#define OLED_SCL  1

// ── Emotion States ──
enum Emotion {
    NEUTRAL,
    HAPPY,
    SAD,
    ANGRY,
    LISTENING,
    THINKING,
    SPEAKING,
    SLEEPY,
    EMOTION_COUNT
};

const char* emotionNames[] = {
    "NEUTRAL", "HAPPY", "SAD", "ANGRY",
    "LISTENING", "THINKING", "SPEAKING", "SLEEPY"
};

Emotion currentEmotion = NEUTRAL;
unsigned long lastEmotionChange = 0;
unsigned long lastBlink = 0;
bool isBlinking = false;
int blinkFrame = 0;
float eyeOffsetX = 0;  // For subtle eye movement
float eyeOffsetY = 0;

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("\n===========================");
    Serial.println("  AI Buddy — OLED Face Test");
    Serial.println("===========================");

    Wire.begin(OLED_SDA, OLED_SCL);

    #if USE_SH1106
      if (!display.begin(0x3C, true)) {
    #else
      if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    #endif
        Serial.println("OLED init FAILED! Check wiring.");
        while (1) delay(100);
    }

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(20, 28);
    display.print("AI Buddy v1.0");
    display.display();
    delay(1500);

    Serial.println("OLED initialized! Cycling emotions...");
    lastBlink = millis();
}

void loop() {
    unsigned long now = millis();

    // Cycle emotion every 3 seconds
    if (now - lastEmotionChange > 3000) {
        currentEmotion = (Emotion)((currentEmotion + 1) % EMOTION_COUNT);
        lastEmotionChange = now;
        Serial.printf("Emotion: %s\n", emotionNames[currentEmotion]);
    }

    // Random blink every 2-5 seconds
    if (!isBlinking && now - lastBlink > random(2000, 5000)) {
        isBlinking = true;
        blinkFrame = 0;
        lastBlink = now;
    }

    // Subtle random eye drift
    eyeOffsetX += (random(-10, 11) / 100.0f);
    eyeOffsetY += (random(-10, 11) / 100.0f);
    eyeOffsetX = constrain(eyeOffsetX, -3.0f, 3.0f);
    eyeOffsetY = constrain(eyeOffsetY, -2.0f, 2.0f);

    drawFace(currentEmotion);
    delay(50);  // ~20fps
}

void drawFace(Emotion emotion) {
    display.clearDisplay();

    int leftEyeX = 32 + (int)eyeOffsetX;
    int rightEyeX = 96 + (int)eyeOffsetX;
    int eyeY = 28 + (int)eyeOffsetY;

    switch (emotion) {
        case NEUTRAL:
            drawEyes(leftEyeX, rightEyeX, eyeY, 14, 18);
            break;

        case HAPPY:
            drawHappyEyes(leftEyeX, rightEyeX, eyeY);
            drawMouth(64, 52, HAPPY);
            break;

        case SAD:
            drawSadEyes(leftEyeX, rightEyeX, eyeY);
            drawMouth(64, 52, SAD);
            break;

        case ANGRY:
            drawAngryEyes(leftEyeX, rightEyeX, eyeY);
            break;

        case LISTENING:
            drawEyes(leftEyeX, rightEyeX, eyeY, 16, 20);  // Wide eyes
            drawListeningIndicator();
            break;

        case THINKING:
            drawThinkingEyes(leftEyeX, rightEyeX, eyeY);
            drawThinkingDots();
            break;

        case SPEAKING:
            drawEyes(leftEyeX, rightEyeX, eyeY, 14, 18);
            drawSpeakingMouth();
            break;

        case SLEEPY:
            drawSleepyEyes(leftEyeX, rightEyeX, eyeY);
            break;
    }

    // Draw blink overlay
    if (isBlinking) {
        drawBlink(leftEyeX, rightEyeX, eyeY);
        blinkFrame++;
        if (blinkFrame > 4) isBlinking = false;
    }

    // Show emotion name at bottom
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 56);
    display.print(emotionNames[emotion]);

    display.display();
}

// ── Eye Drawing Functions ──

void drawEyes(int lx, int rx, int y, int w, int h) {
    // Rounded rectangle eyes
    display.fillRoundRect(lx - w/2, y - h/2, w, h, 4, SSD1306_WHITE);
    display.fillRoundRect(rx - w/2, y - h/2, w, h, 4, SSD1306_WHITE);
    // Pupils
    display.fillCircle(lx + (int)eyeOffsetX, y + (int)eyeOffsetY, 3, SSD1306_BLACK);
    display.fillCircle(rx + (int)eyeOffsetX, y + (int)eyeOffsetY, 3, SSD1306_BLACK);
}

void drawHappyEyes(int lx, int rx, int y) {
    // Upward arcs (^  ^)
    for (int i = -8; i <= 8; i++) {
        int h = -abs(i) + 8;
        display.drawPixel(lx + i, y - h/2, SSD1306_WHITE);
        display.drawPixel(lx + i, y - h/2 - 1, SSD1306_WHITE);
        display.drawPixel(rx + i, y - h/2, SSD1306_WHITE);
        display.drawPixel(rx + i, y - h/2 - 1, SSD1306_WHITE);
    }
}

void drawSadEyes(int lx, int rx, int y) {
    // Droopy eyes
    display.fillRoundRect(lx - 7, y - 6, 14, 14, 4, SSD1306_WHITE);
    display.fillRoundRect(rx - 7, y - 6, 14, 14, 4, SSD1306_WHITE);
    // Droopy eyelids (triangles covering top)
    display.fillTriangle(lx - 9, y - 8, lx + 9, y - 8, lx + 9, y, SSD1306_BLACK);
    display.fillTriangle(rx - 9, y - 8, rx - 9, y, rx + 9, y - 8, SSD1306_BLACK);
    // Pupils looking down
    display.fillCircle(lx, y + 2, 2, SSD1306_BLACK);
    display.fillCircle(rx, y + 2, 2, SSD1306_BLACK);
}

void drawAngryEyes(int lx, int rx, int y) {
    // Squinted eyes
    display.fillRoundRect(lx - 8, y - 4, 16, 10, 3, SSD1306_WHITE);
    display.fillRoundRect(rx - 8, y - 4, 16, 10, 3, SSD1306_WHITE);
    // Angry eyebrows (angled lines)
    display.drawLine(lx - 10, y - 10, lx + 8, y - 6, SSD1306_WHITE);
    display.drawLine(lx - 10, y - 11, lx + 8, y - 7, SSD1306_WHITE);
    display.drawLine(rx + 10, y - 10, rx - 8, y - 6, SSD1306_WHITE);
    display.drawLine(rx + 10, y - 11, rx - 8, y - 7, SSD1306_WHITE);
    // Pupils
    display.fillCircle(lx, y, 2, SSD1306_BLACK);
    display.fillCircle(rx, y, 2, SSD1306_BLACK);
}

void drawThinkingEyes(int lx, int rx, int y) {
    // One eye normal, one squinted — looking to the side
    display.fillRoundRect(lx - 7, y - 9, 14, 18, 4, SSD1306_WHITE);
    display.fillRoundRect(rx - 7, y - 3, 14, 8, 3, SSD1306_WHITE);
    // Pupils looking up-right
    display.fillCircle(lx + 3, y - 3, 3, SSD1306_BLACK);
    display.fillCircle(rx + 3, y, 2, SSD1306_BLACK);
}

void drawSleepyEyes(int lx, int rx, int y) {
    // Horizontal lines (- -)
    display.fillRoundRect(lx - 8, y - 2, 16, 4, 2, SSD1306_WHITE);
    display.fillRoundRect(rx - 8, y - 2, 16, 4, 2, SSD1306_WHITE);
    // Z z z
    int t = (millis() / 500) % 3;
    display.setTextSize(1);
    if (t >= 0) { display.setCursor(108, 10); display.print("z"); }
    if (t >= 1) { display.setCursor(114, 4);  display.print("z"); }
    if (t >= 2) { display.setCursor(120, -2); display.print("Z"); }
}

void drawBlink(int lx, int rx, int y) {
    int blinkHeight = 0;
    if (blinkFrame <= 2) blinkHeight = blinkFrame * 10;
    else blinkHeight = (4 - blinkFrame) * 10;
    // Cover eyes from top
    display.fillRect(lx - 12, y - 12, 24, blinkHeight, SSD1306_BLACK);
    display.fillRect(rx - 12, y - 12, 24, blinkHeight, SSD1306_BLACK);
}

void drawMouth(int x, int y, Emotion e) {
    if (e == HAPPY) {
        // Smile arc
        for (int i = -12; i <= 12; i++) {
            int dy = (i * i) / 12;
            display.drawPixel(x + i, y + dy, SSD1306_WHITE);
        }
    } else if (e == SAD) {
        // Frown arc
        for (int i = -10; i <= 10; i++) {
            int dy = -(i * i) / 10;
            display.drawPixel(x + i, y + 4 + dy, SSD1306_WHITE);
        }
    }
}

void drawListeningIndicator() {
    // Pulsing circles on the sides
    int r = 2 + (millis() / 200) % 4;
    display.drawCircle(10, 32, r, SSD1306_WHITE);
    display.drawCircle(118, 32, r, SSD1306_WHITE);
}

void drawThinkingDots() {
    int dot = (millis() / 400) % 4;
    for (int i = 0; i < 3; i++) {
        if (i < dot) {
            display.fillCircle(52 + i * 12, 52, 2, SSD1306_WHITE);
        } else {
            display.drawCircle(52 + i * 12, 52, 2, SSD1306_WHITE);
        }
    }
}

void drawSpeakingMouth() {
    // Animated mouth opening/closing
    int mouthH = 2 + abs((int)(6 * sin(millis() / 100.0)));
    display.fillRoundRect(52, 48 - mouthH/2, 24, mouthH, 3, SSD1306_WHITE);
}
