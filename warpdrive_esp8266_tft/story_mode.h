#include <vector>
#ifndef STORY_MODE_H
#define STORY_MODE_H

#include <TFT_eSPI.h>
#include <Arduino.h> // For PI constant and other math functions

// Make sure POT_PIN is available from main file (warpdrive_esp32_tft.ino)
#ifndef POT_PIN
#define POT_PIN 7    // Default value if not defined elsewhere
#endif

// Star count definition - use the same value in all files
#define STAR_COUNT 180
#define MAX_STREAK_LENGTH 20

// Forward declarations of external variables and functions
extern TFT_eSPI tft;
extern uint16_t BG_COLOR;
extern void drawStar(const struct Star& star);
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
extern void updateStars();
extern void updateWarpStars();

// Forward declare the Star struct so we can access it
struct Star;
extern Star stars[];
extern uint16_t prevX[][MAX_STREAK_LENGTH + 1];
extern uint16_t prevY[][MAX_STREAK_LENGTH + 1];
extern float warpFactor;

// Also need to access these globals for object positioning
extern int objectX;
extern int objectY;
extern float objectScale;

// Screen dimensions for centering objects
//const int SCREEN_WIDTH = 128; // Now use global SCREEN_WIDTH
//const int SCREEN_HEIGHT = 128; // Now use global SCREEN_HEIGHT

// --- Story Mode Sprites for flicker-free rendering ---
TFT_eSprite storyTitleSprite = TFT_eSprite(&tft);
TFT_eSprite storyNarrationSprite = TFT_eSprite(&tft);
bool storyTitleSpriteCreated = false;
bool storyNarrationSpriteCreated = false;
int storyScrollBoxH_global_var = 0; // For narration sprite height
int storyScrollBoxY_global_var = 0; // For narration sprite Y position
// --- End Story Mode Sprites ---

// Define story stop structure
struct StoryStop {
    const char* name;              // Name of the celestial object
    const char* narration;         // Carl Sagan-inspired narration text
    const char* fact;              // Educational fact or question (stored but not shown by default)
    void (*drawFunction)();        // Function pointer to draw the celestial object
};

// Story navigation state variables
int storyStep = 0;                 // Current position in story array
int storyTextScrollOffset = 0;     // For scrolling long narration text
unsigned long lastScrollTime = 0;  // For controlling text scroll speed
bool storyInitialized = false;     // Flag to track if first-time setup is done
bool potCalibrated = false;        // Track if potentiometer has been calibrated to 0

// Warp drive state tracking for story mode
bool storyWarpActive = false;      // Tracking if currently in warp
bool prevStoryWarpState = false;   // Previous warp state for detecting transitions
unsigned long warpEntryTime = 0;   // When warp was engaged
unsigned long minWarpDuration = 1500; // Minimum warp time before allowing exit to next stop

// Define the story sequence/array
// These are the "stops" in our cosmic journey
const StoryStop storyStops[] = {
    {
        "Our Sun", 
        "A star, our Sun, the giver of light and warmth. Every living being on Earth owes its existence to this ball of nuclear fusion, our quiet companion in the cosmos.",
        "The Sun contains 99.86% of all mass in the solar system.",
        drawStar
    },
    {
        "Planet Earth", 
        "The pale blue dot. Suspended in a sunbeam. A mote of dust in the cosmic dark. Our home, the only world we've known, fragile and precious beyond measure.",
        "Earth is the only planet not named after a mythological god or goddess.",
        drawPlanet
    },
    {
        "Nebula", 
        "The birthplace of stars. These vast clouds of gas and dust are cosmic nurseries where new suns take their first breath, illuminating the darkness with the promise of worlds yet to be.",
        "The word nebula comes from Latin, meaning 'cloud' or 'fog'.",
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
        "The debris of creation. These rocky remnants tell the story of planets that might have been, celestial building blocks left over from the solar system's turbulent youth.",
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
        "Cosmic lighthouses. The rapidly spinning corpses of massive stars, sweeping beams of radiation across the cosmos with clock-like precision, nature's most perfect timepieces.",
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
        "Comets' tails always point away from the Sun due to solar wind.",
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
        "Humanity's first outpost among the stars. A testament to our curiosity and determination to explore beyond our world, a stepping stone to our future among the cosmos.",
        "The International Space Station orbits Earth at about 28,000 km/h.",
        drawSpaceStation
    }
};

