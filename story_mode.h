#include <vector>
#ifndef STORY_MODE_H
#define STORY_MODE_H

#include <TFT_eSPI.h>
#include <Arduino.h> // For PI constant and other math functions
#include "sprite_manager.h"
#include "star.h"
#include "led_animations.h" // Add this include for LED control

// Make sure POT_PIN is available from main file (warpdrive_esp32_tft.ino)
#ifndef POT_PIN
#define POT_PIN 7    // Default value if not defined elsewhere
#endif

// Screen dimensions - ensure these are defined or passed appropriately
// These might be better passed to the class or accessed via extern if truly global
extern const int SCREEN_WIDTH;
extern const int SCREEN_HEIGHT;

// Star count definition - use the same value in all files
#define STAR_COUNT 180
#define MAX_STREAK_LENGTH 20

// Forward declarations of external variables and functions
// These are dependencies that the StoryMode class will need
extern TFT_eSPI tft;
extern uint16_t BG_COLOR;
extern void drawStar(); // Modified to match void (*)() signature
extern void drawPlanet();
extern void drawNebula();
extern void drawGalaxy();
extern void drawSolarSystem();
extern void drawAsteroidField();
extern void drawBlackHole();
extern void drawPulsar();
extern void drawSupernova();
extern void drawComet();
extern void drawBinaryStar();
extern void drawSpaceStation();
extern void updateStars(); // For background star twinkling
extern void updateWarpStars(); // For warp effect

// Forward declare the Star struct so we can access it
struct Star; // Ensure this is defined, perhaps in star.h
extern Star stars[];
extern uint16_t prevX[][MAX_STREAK_LENGTH + 1];
extern uint16_t prevY[][MAX_STREAK_LENGTH + 1];

// Also need to access these globals for object positioning
// These should ideally be managed by the StoryMode class or passed to draw functions
extern int objectX;
extern int objectY;
extern float objectScale;

// Screen dimensions for centering objects
//const int SCREEN_WIDTH = 128; // Now use global SCREEN_WIDTH
//const int SCREEN_HEIGHT = 128; // Now use global SCREEN_HEIGHT

// Define story stop structure
struct StoryStop {
    const char* name;              // Name of the celestial object
    const char* narration;         // Carl Sagan-inspired narration text
    const char* fact;              // Educational fact or question (stored but not shown by default)
    void (*drawFunction)();        // Function pointer to draw the celestial object
};

// Store the actual story stops data with the class
// This needs to be defined in a .cpp file or carefully managed if only in .h
// For now, we'll keep the definition outside but the declaration inside the class for static const members.

const StoryStop STORY_STOPS_DATA[] = {
    {
        "Our Sun", 
        "A star, our Sun, the giver of light and warmth. Every living being on Earth owes its existence to this ball of nuclear fusion, our quiet companion in the cosmos.",
        "The Sun contains 99.86% of all mass in the solar system.",
        drawStar
    },
    {
        "Planet Earth", 
        "The pale blue dot. Suspended in a sunbeam. A mote of dust in the cosmic dark. Our home, the only world we\'ve known, fragile and precious beyond measure.",
        "Earth is the only planet not named after a mythological god or goddess.",
        drawPlanet
    },
    {
        "Nebula", 
        "The birthplace of stars. These vast clouds of gas and dust are cosmic nurseries where new suns take their first breath, illuminating the darkness with the promise of worlds yet to be.",
        "The word nebula comes from Latin, meaning \'cloud\' or \'fog\'.",
        drawNebula
    },
    {
        "Galaxy", 
        "Islands of stars in the cosmic ocean. Each galaxy contains billions of suns, and each of those stars might cradle worlds of unimaginable wonder waiting to be discovered.",
        "There are estimated to be over 2 trillion galaxies in the observable universe.",
        drawGalaxy
    },
    {
        "Solar System", 
        "A family of worlds. Planets, moons, asteroids, and comets - all dancing to the gravitational symphony of their parent star, a cosmic mobile of matter in elegant orbit.",
        "If the Sun were hollow, it could fit about 1.3 million Earths inside it.",
        drawSolarSystem
    },
    {
        "Asteroid Field", 
        "The debris of creation. These rocky remnants tell the story of planets that might have been, celestial building blocks left over from the solar system\'s turbulent youth.",
        "Most asteroids orbit in a belt between Mars and Jupiter.",
        drawAsteroidField
    },
    {
        "Black Hole", 
        "Where space and time end. These cosmic devourers bend reality itself, teaching us that gravity is not just a force, but a curvature in the fabric of spacetime.",
        "If Earth were compressed to the density of a black hole, it would be the size of a cherry.",
        drawBlackHole
    },
    {
        "Pulsar", 
        "Cosmic lighthouses. The rapidly spinning corpses of massive stars, sweeping beams of radiation across the cosmos with clock-like precision, nature\'s most perfect timepieces.",
        "Some pulsars rotate more than 700 times per second.",
        drawPulsar
    },
    {
        "Supernova", 
        "The spectacular death of stars. In their final moments, these stellar giants seed the cosmos with the heavy elements needed for life, a cosmic phoenix creating renewal from destruction.",
        "A supernova can briefly outshine an entire galaxy.",
        drawSupernova
    },
    {
        "Comet", 
        "Cosmic messengers from the frozen outskirts. These icy wanderers carry the pristine building blocks of our solar system, time capsules from the dawn of our cosmic neighborhood.",
        "Comets\' tails always point away from the Sun due to solar wind.",
        drawComet
    },
    {
        "Binary Star", 
        "Stellar siblings locked in an eternal dance. These pairs of stars orbit their common center of mass, showing us that even in the cosmos, companionship is a fundamental pattern.",
        "More than half of all stars in our galaxy are part of binary or multiple star systems.",
        drawBinaryStar
    },
    {
        "Space Station", 
        "Humanity\'s first outpost among the stars. A testament to our curiosity and determination to explore beyond our world, a stepping stone to our future among the cosmos.",
        "The International Space Station orbits Earth at about 28,000 km/h.",
        drawSpaceStation
    }
};

