#include "led_animations.h"
#include <Arduino.h> // For millis()

// Define the LED array
CRGB leds[NUM_LEDS];

// Internal state variables
unsigned long lastLedUpdateTime = 0;
const unsigned long LED_UPDATE_INTERVAL = 50; // Update LEDs every 50ms

enum class LedMode {
    OFF,
    MENU,
    QUIZ,
    STORY
};
static LedMode currentLedMode = LedMode::OFF;
static int menuSelection = 0;
static int totalMenuItems = 0;
static bool quizAnswerStatus = false;
static bool quizWaiting = true;

void setupLeds() {
    FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
    FastLED.setBrightness(BRIGHTNESS);
    FastLED.clear();
    FastLED.show();
    currentLedMode = LedMode::OFF;
}

void ledsOff() {
    FastLED.clear();
    FastLED.show();
    currentLedMode = LedMode::OFF;
}

void setLedColor(uint8_t ledIndex, uint32_t color) {
    if (ledIndex < NUM_LEDS) {
        if (color <= 0xFFFFFF) { // Heuristic: if it's a packed value
             leds[ledIndex] = CRGB((color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF);
        } else { // If it's a CRGB object (though type is uint32_t in signature, needs care)
            leds[ledIndex].setRGB((color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF);
        }
    }
}

// --- Mode-Specific Update Functions (Called by main sketch) ---

void setLedModeMenu(int currentSelection, int numItems) {
    currentLedMode = LedMode::MENU;
    menuSelection = currentSelection;
    totalMenuItems = numItems;
}

void setLedModeQuiz(bool answerCorrect, bool waitingForAnswer) {
    currentLedMode = LedMode::QUIZ;
    quizAnswerStatus = answerCorrect;
    quizWaiting = waitingForAnswer;
}

void setLedModeOff() {
    currentLedMode = LedMode::OFF;
}

void setLedModeStory() {
    currentLedMode = LedMode::STORY;
}

// --- Internal Effect Logic (Called by updateLedEffects) ---

void _updateMenuEffect() {
    // Example: Simple effect - left LED shows selection, right LED is static
    CHSV leftHSV((255 / totalMenuItems) * menuSelection, 255, 255);
    CRGB leftColor;
    hsv2rgb_rainbow(leftHSV, leftColor); // Convert CHSV to CRGB

    CRGB rightColor = CRGB(0, 0, 50); // Dim blue
    
    leds[0] = leftColor;
    leds[1] = rightColor;
}

void _updateQuizEffect() {
    static unsigned long lastBlinkTime = 0;
    static bool blinkState = false;
    const unsigned long blinkInterval = 200; // ms

    if (quizWaiting) {
        // Waiting for answer: Slow pulse white on both LEDs
        uint8_t brightness = (millis() / 20) % 100; // Slow pulse (0-99)
        brightness = (brightness < 50) ? brightness * 2 : (100 - brightness) * 2; // Triangle wave
        brightness = map(brightness, 0, 100, 10, 80); // Map to dim-medium brightness
        CRGB pulseColor = CRGB(brightness, brightness, brightness);
        leds[0] = pulseColor;
        leds[1] = pulseColor;
    } else {
        // Answer given: Blink green for correct, red for incorrect
        if (millis() - lastBlinkTime > blinkInterval) {
            blinkState = !blinkState;
            lastBlinkTime = millis();
        }
        CRGB color = CRGB::Black;
        if (blinkState) {
            color = quizAnswerStatus ? CRGB::Green : CRGB::Red;
            color.nscale8(80); // Dim the color
        }
        leds[0] = color;
        leds[1] = color;
    }
}

void _updateStoryEffect() {
    // Gentle breathing blue effect for story mode
    static unsigned long lastBreathTime = 0;
    static bool breathUp = true;
    static uint8_t breath = 30;
    unsigned long now = millis();
    if (now - lastBreathTime > 12) { // Smooth breathing
        lastBreathTime = now;
        if (breathUp) {
            breath++;
            if (breath >= 80) breathUp = false;
        } else {
            breath--;
            if (breath <= 30) breathUp = true;
        }
    }
    CRGB color = CRGB(0, 0, breath); // Blue breathing
    leds[0] = color;
    leds[1] = color;
}

// --- Main Update Function (Called in main loop) ---

void updateLedEffects() {
    unsigned long currentTime = millis();
    if (currentTime - lastLedUpdateTime < LED_UPDATE_INTERVAL) {
        return; // Update only periodically to avoid blocking
    }
    lastLedUpdateTime = currentTime;

    switch (currentLedMode) {
        case LedMode::MENU:
            _updateMenuEffect();
            break;
        case LedMode::QUIZ:
            _updateQuizEffect();
            break;
        case LedMode::STORY:
            _updateStoryEffect();
            break;
        case LedMode::OFF:
        default:
            FastLED.clear(); // Ensure LEDs are off if mode is OFF or unknown
            break;
    }
    
    FastLED.show(); // Update the strip
} 