const int STORY_STOPS_COUNT = sizeof(storyStops) / sizeof(storyStops[0]);

// Text display constants
int TEXT_BOX_HEIGHT;
int TEXT_BOX_Y;
int TEXT_BOX_PADDING;
constexpr int TEXT_SCROLL_SPEED = 1000; // Milliseconds between scroll steps (half speed)
int CHARS_PER_LINE;

// Add static variable for tracking story step changes
static int lastStoryStep = -1;

inline void setupStoryModeLayout() {
    TEXT_BOX_HEIGHT = SCREEN_HEIGHT / 9;
    TEXT_BOX_Y = SCREEN_HEIGHT - TEXT_BOX_HEIGHT;
    TEXT_BOX_PADDING = SCREEN_WIDTH / 40;
    CHARS_PER_LINE = (SCREEN_WIDTH - 2 * TEXT_BOX_PADDING) / 6;
}

// Draw a fun "Set Dial to Zero" animation/prompt
// Minimal UI: Only show dial with pointer, zero at negative X axis (180°/pi radians)
void drawSetDialToZeroPrompt(int potValue) {
    static int lastPotValue = -1;
    static bool firstDraw = true;
    static uint16_t starfield[30][3]; // x, y, brightness
    static bool starfieldInitialized = false;

    // Responsive layout
    int dialRadius = std::min(SCREEN_WIDTH, SCREEN_HEIGHT) / 8;
    int dialCenterX = SCREEN_WIDTH / 2;
    int dialCenterY = SCREEN_HEIGHT * 3 / 4; // Lower 3/4 of screen

    int boxW = SCREEN_WIDTH * 0.7;
    int boxH = SCREEN_HEIGHT * 0.18;
    int boxX = (SCREEN_WIDTH - boxW) / 2;
    int boxY = SCREEN_HEIGHT * 0.18;

    // Draw the prompt box (background, title, dialog, text) only once
    if (firstDraw) {
        tft.fillScreen(BG_COLOR);
        if (!starfieldInitialized) {
            for (int i = 0; i < 30; i++) {
                starfield[i][0] = random(SCREEN_WIDTH);
                starfield[i][1] = random(SCREEN_HEIGHT);
                starfield[i][2] = random(150, 256);
            }
            starfieldInitialized = true;
        }
        for (int i = 0; i < 30; i++) {
            tft.drawPixel(starfield[i][0], starfield[i][1], tft.color565(starfield[i][2], starfield[i][2], starfield[i][2]));
        }
        // Responsive title bar with gradient
        int titleBarH = SCREEN_HEIGHT / 16;
        for (int y = 0; y < titleBarH; y++) {
            uint16_t c = tft.color565(0, 0, 60 + y * 2);
            tft.drawFastHLine(0, y, SCREEN_WIDTH, c);
        }
        tft.setTextSize(SCREEN_HEIGHT > 160 ? 2 : 1);
        tft.setTextColor(tft.color565(0, 220, 255));
        tft.setCursor(SCREEN_WIDTH * 0.05, titleBarH / 4);
        tft.print("COSMIC JOURNEY");

        // Responsive prompt box with gradient and glow
        for (int y = 0; y < boxH; y++) {
            uint16_t c = tft.color565(20 + y*40/boxH, 40 + y*40/boxH, 80 + y*40/boxH);
            tft.drawFastHLine(boxX, boxY + y, boxW, c);
        }
        tft.drawRoundRect(boxX, boxY, boxW, boxH, boxH / 4, tft.color565(40, 100, 200));
        tft.drawRoundRect(boxX+1, boxY+1, boxW-2, boxH-2, boxH / 4, tft.color565(80, 150, 220));
        tft.setTextColor(tft.color565(255, 255, 0));
        tft.setTextSize(SCREEN_HEIGHT > 160 ? 2 : 1);
        tft.setCursor(boxX + boxW * 0.12, boxY + boxH * 0.25);
        tft.print("SET DIAL TO");
        tft.setCursor(boxX + boxW * 0.38, boxY + boxH * 0.6);
        tft.print("ZERO");
        firstDraw = false;
        lastPotValue = -1; // Force dial to draw on first call after box
    }

    // Only update the dial if potValue changed
    if (potValue != lastPotValue) {
        // Clear only the dial area (minimal flicker)
        tft.fillCircle(dialCenterX, dialCenterY, dialRadius + 6, BG_COLOR);
        
        // Draw enhanced dial with gradient
        for (int r = dialRadius; r > dialRadius-3; r--) {
            tft.drawCircle(dialCenterX, dialCenterY, r, tft.color565(150-r*10, 150-r*5, 150));
        }
        
        // Draw tick marks around the dial
        for (int angle = 0; angle < 360; angle += 30) {
            float rad = angle * PI / 180.0f;
            int innerX = dialCenterX + cos(rad) * (dialRadius - 5);
            int innerY = dialCenterY + sin(rad) * (dialRadius - 5);
            int outerX = dialCenterX + cos(rad) * dialRadius;
            int outerY = dialCenterY + sin(rad) * dialRadius;
            tft.drawLine(innerX, innerY, outerX, outerY, tft.color565(180, 180, 180));
        }
        
        // Draw '0' indicator at leftmost (zero) position with glow
        int zeroX = dialCenterX + cos(PI) * (dialRadius + 10);
        int zeroY = dialCenterY + sin(PI) * (dialRadius + 10);
        // Glow effect
        tft.fillCircle(zeroX, zeroY, 6, tft.color565(0, 50, 0));
        tft.fillCircle(zeroX, zeroY, 4, tft.color565(0, 100, 0));
        tft.setTextColor(tft.color565(0, 255, 0));
        tft.setTextSize(1);
        tft.setCursor(zeroX - 3, zeroY - 4);
        tft.print("0");
        
        // Draw pointer with highlight
        float angle = PI + ((potValue / 4095.0f) * (300.0f * PI / 180.0f));
        int pointerX = dialCenterX + cos(angle) * (dialRadius - 3);
        int pointerY = dialCenterY + sin(angle) * (dialRadius - 3);
        // Draw wider base for pointer
        for (int w = 2; w >= 0; w--) {
            uint16_t pointerColor = w == 0 ? tft.color565(255, 0, 0) : tft.color565(180, 0, 0);
            tft.drawLine(dialCenterX, dialCenterY, pointerX, pointerY, pointerColor);
        }
        tft.fillCircle(dialCenterX, dialCenterY, 3, tft.color565(200, 200, 220));
        tft.fillCircle(dialCenterX, dialCenterY, 1, tft.color565(255, 255, 255));
        lastPotValue = potValue;
    }
}