const int STORY_STOPS_DATA_COUNT = sizeof(STORY_STOPS_DATA) / sizeof(STORY_STOPS_DATA[0]);

class StoryMode {
public:
    StoryMode() : 
        storyTitleHandle({0}),
        storyNarrationHandle({0}),
        currentStoryStep(0),
        textScrollOffset(0),
        lastScrollTimeNs(0),
        initialized(false),
        potentiometerCalibrated(false),
        storyExitButton_lastPressTime(0), 
        storyExitButton_wasPressed(false), 
        warpActive(false),
        previousWarpState(false),
        warpEngageTime(0),
        minWarpTravelDuration(1500),
        narrationBoxHeight(0),
        narrationBoxYPos(0),
        narrationBoxPadding(0),
        charsPerNarrationLine(0),
        lastDisplayedStoryStep(-1),
        narrationScrollY(0),
        lastNarrationScrollTime(0),
        narrationScrollPixelSpeed(50), // ms per pixel
        currentWarpFactor(0.0f)
    {
        // Constructor body (if needed for more complex init)
    }

    ~StoryMode() {
        deinitSprites(); // Ensure sprites are cleaned up
    }

    void init();
    bool update(int potValue, bool buttonPinState_LOW);
    bool processInput(int potValue, bool buttonPinState_LOW);
    void render(); // New function to separate update logic from drawing
    bool isActive() const { return initialized; } // Simple check for now
    void exit() {
        deinitSprites();
        initialized = false;
        potentiometerCalibrated = false;
        currentStoryStep = 0; // Reset to start
        // Add any other necessary cleanup for exiting story mode
        Serial.println("Exited Story Mode and cleaned up.");
    }

    float getWarpFactor() const { return currentWarpFactor; }
    int getCurrentStoryStep() const { return currentStoryStep; }

private:
    // --- Story Mode Sprites for flicker-free rendering ---
    SpriteHandle storyTitleHandle = {0};
    SpriteHandle storyNarrationHandle = {0};

    void createSprites();
    void deinitSprites(); 

    // Story navigation state variables
    int currentStoryStep;
    int textScrollOffset;
    unsigned long lastScrollTimeNs;
    bool initialized;
    bool potentiometerCalibrated;
    // Debounce for exit button within StoryMode
    unsigned long storyExitButton_lastPressTime;
    bool storyExitButton_wasPressed;

    // Warp drive state tracking for story mode
    bool warpActive;
    bool previousWarpState;
    unsigned long warpEngageTime;
    unsigned long minWarpTravelDuration;
    float currentWarpFactor; // New member variable for warpFactor

    // Story sequence - now static const members
    static const StoryStop storyStopsList[];
    static const int TOTAL_STORY_STOPS;

// Text display constants
    int narrationBoxHeight;
    int narrationBoxYPos;
    int narrationBoxPadding;
    int charsPerNarrationLine;

    // Static variable for tracking story step changes
    int lastDisplayedStoryStep;

    // Vertical scrolling text state for narration sprite
    int narrationScrollY;
    unsigned long lastNarrationScrollTime;
    int narrationScrollPixelSpeed;

