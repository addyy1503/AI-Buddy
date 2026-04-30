/*
 * AI Buddy — Display Manager
 * ============================
 * Manages OLED face animations and emotion states.
 */

#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include "config.h"

#if USE_SH1106
  #include <Adafruit_SH110X.h>
  typedef Adafruit_SH1106G DisplayDriver;
#else
  #include <Adafruit_SSD1306.h>
  typedef Adafruit_SSD1306 DisplayDriver;
#endif

enum Emotion {
    EMO_NEUTRAL = 0,
    EMO_HAPPY,
    EMO_SAD,
    EMO_ANGRY,
    EMO_LISTENING,
    EMO_THINKING,
    EMO_SPEAKING,
    EMO_SLEEPY,
    EMO_COUNT
};

class DisplayManager {
public:
    DisplayManager();
    bool begin();

    // Set current emotion (face changes smoothly)
    void setEmotion(Emotion emo);
    Emotion getEmotion() const { return _emotion; }

    // Call every frame (~20fps) to animate
    void update();

    // Show a text message below the face
    void showMessage(const char* msg);
    void clearMessage();

    // Show connection status
    void showStatus(const char* line1, const char* line2 = nullptr);

    // Show a startup splash
    void showSplash();

private:
    DisplayDriver* _display;
    Emotion _emotion;
    char _message[32];
    bool _hasMessage;

    // Animation state
    unsigned long _lastBlink;
    bool _isBlinking;
    int _blinkFrame;
    float _eyeX, _eyeY;        // Subtle eye drift
    float _pupilX, _pupilY;    // Pupil offset

    void drawFace();
    void drawEyes(int lx, int rx, int y, int w, int h);
    void drawHappyEyes(int lx, int rx, int y);
    void drawSadEyes(int lx, int rx, int y);
    void drawAngryEyes(int lx, int rx, int y);
    void drawThinkingEyes(int lx, int rx, int y);
    void drawSleepyEyes(int lx, int rx, int y);
    void drawBlink(int lx, int rx, int y);
    void drawMouth(Emotion e);
    void drawListeningAnim();
    void drawThinkingDots();
    void drawSpeakingMouth();
};

#endif // DISPLAY_MANAGER_H