// --- New: Vertical scrolling text state ---
static int storyScrollY = 0;
static unsigned long lastStoryScrollTime = 0;
static int storyScrollBoxH = 0;
static int storyScrollBoxY = 0;
static int storyScrollSpeed = 50; // Increased from 30 to 50 ms per pixel

// --- New: Draw a modern title bar with double-buffering approach (draws to sprite) ---
inline void prepareStoryTitleSprite(const char* name) {
    if (!storyTitleSpriteCreated) return;

    int barH = storyTitleSprite.height();
    int paddingY = barH / 6; // Add vertical padding
    int centerX = storyTitleSprite.width() / 2;
    int centerY = barH / 2 - 6 + paddingY;

    // Fully clear sprite with black
    //storyTitleSprite.fillSprite(TFT_BLACK);

    // Draw gradient for the entire height (ensure no off-by-one)
    for (int y = 0; y < barH; ++y) {
        uint16_t c = tft.color565(0, 40 + y * 2, 80 + y * 3);
        storyTitleSprite.drawFastHLine(0, y, storyTitleSprite.width(), c);
    }

    // Calculate text width for proper glow placement
    int textWidth = storyTitleSprite.textWidth(name);
    int glowRadius = textWidth / 2 + 10;

    // Add subtle glow effect around text area
    for (int r = glowRadius; r > 0; r -= 2) {
        uint8_t alpha = 100 - (r * 90 / glowRadius); // Fade out
        uint16_t glowColor = tft.color565(0, 50 + alpha, 100 + alpha);
        storyTitleSprite.drawCircle(centerX, centerY, r, glowColor);
    }

    // Text rendering with optimized transparency
    storyTitleSprite.setTextDatum(MC_DATUM);
    storyTitleSprite.setTextColor(tft.color565(220, 240, 255));
    storyTitleSprite.setTextSize(SCREEN_HEIGHT > 160 ? 2 : 1);
    storyTitleSprite.drawString(name, centerX, centerY);

    // Draw top and bottom borders INSIDE the sprite bounds
    uint16_t borderColor = tft.color565(60, 140, 240);
    storyTitleSprite.drawFastHLine(0, 0, storyTitleSprite.width(), borderColor); // Top
    storyTitleSprite.drawFastHLine(0, barH - 1, storyTitleSprite.width(), borderColor); // Bottom
}

