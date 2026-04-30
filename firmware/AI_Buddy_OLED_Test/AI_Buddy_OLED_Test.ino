/*
 * AI Buddy — OLED Cute Face Test
 * ================================
 * Preview all 8 cute face emotions!
 * Cycles through each emotion every 3 seconds.
 *
 * Wiring: SDA→GPIO 0, SCL→GPIO 1, VCC→3.3V, GND→GND
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

Adafruit_SSD1306 display(128, 64, &Wire, -1);

#define OLED_SDA 0
#define OLED_SCL 1

enum Emotion {
    NEUTRAL, HAPPY, SAD, ANGRY,
    LISTENING, THINKING, SPEAKING, SLEEPY,
    EMOTION_COUNT
};

const char* emoNames[] = {
    "NEUTRAL", "HAPPY", "SAD", "ANGRY",
    "LISTENING", "THINKING", "SPEAKING", "SLEEPY"
};

Emotion currentEmo = NEUTRAL;
unsigned long lastChange = 0;
unsigned long lastBlink = 0;
bool isBlinking = false;
int blinkFrame = 0;
float eyeX = 0, eyeY = 0, pupilX = 0, pupilY = 0;

void setup() {
    Serial.begin(115200);
    delay(500);
    Wire.begin(OLED_SDA, OLED_SCL);

    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println("OLED FAILED!");
        while (1);
    }

    // Splash screen
    display.clearDisplay();
    display.fillRoundRect(30, 8, 26, 26, 10, SSD1306_WHITE);
    display.fillRoundRect(72, 8, 26, 26, 10, SSD1306_WHITE);
    display.fillCircle(43, 21, 6, SSD1306_BLACK);
    display.fillCircle(85, 21, 6, SSD1306_BLACK);
    display.fillCircle(40, 17, 2, SSD1306_WHITE);
    display.fillCircle(82, 17, 2, SSD1306_WHITE);
    for (int i = -10; i <= 10; i++) {
        display.drawPixel(64 + i, 40 + (i*i)/12, SSD1306_WHITE);
    }
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(30, 54);
    display.print("AI Buddy v1.0");
    display.display();
    delay(2000);

    Serial.println("Cute face test — cycling emotions!");
    lastBlink = millis();
}

void loop() {
    unsigned long now = millis();

    if (now - lastChange > 3000) {
        currentEmo = (Emotion)((currentEmo + 1) % EMOTION_COUNT);
        lastChange = now;
        Serial.printf("Emotion: %s\n", emoNames[currentEmo]);
    }

    // Random blink
    if (!isBlinking && now - lastBlink > (unsigned long)random(2500, 5000)) {
        isBlinking = true;
        blinkFrame = 0;
        lastBlink = now;
    }

    // Smooth eye drift
    eyeX += random(-10, 11) / 200.0f;
    eyeY += random(-10, 11) / 250.0f;
    eyeX = constrain(eyeX, -2.5f, 2.5f);
    eyeY = constrain(eyeY, -1.5f, 1.5f);
    pupilX += (eyeX * 1.5f - pupilX) * 0.3f;
    pupilY += (eyeY * 1.5f - pupilY) * 0.3f;

    drawFace(currentEmo);
    delay(40);  // ~25fps
}

// ═══════ Drawing Functions ═══════

void drawCuteEyes(int lx, int rx, int y, int w, int h) {
    display.fillRoundRect(lx - w/2, y - h/2, w, h, 10, SSD1306_WHITE);
    display.fillRoundRect(rx - w/2, y - h/2, w, h, 10, SSD1306_WHITE);
    int px = (int)(pupilX * 2);
    int py = (int)(pupilY * 2);
    display.fillCircle(lx + px, y + py + 1, 6, SSD1306_BLACK);
    display.fillCircle(rx + px, y + py + 1, 6, SSD1306_BLACK);
    // Shiny highlights ✨
    display.fillCircle(lx + px - 3, y + py - 3, 2, SSD1306_WHITE);
    display.fillCircle(rx + px - 3, y + py - 3, 2, SSD1306_WHITE);
    display.fillCircle(lx + px + 2, y + py + 2, 1, SSD1306_WHITE);
    display.fillCircle(rx + px + 2, y + py + 2, 1, SSD1306_WHITE);
}

void drawBlink(int lx, int rx, int y) {
    int maxH = 30;
    int bh = (blinkFrame <= 2) ? (blinkFrame * maxH) / 2 : ((5 - blinkFrame) * maxH) / 2;
    display.fillRect(lx - 16, y - 16, 32, bh, SSD1306_BLACK);
    display.fillRect(rx - 16, y - 16, 32, bh, SSD1306_BLACK);
    if (bh > 2 && bh < maxH - 2) {
        display.drawLine(lx - 12, y - 16 + bh, lx + 12, y - 16 + bh, SSD1306_WHITE);
        display.drawLine(rx - 12, y - 16 + bh, rx + 12, y - 16 + bh, SSD1306_WHITE);
    }
}

void drawFace(Emotion emo) {
    display.clearDisplay();

    int lx = 38 + (int)eyeX;
    int rx = 90 + (int)eyeX;
    int ey = 22 + (int)eyeY;

    switch (emo) {
        case NEUTRAL:
            drawCuteEyes(lx, rx, ey, 24, 26);
            break;

        case HAPPY: {
            // Happy squished arcs ^_^
            for (int i = -11; i <= 11; i++) {
                float n = (float)i / 11.0f;
                int c = (int)(10.0f * (1.0f - n * n));
                display.drawPixel(lx + i, ey - c, SSD1306_WHITE);
                display.drawPixel(lx + i, ey - c + 1, SSD1306_WHITE);
                display.drawPixel(lx + i, ey - c + 2, SSD1306_WHITE);
                display.drawPixel(rx + i, ey - c, SSD1306_WHITE);
                display.drawPixel(rx + i, ey - c + 1, SSD1306_WHITE);
                display.drawPixel(rx + i, ey - c + 2, SSD1306_WHITE);
            }
            // Blush marks
            display.fillCircle(lx - 14, ey + 4, 3, SSD1306_WHITE);
            display.fillCircle(lx - 14, ey + 4, 1, SSD1306_BLACK);
            display.fillCircle(rx + 14, ey + 4, 3, SSD1306_WHITE);
            display.fillCircle(rx + 14, ey + 4, 1, SSD1306_BLACK);
            // Cute smile
            for (int i = -16; i <= 16; i++) {
                float n = (float)i / 16.0f;
                display.drawPixel(64 + i, 46 + (int)(6.0f * n * n), SSD1306_WHITE);
                display.drawPixel(64 + i, 47 + (int)(6.0f * n * n), SSD1306_WHITE);
            }
            break;
        }

        case SAD: {
            display.fillRoundRect(lx - 11, ey - 10, 22, 22, 9, SSD1306_WHITE);
            display.fillRoundRect(rx - 11, ey - 10, 22, 22, 9, SSD1306_WHITE);
            display.fillTriangle(lx - 14, ey - 14, lx + 14, ey - 14, lx + 14, ey - 2, SSD1306_BLACK);
            display.fillTriangle(rx - 14, ey - 14, rx - 14, ey - 2, rx + 14, ey - 14, SSD1306_BLACK);
            display.fillCircle(lx, ey + 4, 5, SSD1306_BLACK);
            display.fillCircle(rx, ey + 4, 5, SSD1306_BLACK);
            display.fillCircle(lx - 2, ey + 1, 2, SSD1306_WHITE);
            display.fillCircle(rx - 2, ey + 1, 2, SSD1306_WHITE);
            // Teardrop
            int ty = ey + 14 + ((millis() / 150) % 8);
            if (ty < ey + 22) {
                display.fillCircle(lx + 6, ty, 2, SSD1306_WHITE);
                display.fillTriangle(lx + 4, ty, lx + 8, ty, lx + 6, ty - 4, SSD1306_WHITE);
            }
            // Frown
            for (int i = -10; i <= 10; i++) {
                float n = (float)i / 10.0f;
                display.drawPixel(64 + i, 52 - (int)(5.0f * n * n), SSD1306_WHITE);
            }
            break;
        }

        case ANGRY: {
            display.fillRoundRect(lx - 12, ey - 5, 24, 12, 4, SSD1306_WHITE);
            display.fillRoundRect(rx - 12, ey - 5, 24, 12, 4, SSD1306_WHITE);
            for (int t = -1; t <= 1; t++) {
                display.drawLine(lx - 14, ey - 14 + t, lx + 10, ey - 7 + t, SSD1306_WHITE);
                display.drawLine(rx + 14, ey - 14 + t, rx - 10, ey - 7 + t, SSD1306_WHITE);
            }
            display.fillCircle(lx + 1, ey, 3, SSD1306_BLACK);
            display.fillCircle(rx - 1, ey, 3, SSD1306_BLACK);
            // Gritted teeth
            display.fillRoundRect(55, 46, 18, 6, 2, SSD1306_WHITE);
            display.drawLine(60, 46, 60, 52, SSD1306_BLACK);
            display.drawLine(64, 46, 64, 52, SSD1306_BLACK);
            display.drawLine(68, 46, 68, 52, SSD1306_BLACK);
            break;
        }

        case LISTENING: {
            drawCuteEyes(lx, rx, ey, 28, 30);
            float pulse = (millis() % 1000) / 1000.0f;
            int r1 = 3 + (int)(pulse * 6);
            int r2 = 3 + (int)(fmod(pulse + 0.5f, 1.0f) * 6);
            display.drawCircle(6, 22, r1, SSD1306_WHITE);
            display.drawCircle(122, 22, r1, SSD1306_WHITE);
            display.drawCircle(6, 22, r2, SSD1306_WHITE);
            display.drawCircle(122, 22, r2, SSD1306_WHITE);
            int dot = (millis() / 300) % 4;
            for (int i = 0; i < 3; i++) {
                if (i <= dot) display.fillCircle(54 + i * 10, 54, 2, SSD1306_WHITE);
                else display.drawCircle(54 + i * 10, 54, 2, SSD1306_WHITE);
            }
            break;
        }

        case THINKING: {
            display.fillRoundRect(lx - 12, ey - 13, 24, 26, 10, SSD1306_WHITE);
            display.fillCircle(lx + 4, ey - 3, 5, SSD1306_BLACK);
            display.fillCircle(lx + 1, ey - 6, 2, SSD1306_WHITE);
            display.fillRoundRect(rx - 10, ey - 3, 20, 8, 4, SSD1306_WHITE);
            display.fillCircle(rx + 3, ey + 1, 2, SSD1306_BLACK);
            // Thought bubble
            int f = (millis() / 500) % 4;
            if (f >= 1) display.fillCircle(100, 44, 2, SSD1306_WHITE);
            if (f >= 2) display.fillCircle(108, 40, 3, SSD1306_WHITE);
            if (f >= 3) {
                display.fillRoundRect(110, 26, 18, 12, 5, SSD1306_WHITE);
                display.setTextSize(1); display.setTextColor(SSD1306_BLACK);
                display.setCursor(113, 28); display.print("?");
            }
            break;
        }

        case SPEAKING: {
            drawCuteEyes(lx, rx, ey, 24, 26);
            float t = millis() / 100.0f;
            int mh = 3 + abs((int)(6.0f * sinf(t)));
            int mw = 16 + abs((int)(4.0f * sinf(t * 0.7f)));
            display.fillRoundRect(64 - mw/2, 46 - mh/2, mw, mh, 4, SSD1306_WHITE);
            if (mh > 6)
                display.fillRoundRect(64 - mw/2 + 3, 46 - mh/2 + 2, mw - 6, mh - 4, 3, SSD1306_BLACK);
            break;
        }

        case SLEEPY: {
            float br = sin(millis() / 1500.0f) * 2.0f;
            int bY = ey + (int)br;
            display.fillRoundRect(lx - 11, bY - 2, 22, 5, 2, SSD1306_WHITE);
            display.fillRoundRect(rx - 11, bY - 2, 22, 5, 2, SSD1306_WHITE);
            display.drawLine(lx - 11, bY - 3, lx + 11, bY - 3, SSD1306_WHITE);
            display.drawLine(rx - 11, bY - 3, rx + 11, bY - 3, SSD1306_WHITE);
            int zf = (millis() / 500) % 4;
            display.setTextSize(1); display.setTextColor(SSD1306_WHITE);
            if (zf >= 1) { display.setCursor(104, 18); display.print("z"); }
            if (zf >= 2) { display.setCursor(112, 10); display.print("z"); }
            if (zf >= 3) { display.setCursor(118, 2);  display.print("Z"); }
            break;
        }
    }

    // Blink overlay
    if (isBlinking) {
        drawBlink(lx, rx, ey);
        blinkFrame++;
        if (blinkFrame > 5) isBlinking = false;
    }

    // Emotion label
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 56);
    display.print(emoNames[currentEmo]);

    display.display();
}
