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
    STORY,
    WARP,
    DISCOVERY
};

enum class CelestialObjectType {
    STAR,
    PLANET,
    NEBULA,
    GALAXY,
    SOLAR_SYSTEM,
    ASTEROID_FIELD,
    BLACK_HOLE,
    PULSAR,
    SUPERNOVA,
    COMET,
    BINARY_STAR,
    SPACE_STATION,
    NONE
};

static LedMode currentLedMode = LedMode::OFF;
static CelestialObjectType currentObject = CelestialObjectType::NONE;
static int menuSelection = 0;
static int totalMenuItems = 0;
static bool quizAnswerStatus = false;
static bool quizWaiting = true;

// Animation state variables
static float pulsePhase = 0.0f;
static float rotationPhase = 0.0f;
static float warpPhase = 0.0f;
static uint8_t fadeValue = 0;
static bool fadeDirection = true;
static unsigned long lastObjectUpdateTime = 0;

void setupLeds() {
    FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
    FastLED.setBrightness(BRIGHTNESS);
    FastLED.clear();
    FastLED.show();
    currentLedMode = LedMode::OFF;
    currentObject = CelestialObjectType::NONE;
}

void ledsOff() {
    FastLED.clear();
    FastLED.show();
    currentLedMode = LedMode::OFF;
    currentObject = CelestialObjectType::NONE;
}

void setLedColor(uint8_t ledIndex, uint32_t color) {
    if (ledIndex < NUM_LEDS) {
        leds[ledIndex] = CRGB((color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF);
    }
}

// --- Mode-Specific Update Functions ---

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
    currentObject = CelestialObjectType::NONE;
}

void setLedModeStory() {
    currentLedMode = LedMode::STORY;
}

void setLedModeWarp() {
    currentLedMode = LedMode::WARP;
}

void setLedModeDiscovery(const char* objectName) {
    currentLedMode = LedMode::DISCOVERY;
    
    // Map object name to type
    if (objectName == nullptr || strlen(objectName) == 0 || strcmp(objectName, "None") == 0) {
        currentObject = CelestialObjectType::NONE;
    } else if (strstr(objectName, "Star") && !strstr(objectName, "Binary")) {
        currentObject = CelestialObjectType::STAR;
    } else if (strstr(objectName, "Planet")) {
        currentObject = CelestialObjectType::PLANET;
    } else if (strstr(objectName, "Nebula")) {
        currentObject = CelestialObjectType::NEBULA;
    } else if (strstr(objectName, "Galaxy")) {
        currentObject = CelestialObjectType::GALAXY;
    } else if (strstr(objectName, "Solar System")) {
        currentObject = CelestialObjectType::SOLAR_SYSTEM;
    } else if (strstr(objectName, "Asteroid")) {
        currentObject = CelestialObjectType::ASTEROID_FIELD;
    } else if (strstr(objectName, "Black Hole")) {
        currentObject = CelestialObjectType::BLACK_HOLE;
    } else if (strstr(objectName, "Pulsar")) {
        currentObject = CelestialObjectType::PULSAR;
    } else if (strstr(objectName, "Supernova")) {
        currentObject = CelestialObjectType::SUPERNOVA;
    } else if (strstr(objectName, "Comet")) {
        currentObject = CelestialObjectType::COMET;
    } else if (strstr(objectName, "Binary Star")) {
        currentObject = CelestialObjectType::BINARY_STAR;
    } else if (strstr(objectName, "Space Station")) {
        currentObject = CelestialObjectType::SPACE_STATION;
    } else {
        currentObject = CelestialObjectType::NONE;
    }
    
    // Reset animation states
    pulsePhase = 0.0f;
    rotationPhase = 0.0f;
    warpPhase = 0.0f;
    fadeValue = 0;
    fadeDirection = true;
    lastObjectUpdateTime = millis();
}

// --- Internal Effect Logic ---

void _updateMenuEffect() {
    CHSV leftHSV((255 / totalMenuItems) * menuSelection, 255, 255);
    CRGB leftColor;
    hsv2rgb_rainbow(leftHSV, leftColor);
    CRGB rightColor = CRGB(0, 0, 50);
    leds[0] = leftColor;
    leds[1] = rightColor;
}