// --- New: Draw a modern narration box with improved approach for rendering (draws to sprite) ---
inline void prepareStoryNarrationSprite(const char* text) {
    if (!storyNarrationSpriteCreated) return;

    // Box dimensions are based on sprite's own dimensions
    int boxW = storyNarrationSprite.width();
    int boxH = storyNarrationSprite.height();
    int radius = boxH / 4;

    // Start with a clean sprite - crucial for preventing artifacts
    storyNarrationSprite.fillSprite(TFT_BLACK); // Fill with background color first
    
    // Enhanced gradient background for narration box - redraw the entire box each time
    for (int y = 0; y < boxH; y++) {
        uint16_t c = tft.color565(10 + (y * 15 / boxH), 20 + (y * 10 / boxH), 40 + (y * 5 / boxH));
        storyNarrationSprite.drawFastHLine(0, y, boxW, c);
    }
    
    // Draw border with subtle glow effect
    for (int i = 0; i < 3; i++) {
        uint16_t borderColor = tft.color565(0, 100 - i*20, 200 - i*30);
        storyNarrationSprite.drawRoundRect(i, i, boxW-i*2, boxH-i*2, radius, borderColor);
    }

    // Pre-process text into lines for better rendering control
    storyNarrationSprite.setTextSize(1);
    int maxLineW = boxW - 14; // Slightly more padding for aesthetics
    std::vector<String> lines;
    
    // Word-wrap text processing
    String stext = String(text);
    int start = 0;
    while (start < stext.length()) {
        int end = start;
        int lastSpace = -1;
        while (end < stext.length()) {
            String sub = stext.substring(start, end + 1);
            if (storyNarrationSprite.textWidth(sub) > maxLineW) break;
            if (stext[end] == ' ') lastSpace = end;
            end++;
        }
        if (end < stext.length() && lastSpace > start) end = lastSpace;
        String line = stext.substring(start, end);
        line.trim();
        lines.push_back(line);
        while (end < stext.length() && stext[end] == ' ') end++; // Skip multiple spaces
        start = end;
    }

    // Calculate total text height for scrolling calculations
    int lineH = 14; // Font height for size 1 (approx)
    int totalTextH = lines.size() * lineH;

    // Vertical scrolling logic with consistent timing
    unsigned long now = millis();
    if (now - lastStoryScrollTime > storyScrollSpeed) {
        lastStoryScrollTime = now;
        storyScrollY++;
        // Reset scrolling when text has scrolled through
        if (storyScrollY > totalTextH + boxH) storyScrollY = 0;
    }

    // Calculate starting Y position for text
    int y0 = boxH - lineH - storyScrollY;
    storyNarrationSprite.setTextDatum(MC_DATUM);
    
    // Shadow and text colors
    uint16_t shadowColor = tft.color565(5, 10, 20);
    uint16_t textColor = tft.color565(220, 255, 220);
    
    // Draw all visible lines with a shadow effect for better readability
    for (size_t i = 0; i < lines.size(); ++i) {
        int lineY = y0 + i * lineH;
        // Only render lines that would be visible in the sprite
        if (lineY >= -lineH && lineY <= boxH) {
            // Shadow first (offset by 1 pixel diagonally)
            storyNarrationSprite.setTextColor(shadowColor);
            storyNarrationSprite.drawString(lines[i], boxW / 2 + 1, lineY + 1);
            // Then actual text with no background
            storyNarrationSprite.setTextColor(textColor);
            storyNarrationSprite.drawString(lines[i], boxW / 2, lineY);
        }
    }
}