    // Layout and drawing helper methods
    void setupLayout();
    void drawCalibrationPrompt(int potValue);
    void prepareTitleSprite(const char* name);
    void prepareNarrationSprite(const char* text);
    void updateNarrationScrolling(const char* text);
    void advanceToNextStop();
    void updateCurrentStepVisuals();
};

// Define static members of the class
const StoryStop StoryMode::storyStopsList[] = {
    {
        "Our Sun", 
        "A star, our Sun, the giver of light and warmth. Every living being on Earth owes its existence to this ball of nuclear fusion, our quiet companion in the cosmos.",
        "The Sun contains 99.86% of all mass in the solar system.",
        drawStar
    },
    {
        "Planet Earth", 
        "The pale blue dot. Suspended in a sunbeam. A mote of dust in the cosmic dark. Our home, the only world we\'ve known, fragile and precious beyond measure.",
        "Earth is the only planet not named after a mythological god or goddess.",
        drawPlanet
    },
    {
        "Nebula", 
        "The birthplace of stars. These vast clouds of gas and dust are cosmic nurseries where new suns take their first breath, illuminating the darkness with the promise of worlds yet to be.",
        "The word nebula comes from Latin, meaning \'cloud\' or \'fog\'.",
        drawNebula
    },
    {
        "Galaxy", 
        "Islands of stars in the cosmic ocean. Each galaxy contains billions of suns, and each of those stars might cradle worlds of unimaginable wonder waiting to be discovered.",
        "There are estimated to be over 2 trillion galaxies in the observable universe.",
        drawGalaxy
    },
    {
        "Solar System", 
        "A family of worlds. Planets, moons, asteroids, and comets - all dancing to the gravitational symphony of their parent star, a cosmic mobile of matter in elegant orbit.",
        "If the Sun were hollow, it could fit about 1.3 million Earths inside it.",
        drawSolarSystem
    },
    {
        "Asteroid Field", 
        "The debris of creation. These rocky remnants tell the story of planets that might have been, celestial building blocks left over from the solar system\'s turbulent youth.",
        "Most asteroids orbit in a belt between Mars and Jupiter.",
        drawAsteroidField
    },
    {
        "Black Hole", 
        "Where space and time end. These cosmic devourers bend reality itself, teaching us that gravity is not just a force, but a curvature in the fabric of spacetime.",
        "If Earth were compressed to the density of a black hole, it would be the size of a cherry.",
        drawBlackHole
    },
    {
        "Pulsar", 
        "Cosmic lighthouses. The rapidly spinning corpses of massive stars, sweeping beams of radiation across the cosmos with clock-like precision, nature\'s most perfect timepieces.",
        "Some pulsars rotate more than 700 times per second.",
        drawPulsar
    },
    {
        "Supernova", 
        "The spectacular death of stars. In their final moments, these stellar giants seed the cosmos with the heavy elements needed for life, a cosmic phoenix creating renewal from destruction.",
        "A supernova can briefly outshine an entire galaxy.",
        drawSupernova
    },
    {
        "Comet", 
        "Cosmic messengers from the frozen outskirts. These icy wanderers carry the pristine building blocks of our solar system, time capsules from the dawn of our cosmic neighborhood.",
        "Comets\' tails always point away from the Sun due to solar wind.",
        drawComet
    },
    {
        "Binary Star", 
        "Stellar siblings locked in an eternal dance. These pairs of stars orbit their common center of mass, showing us that even in the cosmos, companionship is a fundamental pattern.",
        "More than half of all stars in our galaxy are part of binary or multiple star systems.",
        drawBinaryStar
    },
    {
        "Space Station", 
        "Humanity\'s first outpost among the stars. A testament to our curiosity and determination to explore beyond our world, a stepping stone to our future among the cosmos.",
        "The International Space Station orbits Earth at about 28,000 km/h.",
        drawSpaceStation
    }
};
const int StoryMode::TOTAL_STORY_STOPS = sizeof(StoryMode::storyStopsList) / sizeof(StoryMode::storyStopsList[0]);


// --- Member function definitions for StoryMode class ---

// NOTE: SpriteManager::begin() must be called in setup() before using StoryMode!

void StoryMode::deinitSprites() {
    // No-op: UI elements drawn directly, no sprite cleanup needed
}

void StoryMode::setupLayout() { // Was setupStoryModeLayout
    narrationBoxHeight = SCREEN_HEIGHT / 4; // Made box taller (was SCREEN_HEIGHT / 9)
    narrationBoxYPos = SCREEN_HEIGHT - narrationBoxHeight;
    narrationBoxPadding = SCREEN_WIDTH / 40;
    charsPerNarrationLine = (SCREEN_WIDTH - 2 * narrationBoxPadding) / 6; // Assuming char width of 6
}

