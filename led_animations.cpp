#include "led_animations.h"
#include <Arduino.h> // For millis()

// Define the LED array
CRGB leds[NUM_LEDS];

// ----- Global variables from main sketch -----
extern float warpFactor; 
extern bool hapticOverrideActive;
extern unsigned long hapticOverrideEndTime;
extern bool quizHelplineActive; // Helpline state from main sketch

// ----- Internal state variables -----
unsigned long lastLedUpdateTime = 0;
const unsigned long LED_UPDATE_INTERVAL = 30; // Update LEDs a bit faster for smoother animations

enum class LedMode {
    OFF,
    MENU,
    QUIZ,
    STORY,
    WARP,
    DISCOVERY,
    TRANSITION // New state for fading between modes
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
static LedMode previousLedMode = LedMode::OFF; // For transitions
static CelestialObjectType currentObject = CelestialObjectType::NONE;
static int menuSelection = 0;
static int totalMenuItems = 0;
static bool quizAnswerStatus = false;
static bool quizWaiting = true;

// ----- Animation state variables -----
static uint32_t animationStartTime = 0; // General timer for effects
static float phase = 0.0f; // General phase for sinusoidal animations
static uint8_t step = 0; // General counter for multi-step animations

// For transitions
static unsigned long transitionStartTime = 0;
static const unsigned long TRANSITION_DURATION = 250; // 250ms fade
static CRGB oldLeds[NUM_LEDS]; // Store previous LED state for fading

// Specific animation parameters
static uint8_t menuPulseBrightness = 0;
static bool menuPulseDir = true;

static uint8_t quizSearchPos = 0;

static float storyBreathPhase = 0.0f;

static uint8_t warpChasePos = 0;
static unsigned long lastWarpStepTime = 0;

static unsigned long lastDiscoveryEffectTime = 0;


void setupLeds() {
    FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
    FastLED.setBrightness(BRIGHTNESS);
    FastLED.clear();
    FastLED.show();
    currentLedMode = LedMode::OFF;
    previousLedMode = LedMode::OFF;
    currentObject = CelestialObjectType::NONE;
    animationStartTime = millis();
}

void _startTransition() {
    if (currentLedMode == LedMode::TRANSITION) return; // Already transitioning
    memcpy(oldLeds, leds, sizeof(leds));
    previousLedMode = currentLedMode; // The mode we are transitioning FROM
    // The new 'currentLedMode' is set by the setLedModeX functions
    transitionStartTime = millis();
    currentLedMode = LedMode::TRANSITION;
}


void setLedModeMenu(int currentSelection, int numItems) {
    if (currentLedMode != LedMode::MENU || menuSelection != currentSelection) {
        _startTransition();
        currentLedMode = LedMode::MENU; // This is the target mode
        menuSelection = currentSelection;
        totalMenuItems = numItems;
        animationStartTime = millis();
        menuPulseBrightness = 0;
        menuPulseDir = true;
    }
}

void setLedModeQuiz(bool answerCorrect, bool waitingForAnswer) {
    if (currentLedMode != LedMode::QUIZ || quizAnswerStatus != answerCorrect || quizWaiting != waitingForAnswer) {
        _startTransition();
        currentLedMode = LedMode::QUIZ; // Target mode
        quizAnswerStatus = answerCorrect;
        quizWaiting = waitingForAnswer;
        animationStartTime = millis();
        quizSearchPos = 0;
        step = 0; // For flash effect
    }
}

void setLedModeOff() {
    if (currentLedMode != LedMode::OFF) {
        _startTransition();
        currentLedMode = LedMode::OFF; // Target mode
        currentObject = CelestialObjectType::NONE;
    }
}

void setLedModeStory() {
    if (currentLedMode != LedMode::STORY) {
        _startTransition();
        currentLedMode = LedMode::STORY; // Target mode
        animationStartTime = millis();
        storyBreathPhase = 0.0f;
    }
}

void setLedModeWarp() {
    if (currentLedMode != LedMode::WARP) {
        _startTransition();
        currentLedMode = LedMode::WARP; // Target mode
        animationStartTime = millis();
        warpChasePos = 0;
        lastWarpStepTime = millis();
    }
}

void setLedModeDiscovery(const char* objectName) {
    // Determine new object type without resetting animation state each frame
    CelestialObjectType newObject = CelestialObjectType::NONE;
    if (objectName == nullptr || strlen(objectName) == 0 || strcmp(objectName, "None") == 0) {
        newObject = CelestialObjectType::NONE;
    } else if (strstr(objectName, "Star") && !strstr(objectName, "Binary")) {
        newObject = CelestialObjectType::STAR;
    } else if (strstr(objectName, "Planet")) {
        newObject = CelestialObjectType::PLANET;
    } else if (strstr(objectName, "Nebula")) {
        newObject = CelestialObjectType::NEBULA;
    } else if (strstr(objectName, "Galaxy")) {
        newObject = CelestialObjectType::GALAXY;
    } else if (strstr(objectName, "Solar System")) {
        newObject = CelestialObjectType::SOLAR_SYSTEM;
    } else if (strstr(objectName, "Asteroid")) {
        newObject = CelestialObjectType::ASTEROID_FIELD;
    } else if (strstr(objectName, "Black Hole")) {
        newObject = CelestialObjectType::BLACK_HOLE;
    } else if (strstr(objectName, "Pulsar")) {
        newObject = CelestialObjectType::PULSAR;
    } else if (strstr(objectName, "Supernova")) {
        newObject = CelestialObjectType::SUPERNOVA;
    } else if (strstr(objectName, "Comet")) {
        newObject = CelestialObjectType::COMET;
    } else if (strstr(objectName, "Binary Star")) {
        newObject = CelestialObjectType::BINARY_STAR;
    } else if (strstr(objectName, "Space Station")) {
        newObject = CelestialObjectType::SPACE_STATION;
    }

    // Only reset animation state when entering discovery or switching to a different object
    if (currentLedMode != LedMode::DISCOVERY || newObject != currentObject) {
        _startTransition();
        currentLedMode = LedMode::DISCOVERY;
        animationStartTime = millis();
        phase = 0.0f;
        step = 0;
        lastDiscoveryEffectTime = millis();
    }

    currentObject = newObject;
}

// --- Internal Effect Logic ---

void _updateMenuEffect() {
    unsigned long currentTime = millis();
    // Blink left LED with menu item color (500ms on/off)
    bool ledOn = ((currentTime / 500) % 2) == 0;
    CRGB menuColor;
    switch (menuSelection) {
        case 0: menuColor = CRGB::Blue; break;   // Discovery
        case 1: menuColor = CRGB::Yellow; break; // Quiz
        case 2: menuColor = CRGB::Red; break;    // Story
        default: menuColor = CRGB::White; break;
    }
    leds[0] = ledOn ? menuColor : CRGB::Black;
    // Right LED: red to orange gradient over 2 seconds
    unsigned long cycleTime = 2000;
    uint8_t hue = map(currentTime % cycleTime, 0, cycleTime - 1, 0, 16);
    leds[1] = CHSV(hue, 255, 150);
}


void _updateQuizEffect() {
    // Helpline active override: sophisticated space-console effect
    if (quizHelplineActive) {
        float t = millis() / 1000.0f;
        // Color cycling for depth
        uint8_t baseHue = 16 + (uint8_t)(sin(t * 0.5f) * 16); // Orange/yellow range
        // Sine wave pulse for each LED, with phase offset
        for (int i = 0; i < NUM_LEDS; ++i) {
            float phase = t * 2.5f + i * 0.7f;
            float pulse = (sin(phase) + 1.0f) / 2.0f; // 0..1
            uint8_t val = 80 + pulse * 175;
            uint8_t hue = baseHue + i * 8;
            leds[i] = CHSV(hue, 255 - (pulse * 60), val);
        }
        // Comet tail sweep for extra flair if more than 2 LEDs
        if (NUM_LEDS > 2) {
            float cometPos = fmod(t * 1.2f, NUM_LEDS);
            for (int i = 0; i < NUM_LEDS; ++i) {
                float dist = fabs(i - cometPos);
                if (dist < 1.0f) {
                    // Bright comet head
                    leds[i] = CHSV(baseHue + 8, 200, 255);
                } else if (dist < 2.5f) {
                    // Fading tail
                    uint8_t tailVal = 120 - (dist - 1.0f) * 60;
                    if (tailVal > 10) leds[i] += CHSV(baseHue + 8, 220, tailVal);
                }
            }
        }
        return;
    }
    unsigned long currentTime = millis();
    static unsigned long lastStepTime = 0;

    if (quizWaiting) {
        // "Thinking" - Slow color chase or shifting pattern
        if (currentTime - lastStepTime > 150) { // Slower chase
            lastStepTime = currentTime;
            quizSearchPos = (quizSearchPos + 1) % (NUM_LEDS * 2); // Cycle through positions/colors
        }
        for (int i = 0; i < NUM_LEDS; i++) {
            if (i == quizSearchPos % NUM_LEDS) {
                leds[i] = CHSV(160, 200, 150); // Thinking color (e.g. light blue)
            } else if (i == (quizSearchPos + NUM_LEDS / 2) % NUM_LEDS && NUM_LEDS > 1) {
                leds[i] = CHSV(160, 200, 80); // Dimmer thinking color
            }
            else {
                leds[i] = CRGB::Black; // CHSV(160, 150, 30); // Off or very dim
            }
        }
    } else { // Answered
        // Flash effect
        uint8_t flashDuration = 3; // Number of on/off cycles
        uint16_t flashSpeed = 100; // ms per half cycle

        if (step < flashDuration * 2) { // Total steps for on/off cycles
            if (currentTime - lastStepTime > flashSpeed) {
                lastStepTime = currentTime;
                step++;
            }
            if (step % 2 == 1) { // Odd steps: ON
                leds[0] = leds[1] = quizAnswerStatus ? CRGB(0, 200, 0) : CRGB(200, 0, 0);
            } else { // Even steps: OFF
                leds[0] = leds[1] = CRGB::Black;
            }
        } else {
            // After flash, steady color
            CRGB finalColor = quizAnswerStatus ? CRGB(0, 100, 0) : CRGB(100, 0, 0); // Dimmer steady
            finalColor.nscale8(150); // Apply overall brightness
            leds[0] = leds[1] = finalColor;
        }
    }
}


void _updateStoryEffect() {
    // Enhanced breathing with two subtle colors
    unsigned long currentTime = millis();
    float breathSpeed = 3.0f; // Slower, calmer breath
    storyBreathPhase += breathSpeed * ( (currentTime - animationStartTime) / 1000.0f );
    animationStartTime = currentTime; // Reset for next delta calculation
    if (storyBreathPhase > TWO_PI) storyBreathPhase -= TWO_PI;

    float sinVal = (sin(storyBreathPhase) + 1.0f) / 2.0f; // 0 to 1
    uint8_t mainBrightness = 30 + sinVal * 100; // Dim: 30, Bright: 130

    // Gentle blue/purple theme
    leds[0] = CHSV(150, 220, mainBrightness); // Blue-ish
    if (NUM_LEDS > 1) {
      uint8_t offsetBrightness = 30 + (1.0f - sinVal) * 100; // Opposite phase
      leds[1] = CHSV(170, 200, offsetBrightness); // Purple-ish
    }
}


void _updateWarpEffect() {
    // Speed and intensity based on warpFactor (0.0 to 1.0)
    unsigned long currentTime = millis();
    float wf = constrain(warpFactor, 0.0f, 1.0f);

    // Base speed for chase, faster with warpFactor
    // Lower numbers mean faster update. Max 20ms, Min 150ms.
    unsigned long stepInterval = map(wf * 1000, 0, 1000, 150, 20);

    if (currentTime - lastWarpStepTime > stepInterval) {
        lastWarpStepTime = currentTime;
        warpChasePos = (warpChasePos + 1) % NUM_LEDS;
    }

    // Streaking effect: one LED is bright, others dimmer or off, creating a sense of motion
    for (int i = 0; i < NUM_LEDS; i++) {
        if (i == warpChasePos) {
            // Bright white/blue for the "front" of the streak
            uint8_t mainBrightness = 150 + wf * 105; // Up to 255
            leds[i] = CHSV(160, 50 + wf * 100, mainBrightness); // Hue towards white, less saturation at high warp
        } else {
            // Dimmer tail
            uint8_t tailBrightness = 30 + wf * 70; // Up to 100
            leds[i] = CHSV(160, 150 + wf * 50, tailBrightness); // More saturated blue for tail
        }
    }
     // If only one LED, make it pulse faster with warpFactor
    if (NUM_LEDS == 1) {
        uint8_t brightness = (sin( (currentTime / (200.0f - 180.0f * wf)) ) + 1.0f) / 2.0f * (100 + 155 * wf);
        leds[0] = CHSV(160, 50 + wf * 100, brightness);
    }
}


void _updateDiscoveryEffect() {
    unsigned long currentTime = millis();
    float deltaTime = (currentTime - lastDiscoveryEffectTime) / 1000.0f;
    lastDiscoveryEffectTime = currentTime;
    phase += deltaTime * 1.5f; // General phase increment
    if (phase > TWO_PI) phase -= TWO_PI;

    float sinVal = (sin(phase) + 1.0f) / 2.0f; // 0 to 1 for pulsing

    switch (currentObject) {
        case CelestialObjectType::STAR: {
            uint8_t b = 100 + sinVal * 155;
            leds[0] = CRGB(b, b * 0.8, b * 0.4); // Yellow/Orange
            leds[1] = CRGB(b * 0.9, b * 0.7, b * 0.3); // Slightly dimmer/offset
            if (random8() < 20) { // Twinkle
                leds[random8() % NUM_LEDS] = CRGB::White;
            }
            break;
        }
        case CelestialObjectType::PLANET: { // Earth-like
            uint8_t b1 = 80 + sinVal * 120;
            uint8_t b2 = 80 + (1.0 - sinVal) * 120;
            leds[0] = CHSV(140, 230, b1); // Blue-green
            leds[1] = CHSV(70, 200, b2);  // Green-yellow (land)
            break;
        }
        case CelestialObjectType::NEBULA: {
            // Dynamic color and brightness based on time
            float timeFactor = millis() / 200.0; // Time-based factor for animation
            float pulse = (sin(timeFactor) + 1.0) / 2.0; // Normalize to 0-1
            
            // Color cycling for nebula-like effect
            uint8_t hue1 = 150 + (sin(timeFactor * 0.5) * 40); // Cycle between blue and purple
            uint8_t hue2 = (hue1 + 20) % 255;
            
            // Adjust brightness based on pulse
            uint8_t brightness1 = 50 + (pulse * 80); // Pulsing effect
            uint8_t brightness2 = 50 + ((1.0 - pulse) * 80);
            
            leds[0] = CHSV(hue1, 255, brightness1);
            leds[1] = CHSV(hue2, 240, brightness2);
            break;
        }
        case CelestialObjectType::GALAXY: {
            uint8_t b = 60 + sinVal * 70; // Soft pulse
            leds[0] = CHSV(30, 30, b); // Off-white, cosmic latte-ish
            leds[1] = CHSV(35, 35, b * 0.8f);
            break;
        }
        case CelestialObjectType::SOLAR_SYSTEM: { // Bright "sun"
            leds[0] = CRGB(255, 220, 100); // Bright Yellow Sun
            // Other LED could cycle through planet colors slowly if we add more steps
            uint8_t b = 80 + sinVal * 100;
            leds[1] = CHSV( (uint8_t)(currentTime / 200) % 255, 200, b); // Slow hue cycle
            break;
        }
        case CelestialObjectType::ASTEROID_FIELD: {
            if (random8() < 40) { // Increased chance for more flickers
                uint8_t flickerLed = random8() % NUM_LEDS;
                leds[flickerLed] = CRGB(random8(100,200), random8(100,200), random8(100,200)); // Greyish glint
                 if (NUM_LEDS > 1) leds[1-flickerLed] = CRGB::Black; // Other LED off
            } else {
                leds[0] = leds[1] = CRGB::Black; // Mostly dark
            }
            break;
        }
        case CelestialObjectType::BLACK_HOLE: {
            if (hapticOverrideActive && currentTime < hapticOverrideEndTime) {
                // Flash both LEDs white on star consumption
                leds[0] = leds[1] = CRGB::White;
            } else {
                leds[0] = CRGB(10, 0, 20);
                if (random8() < 10) {
                    leds[1] = CHSV(170 + random8(20), 255, random8(30,60));
                } else {
                    leds[1] = CRGB(5, 0, 10);
                }
            }
            break;
        }
        case CelestialObjectType::PULSAR: { // Sharp pulse
            uint16_t pulseRate = 500; // ms for full cycle
            bool isOn = (currentTime % pulseRate) < (pulseRate / 5); // On for 1/5th of cycle
            leds[0] = leds[1] = isOn ? CRGB(200, 220, 255) : CRGB(0,0,30); // Bright white/blue or dim blue
            break;
        }
        case CelestialObjectType::SUPERNOVA: {
            unsigned long elapsed = currentTime - animationStartTime;
            if (elapsed < 1000) {
                float pulse = (sin(elapsed / 200.0f * TWO_PI) + 1.0f) / 2.0f;
                uint8_t b = 50 + pulse * 150;
                leds[0] = leds[1] = CRGB(b, b, b);
            } else if (elapsed < 1500) {
                leds[0] = leds[1] = CRGB::White;
            } else if (elapsed < 3000) {
                leds[0] = leds[1] = CRGB(random8(200,255), random8(100,200), 0);
            } else if (elapsed < 5000) {
                uint8_t hue = map(elapsed, 3000, 5000, 16, 0);
                uint8_t val = map(elapsed, 3000, 5000, 200, 50);
                leds[0] = leds[1] = CHSV(hue, 255, val);
            } else {
                leds[0] = leds[1] = CHSV(0, 255, 50);
            }
            break;
        }
        case CelestialObjectType::COMET: {
            // Dynamic comet head and tail with pulsing
            unsigned long cometDuration = 3500UL; // tail fade duration
            unsigned long moveSpeed = cometDuration / (NUM_LEDS + 2);
            if (currentTime - animationStartTime > moveSpeed) {
                animationStartTime = currentTime;
                step = (step + 1) % (NUM_LEDS + 2);
            }
            float sinValLocal = (sin(phase) + 1.0f) / 2.0f; // 0 to 1
            for (int i = 0; i < NUM_LEDS; ++i) leds[i] = CRGB::Black;
            if (step < NUM_LEDS) {
                uint8_t headBrightness = 150 + sinValLocal * 100; // dynamic head brightness
                leds[step] = CRGB(headBrightness, headBrightness, headBrightness);
                if (NUM_LEDS > 1 && step > 0) {
                    uint8_t tailBrightness = 50 + sinValLocal * 100;
                    leds[step - 1] = CRGB(tailBrightness * 0.6, tailBrightness * 0.8, tailBrightness);
                }
            } else if (step == NUM_LEDS && NUM_LEDS > 1) {
                uint8_t tailBrightness = 50 + sinValLocal * 100;
                leds[NUM_LEDS-1] = CRGB(tailBrightness * 0.6, tailBrightness * 0.8, tailBrightness);
            }
            break;
        }
        case CelestialObjectType::BINARY_STAR: {
            // Animation speed is now twice the global phase speed
            float sinVal = (sin(phase * 2.0f) + 1.0f) / 2.0f;
            float sinVal2 = (sin(phase * 2.0f + PI) + 1.0f) / 2.0f; // Opposite phase at double speed
            uint8_t b1 = 80 + sinVal * 175;
            uint8_t b2 = 80 + sinVal2 * 175;
            leds[0] = CRGB(b1, b1*0.7, 0); // Star 1 (Orange)
            leds[1] = CRGB(b2*0.7, b2*0.7, b2); // Star 2 (Blue-white)
            break;
        }
        case CelestialObjectType::SPACE_STATION: {
            leds[0] = CRGB(150, 150, 180); // Cool white
            if ( (currentTime / 1000) % 2 == 0) { // Slow blink on second LED
                 leds[1] = CRGB(0, 80, 200); // Blue indicator
            } else {
                 leds[1] = CRGB(0, 20, 50); // Dim blue
            }
            break;
        }
        case CelestialObjectType::NONE:
        default: { // Dim blue for empty space
            float pulse = (sin(millis() / 1000.0) + 1.0) / 2.0; // Normalize to 0-1 for pulsing
            uint8_t minVisibleBrightness = 60; // Ensure LEDs are visibly on
            uint8_t pulseRange = 90; // The amplitude of the pulse
            uint8_t pulsedBrightness = minVisibleBrightness + (pulse * pulseRange); // Subtle pulsing effect from min to min+range
            
            leds[0] = leds[1] = CHSV(150, 255, pulsedBrightness); // Deep blue with pulsing
            
            // Keep the rare twinkle effect
            if (random8() < 5) { // Very rare twinkle
                 leds[random8() % NUM_LEDS].maximizeBrightness(30);
            }
            break;
        }
    }
}


void _updateTransitionEffect() {
    unsigned long elapsedTime = millis() - transitionStartTime;
    if (elapsedTime >= TRANSITION_DURATION) {
        // Transition finished, switch to the actual target mode
        currentLedMode = previousLedMode; // Temporarily set to what it was to call the *actual* update
        if (currentLedMode == LedMode::MENU) _updateMenuEffect();
        else if (currentLedMode == LedMode::QUIZ) _updateQuizEffect();
        else if (currentLedMode == LedMode::STORY) _updateStoryEffect();
        else if (currentLedMode == LedMode::WARP) _updateWarpEffect();
        else if (currentLedMode == LedMode::DISCOVERY) _updateDiscoveryEffect();
        else { leds[0] = leds[1] = CRGB::Black; } // Default for OFF
        
        currentLedMode = previousLedMode; // previousLedMode was set to the target mode.
        LedMode finalMode = currentLedMode; // This is the mode we just transitioned TO.


        // Instead of manipulating global leds, just call the function and it will write to global 'leds'
        if (finalMode == LedMode::MENU) _updateMenuEffect();
        else if (finalMode == LedMode::QUIZ) _updateQuizEffect();
        else if (finalMode == LedMode::STORY) _updateStoryEffect();
        else if (finalMode == LedMode::WARP) _updateWarpEffect();
        else if (finalMode == LedMode::DISCOVERY) _updateDiscoveryEffect();
        else { FastLED.clear(); } // For OFF

    } else {
        float progress = (float)elapsedTime / TRANSITION_DURATION;
        progress = constrain(progress, 0.0f, 1.0f);

        // Calculate the target state for the fade
        CRGB targetStateLeds[NUM_LEDS];
        LedMode actualCurrentMode = currentLedMode;
        currentLedMode = previousLedMode;

        // Use a temporary CRGB array for the target effect calculation
        CRGB tempTargetLeds[NUM_LEDS];
        CRGB* originalLeds = leds;

        if (previousLedMode == LedMode::MENU) _updateMenuEffect();
        else if (previousLedMode == LedMode::QUIZ) _updateQuizEffect();
        else if (previousLedMode == LedMode::STORY) _updateStoryEffect();
        else if (previousLedMode == LedMode::WARP) _updateWarpEffect();
        else if (previousLedMode == LedMode::DISCOVERY) _updateDiscoveryEffect();
        else { leds[0] = CRGB::Black; leds[1] = CRGB::Black; } // For OFF state

        // 'leds' now holds the target state. Blend 'oldLeds' to 'leds'
        for (int i = 0; i < NUM_LEDS; i++) {
            leds[i] = blend(oldLeds[i], leds[i], progress * 255);
        }
        currentLedMode = actualCurrentMode; // Restore to TRANSITION
    }
}


void updateLedEffects() {
    unsigned long currentTime = millis();
    if (currentTime - lastLedUpdateTime < LED_UPDATE_INTERVAL) {
        return; // Update at defined interval
    }
    lastLedUpdateTime = currentTime;

    LedMode modeToExecute = currentLedMode;
    if (currentLedMode == LedMode::TRANSITION) {
         _updateTransitionEffect();
    } else if (currentLedMode == LedMode::MENU) {
        _updateMenuEffect();
    } else if (currentLedMode == LedMode::QUIZ) {
        _updateQuizEffect();
    } else if (currentLedMode == LedMode::STORY) {
        _updateStoryEffect();
    } else if (currentLedMode == LedMode::WARP) {
        _updateWarpEffect();
    } else if (currentLedMode == LedMode::DISCOVERY) {
        _updateDiscoveryEffect();
    } else if (currentLedMode == LedMode::OFF) {
        FastLED.clear(); // Ensure LEDs are off
    }

    FastLED.setBrightness(BRIGHTNESS); // Apply global brightness
    FastLED.show();
}

// Placeholder for setLedColor - not typically used with modes, but good to have
void setLedColor(uint8_t ledIndex, uint32_t color) {
    if (ledIndex < NUM_LEDS) {
        leds[ledIndex] = CRGB((color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF);
    }
} 