// Function to initialize story mode
void initStoryMode() {
    // Setup text box layout for current screen size
    setupStoryModeLayout();
    // Initial setup
    storyStep = 0;  // Always start with first stop (Our Sun)
    storyTextScrollOffset = 0;
    storyInitialized = true;
    lastScrollTime = millis();
    storyWarpActive = false;
    prevStoryWarpState = false;
    potCalibrated = false;  // Not calibrated yet
    lastStoryStep = -1; // Initialize for first draw comparison
    
    // Access required global variables/pins
    extern int potValue;  // For accessing most recent value
    
    // Define threshold for calibration
    const int POT_THRESHOLD = 50;  // Needs to be below this to start
    
    // Get initial reading
    int currentPotValue = analogRead(POT_PIN);
    
    // Check if potentiometer needs to be reset
    potValue = 4095 - currentPotValue;
    if (potValue > POT_THRESHOLD) {
        // Show calibration screen and wait for pot to reach zero
        unsigned long lastUpdateTime = 0;
        const unsigned long UPDATE_INTERVAL = 50;  // Update animation every 50ms
        
        // Continue checking until pot is below threshold (using inverted value)
        while (potValue > POT_THRESHOLD) {
            // Update animation at the specified interval
            unsigned long currentTime = millis();
            if (currentTime - lastUpdateTime > UPDATE_INTERVAL) {
                lastUpdateTime = currentTime;
                
                // Get a fresh pot reading (with noise reduction)
                int rawValue = 0;
                for (int i = 0; i < 4; i++) {
                    rawValue += analogRead(POT_PIN);
                }
                currentPotValue = rawValue / 4; // Average of 4 readings
                
                // Update global potValue to ensure consistency
                potValue = 4095 - currentPotValue;
                
                // Draw the dial prompt with the inverted pot value
                drawSetDialToZeroPrompt(potValue);
            }
            
            // Small delay to prevent display flickering
            delay(5);
        }
        
        // Update the global potValue one more time to ensure consistency
        potValue = 4095 - currentPotValue;
        
        // Show success animation once pot is at zero
        tft.fillRect(10, 70, 108, 15, tft.color565(0, 100, 0));
        tft.setTextColor(tft.color565(255, 255, 255));
        tft.setCursor(25, 73);
        tft.print("READY FOR WARP!");
        delay(1000);  // Show success message for 1 second
    }
    
    // Set as calibrated and fully initialize
    potCalibrated = true;
    
    // Set initial object position and scale
    objectX = SCREEN_WIDTH / 2; // Center X
    objectY = SCREEN_HEIGHT / 2; // Center Y
    objectScale = 1.5;
    
    // Clear screen and draw initial scene
    tft.fillScreen(BG_COLOR);

    // --- Initialize Sprites ---
    // Title Sprite
    if (storyTitleSpriteCreated) { // If already created, delete first
        storyTitleSprite.deleteSprite();
        storyTitleSpriteCreated = false;
        Serial.println("Previously created storyTitleSprite deleted.");
    }
    if (!storyTitleSpriteCreated) {
        int barH = SCREEN_HEIGHT / 12; // Slightly taller title bar for better visuals
        storyTitleSprite.setColorDepth(8); // 8-bit color depth is sufficient for title
        storyTitleSprite.setAttribute(PSRAM_ENABLE, true); // Use PSRAM for larger sprite
        Serial.printf("Attempting to create storyTitleSprite: %d x %d (8-bit)\n", SCREEN_WIDTH, barH);
        if (storyTitleSprite.createSprite(SCREEN_WIDTH, barH) == nullptr) {
            Serial.println("Failed to create storyTitleSprite (returned nullptr).");
            // storyTitleSpriteCreated remains false
        } else {
            storyTitleSpriteCreated = true;
            Serial.printf("storyTitleSprite created successfully: %d x %d, BPP: %d\n", storyTitleSprite.width(), storyTitleSprite.height(), storyTitleSprite.getColorDepth());
            
            // Important: Initialize the sprite content immediately to prevent artifacts
            storyTitleSprite.fillSprite(TFT_BLACK);
        }
    }

    // Narration Sprite
    if (storyNarrationSpriteCreated) { // If already created, delete first
        storyNarrationSprite.deleteSprite();
        storyNarrationSpriteCreated = false;
        Serial.println("Previously created storyNarrationSprite deleted.");
    }
    if (!storyNarrationSpriteCreated) {
        // Dimensions from original drawNarrationScrollBox logic
        storyScrollBoxH_global_var = SCREEN_HEIGHT / 4; // Make the box slightly taller
        storyScrollBoxY_global_var = SCREEN_HEIGHT - storyScrollBoxH_global_var - SCREEN_HEIGHT / 40;
        int narrationBoxWidth = SCREEN_WIDTH - SCREEN_WIDTH / 12; // Slightly wider box

        storyNarrationSprite.setColorDepth(8); // 8-bit color depth
        storyNarrationSprite.setAttribute(PSRAM_ENABLE, true); // Use PSRAM for larger sprite
        Serial.printf("Attempting to create storyNarrationSprite: %d x %d (8-bit)\n", narrationBoxWidth, storyScrollBoxH_global_var);
        if (storyNarrationSprite.createSprite(narrationBoxWidth, storyScrollBoxH_global_var) == nullptr) {
            Serial.println("Failed to create storyNarrationSprite (returned nullptr).");
            // storyNarrationSpriteCreated remains false
        } else {
            storyNarrationSpriteCreated = true;
            Serial.printf("storyNarrationSprite created successfully: %d x %d, BPP: %d\n", storyNarrationSprite.width(), storyNarrationSprite.height(), storyNarrationSprite.getColorDepth());
            
            // Important: Initialize the sprite content immediately to prevent artifacts
            storyNarrationSprite.fillSprite(TFT_BLACK);
        }
    }
    // --- End Initialize Sprites ---
}

