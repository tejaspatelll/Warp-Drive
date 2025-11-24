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
#define POT_PIN 7 // Default value if not defined elsewhere
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
extern void drawJewelBox();                    // Add Jewel Box Cluster function
extern void drawOmegaCentauri();               // Add Omega Centauri function
extern void drawOrion();                       // Add Orion Nebula function
extern void drawPleiades();                    // Add Pleiades function
extern void drawRing();                        // Add Ring Nebula function
extern void drawDoubleCluster();               // Add Double Cluster function
extern void updateStars();                     // For background star twinkling
extern void updateWarpStars();                 // For warp effect
extern void updateWarpSound(float warpFactor); // For warp sound effect
extern void setLedModeWarp();                  // For LED warp mode
extern void initWarpBuffer();                  // Initialize warp buffer
extern void cleanupWarpBuffer();               // Cleanup warp buffer
extern float easeInOutCubic(float t);          // Easing function for warp factor
extern float warpFactor;                       // Global warp factor used by updateWarpStars()
extern unsigned long lastWarpFrame;            // Frame timer for warp animation
extern uint16_t *warpBuffer;                   // Warp buffer for double buffering
extern bool bufferInitialized;                 // Buffer initialization flag

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
struct StoryStop
{
    const char *name;       // Name of the celestial object
    const char *narration;  // Carl Sagan-inspired narration text
    const char *fact;       // Educational fact or question (stored but not shown by default)
    void (*drawFunction)(); // Function pointer to draw the celestial object
};

// Story stops data is defined as a static class member below (StoryMode::storyStopsList)

class StoryMode
{
public:
    StoryMode() : storyTitleHandle({0}),
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
                  minWarpTravelDuration(800), // Reduced from 1500ms to 800ms for faster transitions
                  narrationBoxHeight(0),
                  narrationBoxYPos(0),
                  narrationBoxPadding(0),
                  charsPerNarrationLine(0),
                  lastDisplayedStoryStep(-1),
                  narrationScrollY(0),
                  lastNarrationScrollTime(0),
                  narrationScrollPixelSpeed(30), // Reduced from 50ms to 30ms per pixel for faster scrolling
                  currentWarpFactor(0.0f),
                  scrollPauseTime(500), // Reduced from 1000ms to 500ms pause at start/end
                  lastScrollPauseTime(0),
                  isScrollPaused(true),
                  calibrationScreenFirstDraw(true), // Initialize the new member
                  readyMessageShown(false),         // Add this for the ready message state
                  viewportSize(0),
                  viewportX(0),
                  viewportY(0),
                  titleBarHeight(0),
                  uiFrameDrawn(false),
                  warpUIFrameDrawn(false),
                  uiRedrawFrameCounter(0)
    {
        // Constructor body (if needed for more complex init)
    }

    ~StoryMode()
    {
        deinitSprites(); // Ensure sprites are cleaned up
    }