void _updateQuizEffect() {
    static unsigned long lastBlinkTime = 0;
    static bool blinkState = false;
    const unsigned long blinkInterval = 200;

    if (quizWaiting) {
        uint8_t brightness = (millis() / 20) % 100;
        brightness = (brightness < 50) ? brightness * 2 : (100 - brightness) * 2;
        brightness = map(brightness, 0, 100, 10, 80);
        CRGB pulseColor = CRGB(brightness, brightness, brightness);
        leds[0] = pulseColor;
        leds[1] = pulseColor;
    } else {
        if (millis() - lastBlinkTime > blinkInterval) {
            blinkState = !blinkState;
            lastBlinkTime = millis();
        }
        CRGB color = CRGB::Black;
        if (blinkState) {
            color = quizAnswerStatus ? CRGB::Green : CRGB::Red;
            color.nscale8(80);
        }
        leds[0] = color;
        leds[1] = color;
    }
}

void _updateStoryEffect() {
    static unsigned long lastBreathTime = 0;
    static bool breathUp = true;
    static uint8_t breath = 30;
    unsigned long now = millis();
    if (now - lastBreathTime > 12) {
        lastBreathTime = now;
        if (breathUp) {
            breath++;
            if (breath >= 80) breathUp = false;
        } else {
            breath--;
            if (breath <= 30) breathUp = true;
        }
    }
    CRGB color = CRGB(0, 0, breath);
    leds[0] = color;
    leds[1] = color;
}

void _updateWarpEffect() {
    // Fast blue/white strobe or pulse for warp tunnel effect
    static unsigned long lastPulse = 0;
    static bool pulseOn = false;
    unsigned long now = millis();
    if (now - lastPulse > 60) { // 60ms pulse
        pulseOn = !pulseOn;
        lastPulse = now;
    }
    if (pulseOn) {
        leds[0] = CRGB(0, 180, 255);
        leds[1] = CRGB(180, 220, 255);
    } else {
        leds[0] = CRGB(0, 0, 40);
        leds[1] = CRGB(0, 0, 60);
    }
}