// Function to draw the narration text box at the bottom of screen
void drawNarrationTextBox(const char* text, int scrollOffset) {
    // First clear the text area
    tft.fillRect(0, TEXT_BOX_Y, SCREEN_WIDTH, TEXT_BOX_HEIGHT, tft.color565(0, 0, 40));
    tft.drawRect(0, TEXT_BOX_Y, SCREEN_WIDTH, TEXT_BOX_HEIGHT, tft.color565(0, 100, 200));
    
    // Set text properties
    tft.setTextSize(1);
    tft.setTextColor(tft.color565(200, 255, 200));
    
    // Calculate text length and whether scrolling is needed
    int textLength = strlen(text);
    
    // Draw the text with scroll offset
    tft.setCursor(TEXT_BOX_PADDING, TEXT_BOX_Y + TEXT_BOX_PADDING);
    
    // Handle scrolling text
    const char* displayText = text + scrollOffset;
    
    // Display the text that fits in our box
    tft.print(displayText);
    
 
}

// Function to draw the object name/title at the top
void drawStoryTitle(const char* name) {
    // Create a subtle title bar at the very top
    tft.fillRect(0, 0, SCREEN_WIDTH, 10, tft.color565(0, 20, 50));
    
    // Calculate center position for text
    int16_t textWidth = strlen(name) * 6; // Approximate width at text size 1
    int16_t x = (SCREEN_WIDTH - textWidth) / 2;
    
    // Draw title text
    tft.setTextSize(1);
    tft.setTextColor(tft.color565(180, 220, 255));
    tft.setCursor(x, 2);
    tft.print(name);
}

