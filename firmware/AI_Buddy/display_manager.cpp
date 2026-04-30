/*
 * AI Buddy — Display Manager Implementation
 * Cute & expressive face animations!
 */

#include "display_manager.h"

DisplayManager::DisplayManager()
    : _display(nullptr), _emotion(EMO_NEUTRAL),
      _hasMessage(false), _lastBlink(0),
      _isBlinking(false), _blinkFrame(0),
      _eyeX(0), _eyeY(0), _pupilX(0), _pupilY(0) {
    memset(_message, 0, sizeof(_message));
}

bool DisplayManager::begin() {
    Wire.begin(OLED_SDA_PIN, OLED_SCL_PIN);
    _display = new DisplayDriver(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

    bool ok;
    #if USE_SH1106
      ok = _display->begin(OLED_ADDR, true);
    #else
      ok = _display->begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
    #endif

    if (!ok) {
        Serial.println("[DISPLAY] OLED init FAILED!");
        return false;
    }

    _display->clearDisplay();
    _display->display();
    Serial.println("[DISPLAY] OLED ready");
    return true;
}

void DisplayManager::setEmotion(Emotion emo) {
    if (emo != _emotion) {
        _emotion = emo;
        Serial.printf("[DISPLAY] Emotion → %d\n", emo);
    }
}

void DisplayManager::showMessage(const char* msg) {
    strncpy(_message, msg, sizeof(_message) - 1);
    _hasMessage = true;
}

void DisplayManager::clearMessage() {
    _hasMessage = false;
    _message[0] = '\0';
}

void DisplayManager::showStatus(const char* line1, const char* line2) {
    _display->clearDisplay();
    _display->setTextSize(1);
    _display->setTextColor(SSD1306_WHITE);
    _display->setCursor(0, 20);
    _display->print(line1);
    if (line2) {
        _display->setCursor(0, 36);
        _display->print(line2);
    }
    _display->display();
}

void DisplayManager::showSplash() {
    _display->clearDisplay();

    // Cute robot face for splash
    // Big round eyes
    _display->fillRoundRect(30, 8, 26, 26, 10, SSD1306_WHITE);
    _display->fillRoundRect(72, 8, 26, 26, 10, SSD1306_WHITE);
    // Pupils
    _display->fillCircle(43, 21, 6, SSD1306_BLACK);
    _display->fillCircle(85, 21, 6, SSD1306_BLACK);
    // Shine spots (cute highlight!)
    _display->fillCircle(40, 17, 2, SSD1306_WHITE);
    _display->fillCircle(82, 17, 2, SSD1306_WHITE);
    // Cute smile
    for (int i = -10; i <= 10; i++) {
        int dy = (i * i) / 12;
        _display->drawPixel(64 + i, 40 + dy, SSD1306_WHITE);
        _display->drawPixel(64 + i, 41 + dy, SSD1306_WHITE);
    }

    _display->setTextSize(1);
    _display->setTextColor(SSD1306_WHITE);
    _display->setCursor(30, 54);
    _display->print("AI Buddy v1.0");
    _display->display();
}

void DisplayManager::update() {
    unsigned long now = millis();

    // Random blink every 2-5 seconds
    if (!_isBlinking && now - _lastBlink > (unsigned long)random(2500, 5000)) {
        _isBlinking = true;
        _blinkFrame = 0;
        _lastBlink = now;
    }

    // Smooth eye drift (makes it feel alive)
    _eyeX += random(-10, 11) / 200.0f;
    _eyeY += random(-10, 11) / 250.0f;
    _eyeX = constrain(_eyeX, -2.5f, 2.5f);
    _eyeY = constrain(_eyeY, -1.5f, 1.5f);

    // Smooth pupil movement
    float targetPX = _eyeX * 1.5f;
    float targetPY = _eyeY * 1.5f;
    _pupilX += (targetPX - _pupilX) * 0.3f;
    _pupilY += (targetPY - _pupilY) * 0.3f;

    drawFace();
}

// ══════════════════════════════════════
//  Main Face Renderer
// ══════════════════════════════════════

void DisplayManager::drawFace() {
    _display->clearDisplay();

    // Eye positions
    int lx = 38 + (int)_eyeX;   // Left eye center
    int rx = 90 + (int)_eyeX;   // Right eye center
    int ey = 22 + (int)_eyeY;   // Eye vertical center

    switch (_emotion) {
        case EMO_NEUTRAL:
            drawEyes(lx, rx, ey, 24, 26);
            break;
        case EMO_HAPPY:
            drawHappyEyes(lx, rx, ey);
            drawMouth(EMO_HAPPY);
            break;
        case EMO_SAD:
            drawSadEyes(lx, rx, ey);
            drawMouth(EMO_SAD);
            break;
        case EMO_ANGRY:
            drawAngryEyes(lx, rx, ey);
            break;
        case EMO_LISTENING:
            drawEyes(lx, rx, ey, 28, 30);  // Extra wide!
            drawListeningAnim();
            break;
        case EMO_THINKING:
            drawThinkingEyes(lx, rx, ey);
            drawThinkingDots();
            break;
        case EMO_SPEAKING:
            drawEyes(lx, rx, ey, 24, 26);
            drawSpeakingMouth();
            break;
        case EMO_SLEEPY:
            drawSleepyEyes(lx, rx, ey);
            break;
        default: break;
    }

    // Blink overlay (works on all emotions)
    if (_isBlinking) {
        drawBlink(lx, rx, ey);
        _blinkFrame++;
        if (_blinkFrame > 5) _isBlinking = false;
    }

    // Message at bottom
    if (_hasMessage) {
        _display->setTextSize(1);
        _display->setTextColor(SSD1306_WHITE);
        _display->setCursor(0, 56);
        _display->print(_message);
    }

    _display->display();
}

// ══════════════════════════════════════
//  Cute Eye Styles
// ══════════════════════════════════════

void DisplayManager::drawEyes(int lx, int rx, int y, int w, int h) {
    // Big round white eyes
    _display->fillRoundRect(lx - w/2, y - h/2, w, h, 10, SSD1306_WHITE);
    _display->fillRoundRect(rx - w/2, y - h/2, w, h, 10, SSD1306_WHITE);

    // Big pupils that follow eye drift
    int px = (int)(_pupilX * 2);
    int py = (int)(_pupilY * 2);
    _display->fillCircle(lx + px, y + py + 1, 6, SSD1306_BLACK);
    _display->fillCircle(rx + px, y + py + 1, 6, SSD1306_BLACK);

    // ✨ Cute highlight dots (makes eyes look shiny/alive!)
    _display->fillCircle(lx + px - 3, y + py - 3, 2, SSD1306_WHITE);
    _display->fillCircle(rx + px - 3, y + py - 3, 2, SSD1306_WHITE);
    // Small secondary highlight
    _display->fillCircle(lx + px + 2, y + py + 2, 1, SSD1306_WHITE);
    _display->fillCircle(rx + px + 2, y + py + 2, 1, SSD1306_WHITE);
}

void DisplayManager::drawHappyEyes(int lx, int rx, int y) {
    // Squished happy arcs (^  ^) — cute closed-eye smile
    for (int i = -11; i <= 11; i++) {
        float norm = (float)i / 11.0f;
        int curve = (int)(10.0f * (1.0f - norm * norm));
        // Thick happy arcs
        _display->drawPixel(lx + i, y - curve, SSD1306_WHITE);
        _display->drawPixel(lx + i, y - curve + 1, SSD1306_WHITE);
        _display->drawPixel(lx + i, y - curve + 2, SSD1306_WHITE);
        _display->drawPixel(rx + i, y - curve, SSD1306_WHITE);
        _display->drawPixel(rx + i, y - curve + 1, SSD1306_WHITE);
        _display->drawPixel(rx + i, y - curve + 2, SSD1306_WHITE);
    }
    // Cute blush marks under eyes
    _display->fillCircle(lx - 14, y + 4, 3, SSD1306_WHITE);
    _display->fillCircle(lx - 14, y + 4, 1, SSD1306_BLACK);
    _display->fillCircle(rx + 14, y + 4, 3, SSD1306_WHITE);
    _display->fillCircle(rx + 14, y + 4, 1, SSD1306_BLACK);
}

void DisplayManager::drawSadEyes(int lx, int rx, int y) {
    // Big round sad eyes (slightly smaller, looking down)
    _display->fillRoundRect(lx - 11, y - 10, 22, 22, 9, SSD1306_WHITE);
    _display->fillRoundRect(rx - 11, y - 10, 22, 22, 9, SSD1306_WHITE);

    // Droopy eyelid effect (diagonal cover on top)
    _display->fillTriangle(lx - 14, y - 14, lx + 14, y - 14, lx + 14, y - 2, SSD1306_BLACK);
    _display->fillTriangle(rx - 14, y - 14, rx - 14, y - 2, rx + 14, y - 14, SSD1306_BLACK);

    // Pupils looking down
    _display->fillCircle(lx, y + 4, 5, SSD1306_BLACK);
    _display->fillCircle(rx, y + 4, 5, SSD1306_BLACK);
    // Highlights
    _display->fillCircle(lx - 2, y + 1, 2, SSD1306_WHITE);
    _display->fillCircle(rx - 2, y + 1, 2, SSD1306_WHITE);

    // Cute teardrop on left eye
    int tearY = y + 14 + ((millis() / 150) % 8);
    if (tearY < y + 22) {
        _display->fillCircle(lx + 6, tearY, 2, SSD1306_WHITE);
        _display->fillTriangle(lx + 4, tearY, lx + 8, tearY, lx + 6, tearY - 4, SSD1306_WHITE);
    }
}

void DisplayManager::drawAngryEyes(int lx, int rx, int y) {
    // Squinted angry eyes (narrow)
    _display->fillRoundRect(lx - 12, y - 5, 24, 12, 4, SSD1306_WHITE);
    _display->fillRoundRect(rx - 12, y - 5, 24, 12, 4, SSD1306_WHITE);

    // Angry V-shaped eyebrows (thick!)
    // Left eyebrow: slopes down-right
    for (int t = -1; t <= 1; t++) {
        _display->drawLine(lx - 14, y - 14 + t, lx + 10, y - 7 + t, SSD1306_WHITE);
    }
    // Right eyebrow: slopes down-left
    for (int t = -1; t <= 1; t++) {
        _display->drawLine(rx + 14, y - 14 + t, rx - 10, y - 7 + t, SSD1306_WHITE);
    }

    // Small intense pupils
    _display->fillCircle(lx + 1, y, 3, SSD1306_BLACK);
    _display->fillCircle(rx - 1, y, 3, SSD1306_BLACK);
    // Tiny highlights
    _display->fillCircle(lx - 1, y - 2, 1, SSD1306_WHITE);
    _display->fillCircle(rx - 3, y - 2, 1, SSD1306_WHITE);

    // Gritting teeth / angry mouth
    int mw = 18;
    _display->fillRoundRect(64 - mw/2, 46, mw, 6, 2, SSD1306_WHITE);
    _display->drawLine(64 - 4, 46, 64 - 4, 52, SSD1306_BLACK);
    _display->drawLine(64, 46, 64, 52, SSD1306_BLACK);
    _display->drawLine(64 + 4, 46, 64 + 4, 52, SSD1306_BLACK);
}

void DisplayManager::drawThinkingEyes(int lx, int rx, int y) {
    // One eye big, one squinted — looking up-right
    // Left eye: big and looking up
    _display->fillRoundRect(lx - 12, y - 13, 24, 26, 10, SSD1306_WHITE);
    _display->fillCircle(lx + 4, y - 3, 5, SSD1306_BLACK);
    _display->fillCircle(lx + 1, y - 6, 2, SSD1306_WHITE);

    // Right eye: squinted (thinking squint)
    _display->fillRoundRect(rx - 10, y - 3, 20, 8, 4, SSD1306_WHITE);
    _display->fillCircle(rx + 3, y + 1, 2, SSD1306_BLACK);
}

void DisplayManager::drawSleepyEyes(int lx, int rx, int y) {
    // Drooping half-closed eyes
    float breathe = sin(millis() / 1500.0f) * 2.0f;  // Gentle breathing motion
    int bY = y + (int)breathe;

    // Thin horizontal slits
    _display->fillRoundRect(lx - 11, bY - 2, 22, 5, 2, SSD1306_WHITE);
    _display->fillRoundRect(rx - 11, bY - 2, 22, 5, 2, SSD1306_WHITE);

    // Eyelid lines above
    _display->drawLine(lx - 11, bY - 3, lx + 11, bY - 3, SSD1306_WHITE);
    _display->drawLine(rx - 11, bY - 3, rx + 11, bY - 3, SSD1306_WHITE);

    // Floating Z's with animation
    int frame = (millis() / 500) % 4;
    _display->setTextSize(1);
    _display->setTextColor(SSD1306_WHITE);
    if (frame >= 1) { _display->setCursor(104, 18); _display->print("z"); }
    if (frame >= 2) { _display->setCursor(112, 10); _display->print("z"); }
    if (frame >= 3) { _display->setCursor(118, 2);  _display->print("Z"); }

    // Cute sleepy blush
    _display->drawPixel(lx - 13, bY + 3, SSD1306_WHITE);
    _display->drawPixel(lx - 14, bY + 4, SSD1306_WHITE);
    _display->drawPixel(rx + 13, bY + 3, SSD1306_WHITE);
    _display->drawPixel(rx + 14, bY + 4, SSD1306_WHITE);
}

// ══════════════════════════════════════
//  Blink Animation
// ══════════════════════════════════════

void DisplayManager::drawBlink(int lx, int rx, int y) {
    // Smooth blink: eyelid closes from top
    int maxH = 30;
    int bh;
    if (_blinkFrame <= 2)
        bh = (_blinkFrame * maxH) / 2;        // Closing
    else
        bh = ((5 - _blinkFrame) * maxH) / 2;  // Opening

    _display->fillRect(lx - 16, y - 16, 32, bh, SSD1306_BLACK);
    _display->fillRect(rx - 16, y - 16, 32, bh, SSD1306_BLACK);

    // Draw eyelid line at the blink edge
    if (bh > 2 && bh < maxH - 2) {
        _display->drawLine(lx - 12, y - 16 + bh, lx + 12, y - 16 + bh, SSD1306_WHITE);
        _display->drawLine(rx - 12, y - 16 + bh, rx + 12, y - 16 + bh, SSD1306_WHITE);
    }
}

// ══════════════════════════════════════
//  Mouth & Indicators
// ══════════════════════════════════════

void DisplayManager::drawMouth(Emotion e) {
    int cx = 64;
    if (e == EMO_HAPPY) {
        // Wide cute smile with thickness
        for (int i = -16; i <= 16; i++) {
            float norm = (float)i / 16.0f;
            int dy = (int)(6.0f * norm * norm);
            _display->drawPixel(cx + i, 46 + dy, SSD1306_WHITE);
            _display->drawPixel(cx + i, 47 + dy, SSD1306_WHITE);
        }
        // Smile corners
        _display->fillCircle(cx - 16, 46, 1, SSD1306_WHITE);
        _display->fillCircle(cx + 16, 46, 1, SSD1306_WHITE);
    } else if (e == EMO_SAD) {
        // Wobbly frown
        float wobble = sin(millis() / 800.0f) * 1.5f;
        for (int i = -10; i <= 10; i++) {
            float norm = (float)i / 10.0f;
            int dy = -(int)(5.0f * norm * norm);
            _display->drawPixel(cx + i, 50 + dy + (int)wobble, SSD1306_WHITE);
        }
    }
}

void DisplayManager::drawListeningAnim() {
    // Pulsing sound wave arcs on both sides
    float pulse = (millis() % 1000) / 1000.0f;
    int r1 = 3 + (int)(pulse * 6);
    int r2 = 3 + (int)(fmod(pulse + 0.5f, 1.0f) * 6);

    // Left side arcs
    _display->drawCircle(6, 22, r1, SSD1306_WHITE);
    _display->drawCircle(6, 22, r2, SSD1306_WHITE);
    // Right side arcs
    _display->drawCircle(122, 22, r1, SSD1306_WHITE);
    _display->drawCircle(122, 22, r2, SSD1306_WHITE);

    // Small "listening" indicator dots at bottom
    int dot = (millis() / 300) % 4;
    for (int i = 0; i < 3; i++) {
        int dx = 54 + i * 10;
        if (i <= dot)
            _display->fillCircle(dx, 54, 2, SSD1306_WHITE);
        else
            _display->drawCircle(dx, 54, 2, SSD1306_WHITE);
    }
}

void DisplayManager::drawThinkingDots() {
    // Thought bubble dots (small → medium → large)
    int frame = (millis() / 500) % 4;

    // Three dots appearing one by one
    int baseX = 100, baseY = 44;
    if (frame >= 1) _display->fillCircle(baseX, baseY, 2, SSD1306_WHITE);
    if (frame >= 2) _display->fillCircle(baseX + 8, baseY - 4, 3, SSD1306_WHITE);
    if (frame >= 3) {
        // Thought cloud
        _display->fillRoundRect(baseX + 10, baseY - 18, 18, 12, 5, SSD1306_WHITE);
        _display->setTextSize(1);
        _display->setTextColor(SSD1306_BLACK);
        _display->setCursor(baseX + 13, baseY - 16);
        _display->print("?");
    }
}

void DisplayManager::drawSpeakingMouth() {
    // Animated bouncing mouth — opens and closes rhythmically
    float t = millis() / 100.0f;
    int mouthH = 3 + abs((int)(6.0f * sinf(t)));
    int mouthW = 16 + abs((int)(4.0f * sinf(t * 0.7f)));

    _display->fillRoundRect(64 - mouthW/2, 46 - mouthH/2, mouthW, mouthH, 4, SSD1306_WHITE);

    // Inner mouth (dark inside when open wide)
    if (mouthH > 6) {
        _display->fillRoundRect(64 - mouthW/2 + 3, 46 - mouthH/2 + 2, mouthW - 6, mouthH - 4, 3, SSD1306_BLACK);
    }
}