void StoryMode::drawCalibrationPrompt(int potValue) { // Was drawSetDialToZeroPrompt
    static int lastPotValue = -1;
    static bool firstDraw = true;

    // Responsive layout
    int dialRadius = std::min(SCREEN_WIDTH, SCREEN_HEIGHT) / 8;
    int dialCenterX = SCREEN_WIDTH / 2;
    int dialCenterY = SCREEN_HEIGHT * 3 / 4;

    if (firstDraw) {
        tft.fillScreen(BG_COLOR);
        tft.setTextDatum(MC_DATUM);
        tft.setTextSize(2);
        for(int i = 3; i > 0; i--) {
            tft.setTextColor(tft.color565(0, 60 - i*15, 120 - i*20));
            tft.drawString("COSMIC", SCREEN_WIDTH/2 + i, SCREEN_HEIGHT/8 + i);
            tft.drawString("JOURNEY", SCREEN_WIDTH/2 + i, SCREEN_HEIGHT/8 + 25 + i);
        }
        tft.setTextColor(tft.color565(0, 200, 255));
        tft.drawString("COSMIC", SCREEN_WIDTH/2, SCREEN_HEIGHT/8);
        tft.drawString("JOURNEY", SCREEN_WIDTH/2, SCREEN_HEIGHT/8 + 25);
        for(int i = 0; i < 2; i++) {
            tft.drawFastHLine(SCREEN_WIDTH/2 - 80 + i*2, SCREEN_HEIGHT/8 + 45 + i, 160 - i*4, 
                             tft.color565(0, 200 - i*50, 255 - i*50));
        }
        int boxW = SCREEN_WIDTH * 0.75;
        int boxH = SCREEN_HEIGHT * 0.15;
        int boxX = (SCREEN_WIDTH - boxW) / 2;
        int boxY = SCREEN_HEIGHT * 0.3;
        tft.fillRect(boxX + 4, boxY + 4, boxW - 8, boxH - 8, tft.color565(0, 40, 80));
        tft.drawRect(boxX + 2, boxY + 2, boxW - 4, boxH - 4, tft.color565(0, 160, 255));
        tft.drawRect(boxX, boxY, boxW, boxH, tft.color565(255, 255, 0));
        tft.setTextSize(2);
        tft.setTextColor(tft.color565(100, 200, 255));
        tft.drawString("SET DIAL TO", SCREEN_WIDTH/2 + 1, boxY + boxH/3 + 1);
        tft.drawString("ZERO", SCREEN_WIDTH/2 + 1, boxY + boxH*2/3 + 1);
        tft.setTextColor(tft.color565(255, 255, 0));
        tft.drawString("SET DIAL TO", SCREEN_WIDTH/2, boxY + boxH/3);
        tft.drawString("ZERO", SCREEN_WIDTH/2, boxY + boxH*2/3);
        firstDraw = false;
    }

    if (potValue != lastPotValue) {
        tft.fillCircle(dialCenterX, dialCenterY, dialRadius + 6, BG_COLOR);
        tft.fillCircle(dialCenterX, dialCenterY, dialRadius, tft.color565(0, 40, 80));
        tft.drawCircle(dialCenterX, dialCenterY, dialRadius, tft.color565(0, 160, 255));
        tft.drawCircle(dialCenterX, dialCenterY, dialRadius - 1, tft.color565(0, 100, 200));
        for (int angleDeg = 0; angleDeg < 360; angleDeg += 30) {
            float rad = angleDeg * PI / 180.0f;
            int innerX = dialCenterX + cos(rad) * (dialRadius - 5);
            int innerY = dialCenterY + sin(rad) * (dialRadius - 5);
            int outerX = dialCenterX + cos(rad) * dialRadius;
            int outerY = dialCenterY + sin(rad) * dialRadius;
            tft.drawLine(innerX, innerY, outerX, outerY, tft.color565(0, 200, 255));
        }
        int zeroX = dialCenterX - dialRadius - 10;
        int zeroY = dialCenterY;
        tft.fillCircle(zeroX, zeroY, 6, tft.color565(0, 50, 0));
        tft.fillCircle(zeroX, zeroY, 4, tft.color565(0, 100, 0));
        tft.setTextColor(tft.color565(0, 255, 0));
        tft.setTextSize(1);
        tft.drawString("0", zeroX, zeroY);
        float angle = PI + ((potValue / 4095.0f) * (300.0f * PI / 180.0f));
        int pointerX = dialCenterX + cos(angle) * (dialRadius - 3);
        int pointerY = dialCenterY + sin(angle) * (dialRadius - 3);
        tft.drawLine(dialCenterX, dialCenterY, pointerX, pointerY, tft.color565(180, 0, 0));
        tft.drawLine(dialCenterX + 1, dialCenterY, pointerX + 1, pointerY, tft.color565(255, 0, 0));
        tft.drawLine(dialCenterX - 1, dialCenterY, pointerX - 1, pointerY, tft.color565(255, 0, 0));
        tft.fillCircle(dialCenterX, dialCenterY, 3, tft.color565(0, 160, 255));
        tft.fillCircle(dialCenterX, dialCenterY, 2, tft.color565(255, 255, 255));
        lastPotValue = potValue;
    }
}