void _updateDiscoveryEffect() {
    unsigned long now = millis();
    float deltaTime = (now - lastObjectUpdateTime) / 1000.0f;
    lastObjectUpdateTime = now;

    // Use objectScale to modulate effect intensities
    extern float objectScale;
    float s = constrain(objectScale, 0.5f, 3.0f);

    // Update animation phases with scale influence
    pulsePhase += deltaTime * 2.0f * s;
    if (pulsePhase >= TWO_PI) pulsePhase -= TWO_PI;
    rotationPhase += deltaTime * 3.0f * s;
    if (rotationPhase >= TWO_PI) rotationPhase -= TWO_PI;
    warpPhase += deltaTime * 5.0f * s;
    if (warpPhase >= TWO_PI) warpPhase -= TWO_PI;

    // Adjust fade speed based on object scale
    int fadeStep = int(5 * s);
    if (fadeDirection) {
        fadeValue = min(255, fadeValue + fadeStep);
        if (fadeValue >= 255) fadeDirection = false;
    } else {
        fadeValue = max(0, fadeValue - fadeStep);
        if (fadeValue <= 0) fadeDirection = true;
    }

    switch (currentObject) {
        case CelestialObjectType::STAR: {
            uint8_t b = uint8_t(constrain((128 + sin(pulsePhase) * 127) * s, 0, 255));
            CRGB c(b, b * 0.8, b * 0.5);
            leds[0] = leds[1] = c;
            break;
        }
        case CelestialObjectType::PLANET: {
            uint8_t base = uint8_t(constrain((128 + sin(pulsePhase) * 64) * (0.7f + 0.3f * s), 0, 255));
            leds[0] = CRGB(0, base * 0.7, base);
            leds[1] = CRGB(0, base, base * 0.7);
            break;
        }
        case CelestialObjectType::NEBULA: {
            uint8_t r = uint8_t(constrain((128 + sin(pulsePhase) * 127) * (1.0f + 0.5f * s), 0, 255));
            uint8_t g = uint8_t(constrain((128 + sin(pulsePhase + TWO_PI/3) * 127) * (1.0f + 0.5f * s), 0, 255));
            uint8_t b = uint8_t(constrain((128 + sin(pulsePhase + 2*TWO_PI/3) * 127) * (1.0f + 0.5f * s), 0, 255));
            leds[0] = leds[1] = CRGB(r, g, b);
            break;
        }
        case CelestialObjectType::GALAXY: {
            uint8_t bright = uint8_t(constrain((128 + sin(rotationPhase) * 127) * s, 0, 255));
            CHSV hsv1((rotationPhase * 256/TWO_PI), 255, bright);
            CHSV hsv2((rotationPhase * 256/TWO_PI + 128), 255, bright);
            hsv2rgb_rainbow(hsv1, leds[0]);
            hsv2rgb_rainbow(hsv2, leds[1]);
            break;
        }
        case CelestialObjectType::SOLAR_SYSTEM: {
            uint8_t bright = uint8_t(constrain((128 + sin(pulsePhase) * 127) * s, 0, 255));
            leds[0] = CRGB(bright, bright * 0.8, bright * 0.5);
            leds[1] = CRGB(0, bright * 0.7, bright);
            break;
        }
        case CelestialObjectType::ASTEROID_FIELD: {
            float flickerChance = min(100.0f, 32.0f * s);
            if (random8() < flickerChance) leds[0] = CRGB(random8(128,255),random8(128,255),random8(128,255));
            if (random8() < flickerChance) leds[1] = CRGB(random8(128,255),random8(128,255),random8(128,255));
            leds[0].fadeToBlackBy(uint8_t(16 * s));
            leds[1].fadeToBlackBy(uint8_t(16 * s));
            break;
        }
        case CelestialObjectType::BLACK_HOLE: {
            extern float blackHoleRadius;
            uint8_t disk = uint8_t(constrain(fadeValue * 0.2f + blackHoleRadius * 2.0f, 0, 255));
            uint8_t blue = uint8_t(constrain((128 + sin(pulsePhase) * 64) * s * 0.4f, 0, 255));
            leds[0] = leds[1] = CRGB(disk, 0, blue);
            break;
        }
        case CelestialObjectType::PULSAR: {
            extern float prevAngle;
            uint8_t b = uint8_t(constrain(255 * (sin(prevAngle * s * 2.0f) * 0.5f + 0.5f), 0, 255));
            leds[0] = leds[1] = CRGB(b, b, b);
            break;
        }
        case CelestialObjectType::SUPERNOVA: {
            extern int supernovaPhase;
            uint8_t rVal = uint8_t(constrain(fadeValue * s, 0, 255));
            uint8_t gVal = uint8_t(constrain(fadeValue * 0.7f * s, 0, 255));
            uint8_t bVal = uint8_t(constrain(fadeValue * 0.5f * s, 0, 255));
            if (supernovaPhase == 1 && sin(pulsePhase * 3.0f) > 0.7f) { rVal=255;gVal=220;bVal=180; }
            leds[0] = leds[1] = CRGB(rVal, gVal, bVal);
            break;
        }
        case CelestialObjectType::COMET: {
            uint8_t head = uint8_t(constrain((128 + sin(pulsePhase) * 127) * s, 0, 255));
            uint8_t tail = uint8_t(constrain(head * 0.3f * s, 0, 255));
            leds[0] = CRGB(head, head, head);
            leds[1] = CRGB(tail, tail, head);
            break;
        }
        case CelestialObjectType::BINARY_STAR: {
            uint8_t b1 = uint8_t(constrain((128 + sin(pulsePhase) * 127) * s, 0, 255));
            uint8_t b2 = uint8_t(constrain((128 + sin(pulsePhase + PI) * 127) * s, 0, 255));
            leds[0] = CRGB(b1, b1*0.8, b1*0.5);
            leds[1] = CRGB(b2, b2*0.8, b2*0.5);
            break;
        }
        case CelestialObjectType::SPACE_STATION: {
            static unsigned long lastFlash=0; static bool flash=false;
            unsigned long now2 = millis();
            if(now2-lastFlash>2000*s){ flash=true; lastFlash=now2;}
            if(flash && now2-lastFlash<100*s){ leds[0]=leds[1]=CRGB(0,0,255); }
            else{ flash=false; leds[0]=leds[1]=CRGB(64,64,64); }
            break;
        }
        default: {
            static unsigned long lastB=0; static bool up=true; static uint8_t br=20;
            unsigned long now3=millis();
            if(now3-lastB>16){ lastB=now3; if(up){ br++; if(br>=80)up=false;} else{ br--; if(br<=20)up=true;} }
            leds[0]=leds[1]=CRGB(0,0,br);
            break;
        }
    }
}

// --- Main Update Function ---

void updateLedEffects() {
    unsigned long currentTime = millis();
    if (currentTime - lastLedUpdateTime < LED_UPDATE_INTERVAL) {
        return;
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
        case LedMode::WARP:
            _updateWarpEffect();
            break;
        case LedMode::DISCOVERY:
            _updateDiscoveryEffect();
            break;
        case LedMode::OFF:
        default:
            FastLED.clear();
            break;
    }
    
    FastLED.show();
} 