// Function to update scrolling text
void updateScrollingText(const char* text) {
    unsigned long currentTime = millis();
    if (currentTime - lastScrollTime > TEXT_SCROLL_SPEED) {
        lastScrollTime = currentTime;
        
        int textLength = strlen(text);
        int maxOffset = max(0, textLength - CHARS_PER_LINE);
        
        // Auto-scroll text
        if (textLength > CHARS_PER_LINE) {
            storyTextScrollOffset++;
            if (storyTextScrollOffset > maxOffset) {
                storyTextScrollOffset = 0; // Reset to beginning
                delay(500); // Pause briefly at the start
            }
            
            // Redraw the text with updated offset
            drawNarrationTextBox(text, storyTextScrollOffset);
        }
    }
}

// Function to advance to next story step
void advanceToNextStoryStep() {
    // Clear screen for new story stop
    tft.fillScreen(BG_COLOR);
    
    // Reset text scroll position
    storyTextScrollOffset = 0;
    storyScrollY = 0;         // Reset vertical scroll for the narration sprite content
    lastStoryScrollTime = millis(); // Reset scroll timer

    // Move to next story step (with wraparound)
    storyStep = (storyStep + 1) % STORY_STOPS_COUNT;
    
    // Ensure object is positioned in center
    objectX = SCREEN_WIDTH / 2;
    objectY = SCREEN_HEIGHT / 2;
    objectScale = 1.5;
    
    // Reset animation state variables for different cosmic objects
    extern bool nebulaInitialized;
    nebulaInitialized = false;
    
    extern bool asteroidFieldInitialized;
    asteroidFieldInitialized = false;
    
    extern bool solarSystemInitialized; 
    solarSystemInitialized = false;
}

// Main function to update and render the current story step
void updateStoryStep() {
    // Get current story stop
    const StoryStop& currentStop = storyStops[storyStep];

    // Draw object in center of screen only when not in warp
    if (!storyWarpActive) {
        // Clear only what needs to be cleared - avoid full screen clear
        static int lastObjectX = -1;
        static int lastObjectY = -1;
        static float lastObjectScale = -1;
        
        // Check if object position or scale has changed
        bool objectMoved = (objectX != lastObjectX || objectY != lastObjectY || objectScale != lastObjectScale);
        
        if (objectMoved || storyStep != lastStoryStep) {
            // Only clear the central area where objects are drawn
            int clearMargin = SCREEN_WIDTH / 3; // Clear area slightly larger than objects
            tft.fillRect(
                (SCREEN_WIDTH - clearMargin) / 2,
                (SCREEN_HEIGHT - clearMargin) / 2,
                clearMargin,
                clearMargin,
                BG_COLOR
            );
            
            // Update position tracking
            lastObjectX = objectX;
            lastObjectY = objectY;
            lastObjectScale = objectScale;
        }

        // Set position to center of screen for the object
        objectX = SCREEN_WIDTH / 2;
        objectY = SCREEN_HEIGHT / 2;
        objectScale = 1.5;

        // Draw celestial object directly to tft
        currentStop.drawFunction();

        // --- Title Bar via Sprite ---
        if (storyTitleSpriteCreated) {
            // Always prepare the title sprite - fixes issues with partial updates
            prepareStoryTitleSprite(currentStop.name);
            
            // Clear the title area first to prevent any artifacts
            tft.fillRect(0, 0, SCREEN_WIDTH, storyTitleSprite.height(), TFT_BLACK);
            
            // Push the sprite exactly to its dedicated area
            storyTitleSprite.pushSprite(0, 0);
        } else {
            // Fallback to direct drawing if sprite failed
            drawStoryTitle(currentStop.name); // Original function
        }

        // --- Narration Box via Sprite ---
        if (storyNarrationSpriteCreated) {
            // Narration sprite needs to be updated every frame for scrolling
            prepareStoryNarrationSprite(currentStop.narration);
            
            // Calculate X position for the narration sprite (it's centered)
            int narrationBoxSpriteX = (SCREEN_WIDTH - storyNarrationSprite.width()) / 2;
            
            // Clear the narration area first to prevent artifacts
            tft.fillRect(
                narrationBoxSpriteX, 
                storyScrollBoxY_global_var, 
                storyNarrationSprite.width(), 
                storyNarrationSprite.height(), 
                TFT_BLACK
            );
            
            // Push sprite to its fixed position
            storyNarrationSprite.pushSprite(narrationBoxSpriteX, storyScrollBoxY_global_var);
        } else {
            // Fallback to direct drawing if sprite failed
            // Original drawNarrationTextBox also needs updateScrollingText
            updateScrollingText(currentStop.narration); // Manage scroll for old func
            drawNarrationTextBox(currentStop.narration, storyTextScrollOffset); // Original func
        }
        lastStoryStep = storyStep; // Update for next frame comparison
    }
    else {
        // In warp mode, we just show the warp effect
        // The title and narration text are hidden during warp
        updateWarpStars(); // Draw warp star field
        lastStoryStep = -1; // Ensure refresh when exiting warp
    }
}