void StoryMode::prepareTitleSprite(const char* name) {
    // Direct draw title bar instead of using a sprite
    tft.fillRect(0, 0, SCREEN_WIDTH, 26, tft.color565(0, 40, 80));
    tft.setTextDatum(MC_DATUM);
    tft.setTextSize(2);
    tft.setTextColor(tft.color565(100, 200, 255));
    tft.drawCentreString(name, SCREEN_WIDTH / 2, 13, 1);
}

void StoryMode::prepareNarrationSprite(const char* text) {
    // Direct draw narration box instead of using a sprite
    tft.fillRect(0, narrationBoxYPos, SCREEN_WIDTH, narrationBoxHeight, tft.color565(0, 0, 40));
    tft.setTextSize(1);
    tft.setTextColor(tft.color565(200, 255, 200));
    tft.setCursor(narrationBoxPadding, narrationBoxYPos + narrationBoxPadding);
    tft.print(text);
}

void StoryMode::createSprites() {
    // No-op: UI elements drawn directly, no sprite creation needed
}

void StoryMode::init() { // Was initStoryMode
    setupLayout();
    currentStoryStep = 0;
    textScrollOffset = 0; // For old text box system, might remove
    lastScrollTimeNs = millis();
    warpActive = false;
    previousWarpState = false;
    potentiometerCalibrated = false;
    lastDisplayedStoryStep = -1;
    narrationScrollY = 0; // Reset vertical scroll for narration sprite
    lastNarrationScrollTime = millis();

    deinitSprites(); // Clear any existing sprites first
    createSprites(); // Create new sprites

    const int POT_THRESHOLD = 50;
    int calibratedPotValue = 0; // Use a local variable for calibration

    // Perform initial potentiometer read for calibration check
    int currentRawPotVal = 0;
    for (int i = 0; i < 4; i++) { // Match averaging from main sketch's readPotentiometer
        currentRawPotVal += analogRead(POT_PIN);
    }
    calibratedPotValue = 4095 - (currentRawPotVal / 4);

    if (calibratedPotValue > POT_THRESHOLD) {
        unsigned long lastUpdateTime = 0;
        const unsigned long UPDATE_INTERVAL = 50;
        while (calibratedPotValue > POT_THRESHOLD) {
            unsigned long currentTime = millis();
            if (currentTime - lastUpdateTime > UPDATE_INTERVAL) {
                lastUpdateTime = currentTime;
                int rawValueSamples = 0;
                for (int i = 0; i < 4; i++) { rawValueSamples += analogRead(POT_PIN); }
                currentRawPotVal = rawValueSamples / 4;
                calibratedPotValue = 4095 - currentRawPotVal;
                drawCalibrationPrompt(calibratedPotValue); // Pass the locally managed value
            }
            delay(5);
        }
        // Success message (consider moving to drawCalibrationPrompt or a new method)
        tft.fillRect(SCREEN_WIDTH/2 - 54, SCREEN_HEIGHT * 0.5 - 7, 108, 15, tft.color565(0, 100, 0)); 
        tft.setTextColor(tft.color565(255, 255, 255));
        tft.setTextDatum(MC_DATUM);
        tft.drawString("READY FOR WARP!", SCREEN_WIDTH/2, SCREEN_HEIGHT * 0.5);
        delay(1000);
    }
    potentiometerCalibrated = true;

    // Reset extern object positions (these should ideally be passed around or members)
    objectX = SCREEN_WIDTH / 2;
    objectY = SCREEN_HEIGHT / 2;
    objectScale = 1.5;

    tft.fillScreen(BG_COLOR);
    initialized = true;
    Serial.println("StoryMode initialized.");

    // Render the initial visuals for the first step
    tft.fillScreen(BG_COLOR); // Clear screen first
    createSprites(); // Create sprites *after* layout is calculated
    if (initialized) { // Check if init succeeded before drawing
        updateCurrentStepVisuals();
        // Draw the static starfield initially
        updateStars(); 
    }
}


