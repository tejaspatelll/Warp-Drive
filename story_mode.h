#include <vector>
#ifndef STORY_MODE_H
#define STORY_MODE_H

#include <TFT_eSPI.h>
#include <Arduino.h> // For PI constant and other math functions
#include "sprite_manager.h"
#include "star.h"
#include "led_animations.h" // Add this include for LED control
#include "celestial_animations.h" // For easeInOutCubic function

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
extern float warpFactor; // Global warp factor used by updateWarpStars()
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
extern void drawJewelBox(); // Add Jewel Box Cluster function
extern void drawOmegaCentauri(); // Add Omega Centauri function
extern void drawOrion(); // Add Orion Nebula function
extern void drawPleiades(); // Add Pleiades function
extern void drawRing(); // Add Ring Nebula function
extern void drawDoubleCluster(); // Add Double Cluster function
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
        "At the heart of our cosmic neighborhood shines our Sun, a colossal ball of glowing gas. This magnificent star creates light and heat through nuclear fusion, turning hydrogen into helium deep within its core. Every plant that grows, every animal that moves, and every human who has ever lived depends on its steady radiance.",
        "The Sun is so massive it contains 99.86% of all the matter in our solar system!",
        drawStar
    },
    {
        "Solar System", 
        "Around our Sun orbits a family of worlds - planets, moons, asteroids, and comets. Like children around a parent, these bodies follow invisible paths shaped by gravity. Some planets are rocky like Earth, others are giant balls of gas like Jupiter. Each world tells its own unique story in our cosmic neighborhood.",
        "If the Sun were hollow, more than a million Earths could fit inside it!",
        drawSolarSystem
    },
    {
        "Planet Earth", 
        "Here is our home - the pale blue dot suspended in a sunbeam. From space, there are no borders or countries, just oceans, clouds, and land. Earth is special because it has liquid water, breathable air, and thousands of different living things. It's the only place we know of where life exists in the entire universe.",
        "Earth is the only planet not named after a god or goddess from ancient myths.",
        drawPlanet
    },
    {
        "Space Station", 
        "Circling our Earth is humanity's first home away from home. The Space Station is like a house floating in space where astronauts live and work. From this outpost, people conduct experiments, observe our planet, and learn how to live in zero gravity. It represents our first step into becoming a spacefaring species.",
        "The International Space Station zooms around Earth at about 28,000 kilometers per hour!",
        drawSpaceStation
    },
    {
        "Asteroid Field", 
        "Between Mars and Jupiter floats a ring of rocky leftovers from when our solar system formed. These asteroids are like cosmic building blocks that never became a planet. Some are tiny pebbles while others are as big as mountains. By studying them, scientists learn about the materials that built our solar system billions of years ago.",
        "If you gathered all asteroids together, they would make a world smaller than our Moon.",
        drawAsteroidField
    },
    {
        "Comet", 
        "From the cold edges of our solar system come visitors wrapped in glowing cloaks. Comets are cosmic snowballs made of ice, dust, and frozen gases. When they approach the Sun, the heat transforms some ice directly into gas, creating a beautiful tail that can stretch for millions of kilometers. These ancient travelers have remained mostly unchanged since the birth of our solar system.",
        "A comet's tail always points away from the Sun, pushed by the solar wind.",
        drawComet
    },
    {
        "Nebula", 
        "Imagine cosmic clouds stretching trillions of kilometers across space. These nebulae are where stars are born. Inside these colorful mixtures of gas and dust, gravity pulls material together until it becomes hot enough to spark the fire of a new star. When you look at a nebula, you're seeing both stellar nurseries and the beautiful remains of stars that died long ago.",
        "The word 'nebula' comes from Latin, meaning 'cloud' or 'fog'.",
        drawNebula
    },
    {
        "Binary Star", 
        "Not all stars live alone like our Sun. Many have companions, dancing with another star in an endless cosmic waltz. These binary stars orbit around each other, pulled by their mutual gravity. Sometimes they're twins of equal size, other times one is giant and the other small. Their dance can last billions of years, showing that even in the vastness of space, pairs are common.",
        "More than half of all stars in our galaxy have at least one stellar companion!",
        drawBinaryStar
    },
    {
        "Pulsar", 
        "When certain stars die, they leave behind rapidly spinning cores called pulsars. These stellar lighthouses sweep beams of energy across space like cosmic beacons. A pulsar can spin hundreds of times each second with such perfect timing that they're more accurate than our best atomic clocks. They're nature's most precise timepieces, ticking away in the darkness of space.",
        "Some pulsars rotate more than 700 times every second - faster than a kitchen blender!",
        drawPulsar
    },
    {
        "Supernova", 
        "The most dramatic moment in a star's life is its explosive death as a supernova. In just seconds, a dying star can shine brighter than billions of normal stars combined! This cosmic fireworks display spreads elements like carbon, oxygen, and iron across space - the very elements that eventually form planets and even people. We are all made of star-stuff scattered by ancient supernovas.",
        "A supernova explosion can briefly outshine an entire galaxy of 100 billion stars.",
        drawSupernova
    },
    {
        "Black Hole", 
        "In some places, gravity becomes so strong that nothing can escape - not even light itself. These are black holes, where space and time are stretched to their limits. Though invisible directly, we can detect them by how they affect nearby stars and gas. Black holes teach us that the universe is stranger and more wonderful than we ever imagined.",
        "If Earth were squeezed to the density of a black hole, it would be smaller than a cherry!",
        drawBlackHole
    },
    {
        "Galaxy", 
        "Our journey ends with a view of entire galaxies - vast islands of stars floating in the cosmic ocean. Our own Milky Way contains hundreds of billions of stars, and the universe holds trillions of galaxies, each with its own collection of stars, planets, and wonders. When we gaze at these distant star cities, we glimpse the true scale and beauty of our universe.",
        "Scientists estimate there are over 2 trillion galaxies in the observable universe.",
        drawGalaxy
    },
    {
        "Jewel Box Cluster", 
        "In the southern sky shines one of the most beautiful star clusters visible to the human eye. The Jewel Box Cluster displays a stunning array of colored stars - brilliant blues, warm yellows, and fiery oranges - all nestled together like gems in a cosmic jewelry box. This young open cluster shows us how stars of different masses and temperatures can create a dazzling celestial display.",
        "The Jewel Box Cluster contains some of the most massive and luminous stars known, shining with the power of hundreds of thousands of suns.",
        drawJewelBox
    },
    {
        "Omega Centauri", 
        "Now we encounter the crown jewel of globular clusters - Omega Centauri. This magnificent sphere contains millions of ancient stars, all bound together by gravity in a cosmic ballet that has danced for over 12 billion years. It's the largest and brightest globular cluster visible from Earth, a remnant from our galaxy's earliest days.",
        "Omega Centauri contains about 10 million stars and is over 12 billion years old.",
        drawOmegaCentauri
    },
    {
        "Orion Nebula", 
        "We now enter one of the most active star-forming regions in our local neighborhood - the Orion Nebula. This stellar nursery glows with the light of hot young stars, their radiation illuminating the surrounding gas and dust. At its heart, the Trapezium cluster of four bright blue stars powers this cosmic light show.",
        "The Orion Nebula is about 1,344 light-years away and is visible to the naked eye.",
        drawOrion
    },
    {
        "Pleiades", 
        "Here we find the Seven Sisters - the Pleiades star cluster. These hot blue stars are cosmic teenagers, only about 100 million years old. They're surrounded by wispy reflection nebulae that glow blue from the starlight reflecting off cosmic dust. This cluster has inspired myths and legends in cultures around the world.",
        "The Pleiades contains over 1,000 stars and is about 444 light-years from Earth.",
        drawPleiades
    },
    {
        "Ring Nebula", 
        "Our path leads us to witness the fate of a star like our Sun - the Ring Nebula. This planetary nebula formed when a dying star expelled its outer layers, creating this cosmic donut of glowing gas. At its center sits a white dwarf, the hot dense core of the original star, slowly cooling over billions of years.",
        "The Ring Nebula is about 2,000 light-years away and was formed about 20,000 years ago.",
        drawRing
    },
    {
        "Double Cluster", 
        "We approach a spectacular sight - the Double Cluster in Perseus. Two distinct star clusters, NGC 869 and NGC 884, appear to dance together in space. These young, hot star clusters contain hundreds of brilliant blue and white stars, creating one of the most beautiful deep-sky objects visible from Earth.",
        "The Double Cluster contains over 700 stars and is about 7,500 light-years away.",
        drawDoubleCluster
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
        currentWarpFactor(0.0f),
        scrollPauseTime(1000), // 1 second pause at start/end
        lastScrollPauseTime(0),
        isScrollPaused(true),
        calibrationScreenFirstDraw(true), // Initialize the new member
        readyMessageShown(false) // Add this for the ready message state
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
        // Reset calibration screen state
        calibrationScreenFirstDraw = true; // Add this as a class member
        // Add any other necessary cleanup for exiting story mode
        Serial.println("Exited Story Mode and cleaned up.");
    }

    float getWarpFactor() const { return currentWarpFactor; }
    int getCurrentStoryStep() const { return currentStoryStep; }
    // Expose current fact for story stops when secondary button is pressed
    const char* getCurrentFact() const { return storyStopsList[currentStoryStep].fact; }

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

    unsigned long scrollPauseTime;
    unsigned long lastScrollPauseTime;
    bool isScrollPaused;

    // New member variable for calibration screen state
    bool calibrationScreenFirstDraw;
    bool readyMessageShown; // Add this for the ready message state
};

