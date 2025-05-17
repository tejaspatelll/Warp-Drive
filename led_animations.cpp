#include "led_animations.h"
#include <Arduino.h> // For millis()

// Define the LED array
CRGB leds[NUM_LEDS];

// ----- Global variables from main sketch -----
// We need warpFactor for the warp LED effect.
// Make sure this is defined in your main .ino file: float warpFactor = 0.0f;
extern float warpFactor; 
// We might need objectScale for discovery effects, if not already available
// extern float objectScale; // Already declared in .ino and used in _updateDiscoveryEffect
// Haptic override for special events like star consumption
extern bool hapticOverrideActive;
extern unsigned long hapticOverrideEndTime;

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

    // Use extern float objectScale; (defined in .ino)
    // extern float objectScale; // Ensure it's available
    // float s = constrain(objectScale, 0.5f, 3.0f); // Already in original code

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
            uint8_t hue1 = 150 + sinVal * 40; // Cycle between blue and purple
            uint8_t hue2 = (hue1 + 20) % 255;
            leds[0] = CHSV(hue1, 255, 50 + sinVal * 80);
            leds[1] = CHSV(hue2, 240, 50 + (1.0-sinVal) * 70);
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
            uint8_t b = 10 + sinVal * 20; // Very subtle pulse
            leds[0] = leds[1] = CHSV(160, 255, b); // Dark, deep blue
            if (random8() < 5) { // Very rare twinkle
                 leds[random8()%NUM_LEDS].maximizeBrightness(30);
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
        
        // Now, restore the intended currentLedMode that was set by setLedModeX functions before _startTransition
        // This logic is tricky. The `previousLedMode` in _startTransition was the state *before* calling setLedModeX.
        // The `currentLedMode` was then set to the *new* mode by setLedModeX.
        // Then `_startTransition` changed `currentLedMode` to `LedMode::TRANSITION` and stored the *new* mode (the one we are going to) in `previousLedMode` for this context.
        // This is a bit confusing. Let's simplify.
        // The setLedModeX functions should set the *target* mode into a new variable, e.g. `targetLedMode`.
        // For now, I'll assume the `setLedModeX` has ALREADY set the `currentLedMode` to the desired final mode.
        // So, after transition, currentLedMode is ALREADY the target.
        // The `previousLedMode` here would be the mode we were in *during* the transition effect.
        // This needs fixing.

        // Corrected logic: setLedModeX sets currentLedMode. _startTransition copies currentLedMode to targetLedMode, then sets currentLedMode = TRANSITION.
        // Let's adjust: setLedModeX sets a `targetLedMode`. _startTransition sets `currentLedMode = TRANSITION`.
        // The effect functions will produce the `leds` state for the *target* mode.
        // The `previousLedMode` was the one we were fading *from*.
        // The `currentLedMode` is the one we are fading *to*. This is what is set by the `setLedModeX` functions.
        // So when transition is done, `currentLedMode` is already correct.

        // The `setLedModeX` functions now call _startTransition which sets currentLedMode = LedMode::TRANSITION.
        // The original currentLedMode (the target) needs to be restored or used.
        // Let's make `previousLedMode` the mode we are going TO.
        // In setLedModeMenu:
        //   _startTransition(); // currentLedMode becomes TRANSITION, oldLeds is current state
        //   previousLedMode = LedMode::MENU; // Store TARGET mode here
        //   ... update menuSelection etc.

        // In _updateTransitionEffect, when done:
        //   currentLedMode = previousLedMode; // Restore target mode
        //   // And call the effect for this frame
        //   if (currentLedMode == LedMode::MENU) _updateMenuEffect(); ... etc.
        // This way, the first frame after transition renders the new mode correctly.

        // Let's refine the state management for transitions:
        // 1. `setLedModeX` will:
        //    - If mode is changing:
        //        - Store `leds` into `oldLeds`.
        //        - Set `transitionStartTime = millis()`.
        //        - Set `targetLedMode = new_mode`. (Need a new static var `targetLedMode`)
        //        - Set `currentLedMode = LedMode::TRANSITION`.
        //        - Update specific parameters for the new_mode.
        // 2. `_updateTransitionEffect` will:
        //    - Calculate blend factor.
        //    - Generate `targetLeds` by calling the effect function for `targetLedMode`.
        //    - Blend `oldLeds` and `targetLeds` into `leds`.
        //    - If `elapsedTime >= TRANSITION_DURATION`:
        //        - Set `currentLedMode = targetLedMode`.
        //        - `memcpy(leds, targetLeds, sizeof(leds));` // Ensure final state is pure target

        // For now, the simpler approach is: setLedModeX sets `currentLedMode` to the target.
        // `_startTransition` saves `leds` to `oldLeds`, sets `previousLedMode = currentLedMode` (the one we're going to),
        // and then sets `currentLedMode = LedMode::TRANSITION`.
        // When transition ends, `currentLedMode` becomes `previousLedMode` (which was the target).
        currentLedMode = previousLedMode; // previousLedMode was set to the target mode.
        // The effect for the target mode will be called on the next updateLedEffects() call.
        // To avoid a 1-frame glitch or missed update, we should probably calculate the final state right here.
        LedMode finalMode = currentLedMode; // This is the mode we just transitioned TO.
        // Calculate the final state for the new mode
        CRGB finalLeds[NUM_LEDS];
        // Temporarily set leds pointer for the effect functions
        CRGB* actualLedsPtr = leds; // Save current leds array
        // CRGB tempLeds[NUM_LEDS]; leds = tempLeds; // This doesn't work as leds is global array not pointer

        // Instead of manipulating global leds, just call the function and it will write to global 'leds'
        if (finalMode == LedMode::MENU) _updateMenuEffect();
        else if (finalMode == LedMode::QUIZ) _updateQuizEffect();
        else if (finalMode == LedMode::STORY) _updateStoryEffect();
        else if (finalMode == LedMode::WARP) _updateWarpEffect();
        else if (finalMode == LedMode::DISCOVERY) _updateDiscoveryEffect();
        else { FastLED.clear(); } // For OFF
        // leds = actualLedsPtr; // Restore (not needed if effect functions write to global leds)
        // FastLED.show(); // Show the pure final state immediately.

    } else {
        float progress = (float)elapsedTime / TRANSITION_DURATION;
        progress = constrain(progress, 0.0f, 1.0f);

        // Calculate the target state for the fade
        CRGB targetStateLeds[NUM_LEDS];
        // Hack: to get the target state, temporarily set mode and call update
        LedMode actualCurrentMode = currentLedMode; // This is TRANSITION
        currentLedMode = previousLedMode; // Set to target mode to get its state

        // Use a temporary CRGB array for the target effect calculation
        CRGB tempTargetLeds[NUM_LEDS];
        CRGB* originalLeds = leds; // leds is global, so effect functions write to it.
                                   // This is problematic. Effect functions need to write to a buffer.

        // Let's redesign effect functions to fill a passed buffer or return CRGB array
        // For now, a simpler blend directly using the global 'leds' which will be overwritten
        // Get target state by calling its update function. This will modify global 'leds'.
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
        // If you call this, you might want to switch to a manual/OFF mode
        // or a specific "direct control" mode.
        // For now, it just sets the color but mode logic will override on next update.
        leds[ledIndex] = CRGB((color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF);
        // To make this stick, you'd do:
        // currentLedMode = LedMode::OFF; // Or some LedMode::MANUAL
        // FastLED.show();
    }
} 