// Old drawNarrationTextBox - to be removed or adapted if sprite fails
/*
void StoryMode::drawCurrentNarrationBox(const char* text, int scrollOffset) { 
    tft.fillRect(0, narrationBoxYPos, SCREEN_WIDTH, narrationBoxHeight, tft.color565(0, 0, 40));
    tft.drawRect(0, narrationBoxYPos, SCREEN_WIDTH, narrationBoxHeight, tft.color565(0, 100, 200));
    tft.setTextSize(1);
    tft.setTextColor(tft.color565(200, 255, 200));
    tft.setCursor(narrationBoxPadding, narrationBoxYPos + narrationBoxPadding);
    const char* displayText = text + scrollOffset;
    tft.print(displayText);
}
*/

// Old drawStoryTitle - to be removed or adapted if sprite fails
/*
void StoryMode::drawCurrentStoryTitle(const char* name) { 
    tft.fillRect(0, 0, SCREEN_WIDTH, 10, tft.color565(0, 20, 50)); // Simple top bar
    int16_t textWidth = strlen(name) * 6; // Approx width for text size 1
    int16_t x = (SCREEN_WIDTH - textWidth) / 2;
    tft.setTextSize(1);
    tft.setTextColor(tft.color565(180, 220, 255));
    tft.setCursor(x, 2);
    tft.print(name);
}
*/

void StoryMode::updateNarrationScrolling(const char* text) { // Was updateScrollingText, now for old text box
    // This function was for the old non-sprite text box. 
    // The new prepareNarrationSprite handles its own scrolling.
    // Keeping this logic here in case of fallback or if needed for a non-sprite version.
    unsigned long currentTime = millis();
    if (currentTime - lastScrollTimeNs > 1000) { // TEXT_SCROLL_SPEED was 1000
        lastScrollTimeNs = currentTime;
        int textLength = strlen(text);
        int maxOffset = std::max(0, textLength - charsPerNarrationLine);
        if (textLength > charsPerNarrationLine) {
            textScrollOffset++;
            if (textScrollOffset > maxOffset) {
                textScrollOffset = 0;
                delay(500);
            }
            // Redrawing is handled by the main render loop if using this old method
            // drawCurrentNarrationBox(text, textScrollOffset); 
        }
    }
}

void StoryMode::advanceToNextStop() { // Was advanceToNextStoryStep
    tft.fillScreen(BG_COLOR);
    textScrollOffset = 0; // For old text box system
    narrationScrollY = 0; // Reset vertical scroll for new narration sprite content
    lastNarrationScrollTime = millis(); // Reset scroll timer for narration sprite

    currentStoryStep = (currentStoryStep + 1) % TOTAL_STORY_STOPS;

    objectX = SCREEN_WIDTH / 2;
    objectY = SCREEN_HEIGHT / 2;
    objectScale = 1.5;
    
    // Reset animation states for specific objects (these should be managed by object classes or a manager)
    extern bool nebulaInitialized; nebulaInitialized = false;
    extern bool asteroidFieldInitialized; asteroidFieldInitialized = false;
    extern bool solarSystemInitialized; solarSystemInitialized = false;
    
    Serial.printf("Advanced to story stop: %d\n", currentStoryStep);
}