// Define static members of the class
const StoryStop StoryMode::storyStopsList[] = {
    {
        "Our Sun", 
        "At the heart of our cosmic neighborhood shines our Sun, a colossal ball of glowing gas. This magnificent star creates light and heat through nuclear fusion, turning hydrogen into helium deep within its core. Every plant that grows, every animal that moves, and every human who has ever lived depends on its steady radiance.",
        "The Sun is so massive it contains 99.86% of all the matter in our solar system!",
        drawStar
    },
    {
        "Solar System", 
        "Around our Sun orbits a family of worlds - planets, moons, asteroids, and comets. Like children around a parent, these bodies follow invisible paths shaped by gravity. Some planets are rocky like Earth, others are giant balls of gas like Jupiter. Each world tells its own unique story in our cosmic neighborhood.",
        "If the Sun were hollow, more than a million Earths could fit inside it!",
        drawSolarSystem
    },
    {
        "Planet Earth", 
        "Here is our home - the pale blue dot suspended in a sunbeam. From space, there are no borders or countries, just oceans, clouds, and land. Earth is special because it has liquid water, breathable air, and thousands of different living things. It's the only place we know of where life exists in the entire universe.",
        "Earth is the only planet not named after a god or goddess from ancient myths.",
        drawPlanet
    },
    {
        "Space Station", 
        "Circling our Earth is humanity's first home away from home. The Space Station is like a house floating in space where astronauts live and work. From this outpost, people conduct experiments, observe our planet, and learn how to live in zero gravity. It represents our first step into becoming a spacefaring species.",
        "The International Space Station zooms around Earth at about 28,000 kilometers per hour!",
        drawSpaceStation
    },
    {
        "Asteroid Field", 
        "Between Mars and Jupiter floats a ring of rocky leftovers from when our solar system formed. These asteroids are like cosmic building blocks that never became a planet. Some are tiny pebbles while others are as big as mountains. By studying them, scientists learn about the materials that built our solar system billions of years ago.",
        "If you gathered all asteroids together, they would make a world smaller than our Moon.",
        drawAsteroidField
    },
    {
        "Comet", 
        "From the cold edges of our solar system come visitors wrapped in glowing cloaks. Comets are cosmic snowballs made of ice, dust, and frozen gases. When they approach the Sun, the heat transforms some ice directly into gas, creating a beautiful tail that can stretch for millions of kilometers. These ancient travelers have remained mostly unchanged since the birth of our solar system.",
        "A comet's tail always points away from the Sun, pushed by the solar wind.",
        drawComet
    },
    {
        "Nebula", 
        "Imagine cosmic clouds stretching trillions of kilometers across space. These nebulae are where stars are born. Inside these colorful mixtures of gas and dust, gravity pulls material together until it becomes hot enough to spark the fire of a new star. When you look at a nebula, you're seeing both stellar nurseries and the beautiful remains of stars that died long ago.",
        "The word 'nebula' comes from Latin, meaning 'cloud' or 'fog'.",
        drawNebula
    },
    {
        "Binary Star", 
        "Not all stars live alone like our Sun. Many have companions, dancing with another star in an endless cosmic waltz. These binary stars orbit around each other, pulled by their mutual gravity. Sometimes they're twins of equal size, other times one is giant and the other small. Their dance can last billions of years, showing that even in the vastness of space, pairs are common.",
        "More than half of all stars in our galaxy have at least one stellar companion!",
        drawBinaryStar
    },
    {
        "Pulsar", 
        "When certain stars die, they leave behind rapidly spinning cores called pulsars. These stellar lighthouses sweep beams of energy across space like cosmic beacons. A pulsar can spin hundreds of times each second with such perfect timing that they're more accurate than our best atomic clocks. They're nature's most precise timepieces, ticking away in the darkness of space.",
        "Some pulsars rotate more than 700 times every second - faster than a kitchen blender!",
        drawPulsar
    },
    {
        "Supernova", 
        "The most dramatic moment in a star's life is its explosive death as a supernova. In just seconds, a dying star can shine brighter than billions of normal stars combined! This cosmic fireworks display spreads elements like carbon, oxygen, and iron across space - the very elements that eventually form planets and even people. We are all made of star-stuff scattered by ancient supernovas.",
        "A supernova explosion can briefly outshine an entire galaxy of 100 billion stars.",
        drawSupernova
    },
    {
        "Black Hole", 
        "In some places, gravity becomes so strong that nothing can escape - not even light itself. These are black holes, where space and time are stretched to their limits. Though invisible directly, we can detect them by how they affect nearby stars and gas. Black holes teach us that the universe is stranger and more wonderful than we ever imagined.",
        "If Earth were squeezed to the density of a black hole, it would be smaller than a cherry!",
        drawBlackHole
    },
    {
        "Galaxy", 
        "Our journey ends with a view of entire galaxies - vast islands of stars floating in the cosmic ocean. Our own Milky Way contains hundreds of billions of stars, and the universe holds trillions of galaxies, each with its own collection of stars, planets, and wonders. When we gaze at these distant star cities, we glimpse the true scale and beauty of our universe.",
        "Scientists estimate there are over 2 trillion galaxies in the observable universe.",
        drawGalaxy
    },
    {
        "Jewel Box Cluster", 
        "In the southern sky shines one of the most beautiful star clusters visible to the human eye. The Jewel Box Cluster displays a stunning array of colored stars - brilliant blues, warm yellows, and fiery oranges - all nestled together like gems in a cosmic jewelry box. This young open cluster shows us how stars of different masses and temperatures can create a dazzling celestial display.",
        "The Jewel Box Cluster contains some of the most massive and luminous stars known, shining with the power of hundreds of thousands of suns.",
        drawJewelBox
    },
    {
        "Omega Centauri", 
        "Now we encounter the crown jewel of globular clusters - Omega Centauri. This magnificent sphere contains millions of ancient stars, all bound together by gravity in a cosmic ballet that has danced for over 12 billion years. It's the largest and brightest globular cluster visible from Earth, a remnant from our galaxy's earliest days.",
        "Omega Centauri contains about 10 million stars and is over 12 billion years old.",
        drawOmegaCentauri
    },
    {
        "Orion Nebula", 
        "We now enter one of the most active star-forming regions in our local neighborhood - the Orion Nebula. This stellar nursery glows with the light of hot young stars, their radiation illuminating the surrounding gas and dust. At its heart, the Trapezium cluster of four bright blue stars powers this cosmic light show.",
        "The Orion Nebula is about 1,344 light-years away and is visible to the naked eye.",
        drawOrion
    },
    {
        "Pleiades", 
        "Here we find the Seven Sisters - the Pleiades star cluster. These hot blue stars are cosmic teenagers, only about 100 million years old. They're surrounded by wispy reflection nebulae that glow blue from the starlight reflecting off cosmic dust. This cluster has inspired myths and legends in cultures around the world.",
        "The Pleiades contains over 1,000 stars and is about 444 light-years from Earth.",
        drawPleiades
    },
    {
        "Ring Nebula", 
        "Our path leads us to witness the fate of a star like our Sun - the Ring Nebula. This planetary nebula formed when a dying star expelled its outer layers, creating this cosmic donut of glowing gas. At its center sits a white dwarf, the hot dense core of the original star, slowly cooling over billions of years.",
        "The Ring Nebula is about 2,000 light-years away and was formed about 20,000 years ago.",
        drawRing
    },
    {
        "Double Cluster", 
        "We approach a spectacular sight - the Double Cluster in Perseus. Two distinct star clusters, NGC 869 and NGC 884, appear to dance together in space. These young, hot star clusters contain hundreds of brilliant blue and white stars, creating one of the most beautiful deep-sky objects visible from Earth.",
        "The Double Cluster contains over 700 stars and is about 7,500 light-years away.",
        drawDoubleCluster
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

void StoryMode::drawCalibrationPrompt(int potValue) {
    static int lastPotValue = -1;
    
    // Responsive layout
    int dialRadius = std::min(SCREEN_WIDTH, SCREEN_HEIGHT) / 8;
    int dialCenterX = SCREEN_WIDTH / 2;
    int dialCenterY = SCREEN_HEIGHT * 3 / 4;

    if (calibrationScreenFirstDraw) {
        // Clear screen and draw background stars
        tft.fillScreen(BG_COLOR);
        updateStars(); // Draw initial starfield

        // Draw title with shadow effect
        tft.setTextDatum(MC_DATUM);
        tft.setTextSize(2);
        // Draw shadow layers
        for(int i = 3; i > 0; i--) {
            tft.setTextColor(tft.color565(0, 60 - i*15, 120 - i*20));
            tft.drawString("PREPARE FOR", SCREEN_WIDTH/2 + i, SCREEN_HEIGHT/8 + i);
            tft.drawString("WARP TRAVEL", SCREEN_WIDTH/2 + i, SCREEN_HEIGHT/8 + 25 + i);
        }
        // Draw main text
        tft.setTextColor(tft.color565(0, 200, 255));
        tft.drawString("PREPARE FOR", SCREEN_WIDTH/2, SCREEN_HEIGHT/8);
        tft.drawString("WARP TRAVEL", SCREEN_WIDTH/2, SCREEN_HEIGHT/8 + 25);

        // Draw decorative lines
        for(int i = 0; i < 2; i++) {
            tft.drawFastHLine(SCREEN_WIDTH/2 - 80 + i*2, SCREEN_HEIGHT/8 + 45 + i, 160 - i*4, 
                             tft.color565(0, 200 - i*50, 255 - i*50));
        }

        // Draw instruction box with glow effect
        int boxW = SCREEN_WIDTH * 0.75;
        int boxH = SCREEN_HEIGHT * 0.15;
        int boxX = (SCREEN_WIDTH - boxW) / 2;
        int boxY = SCREEN_HEIGHT * 0.3;

        // Box glow effect
        for(int i = 3; i >= 0; i--) {
            tft.drawRect(boxX - i, boxY - i, boxW + i*2, boxH + i*2, 
                        tft.color565(0, 40 + i*40, 80 + i*40));
        }
        
        // Main box
        tft.fillRect(boxX, boxY, boxW, boxH, tft.color565(0, 20, 40));
        tft.drawRect(boxX, boxY, boxW, boxH, tft.color565(0, 160, 255));
        
        // Box text with glow
        tft.setTextSize(2);
        for(int i = 2; i > 0; i--) {
            tft.setTextColor(tft.color565(0, 100 + i*50, 200 + i*20));
            tft.drawString("SET DIAL TO", SCREEN_WIDTH/2 + i, boxY + boxH/3 + i);
            tft.drawString("ZERO", SCREEN_WIDTH/2 + i, boxY + boxH*2/3 + i);
        }
        tft.setTextColor(tft.color565(255, 255, 255));
        tft.drawString("SET DIAL TO", SCREEN_WIDTH/2, boxY + boxH/3);
        tft.drawString("ZERO", SCREEN_WIDTH/2, boxY + boxH*2/3);

        calibrationScreenFirstDraw = false;
        lastPotValue = -1; // Force dial redraw
    }

    // Draw the dial with dynamic updates
    if (potValue != lastPotValue) {
        // Clear previous dial
        tft.fillCircle(dialCenterX, dialCenterY, dialRadius + 6, BG_COLOR);
        
        // Draw dial base with glow
        for(int i = 2; i >= 0; i--) {
            tft.drawCircle(dialCenterX, dialCenterY, dialRadius + i, 
                          tft.color565(0, 80 + i*40, 160 + i*40));
        }
        tft.fillCircle(dialCenterX, dialCenterY, dialRadius, tft.color565(0, 20, 40));
        
        // Draw tick marks with glow
        for (int angleDeg = 0; angleDeg < 360; angleDeg += 30) {
            float rad = angleDeg * PI / 180.0f;
            int innerX = dialCenterX + cos(rad) * (dialRadius - 5);
            int innerY = dialCenterY + sin(rad) * (dialRadius - 5);
            int outerX = dialCenterX + cos(rad) * dialRadius;
            int outerY = dialCenterY + sin(rad) * dialRadius;
            // Glow effect for tick marks
            for(int i = 1; i >= 0; i--) {
                tft.drawLine(innerX + i, innerY + i, outerX + i, outerY + i, 
                            tft.color565(0, 160 + i*40, 255));
            }
        }

        // Draw zero marker with glow
        int zeroX = dialCenterX - dialRadius - 10;
        int zeroY = dialCenterY;
        // Glow effect for zero marker
        for(int i = 2; i >= 0; i--) {
            tft.fillCircle(zeroX, zeroY, 6 - i, tft.color565(0, 80 + i*60, 0));
        }
        tft.setTextColor(tft.color565(0, 255, 0));
        tft.setTextSize(1);
        tft.drawString("0", zeroX, zeroY);

        // Draw pointer with glow effect
        float angle = PI + ((potValue / 4095.0f) * (300.0f * PI / 180.0f));
        int pointerX = dialCenterX + cos(angle) * (dialRadius - 3);
        int pointerY = dialCenterY + sin(angle) * (dialRadius - 3);
        
        // Pointer glow
        for(int i = 2; i >= 0; i--) {
            tft.drawLine(dialCenterX + i, dialCenterY, pointerX + i, pointerY, 
                        tft.color565(200 + i*20, i*40, i*40));
        }
        
        // Center dot with glow
        for(int i = 3; i >= 0; i--) {
            tft.fillCircle(dialCenterX, dialCenterY, 3 - i, 
                          tft.color565(0, 160 + i*30, 255));
        }

        lastPotValue = potValue;
    }

    // Draw "Ready for Warp" message when dial is at zero
    if (potValue < 50) {
        if (!readyMessageShown) {
            // Draw success message box with glow effect
            int msgBoxW = 160;
            int msgBoxH = 30;
            int msgBoxX = (SCREEN_WIDTH - msgBoxW) / 2;
            int msgBoxY = SCREEN_HEIGHT * 0.5 - msgBoxH/2;
            
            // Box glow
            for(int i = 3; i >= 0; i--) {
                tft.drawRect(msgBoxX - i, msgBoxY - i, msgBoxW + i*2, msgBoxH + i*2, 
                            tft.color565(0, 80 + i*40, 0));
            }
            tft.fillRect(msgBoxX, msgBoxY, msgBoxW, msgBoxH, tft.color565(0, 60, 0));
            
            // Message with glow effect
            tft.setTextColor(tft.color565(200, 255, 200));
            tft.setTextSize(1);
            tft.setTextDatum(MC_DATUM);
            for(int i = 2; i > 0; i--) {
                tft.drawString("READY FOR WARP!", SCREEN_WIDTH/2 + i, SCREEN_HEIGHT * 0.5 + i);
            }
            tft.setTextColor(tft.color565(255, 255, 255));
            tft.drawString("READY FOR WARP!", SCREEN_WIDTH/2, SCREEN_HEIGHT * 0.5);
            
            readyMessageShown = true;
        }
    } else {
        readyMessageShown = false; // Reset when dial moves away from zero
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
    narrationScrollY = -narrationBoxHeight; // Start text from just below the box
    lastNarrationScrollTime = millis(); // Reset scroll timer for narration sprite
    isScrollPaused = true; // Start with a pause
    lastScrollPauseTime = millis(); // Reset pause timer

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
        int textPaddingTop = 8;
        int textDisplayWidth = narrBoxW - 16 - (2 * textPaddingInsideBox); // Reduced width for better margins
        int lineHeight = 14; // Line height for text
        int maxCharsPerLine = textDisplayWidth / 6;
        String narrationText = currentStop.narration;
        
        // Vector to store all lines before drawing
        std::vector<String> lines;
        
        // First pass: split text into lines
        int startChar = 0;
        while(startChar < (int)narrationText.length()) {
            int endChar = startChar + maxCharsPerLine;
            if(endChar > (int)narrationText.length()) endChar = narrationText.length();
            
            String line = narrationText.substring(startChar, endChar);
            
            // Word wrapping
            if(endChar < (int)narrationText.length()) {
                int lastSpace = line.lastIndexOf(' ');
                if(lastSpace > 0 && (int)line.length() == maxCharsPerLine) {
                    line = line.substring(0, lastSpace);
                    endChar = startChar + line.length() + 1;
                }
            }
            
            lines.push_back(line);
            startChar = endChar;
            while(startChar < (int)narrationText.length() && narrationText.charAt(startChar) == ' ') startChar++;
        }
        
        // Update scroll position with pauses
        unsigned long currentTime = millis();
        
        // Calculate total text height
        int totalTextHeight = lines.size() * lineHeight;
        
        // Adjust initial scroll position to start at the bottom edge of the box
        // if a new step has just started.
        if (currentStoryStep != lastDisplayedStoryStep) {
             narrationScrollY = 0; // Start the scroll offset at 0 from the bottom edge
             isScrollPaused = true; // Start with a pause
             lastScrollPauseTime = currentTime; // Reset pause timer
        }

        // Handle scroll pauses
        if (isScrollPaused) {
            if (currentTime - lastScrollPauseTime > scrollPauseTime) {
                isScrollPaused = false;
                lastNarrationScrollTime = currentTime; // Reset scroll timer when pause ends
            }
        } else if (currentTime - lastNarrationScrollTime > narrationScrollPixelSpeed) {
            lastNarrationScrollTime = currentTime;
            narrationScrollY++; // Scroll upward
            
            // Check if we need to pause at the end or loop
            if (narrationScrollY >= totalTextHeight + narrationBoxHeight) { // Scroll until the last line has exited the top of the box
                 narrationScrollY = 0; // Reset to start scrolling again from the bottom
                 isScrollPaused = true; // Pause before restarting
                 lastScrollPauseTime = currentTime; // Reset pause timer
            }
        }

        // Calculate starting Y position for text to scroll from bottom edge of the box
        int textStartY = narrationBoxYPos + narrationBoxHeight - narrationScrollY;
        
        // Second pass: draw all lines with strict bounds checking
        int visibleTop = narrationBoxYPos + textPaddingTop; // Top boundary for drawing text
        int visibleBottom = narrationBoxYPos + narrationBoxHeight - textPaddingInsideBox; // Bottom boundary for drawing text
        
        // Save the current text color for later restoration
        uint16_t savedColor = tft.textcolor;
        
        // Clear the text area before drawing new lines
        tft.fillRect(narrBoxX + textPaddingInsideBox, visibleTop, 
                     textDisplayWidth + 16, narrationBoxHeight - textPaddingTop - textPaddingInsideBox, 
                     tft.color565(0, 40, 80)); // Use the narration box fill color for clearing

        for(size_t i = 0; i < lines.size(); i++) {
            int currentLineBaseY = textStartY + (i * lineHeight);
            int currentLineCenterY = currentLineBaseY + lineHeight/2; // For MC_DATUM
            
            // Only draw if the center of the line is within the visible area of the box
            if (currentLineCenterY >= visibleTop && currentLineCenterY <= visibleBottom) {
                 tft.setTextColor(tft.color565(220, 220, 240)); // Text color
                 tft.drawString(lines[i], SCREEN_WIDTH/2, currentLineCenterY); // Draw centered
            }
        }
        
        // Restore the original text color
        tft.setTextColor(savedColor);

        lastDisplayedStoryStep = currentStoryStep; // Mark this step as displayed
    } else {
        updateWarpStars();
        // Reset lastDisplayedStoryStep when entering warp, so the next stop visual is prepared fully
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
    
    // Calculate warpFactor the same way as discovery mode (smooth from 0.0 to 1.0)
    // Scale from 0-4095 to 0-1.0 for 12-bit ADC, then apply easing
    float rawWarpFactor = static_cast<float>(potValue) / static_cast<float>(MAX_POT_VALUE);
    float calculatedWarpFactor = easeInOutCubic(rawWarpFactor);
    
    // Update both the global warpFactor (used by updateWarpStars) and currentWarpFactor (used by getWarpFactor)
    warpFactor = calculatedWarpFactor;
    this->currentWarpFactor = calculatedWarpFactor;

    if (shouldWarp && !previousWarpState) {
        warpActive = true;
        warpEngageTime = millis();
        tft.fillScreen(BG_COLOR); // Clear for warp effect

        // Stars will continue from their current positions (same as discovery mode)
        // updateWarpStars() will handle their radial movement.
        
        // Clear any streak history for all stars, as they are entering warp
        for (int i = 0; i < STAR_COUNT; i++) {
            stars[i].streakLength = 0; // Reset streak length
            for (int j = 0; j <= MAX_STREAK_LENGTH; j++) {
                prevX[i][j] = SCREEN_WIDTH;  // Mark as invalid/off-screen
                prevY[i][j] = SCREEN_HEIGHT; // Mark as invalid/off-screen
            }
        }
        Serial.println("StoryMode: Entered Warp.");
    } else if (!shouldWarp && previousWarpState) {
        // Clear warp streak buffers to prevent ghost artifacts (same as discovery mode)
        for (int i = 0; i < STAR_COUNT; i++) {
            stars[i].streakLength = 0; // Reset streak length
            for (int j = 0; j <= MAX_STREAK_LENGTH; j++) {
                prevX[i][j] = SCREEN_WIDTH;  // Mark as invalid/off-screen
                prevY[i][j] = SCREEN_HEIGHT; // Mark as invalid/off-screen
            }
        }
        
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