    void init();
    bool update(int potValue, bool buttonPinState_LOW);
    bool processInput(int potValue, bool buttonPinState_LOW);
    void render();                                // New function to separate update logic from drawing
    bool isActive() const { return initialized; } // Simple check for now
    bool isWarping() const { return warpActive; } // Check if currently in warp mode
    void exit()
    {
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
    const char *getCurrentFact() const { return storyStopsList[currentStoryStep].fact; }
    // Get viewport dimensions for constraining stars/objects
    int getViewportX() const { return viewportX; }
    int getViewportY() const { return viewportY; }
    int getViewportSize() const { return viewportSize; }
    int getTitleBarHeight() const { return titleBarHeight; }
    // Get the narration box Y position and height for preventing star overlap
    int getNarrationBoxYPos() const { return narrationBoxYPos; }
    int getNarrationBoxHeight() const { return narrationBoxHeight; }

    // Story sequence - public static const members (accessible by quiz_mode.h)
    static const StoryStop storyStopsList[];
    static const int TOTAL_STORY_STOPS;

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

    // Text display constants
    int narrationBoxHeight;
    int narrationBoxYPos;
    int narrationBoxPadding;
    int charsPerNarrationLine;

    // Viewport constants for spaceship console layout
    int viewportSize;         // Square viewport dimension
    int viewportX;            // Viewport X position
    int viewportY;            // Viewport Y position
    int titleBarHeight;       // Height of title bar at top
    bool uiFrameDrawn;        // Track if permanent UI frame is drawn
    bool warpUIFrameDrawn;    // Track if warp UI frame has been drawn
    int uiRedrawFrameCounter; // Counter to redraw UI every 5-10 frames

    // Static variable for tracking story step changes
    int lastDisplayedStoryStep;

    // Vertical scrolling text state for narration sprite
    int narrationScrollY;
    unsigned long lastNarrationScrollTime;
    int narrationScrollPixelSpeed;

    // Pre-calculated narration text lines for performance
    std::vector<String> currentNarrationLines;
    int currentNarrationTotalHeight;

    // Layout and drawing helper methods
    void setupLayout();
    void drawPermanentUIFrame(); // Draw the spaceship console frame once
    void drawCalibrationPrompt(int potValue);
    void prepareTitleSprite(const char *name);
    void prepareNarrationSprite(const char *text);
    void updateNarrationScrolling(const char *text);
    void advanceToNextStop();
    void updateCurrentStepVisuals();
    void precalculateNarrationLines(); // Pre-calculate text wrapping for performance

    unsigned long scrollPauseTime;
    unsigned long lastScrollPauseTime;
    bool isScrollPaused;

    // New member variable for calibration screen state
    bool calibrationScreenFirstDraw;
    bool readyMessageShown; // Add this for the ready message state
};

// Define static members of the class
const StoryStop StoryMode::storyStopsList[] = {
    {"Our Sun",
     "At the heart of our cosmic neighborhood shines our Sun, a colossal ball of glowing gas. This magnificent star creates light and heat through nuclear fusion, turning hydrogen into helium deep within its core. Every plant that grows, every animal that moves, and every human who has ever lived depends on its steady radiance.",
     "The Sun is so massive it contains 99.86% of all the matter in our solar system!",
     drawStar},
    {"Solar System",
     "Around our Sun orbits a family of worlds - planets, moons, asteroids, and comets. Like children around a parent, these bodies follow invisible paths shaped by gravity. Some planets are rocky like Earth, others are giant balls of gas like Jupiter. Each world tells its own unique story in our cosmic neighborhood.",
     "If the Sun were hollow, more than a million Earths could fit inside it!",
     drawSolarSystem},
    {"Planet Earth",
     "Here is our home - the pale blue dot suspended in a sunbeam. From space, there are no borders or countries, just oceans, clouds, and land. Earth is special because it has liquid water, breathable air, and thousands of different living things. It's the only place we know of where life exists in the entire universe.",
     "Earth is the only planet not named after a god or goddess from ancient myths.",
     drawPlanet},
    {"Space Station",
     "Circling our Earth is humanity's first home away from home. The Space Station is like a house floating in space where astronauts live and work. From this outpost, people conduct experiments, observe our planet, and learn how to live in zero gravity. It represents our first step into becoming a spacefaring species.",
     "The International Space Station zooms around Earth at about 28,000 kilometers per hour!",
     drawSpaceStation},
    {"Asteroid Field",
     "Between Mars and Jupiter floats a ring of rocky leftovers from when our solar system formed. These asteroids are like cosmic building blocks that never became a planet. Some are tiny pebbles while others are as big as mountains. By studying them, scientists learn about the materials that built our solar system billions of years ago.",
     "If you gathered all asteroids together, they would make a world smaller than our Moon.",
     drawAsteroidField},
    {"Comet",
     "From the cold edges of our solar system come visitors wrapped in glowing cloaks. Comets are cosmic snowballs made of ice, dust, and frozen gases. When they approach the Sun, the heat transforms some ice directly into gas, creating a beautiful tail that can stretch for millions of kilometers. These ancient travelers have remained mostly unchanged since the birth of our solar system.",
     "A comet's tail always points away from the Sun, pushed by the solar wind.",
     drawComet},
    {"Nebula",
     "Imagine cosmic clouds stretching trillions of kilometers across space. These nebulae are where stars are born. Inside these colorful mixtures of gas and dust, gravity pulls material together until it becomes hot enough to spark the fire of a new star. When you look at a nebula, you're seeing both stellar nurseries and the beautiful remains of stars that died long ago.",
     "The word 'nebula' comes from Latin, meaning 'cloud' or 'fog'.",
     drawNebula},
    {"Binary Star",
     "Not all stars live alone like our Sun. Many have companions, dancing with another star in an endless cosmic waltz. These binary stars orbit around each other, pulled by their mutual gravity. Sometimes they're twins of equal size, other times one is giant and the other small. Their dance can last billions of years, showing that even in the vastness of space, pairs are common.",
     "More than half of all stars in our galaxy have at least one stellar companion!",
     drawBinaryStar},
    {"Pulsar",
     "When certain stars die, they leave behind rapidly spinning cores called pulsars. These stellar lighthouses sweep beams of energy across space like cosmic beacons. A pulsar can spin hundreds of times each second with such perfect timing that they're more accurate than our best atomic clocks. They're nature's most precise timepieces, ticking away in the darkness of space.",
     "Some pulsars rotate more than 700 times every second - faster than a kitchen blender!",
     drawPulsar},
    {"Supernova",
     "The most dramatic moment in a star's life is its explosive death as a supernova. In just seconds, a dying star can shine brighter than billions of normal stars combined! This cosmic fireworks display spreads elements like carbon, oxygen, and iron across space - the very elements that eventually form planets and even people. We are all made of star-stuff scattered by ancient supernovas.",
     "A supernova explosion can briefly outshine an entire galaxy of 100 billion stars.",
     drawSupernova},
    {"Black Hole",
     "In some places, gravity becomes so strong that nothing can escape - not even light itself. These are black holes, where space and time are stretched to their limits. Though invisible directly, we can detect them by how they affect nearby stars and gas. Black holes teach us that the universe is stranger and more wonderful than we ever imagined.",
     "If Earth were squeezed to the density of a black hole, it would be smaller than a cherry!",
     drawBlackHole},
    {"Galaxy",
     "Our journey ends with a view of entire galaxies - vast islands of stars floating in the cosmic ocean. Our own Milky Way contains hundreds of billions of stars, and the universe holds trillions of galaxies, each with its own collection of stars, planets, and wonders. When we gaze at these distant star cities, we glimpse the true scale and beauty of our universe.",
     "Scientists estimate there are over 2 trillion galaxies in the observable universe.",
     drawGalaxy},
    {"Jewel Box Cluster",
     "In the southern sky shines one of the most beautiful star clusters visible to the human eye. The Jewel Box Cluster displays a stunning array of colored stars - brilliant blues, warm yellows, and fiery oranges - all nestled together like gems in a cosmic jewelry box. This young open cluster shows us how stars of different masses and temperatures can create a dazzling celestial display.",
     "The Jewel Box Cluster contains some of the most massive and luminous stars known, shining with the power of hundreds of thousands of suns.",
     drawJewelBox},
    {"Omega Centauri",
     "Now we encounter the crown jewel of globular clusters - Omega Centauri. This magnificent sphere contains millions of ancient stars, all bound together by gravity in a cosmic ballet that has danced for over 12 billion years. It's the largest and brightest globular cluster visible from Earth, a remnant from our galaxy's earliest days.",
     "Omega Centauri contains about 10 million stars and is over 12 billion years old.",
     drawOmegaCentauri},
    {"Orion Nebula",
     "We now enter one of the most active star-forming regions in our local neighborhood - the Orion Nebula. This stellar nursery glows with the light of hot young stars, their radiation illuminating the surrounding gas and dust. At its heart, the Trapezium cluster of four bright blue stars powers this cosmic light show.",
     "The Orion Nebula is about 1,344 light-years away and is visible to the naked eye.",
     drawOrion},
    {"Pleiades",
     "Here we find the Seven Sisters - the Pleiades star cluster. These hot blue stars are cosmic teenagers, only about 100 million years old. They're surrounded by wispy reflection nebulae that glow blue from the starlight reflecting off cosmic dust. This cluster has inspired myths and legends in cultures around the world.",
     "The Pleiades contains over 1,000 stars and is about 444 light-years from Earth.",
     drawPleiades},
    {"Ring Nebula",
     "Our path leads us to witness the fate of a star like our Sun - the Ring Nebula. This planetary nebula formed when a dying star expelled its outer layers, creating this cosmic donut of glowing gas. At its center sits a white dwarf, the hot dense core of the original star, slowly cooling over billions of years.",
     "The Ring Nebula is about 2,000 light-years away and was formed about 20,000 years ago.",
     drawRing},
    {"Double Cluster",
     "We approach a spectacular sight - the Double Cluster in Perseus. Two distinct star clusters, NGC 869 and NGC 884, appear to dance together in space. These young, hot star clusters contain hundreds of brilliant blue and white stars, creating one of the most beautiful deep-sky objects visible from Earth.",
     "The Double Cluster contains over 700 stars and is about 7,500 light-years away.",
     drawDoubleCluster}};
const int StoryMode::TOTAL_STORY_STOPS = sizeof(StoryMode::storyStopsList) / sizeof(StoryMode::storyStopsList[0]);

// Global references for backward compatibility with quiz_mode.h
// These reference the class static members defined above
const StoryStop *const STORY_STOPS_DATA = StoryMode::storyStopsList;
const int STORY_STOPS_DATA_COUNT = StoryMode::TOTAL_STORY_STOPS;

// --- Member function definitions for StoryMode class ---

// NOTE: SpriteManager::begin() must be called in setup() before using StoryMode!

void StoryMode::deinitSprites()
{
    // No-op: UI elements drawn directly, no sprite cleanup needed
}

void StoryMode::setupLayout()
{ // Was setupStoryModeLayout
    // Spaceship console layout with permanent UI frames
    titleBarHeight = 30;                    // Fixed height for title bar at top
    narrationBoxHeight = SCREEN_HEIGHT / 4; // Fixed height for narration box at bottom
    narrationBoxYPos = SCREEN_HEIGHT - narrationBoxHeight;
    narrationBoxPadding = SCREEN_WIDTH / 40;
    charsPerNarrationLine = (SCREEN_WIDTH - 2 * narrationBoxPadding) / 6; // Assuming char width of 6

    // Calculate square viewport in the center
    int availableHeight = SCREEN_HEIGHT - titleBarHeight - narrationBoxHeight;
    int availableWidth = SCREEN_WIDTH;

    // Make it a perfect square - use the smaller dimension
    viewportSize = std::min(availableWidth, availableHeight) - 4; // -4 for border padding

    // Center the viewport
    viewportX = (SCREEN_WIDTH - viewportSize) / 2;
    viewportY = titleBarHeight + (availableHeight - viewportSize) / 2;

    uiFrameDrawn = false;     // Reset frame drawn flag
    uiRedrawFrameCounter = 0; // Reset frame counter
}

void StoryMode::drawPermanentUIFrame()
{
    // Redraw UI every 5-10 frames to prevent celestial objects from overwriting it
    const int UI_REDRAW_INTERVAL = 7; // Redraw every 7 frames (middle of 5-10 range)

    // Allow redraw if frame counter reaches interval, or if UI hasn't been drawn yet
    if (uiFrameDrawn && (uiRedrawFrameCounter % UI_REDRAW_INTERVAL != 0))
        return; // Skip redraw if not at interval

    // === ADVANCED SPACESHIP CONSOLE INTERFACE ===
    // Note: fillScreen() removed to prevent flicker
    // Note: viewport fillRect() removed - viewport is only cleared when story step changes
    // or when entering/exiting warp mode, not during UI frame redraws

    // === TITLE BAR === Angular design with beveled corners
    int cornerCut = 10; // Diagonal corner cutout

    // Gradient background with angular cuts
    for (int y = 0; y < titleBarHeight; y++)
    {
        int brightness = 8 + (y * 38) / titleBarHeight;
        int leftStart = (y < cornerCut) ? cornerCut - y : 0;
        int rightEnd = (y < cornerCut) ? SCREEN_WIDTH - (cornerCut - y) : SCREEN_WIDTH;
        tft.drawFastHLine(leftStart, y, rightEnd - leftStart, tft.color565(0, brightness, brightness * 2.3));
    }

    // Angular multi-layer borders with depth
    for (int i = 0; i < 2; i++)
    {
        uint16_t color = tft.color565(0, 230 + i * 10, 255 - i * 5);
        // Top border with angular cuts
        for (int x = cornerCut; x < SCREEN_WIDTH - cornerCut; x++)
        {
            tft.drawPixel(x, i, color);
        }
        // Bottom border
        tft.drawFastHLine(0, titleBarHeight - 1 - i, SCREEN_WIDTH, color);
        // Diagonal corner edges
        for (int j = 0; j < cornerCut - i; j++)
        {
            tft.drawPixel(j + i, cornerCut - j, color);
            tft.drawPixel(SCREEN_WIDTH - 1 - j - i, cornerCut - j, color);
        }
    }

    // Dynamic hexagonal status indicators
    for (int i = 0; i < 4; i++)
    {
        int x = 2 + i * 7;
        int h = titleBarHeight - 12;
        uint16_t barColor = tft.color565(0, 190 + i * 12, 240 + i * 3);
        // Hexagonal shape
        tft.fillRect(x, 6, 4, h, barColor);
        tft.drawPixel(x + 1, 5, barColor);
        tft.drawPixel(x + 2, 5, barColor);
        tft.drawPixel(x + 1, 6 + h, barColor);
        tft.drawPixel(x + 2, 6 + h, barColor);
        // Mirror on right
        int rx = SCREEN_WIDTH - x - 4;
        tft.fillRect(rx, 6, 4, h, barColor);
        tft.drawPixel(rx + 1, 5, barColor);
        tft.drawPixel(rx + 2, 5, barColor);
        tft.drawPixel(rx + 1, 6 + h, barColor);
        tft.drawPixel(rx + 2, 6 + h, barColor);
    }

    // === VIEWPORT FRAME === Hexagonal/octagonal scanner display
    // Viewport already cleared above (line 422)

    // Octagonal frame with angled corners
    int vpCornerCut = 12; // Corner bevel size

    // Multi-layer octagonal borders with cyan glow
    for (int layer = 0; layer < 5; layer++)
    {
        uint16_t glowColor;
        if (layer < 2)
            glowColor = tft.color565(0, 255, 255); // Bright cyan
        else if (layer < 4)
            glowColor = tft.color565(0, 210, 255); // Medium cyan
        else
            glowColor = tft.color565(0, 170, 230); // Deep cyan

        int offset = 7 - layer;
        int x1 = viewportX - offset;
        int y1 = viewportY - offset;
        int size = viewportSize + offset * 2;

        // Octagonal edges (8 segments)
        // Top edge
        tft.drawFastHLine(x1 + vpCornerCut, y1, size - vpCornerCut * 2, glowColor);
        // Bottom edge
        tft.drawFastHLine(x1 + vpCornerCut, y1 + size - 1, size - vpCornerCut * 2, glowColor);
        // Left edge
        tft.drawFastVLine(x1, y1 + vpCornerCut, size - vpCornerCut * 2, glowColor);
        // Right edge
        tft.drawFastVLine(x1 + size - 1, y1 + vpCornerCut, size - vpCornerCut * 2, glowColor);

        // Diagonal corners (octagonal bevels)
        for (int i = 0; i < vpCornerCut; i++)
        {
            // Top-left
            tft.drawPixel(x1 + i, y1 + vpCornerCut - i, glowColor);
            // Top-right
            tft.drawPixel(x1 + size - 1 - i, y1 + vpCornerCut - i, glowColor);
            // Bottom-left
            tft.drawPixel(x1 + i, y1 + size - 1 - vpCornerCut + i, glowColor);
            // Bottom-right
            tft.drawPixel(x1 + size - 1 - i, y1 + size - 1 - vpCornerCut + i, glowColor);
        }
    }

    // Angular corner indicators with depth
    int bracketLen = 22;
    for (int corner = 0; corner < 4; corner++)
    {
        int cx = (corner % 2 == 0) ? viewportX - 10 : viewportX + viewportSize + 7;
        int cy = (corner < 2) ? viewportY - 10 : viewportY + viewportSize + 7;
        int xDir = (corner % 2 == 0) ? 1 : -1;
        int yDir = (corner < 2) ? 1 : -1;

        // Multi-layer angular brackets
        for (int t = 0; t < 3; t++)
        {
            uint16_t color = (t == 0) ? tft.color565(0, 255, 255) : (t == 1) ? tft.color565(0, 230, 255)
                                                                             : tft.color565(0, 200, 240);
            int len = bracketLen - t * 3;

            // Main L-bracket
            for (int i = 0; i < len; i++)
            {
                tft.drawPixel(cx + i * xDir, cy + t * yDir, color);
                tft.drawPixel(cx + t * xDir, cy + i * yDir, color);
            }

            // Angular accent
            for (int i = 0; i < 5; i++)
            {
                tft.drawPixel(cx + (len - i - 1) * xDir, cy + (i + t) * yDir, color);
                tft.drawPixel(cx + (i + t) * xDir, cy + (len - i - 1) * yDir, color);
            }
        }
    }

    // Viewport labels with scan line effect
    tft.setTextSize(1);
    tft.setTextColor(tft.color565(0, 255, 255), tft.color565(2, 4, 10));
    tft.setTextDatum(TL_DATUM);
    tft.drawString("<<SCANNER>>", viewportX - 8, viewportY - 25);
    tft.setTextDatum(TR_DATUM);
    tft.drawString("<<DISPLAY>>", viewportX + viewportSize + 8, viewportY - 25);

    // === NARRATION BOX === Angular console panel with beveled edges
    int narrBoxX = narrationBoxPadding;
    int narrBoxW = SCREEN_WIDTH - 2 * narrationBoxPadding;
    int nBoxCornerCut = 8; // Beveled corner size

    // Gradient background with subtle scan lines
    for (int y = 0; y < narrationBoxHeight - 8; y++)
    {
        int brightness = 8 + (y * 28) / (narrationBoxHeight - 8);
        // Add subtle scan line effect (every 4th line slightly brighter)
        if (y % 4 == 0)
            brightness += 3;
        tft.drawFastHLine(narrBoxX + 4, narrationBoxYPos + 4 + y, narrBoxW - 8,
                          tft.color565(0, brightness, brightness * 2.2));
    }

    // Multi-layer beveled borders
    for (int layer = 0; layer < 3; layer++)
    {
        uint16_t borderColor = tft.color565(0, 210 + layer * 15, 245 + layer * 3);
        int x = narrBoxX + layer;
        int y = narrationBoxYPos + layer;
        int w = narrBoxW - layer * 2;
        int h = narrationBoxHeight - layer * 2;

        // Top edge (with beveled corners)
        tft.drawFastHLine(x + nBoxCornerCut, y, w - nBoxCornerCut * 2, borderColor);
        // Bottom edge
        tft.drawFastHLine(x + nBoxCornerCut, y + h - 1, w - nBoxCornerCut * 2, borderColor);
        // Left edge
        tft.drawFastVLine(x, y + nBoxCornerCut, h - nBoxCornerCut * 2, borderColor);
        // Right edge
        tft.drawFastVLine(x + w - 1, y + nBoxCornerCut, h - nBoxCornerCut * 2, borderColor);

        // Beveled corners
        for (int i = 0; i < nBoxCornerCut; i++)
        {
            // Top corners
            tft.drawPixel(x + i, y + nBoxCornerCut - i, borderColor);
            tft.drawPixel(x + w - 1 - i, y + nBoxCornerCut - i, borderColor);
            // Bottom corners
            tft.drawPixel(x + i, y + h - 1 - nBoxCornerCut + i, borderColor);
            tft.drawPixel(x + w - 1 - i, y + h - 1 - nBoxCornerCut + i, borderColor);
        }
    }

    // Top glow accent with scan effect
    for (int i = 0; i < 5; i++)
    {
        uint16_t glowColor = tft.color565(0, 250 - i * 35, 255 - i * 25);
        tft.drawFastHLine(narrBoxX, narrationBoxYPos - i - 1, narrBoxW, glowColor);
    }

    // Angular corner indicators
    int nCornerLen = 15;
    for (int side = 0; side < 2; side++)
    {
        int x = (side == 0) ? narrBoxX : narrBoxX + narrBoxW - 1;
        int xDir = (side == 0) ? 1 : -1;

        for (int layer = 0; layer < 2; layer++)
        {
            uint16_t cornerColor = tft.color565(0, 255 - layer * 30, 255 - layer * 15);
            // Top indicator
            for (int i = 0; i < nCornerLen - layer * 3; i++)
            {
                tft.drawPixel(x + i * xDir, narrationBoxYPos + layer, cornerColor);
                if (i < 6)
                    tft.drawPixel(x + layer * xDir, narrationBoxYPos + i, cornerColor);
            }
            // Angular accent
            for (int i = 0; i < 4; i++)
            {
                tft.drawPixel(x + (nCornerLen - i - 2) * xDir, narrationBoxYPos + i + layer + 1, cornerColor);
            }
        }
    }

    // Data stream labels with brackets
    tft.setTextSize(1);
    tft.setTextColor(tft.color565(0, 255, 255), tft.color565(2, 4, 10));
    tft.setTextDatum(TL_DATUM);
    tft.drawString("<<DATA STREAM>>", narrBoxX, narrationBoxYPos - 20);

    // Status indicators
    for (int i = 0; i < 3; i++)
    {
        int dotX = narrBoxX + narrBoxW - 15 - i * 6;
        tft.fillCircle(dotX, narrationBoxYPos - 18, 2, tft.color565(0, 220 + i * 10, 250 + i * 2));
    }

    uiFrameDrawn = true;
}

void StoryMode::drawCalibrationPrompt(int potValue)
{
    static int lastPotValue = -1;

    // Responsive layout
    int dialRadius = std::min(SCREEN_WIDTH, SCREEN_HEIGHT) / 8;
    int dialCenterX = SCREEN_WIDTH / 2;
    int dialCenterY = SCREEN_HEIGHT * 3 / 4;

    if (calibrationScreenFirstDraw)
    {
        // Clear screen and draw background stars
        tft.fillScreen(BG_COLOR);
        updateStars(); // Draw initial starfield

        // Draw title with shadow effect
        tft.setTextDatum(MC_DATUM);
        tft.setTextSize(2);
        // Draw shadow layers
        for (int i = 3; i > 0; i--)
        {
            tft.setTextColor(tft.color565(0, 60 - i * 15, 120 - i * 20));
            tft.drawString("PREPARE FOR", SCREEN_WIDTH / 2 + i, SCREEN_HEIGHT / 8 + i);
            tft.drawString("WARP TRAVEL", SCREEN_WIDTH / 2 + i, SCREEN_HEIGHT / 8 + 25 + i);
        }
        // Draw main text
        tft.setTextColor(tft.color565(0, 200, 255));
        tft.drawString("PREPARE FOR", SCREEN_WIDTH / 2, SCREEN_HEIGHT / 8);
        tft.drawString("WARP TRAVEL", SCREEN_WIDTH / 2, SCREEN_HEIGHT / 8 + 25);

        // Draw decorative lines
        for (int i = 0; i < 2; i++)
        {
            tft.drawFastHLine(SCREEN_WIDTH / 2 - 80 + i * 2, SCREEN_HEIGHT / 8 + 45 + i, 160 - i * 4,
                              tft.color565(0, 200 - i * 50, 255 - i * 50));
        }

        // Draw instruction box with glow effect
        int boxW = SCREEN_WIDTH * 0.75;
        int boxH = SCREEN_HEIGHT * 0.15;
        int boxX = (SCREEN_WIDTH - boxW) / 2;
        int boxY = SCREEN_HEIGHT * 0.3;

        // Box glow effect
        for (int i = 3; i >= 0; i--)
        {
            tft.drawRect(boxX - i, boxY - i, boxW + i * 2, boxH + i * 2,
                         tft.color565(0, 40 + i * 40, 80 + i * 40));
        }

        // Main box
        tft.fillRect(boxX, boxY, boxW, boxH, tft.color565(0, 20, 40));
        tft.drawRect(boxX, boxY, boxW, boxH, tft.color565(0, 160, 255));

        // Box text with glow
        tft.setTextSize(2);
        for (int i = 2; i > 0; i--)
        {
            tft.setTextColor(tft.color565(0, 100 + i * 50, 200 + i * 20));
            tft.drawString("SET DIAL TO", SCREEN_WIDTH / 2 + i, boxY + boxH / 3 + i);
            tft.drawString("ZERO", SCREEN_WIDTH / 2 + i, boxY + boxH * 2 / 3 + i);
        }
        tft.setTextColor(tft.color565(255, 255, 255));
        tft.drawString("SET DIAL TO", SCREEN_WIDTH / 2, boxY + boxH / 3);
        tft.drawString("ZERO", SCREEN_WIDTH / 2, boxY + boxH * 2 / 3);

        calibrationScreenFirstDraw = false;
        lastPotValue = -1; // Force dial redraw
    }

    // Draw the dial with dynamic updates
    if (potValue != lastPotValue)
    {
        // Clear previous dial
        tft.fillCircle(dialCenterX, dialCenterY, dialRadius + 6, BG_COLOR);

        // Draw dial base with glow
        for (int i = 2; i >= 0; i--)
        {
            tft.drawCircle(dialCenterX, dialCenterY, dialRadius + i,
                           tft.color565(0, 80 + i * 40, 160 + i * 40));
        }
        tft.fillCircle(dialCenterX, dialCenterY, dialRadius, tft.color565(0, 20, 40));

        // Draw tick marks with glow
        for (int angleDeg = 0; angleDeg < 360; angleDeg += 30)
        {
            float rad = angleDeg * PI / 180.0f;
            int innerX = dialCenterX + cos(rad) * (dialRadius - 5);
            int innerY = dialCenterY + sin(rad) * (dialRadius - 5);
            int outerX = dialCenterX + cos(rad) * dialRadius;
            int outerY = dialCenterY + sin(rad) * dialRadius;
            // Glow effect for tick marks
            for (int i = 1; i >= 0; i--)
            {
                tft.drawLine(innerX + i, innerY + i, outerX + i, outerY + i,
                             tft.color565(0, 160 + i * 40, 255));
            }
        }

        // Draw zero marker with glow
        int zeroX = dialCenterX - dialRadius - 10;
        int zeroY = dialCenterY;
        // Glow effect for zero marker
        for (int i = 2; i >= 0; i--)
        {
            tft.fillCircle(zeroX, zeroY, 6 - i, tft.color565(0, 80 + i * 60, 0));
        }
        tft.setTextColor(tft.color565(0, 255, 0));
        tft.setTextSize(1);
        tft.drawString("0", zeroX, zeroY);

        // Draw pointer with glow effect
        float angle = PI + ((potValue / 4095.0f) * (300.0f * PI / 180.0f));
        int pointerX = dialCenterX + cos(angle) * (dialRadius - 3);
        int pointerY = dialCenterY + sin(angle) * (dialRadius - 3);

        // Pointer glow
        for (int i = 2; i >= 0; i--)
        {
            tft.drawLine(dialCenterX + i, dialCenterY, pointerX + i, pointerY,
                         tft.color565(200 + i * 20, i * 40, i * 40));
        }

        // Center dot with glow
        for (int i = 3; i >= 0; i--)
        {
            tft.fillCircle(dialCenterX, dialCenterY, 3 - i,
                           tft.color565(0, 160 + i * 30, 255));
        }

        lastPotValue = potValue;
    }

    // Draw "Ready for Warp" message when dial is at zero
    if (potValue < 50)
    {
        if (!readyMessageShown)
        {
            // Draw success message box with glow effect
            int msgBoxW = 160;
            int msgBoxH = 30;
            int msgBoxX = (SCREEN_WIDTH - msgBoxW) / 2;
            int msgBoxY = SCREEN_HEIGHT * 0.5 - msgBoxH / 2;

            // Box glow
            for (int i = 3; i >= 0; i--)
            {
                tft.drawRect(msgBoxX - i, msgBoxY - i, msgBoxW + i * 2, msgBoxH + i * 2,
                             tft.color565(0, 80 + i * 40, 0));
            }
            tft.fillRect(msgBoxX, msgBoxY, msgBoxW, msgBoxH, tft.color565(0, 60, 0));

            // Message with glow effect
            tft.setTextColor(tft.color565(200, 255, 200));
            tft.setTextSize(1);
            tft.setTextDatum(MC_DATUM);
            for (int i = 2; i > 0; i--)
            {
                tft.drawString("READY FOR WARP!", SCREEN_WIDTH / 2 + i, SCREEN_HEIGHT * 0.5 + i);
            }
            tft.setTextColor(tft.color565(255, 255, 255));
            tft.drawString("READY FOR WARP!", SCREEN_WIDTH / 2, SCREEN_HEIGHT * 0.5);

            readyMessageShown = true;
        }
    }
    else
    {
        readyMessageShown = false; // Reset when dial moves away from zero
    }
}

void StoryMode::prepareTitleSprite(const char *name)
{
    // Direct draw title bar instead of using a sprite
    tft.fillRect(0, 0, SCREEN_WIDTH, 26, tft.color565(0, 40, 80));
    tft.setTextDatum(MC_DATUM);
    tft.setTextSize(2);
    tft.setTextColor(tft.color565(100, 200, 255));
    tft.drawCentreString(name, SCREEN_WIDTH / 2, 13, 1);
}

void StoryMode::prepareNarrationSprite(const char *text)
{
    // Direct draw narration box instead of using a sprite
    tft.fillRect(0, narrationBoxYPos, SCREEN_WIDTH, narrationBoxHeight, tft.color565(0, 0, 40));
    tft.setTextSize(1);
    tft.setTextColor(tft.color565(200, 255, 200));
    tft.setCursor(narrationBoxPadding, narrationBoxYPos + narrationBoxPadding);
    tft.print(text);
}

void StoryMode::createSprites()
{
    // No-op: UI elements drawn directly, no sprite creation needed
}

void StoryMode::init()
{ // Was initStoryMode
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
    for (int i = 0; i < 4; i++)
    { // Match averaging from main sketch's readPotentiometer
        currentRawPotVal += analogRead(POT_PIN);
    }
    calibratedPotValue = 4095 - (currentRawPotVal / 4);

    // Non-blocking calibration: just check and set flag
    // The update() function will handle showing the calibration prompt
    if (calibratedPotValue > POT_THRESHOLD)
    {
        potentiometerCalibrated = false; // Needs calibration
    }
    else
    {
        potentiometerCalibrated = true; // Already calibrated
    }

    // Reset extern object positions (these should ideally be passed around or members)
    objectX = viewportX + viewportSize / 2;
    objectY = viewportY + viewportSize / 2;
    objectScale = 1.5;

    tft.fillScreen(BG_COLOR);
    initialized = true;
    uiFrameDrawn = false;     // Ensure frame gets drawn
    uiRedrawFrameCounter = 0; // Reset frame counter
    Serial.println("StoryMode initialized with viewport console UI.");

    // Draw permanent UI frame
    drawPermanentUIFrame();

    // Render the initial visuals for the first step
    createSprites(); // Create sprites *after* layout is calculated
    if (initialized)
    { // Check if init succeeded before drawing
        updateCurrentStepVisuals();
        // Draw the static starfield initially (constrained to viewport)
        updateStars();
        // Pre-calculate narration lines for performance
        precalculateNarrationLines();
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

void StoryMode::updateNarrationScrolling(const char *text)
{ // Was updateScrollingText, now for old text box
    // This function was for the old non-sprite text box.
    // The new prepareNarrationSprite handles its own scrolling.
    // Keeping this logic here in case of fallback or if needed for a non-sprite version.
    unsigned long currentTime = millis();
    if (currentTime - lastScrollTimeNs > 1000)
    { // TEXT_SCROLL_SPEED was 1000
        lastScrollTimeNs = currentTime;
        int textLength = strlen(text);
        int maxOffset = std::max(0, textLength - charsPerNarrationLine);
        if (textLength > charsPerNarrationLine)
        {
            textScrollOffset++;
            if (textScrollOffset > maxOffset)
            {
                textScrollOffset = 0;
                delay(500);
            }
            // Redrawing is handled by the main render loop if using this old method
            // drawCurrentNarrationBox(text, textScrollOffset);
        }
    }
}

void StoryMode::advanceToNextStop()
{                                       // Was advanceToNextStoryStep
    textScrollOffset = 0;               // For old text box system
    narrationScrollY = 0;               // Start text at bottom
    lastNarrationScrollTime = millis(); // Reset scroll timer for narration sprite
    isScrollPaused = true;              // Start with a pause
    lastScrollPauseTime = millis();     // Reset pause timer
    uiFrameDrawn = false;               // Force frame redraw for new story step
    uiRedrawFrameCounter = 0;           // Reset frame counter for new story step

    currentStoryStep = (currentStoryStep + 1) % TOTAL_STORY_STOPS;

    // Pre-calculate narration lines for new story step
    precalculateNarrationLines();

    // Position object in center of viewport
    objectX = viewportX + viewportSize / 2;
    objectY = viewportY + viewportSize / 2;
    objectScale = 1.5;

    // Reset animation states for specific objects (these should be managed by object classes or a manager)
    extern bool nebulaInitialized;
    nebulaInitialized = false;
    extern bool asteroidFieldInitialized;
    asteroidFieldInitialized = false;
    extern bool solarSystemInitialized;
    solarSystemInitialized = false;

    Serial.printf("Advanced to story stop: %d\n", currentStoryStep);
}

void StoryMode::precalculateNarrationLines()
{
    currentNarrationLines.clear();
    const StoryStop &currentStop = storyStopsList[currentStoryStep];
    String narrationText = currentStop.narration;

    // Calculate wrapping based on layout (must match updateCurrentStepVisuals)
    int textDisplayWidth = (SCREEN_WIDTH - 2 * narrationBoxPadding) - 16 - (2 * 8);
    int maxCharsPerLine = textDisplayWidth / 6;
    int lineHeight = 14;
    int startChar = 0;

    while (startChar < (int)narrationText.length())
    {
        int endChar = startChar + maxCharsPerLine;
        if (endChar > (int)narrationText.length())
            endChar = narrationText.length();

        String line = narrationText.substring(startChar, endChar);

        // Word wrapping
        if (endChar < (int)narrationText.length())
        {
            int lastSpace = line.lastIndexOf(' ');
            if (lastSpace > 0 && (int)line.length() == maxCharsPerLine)
            {
                line = line.substring(0, lastSpace);
                endChar = startChar + line.length() + 1;
            }
        }

        currentNarrationLines.push_back(line);
        startChar = endChar;
        while (startChar < (int)narrationText.length() && narrationText.charAt(startChar) == ' ')
            startChar++;
    }

    currentNarrationTotalHeight = currentNarrationLines.size() * lineHeight;
}

void StoryMode::updateCurrentStepVisuals()
{
    if (!initialized)
        return;

    // Increment frame counter for UI redraw logic
    uiRedrawFrameCounter++;

    const StoryStop &currentStop = storyStopsList[currentStoryStep];
    if (!warpActive)
    {
        setLedModeStory();

        // Draw permanent UI frame (borders/backgrounds) - redraws every 5-10 frames
        drawPermanentUIFrame();

        // Position object in center of viewport
        objectX = viewportX + viewportSize / 2;
        objectY = viewportY + viewportSize / 2;
        objectScale = 1.5;

        // Clear and draw celestial object in viewport only
        if (currentStoryStep != lastDisplayedStoryStep)
        {
            // Clear viewport area for new object
            tft.fillRect(viewportX, viewportY, viewportSize, viewportSize, BG_COLOR);
        }

        // Draw celestial object (contained within viewport)
        currentStop.drawFunction();

        // --- Title Text Update (borders already drawn by drawPermanentUIFrame) ---
        // Only redraw title when story step changes
        if (currentStoryStep != lastDisplayedStoryStep)
        {
            // Clear title text area only
            tft.fillRect(2, 2, SCREEN_WIDTH - 4, titleBarHeight - 4, tft.color565(0, 20, 40));

            // Draw title text with glow effect
            tft.setTextDatum(MC_DATUM);
            tft.setTextSize(2);
            for (int i = 2; i > 0; i--)
            {
                tft.setTextColor(tft.color565(0, 60 + i * 30, 100 + i * 40));
                tft.drawString(currentStop.name, SCREEN_WIDTH / 2 + i, titleBarHeight / 2 + i);
            }
            tft.setTextColor(tft.color565(150, 220, 255));
            tft.drawString(currentStop.name, SCREEN_WIDTH / 2, titleBarHeight / 2);
        }

        // --- Narration Box Text Update (borders already drawn by drawPermanentUIFrame) ---
        // Only update narration text if fact popup is not active
        if (!factPopupActive)
        {
            int narrBoxX = narrationBoxPadding;
            int narrBoxW = SCREEN_WIDTH - 2 * narrationBoxPadding;

            // Text rendering with Star Wars style scrolling
            tft.setTextSize(1);
            tft.setTextColor(tft.color565(220, 220, 240));
            tft.setTextWrap(true);
            tft.setTextDatum(MC_DATUM);

            // Calculate text dimensions and positioning
            int textPaddingInsideBox = 8;
            int textPaddingTop = 8;
            int lineHeight = 14; // Line height for text

            // Use pre-calculated lines (calculated once per story step for performance)
            // No need to recalculate text wrapping every frame

            // Update scroll position with pauses
            unsigned long currentTime = millis();

            // Use pre-calculated total text height
            int totalTextHeight = currentNarrationTotalHeight;

            // Adjust initial scroll position to start at the bottom edge of the box
            // if a new step has just started.
            if (currentStoryStep != lastDisplayedStoryStep)
            {
                narrationScrollY = 0;              // Start the scroll offset at 0 from the bottom edge
                isScrollPaused = true;             // Start with a pause
                lastScrollPauseTime = currentTime; // Reset pause timer
            }

            // Handle scroll pauses
            if (isScrollPaused)
            {
                if (currentTime - lastScrollPauseTime > scrollPauseTime)
                {
                    isScrollPaused = false;
                    lastNarrationScrollTime = currentTime; // Reset scroll timer when pause ends
                }
            }
            else if (currentTime - lastNarrationScrollTime > narrationScrollPixelSpeed)
            {
                lastNarrationScrollTime = currentTime;
                narrationScrollY++; // Scroll upward

                // Check if we need to pause at the end or loop
                if (narrationScrollY >= totalTextHeight + narrationBoxHeight)
                {                                      // Scroll until the last line has exited the top of the box
                    narrationScrollY = 0;              // Reset to start scrolling again from the bottom
                    isScrollPaused = true;             // Pause before restarting
                    lastScrollPauseTime = currentTime; // Reset pause timer
                }
            }

            // Calculate starting Y position for text to scroll from bottom edge of the box
            int textStartY = narrationBoxYPos + narrationBoxHeight - narrationScrollY;

            // Second pass: draw all lines with strict bounds checking
            int visibleTop = narrationBoxYPos + textPaddingTop;                               // Top boundary for drawing text
            int visibleBottom = narrationBoxYPos + narrationBoxHeight - textPaddingInsideBox; // Bottom boundary for drawing text

            // Save the current text color for later restoration
            uint16_t savedColor = tft.textcolor;

            // Clear only the text area (not borders) with background color
            tft.fillRect(narrBoxX + 4, narrationBoxYPos + 4, narrBoxW - 8, narrationBoxHeight - 8,
                         tft.color565(0, 20, 40)); // Match the frame background

            // Draw all visible text lines (using pre-calculated lines)
            for (size_t i = 0; i < currentNarrationLines.size(); i++)
            {
                int currentLineBaseY = textStartY + (i * lineHeight);
                int currentLineCenterY = currentLineBaseY + lineHeight / 2; // For MC_DATUM

                // Only draw if the center of the line is within the visible area of the box
                if (currentLineCenterY >= visibleTop && currentLineCenterY <= visibleBottom)
                {
                    tft.setTextColor(tft.color565(220, 220, 240));                                  // Text color
                    tft.drawString(currentNarrationLines[i], SCREEN_WIDTH / 2, currentLineCenterY); // Draw centered
                }
            }

            // Restore the original text color
            tft.setTextColor(savedColor);
        } // End of if (!factPopupActive)

        lastDisplayedStoryStep = currentStoryStep; // Mark this step as displayed
    }
    else
    {
        // === WARP MODE === UI stays, warp only in viewport
        // Draw UI frame periodically (every 5-10 frames) to prevent overwrites
        if (!warpUIFrameDrawn || (uiRedrawFrameCounter % 7 == 0))
        {
            drawPermanentUIFrame();
            warpUIFrameDrawn = true;
        }

        // Update title with immersive warp styling (only text area, not full frame)
        tft.fillRect(2, 2, SCREEN_WIDTH - 4, titleBarHeight - 4, tft.color565(0, 15, 30));
        tft.setTextDatum(MC_DATUM);
        tft.setTextSize(2);
        // Animated glow effect
        for (int i = 3; i > 0; i--)
        {
            uint16_t glowColor = tft.color565(255 - i * 50, 180 - i * 30, 0);
            tft.setTextColor(glowColor);
            tft.drawString(">>> WARP DRIVE <<<", SCREEN_WIDTH / 2 + i, titleBarHeight / 2 + i);
        }
        tft.setTextColor(tft.color565(255, 220, 50));
        tft.drawString(">>> WARP DRIVE <<<", SCREEN_WIDTH / 2, titleBarHeight / 2);

        // Update narration with dynamic warp information
        int narrBoxX = narrationBoxPadding;
        int narrBoxW = SCREEN_WIDTH - 2 * narrationBoxPadding;
        // Clear narration content area
        for (int y = 0; y < narrationBoxHeight - 8; y++)
        {
            int brightness = 15 + (y * 20) / (narrationBoxHeight - 8);
            tft.drawFastHLine(narrBoxX + 4, narrationBoxYPos + 4 + y, narrBoxW - 8,
                              tft.color565(brightness, brightness * 2, 0));
        }

        tft.setTextDatum(MC_DATUM);
        tft.setTextSize(1);

        // Warp factor display with visual bars
        int barY = narrationBoxYPos + 10;
        tft.setTextColor(tft.color565(255, 220, 100));
        tft.drawString("WARP FACTOR", SCREEN_WIDTH / 2, barY);

        // Warp speed bar - clear old bar first
        barY += 15;
        int barWidth = narrBoxW - 40;
        int barHeight = 8;
        int barX = narrBoxX + 20;
        // Clear bar area
        tft.fillRect(barX, barY, barWidth, barHeight, tft.color565(15, 15, 0));
        // Background bar
        tft.drawRect(barX, barY, barWidth, barHeight, tft.color565(255, 200, 0));
        // Fill bar based on warp factor
        int fillWidth = (int)(barWidth * currentWarpFactor);
        if (fillWidth > 2)
        {
            for (int i = 0; i < fillWidth - 2; i++)
            {
                uint16_t barColor = tft.color565(255, 200 - (i * 100 / barWidth), 0);
                tft.drawFastVLine(barX + 1 + i, barY + 1, barHeight - 2, barColor);
            }
        }

        // Warp number display - clear old text first
        barY += 18;
        tft.fillRect(SCREEN_WIDTH / 2 - 30, barY - 8, 60, 16, tft.color565(15, 15, 0));
        tft.setTextSize(2);
        tft.setTextColor(tft.color565(255, 255, 100));
        char warpText[20];
        sprintf(warpText, "%.2f", currentWarpFactor * 9.9);
        tft.drawString(warpText, SCREEN_WIDTH / 2, barY);

        // Status indicator
        tft.setTextSize(1);
        tft.setTextColor(tft.color565(100, 255, 100));
        tft.drawString("[ENGAGED]", SCREEN_WIDTH / 2, barY + 25);

        // Update global warpFactor before calling updateWarpStars()
        // updateWarpStars() uses the global warpFactor variable
        warpFactor = currentWarpFactor;

        // Warp stars drawn ONLY in viewport (contained)
        // updateWarpStars() handles clearing the viewport in its buffer
        // Force update by resetting frame timer to ensure stars are drawn every frame
        lastWarpFrame = 0; // Reset frame timer to force immediate update
        updateWarpStars();

        // Reset for next stop
        lastDisplayedStoryStep = -1;
    }
}

// Modified to handle button and return bool for exit request
bool StoryMode::processInput(int potValue, bool buttonPinState_LOW)
{
    if (!initialized || !potentiometerCalibrated)
        return false; // No exit requested

    // Handle Exit Button Press
    if (buttonPinState_LOW && !storyExitButton_wasPressed)
    {
        unsigned long currentTime = millis();
        if (currentTime - storyExitButton_lastPressTime > 300)
        {                                      // Debounce (300ms)
            storyExitButton_wasPressed = true; // Mark as pressed (acts like latch until release)
            storyExitButton_lastPressTime = currentTime;

            // Serial.println("StoryMode: Exit button pressed, calling this->exit().");
            this->exit(); // Call internal exit to deinit sprites, set initialized to false etc.
            return true;  // Signal that an exit is requested
        }
    }
    else if (!buttonPinState_LOW)
    {
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

    if (shouldWarp && !previousWarpState)
    {
        // We're entering warp mode - keep UI visible!
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
            warpUIFrameDrawn = false; // Reset warp UI flag
            advanceToNextStop();      // This will redraw everything
            Serial.println("StoryMode: Exited Warp, advanced to next stop.");
        }
        else
        {
            warpActive = false;
            warpUIFrameDrawn = false; // Reset warp UI flag
            uiFrameDrawn = false;     // Force UI redraw
            uiRedrawFrameCounter = 0; // Reset frame counter
            Serial.println("StoryMode: Exited Warp too soon, redrawing current stop.");
        }
    }
    else if (shouldWarp)
    {
        // Continue in warp, warpFactor updated above
    }
    previousWarpState = shouldWarp;
    return false; // No exit requested by warp logic / no button press processed this call
}

// Main update and render combined, to be called from sketch's loop when story mode is active
// Modified to handle button state and return true if exit is requested
bool StoryMode::update(int potValueFromMain, bool buttonPinState_LOW)
{
    // --- NON-BLOCKING CALIBRATION HANDLER ---
    // Handle calibration in a non-blocking way
    if (!potentiometerCalibrated)
    {
        if (potValueFromMain < 50)
        { // Calibrated
            potentiometerCalibrated = true;
            calibrationScreenFirstDraw = true; // Reset for next time
            readyMessageShown = false;

            // Draw the initial state now that we are calibrated
            tft.fillScreen(BG_COLOR);
            drawPermanentUIFrame();
            updateCurrentStepVisuals(); // Draw the first step
            updateStars();
            return false; // Calibration done, no exit
        }
        else
        {
            // Still needs calibration, draw the prompt
            drawCalibrationPrompt(potValueFromMain);
            return false; // Not calibrated, no exit
        }
    }
    // --- END NON-BLOCKING CALIBRATION HANDLER ---

    if (!initialized && !this->isActive())
    { // Check if already exited by button press in a previous call this cycle
        // If not initialized (e.g. after this->exit() was called by processInput),
        // then an exit has occurred or it was never started.
        // Return true to ensure main loop transitions out of story mode state.
        return true;
    }
    if (!initialized)
        return false; // Should not happen if isActive() is true, but defensive.

    // processInput now handles the button check and can trigger exit directly.
    // if it returns true, an exit was requested and performed.
    if (processInput(potValueFromMain, buttonPinState_LOW))
    {
        return true; // Exit requested and handled by processInput (which called this->exit())
    }

    // If processInput did not request an exit, continue with visuals.

    if (warpActive)
    {
        // In warp mode - render viewport with warp animation
        // updateCurrentStepVisuals() handles warp mode rendering including viewport
        updateCurrentStepVisuals();
        // Play warp sound effect (same as discovery mode)
        updateWarpSound(this->currentWarpFactor);
        // Set LED for warp mode (same as discovery mode)
        setLedModeWarp();
    }
    else
    {
        // Not in warp - render current story step (object, title, narration)
        updateCurrentStepVisuals();
        // Background stars (twinkling) should update when not in warp
        updateStars(); // Call the extern star update function
        // Stop warp sound effect when not warping (same as discovery mode)
        updateWarpSound(0.0f);
        // Set LED for story mode (not warp)
        setLedModeStory();
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