void StoryMode::updateCurrentStepVisuals() {
    if (!initialized) return;
    const StoryStop& currentStop = storyStopsList[currentStoryStep];
    if (!warpActive) {
        setLedModeStory();
        if (currentStoryStep != lastDisplayedStoryStep) {
            // Clear the screen or specific object area if only the object changes
            // For now, assume full redraw or that object drawing handles its own clearing
        }
        objectX = SCREEN_WIDTH / 2;
        objectY = SCREEN_HEIGHT / 2;
        
        // Adjust object position to account for larger narration box
        int titleHeight = 30;
        int narrationEffectiveYPos = narrationBoxYPos - 5;
        objectY = titleHeight + (narrationEffectiveYPos - titleHeight) / 2;

        objectScale = 1.5;
        currentStop.drawFunction();

        // --- Title Drawing ---
        tft.setTextDatum(MC_DATUM);
        tft.setTextSize(2);
        for(int i = 3; i > 0; i--) {
            tft.setTextColor(tft.color565(0, 20, 50 - i*10));
            tft.drawString(currentStop.name, SCREEN_WIDTH/2 + i, titleHeight/2 + i - 2);
        }
        tft.setTextColor(tft.color565(0, 200, 255));
        tft.drawString(currentStop.name, SCREEN_WIDTH/2, titleHeight/2 - 2);
        for(int i = 0; i < 2; i++) {
            tft.drawFastHLine(SCREEN_WIDTH/2 - 60 + i*2, titleHeight - 4 + i, 120 - i*4,
                             tft.color565(0, 160 - i*40, 200 - i*40));
        }

        // --- Narration Box with Star Wars Style Scrolling ---
        int narrBoxX = narrationBoxPadding;
        int narrBoxW = SCREEN_WIDTH - 2 * narrationBoxPadding;
        
        // Clear previous content first
        tft.fillRect(narrBoxX, narrationBoxYPos, narrBoxW, narrationBoxHeight, BG_COLOR);
        
        // Background and borders
        tft.fillRect(narrBoxX + 4, narrationBoxYPos + 4, narrBoxW - 8, narrationBoxHeight - 8, tft.color565(0, 40, 80));
        tft.drawRect(narrBoxX + 2, narrationBoxYPos + 2, narrBoxW - 4, narrationBoxHeight - 4, tft.color565(0, 160, 255));
        tft.drawRect(narrBoxX, narrationBoxYPos, narrBoxW, narrationBoxHeight, tft.color565(255, 255, 0));

        // Text rendering with Star Wars style scrolling
        tft.setTextSize(1);
        tft.setTextColor(tft.color565(220, 220, 240));
        tft.setTextWrap(true);
        tft.setTextDatum(MC_DATUM);

        // Calculate text dimensions and positioning
        int textPaddingInsideBox = 8;
        int textDisplayWidth = narrBoxW - 16 - (2 * textPaddingInsideBox); // Reduced width for better margins
        String narrationText = currentStop.narration;
        
        // Update scroll position
        unsigned long currentTime = millis();
        if (currentTime - lastNarrationScrollTime > narrationScrollPixelSpeed) {
            lastNarrationScrollTime = currentTime;
            narrationScrollY++; // Scroll upward
            // Reset scroll when text is off screen
            if (narrationScrollY > narrationBoxHeight * 2) {
                narrationScrollY = -narrationBoxHeight;
            }
        }

        // Split text into lines for centered display
        int maxCharsPerLine = textDisplayWidth / 6;
        int startChar = 0;
        int lineHeight = 14; // Increased line spacing to prevent overlap
        
        // Calculate starting Y position for text to scroll from bottom
        // Add extra padding at the bottom to prevent first line overlay
        int textStartY = narrationBoxYPos + narrationBoxHeight + lineHeight - narrationScrollY;
        
        // Vector to store all lines before drawing
        std::vector<String> lines;
        
        // First pass: split text into lines
        while(startChar < narrationText.length()) {
            int endChar = startChar + maxCharsPerLine;
            if(endChar > narrationText.length()) endChar = narrationText.length();
            
            String line = narrationText.substring(startChar, endChar);
            
            // Word wrapping
            if(endChar < narrationText.length()) {
                int lastSpace = line.lastIndexOf(' ');
                if(lastSpace > 0 && line.length() == maxCharsPerLine) {
                    line = line.substring(0, lastSpace);
                    endChar = startChar + line.length() + 1;
                }
            }
            
            lines.push_back(line);
            startChar = endChar;
            while(startChar < narrationText.length() && narrationText.charAt(startChar) == ' ') startChar++;
        }
        
        // Second pass: draw all lines with strict bounds checking
        int visibleTop = narrationBoxYPos + 8;
        int visibleBottom = narrationBoxYPos + narrationBoxHeight - 8;
        
        // Save the current text color for later restoration
        uint16_t savedColor = tft.textcolor;
        
        for(size_t i = 0; i < lines.size(); i++) {
            int currentY = textStartY + (i * lineHeight);
            
            // Only draw if line is within the visible area of the box
            if (currentY >= visibleTop && currentY <= visibleBottom) {
                // Create a temporary background-colored rectangle to clear any previous text
                tft.setTextColor(tft.color565(0, 40, 80)); // Background color
                tft.drawString("                              ", SCREEN_WIDTH/2, currentY); // Clear line with spaces
                
                // Draw the actual text
                tft.setTextColor(tft.color565(220, 220, 240)); // Text color
                tft.drawString(lines[i], SCREEN_WIDTH/2, currentY);
            }
        }
        
        // Restore the original text color
        tft.setTextColor(savedColor);

        lastDisplayedStoryStep = currentStoryStep;
    } else {
        updateWarpStars();
        lastDisplayedStoryStep = -1;
    }
}