// Process potentiometer input for story navigation using warp method
void processStoryInput(int potValue) {
    // Map potentiometer value to warp factor for smooth speed control
    // Typical pot value range is 0-4095 for 12-bit ADC (ESP32)
    const int WARP_THRESHOLD = 100; // Minimum value to start warp
    const int MAX_POT_VALUE = 4095;  // Maximum potentiometer value
    
    // Determine if we should be in warp mode
    bool shouldWarp = (potValue > WARP_THRESHOLD);
    
    // Calculate warp factor when in warp mode (normalize to 0.2-1.0 range)
    if (shouldWarp) {
        // Map pot value to warp factor range
        float potPercent = constrain(potValue, WARP_THRESHOLD, MAX_POT_VALUE);
        potPercent = (potPercent - WARP_THRESHOLD) / (MAX_POT_VALUE - WARP_THRESHOLD);
        
        // Set warp factor with a good range (min 0.2 for visible effect, max 1.0 for full speed)
        extern float warpFactor;
        warpFactor = 0.2f + (potPercent * 0.8f);
    }
    
    // Check for state transitions
    if (shouldWarp && !prevStoryWarpState) {
        // Just entered warp
        storyWarpActive = true;
        warpEntryTime = millis();
        tft.fillScreen(BG_COLOR); // Clear screen for warp effect
        
        // Initialize warp stars for proper movement
        const float centerX = SCREEN_WIDTH / 2.0f; // SCREEN_WIDTH/2
        const float centerY = SCREEN_HEIGHT / 2.0f; // SCREEN_HEIGHT/2
        
        // Make sure stars are distributed across screen for warp effect
        for (int i = 0; i < STAR_COUNT; i++) {
            // Ensure stars are away from the center for better warp effect
            float angle = random(360) * PI / 180.0f;
            // Use full screen radius for distance
            float maxRadius = sqrt((SCREEN_WIDTH/2)*(SCREEN_WIDTH/2) + (SCREEN_HEIGHT/2)*(SCREEN_HEIGHT/2));
            float distance = random(10, maxRadius);
            stars[i].realX = centerX + cos(angle) * distance;
            stars[i].realY = centerY + sin(angle) * distance;
            stars[i].x = round(stars[i].realX);
            stars[i].y = round(stars[i].realY);
            stars[i].brightness = random(150, 256);
            stars[i].streakLength = 0;
        }
    }
    else if (!shouldWarp && prevStoryWarpState) {
        // Just exited warp - check if we were in warp long enough
        if (millis() - warpEntryTime > minWarpDuration) {
            // Transition to next story stop
            storyWarpActive = false;
            advanceToNextStoryStep();
        }
        else {
            // Not in warp long enough, don't advance
            storyWarpActive = false;
            tft.fillScreen(BG_COLOR); // Clear screen to redraw current stop
        }
    }
    else if (shouldWarp) {
        // Continuing in warp - update warp factor based on pot value
        // (already updated above)
    }
    
    // Update previous state for next comparison
    prevStoryWarpState = shouldWarp;
}

#endif // STORY_MODE_H 