// Modified to handle button and return bool for exit request
bool StoryMode::processInput(int potValue, bool buttonPinState_LOW) { 
    if (!initialized || !potentiometerCalibrated) return false; // No exit requested

    // Handle Exit Button Press
    if (buttonPinState_LOW && !storyExitButton_wasPressed) {
        unsigned long currentTime = millis();
        if (currentTime - storyExitButton_lastPressTime > 300) { // Debounce (300ms)
            storyExitButton_wasPressed = true; // Mark as pressed (acts like latch until release)
            storyExitButton_lastPressTime = currentTime;
            
            //Serial.println("StoryMode: Exit button pressed, calling this->exit().");
            this->exit(); // Call internal exit to deinit sprites, set initialized to false etc.
            return true;  // Signal that an exit is requested
        }
    } else if (!buttonPinState_LOW) {
        // If button is not pressed (it's HIGH), reset the wasPressed latch
        storyExitButton_wasPressed = false; 
    }

    const int WARP_THRESHOLD = 100;
    const int MAX_POT_VALUE = 4095;
    bool shouldWarp = (potValue > WARP_THRESHOLD);
    
    if (shouldWarp) {
        float potPercent = constrain(potValue, WARP_THRESHOLD, MAX_POT_VALUE);
        potPercent = (potPercent - WARP_THRESHOLD) / (float)(MAX_POT_VALUE - WARP_THRESHOLD);
        // Update the internal warpFactor
        // extern float warpFactor; // Remove this extern
        this->currentWarpFactor = 0.2f + (potPercent * 0.8f);
    } else {
        this->currentWarpFactor = 0.0f; // Set to 0 if not warping
    }

    if (shouldWarp && !previousWarpState) {
        warpActive = true;
        warpEngageTime = millis();
        tft.fillScreen(BG_COLOR); // Clear for warp effect

        // Initialize stars for warp (this logic might need to be inside updateWarpStars or a dedicated initWarp function)
        const float centerX = SCREEN_WIDTH / 2.0f;
        const float centerY = SCREEN_HEIGHT / 2.0f;
        for (int i = 0; i < STAR_COUNT; i++) { // STAR_COUNT is a #define
            float angle = random(360) * PI / 180.0f;
            float maxRadius = sqrtf(powf(SCREEN_WIDTH/2.0f, 2) + powf(SCREEN_HEIGHT/2.0f, 2));
            float distance = random(10, (int)maxRadius);
            stars[i].realX = centerX + cos(angle) * distance;
            stars[i].realY = centerY + sin(angle) * distance;
            stars[i].x = round(stars[i].realX);
            stars[i].y = round(stars[i].realY);
            stars[i].brightness = random(150, 256);
            stars[i].streakLength = 0;
        }
        Serial.println("StoryMode: Entered Warp.");
    } else if (!shouldWarp && previousWarpState) {
        if (millis() - warpEngageTime > minWarpTravelDuration) {
            warpActive = false;
            advanceToNextStop(); // This already clears the screen
            Serial.println("StoryMode: Exited Warp, advanced to next stop.");
        } else {
            warpActive = false;
            tft.fillScreen(BG_COLOR); // Clear screen to redraw current stop
            Serial.println("StoryMode: Exited Warp too soon, redrawing current stop.");
        }
    } else if (shouldWarp) {
        // Continue in warp, warpFactor updated above
    }
    previousWarpState = shouldWarp;
    return false; // No exit requested by warp logic / no button press processed this call
}

// Main update and render combined, to be called from sketch's loop when story mode is active
// Modified to handle button state and return true if exit is requested
bool StoryMode::update(int potValueFromMain, bool buttonPinState_LOW) {
    if (!initialized && !this->isActive()) { // Check if already exited by button press in a previous call this cycle
        // If not initialized (e.g. after this->exit() was called by processInput), 
        // then an exit has occurred or it was never started.
        // Return true to ensure main loop transitions out of story mode state.
        return true; 
    }
    if (!initialized) return false; // Should not happen if isActive() is true, but defensive.

    // processInput now handles the button check and can trigger exit directly.
    // if it returns true, an exit was requested and performed.
    if (processInput(potValueFromMain, buttonPinState_LOW)) { 
        return true; // Exit requested and handled by processInput (which called this->exit())
    }
    
    // If processInput did not request an exit, continue with visuals.
    // If not in warp, render current story step (object, title, narration)
    updateCurrentStepVisuals(); 

    // Background stars (twinkling) should always update if StoryMode is active and not in warp
    if (!warpActive) {
        updateStars(); // Call the extern star update function
    }
    return false; // No exit requested by warp logic / no button press processed this call
}

// The render() function can be called separately if update logic needs to run at a different rate
// For now, updateCurrentStepVisuals inside update() handles rendering.
// void StoryMode::render() {
//     if (!initialized) return;
//     updateCurrentStepVisuals(); 
// }


#endif // STORY_MODE_H 