/**
 * ESP32 Warp Drive Visualization
 * ================================
 * 
 * This sketch creates a Star Trek-inspired warp drive visualization on a ST7735 TFT display.
 * 
 * Features:
 * - Twinkle star field in normal mode
 * - Dynamic warp speed effect based on potentiometer input
 * - Random celestial objects discovery after exiting warp
 * - Beautiful nebulae, galaxies, solar systems, and asteroid fields
 * 
 * Hardware Requirements:
 * - ESP8266 based board (NodeMCU, Wemos D1, etc.)
 * - ST7735 TFT Display (128x128 pixels)
 * - Potentiometer for warp control
 * 
 * Pin Connections:
 * - TFT_CS:  GPIO5 (D1)
 * - TFT_RST: GPIO4 (D2)
 * - TFT_DC:  GPIO0 (D3)
 * - TFT MOSI: GPIO13 (D7)
 * - TFT SCLK: GPIO14 (D5)
 * - POT_PIN: A0 (Analog input)
 * 
 * Controls:
 * - Turn the potentiometer to increase warp speed
 * - Return to low speed to exit warp and potentially discover celestial objects
 * 
 * The code uses batch processing and optimized drawing to maintain smooth animation
 * on the limited hardware resources of the ESP8266.
 */

// Configure ESP32 stack size to prevent stack overflows
#if defined(ESP32)
#define CONFIG_ARDUINO_LOOP_STACK_SIZE 16384  // Increase the stack size
#endif

#include <TFT_eSPI.h> // Replace Adafruit_GFX and Adafruit_ST7735
#include <SPI.h>
#include "star.h"  // Include star.h first so the Star struct is defined
#include "blackhole.h"
#include "pulsar.h" // Include the pulsar header file
#include "supernova.h" // Include the supernova header file
#include "comet.h" // Include the comet header file
#include "story_mode.h" // Include the story mode header file
#include "quiz_mode.h"
#include "quiz_popup.h"
#include "spacestation.h" // Add space station header
#include "binarystar.h" // Add binary star header
#include "planet.h" // Add planet header
#include "nebula.h" // Add nebula header
#include "solar_system.h" // Add solar system header
#include "galaxy_and_asteroid.h" // Add galaxy and asteroid field header
#include "sprite_manager.h" // Ensure SpriteManager is included for safeDeleteSprite
#include <algorithm> // Add at the top with other includes
#include <FastLED.h> // Added for LED animations
//#include <psram.h> // Include for PSRAM allocation

// Define globals for quiz mode
bool quizActive = false;
#include <esp_sleep.h>
#include <driver/rtc_io.h>

// Include LED animations header
#include "led_animations.h"

// Power management defines
#define BUTTON_PIN 3   // Make sure this matches your actual button pin
#define LONG_PRESS_TIME 3000 // Time in milliseconds for a long press to power off
#define SHORT_PRESS_TIME 50  // Minimum time for a valid short press

// Power management variables
bool powerOffRequested = false;
unsigned long buttonPressStartTime = 0;

// Change the TFT_LED pin definition
#define TFT_LED 19  // Changed from 16 to 19
#define VIBRATION_PIN 26  // Vibration motor pin

// Haptic feedback levels
enum class HapticLevel {
  NONE,
  SLOW,
  FAST,
  WARP
};


// Potentiometer pin
//#define POT_PIN   35  // ADC1_CHANNEL_7 for ESP32
#define POT_PIN   7  // ADC1_CHANNEL_7 for ESP32s3

#define ST7735_GRAY 0x8410  // Medium gray in RGB565 format
//#define ST7735_GREEN 0x07E0  // Green color in RGB565 format

// Previous positions for nebula
#define MAX_NEBULA_CIRCLES 15
int prevNebulaX[MAX_NEBULA_CIRCLES], prevNebulaY[MAX_NEBULA_CIRCLES];
int prevNebulaR[MAX_NEBULA_CIRCLES];
int prevNebulaColors[MAX_NEBULA_CIRCLES];
int prevNebulaCount = 0;
bool nebulaInitialized = false; // Definition for nebula.h
bool solarSystemInitialized = false; // Definition for solar_system.h

// Previous positions for galaxy
int prevGalaxyCenterX, prevGalaxyCenterY;
int prevGalaxyCoreRadius;
#define MAX_GALAXY_ARMS 6  // Changed from 4 to 6 to match galaxy_and_asteroid.h
#define MAX_GALAXY_POINTS 50
int prevGalaxyPoints[MAX_GALAXY_ARMS][MAX_GALAXY_POINTS][2]; // [arm][point][x,y]
int prevGalaxyPointCount[MAX_GALAXY_ARMS];

// Previous positions for solar system
//int prevSunX, prevSunY, prevSunRadius;
//int prevOrbitRadii[4];
//int prevPlanetX[4], prevPlanetY[4], prevPlanetRadius[4];

// Shooting star parameters
#define MAX_SHOOTING_STARS 1
struct ShootingStar {
  float x, y;          // Current position
  float vx, vy;        // Velocity
  float length;        // Trail length
  bool active;         // Is it currently active?
  unsigned long startTime; // When it started
  unsigned long lifetime;  // How long it will last (ms)
};
ShootingStar shootingStars[MAX_SHOOTING_STARS];

// Asteroid field parameters
#define MAX_ASTEROIDS 15
Asteroid asteroids[MAX_ASTEROIDS];
bool asteroidFieldInitialized = false;

// Initialize TFT object
TFT_eSPI tft = TFT_eSPI();

// Display dimensions
const int SCREEN_WIDTH  = 240; // or whatever your width is
const int SCREEN_HEIGHT = 320;
int potValue = 0;  // Potentiometer value

// Starfield parameters
// constexpr int STAR_COUNT = 60; // Now defined in story_mode.h
Star stars[STAR_COUNT];

// Previous positions for streak erasure in warp mode
// constexpr int MAX_STREAK_LENGTH = 15; // Now defined in story_mode.h
uint16_t prevX[STAR_COUNT][MAX_STREAK_LENGTH + 1]; // Changed to uint16_t
uint16_t prevY[STAR_COUNT][MAX_STREAK_LENGTH + 1]; // Changed to uint16_t

// --- Color Scheme (Reverted to working version) ---
constexpr uint16_t COLOR_BG   = TFT_BLACK;   // Background
constexpr uint16_t COLOR_STAR = TFT_WHITE;   // Stars
uint16_t COLOR_TEXT;
uint16_t COLOR_TEXT_ALT;
uint16_t COLOR_TITLE_BG;
uint16_t COLOR_BORDER;
uint16_t COLOR_BORDER_ALT;
uint16_t COLOR_HIGHLIGHT;
uint16_t COLOR_GREEN;
uint16_t COLOR_MENU_GRID;
uint16_t COLOR_MENU_SEL;
uint16_t COLOR_MENU_BTN;
uint16_t COLOR_SHADOW;
uint16_t COLOR_THRUSTER;
uint16_t COLOR_EXHAUST;
uint16_t COLOR_STARFIELD;
uint16_t COLOR_SCANLINE;
uint16_t COLOR_READY;
uint16_t COLOR_ERROR;

uint16_t BG_COLOR = COLOR_BG;
constexpr uint16_t STAR_COLOR = COLOR_STAR;


// --- Color Variable Initialization ---
void initColors() {
  COLOR_TEXT       = tft.color565(180, 220, 255); // Main text
  COLOR_TEXT_ALT   = tft.color565(200, 255, 200); // Alt text (narration)
  COLOR_TITLE_BG   = tft.color565(0, 20, 50);     // Title bar
  COLOR_BORDER     = tft.color565(60, 80, 200);   // Border
  COLOR_BORDER_ALT = tft.color565(30, 50, 150);   // Alt border
  COLOR_HIGHLIGHT  = tft.color565(255, 255, 0);   // Highlight (yellow)
  COLOR_GREEN      = tft.color565(0, 255, 0);     // Green (instructions)
  COLOR_MENU_GRID  = tft.color565(20, 20, 40);    // Menu grid
  COLOR_MENU_SEL   = tft.color565(255, 255, 0);   // Menu selector
  COLOR_MENU_BTN   = tft.color565(0, 255, 0);     // Button text
  COLOR_SHADOW     = tft.color565(0, 0, 80);      // Shadow
  COLOR_THRUSTER   = tft.color565(255, 180, 0);   // Ship thruster
  COLOR_EXHAUST    = tft.color565(200, 100, 0);   // Ship exhaust
  COLOR_STARFIELD  = tft.color565(255, 255, 255); // Starfield white
  COLOR_SCANLINE   = tft.color565(30, 30, 30);    // Scanline effect
  COLOR_READY      = tft.color565(0, 100, 0);     // Success/ready message
  COLOR_ERROR      = tft.color565(255, 0, 0);     // Error/alert
}


// Warp drive state
float warpFactor = 0.0f;    // 0.0 (no warp) to 1.0 (full warp)
bool warpEnabled = false;   // Warp mode toggle
constexpr float MIN_WARP_SPEED = 0.5f; // Minimum speed to avoid static stars

// Haptic feedback override for special events
bool hapticOverrideActive = false;
float hapticOverrideValue = 0.0f;
unsigned long hapticOverrideEndTime = 0;

// Haptic feedback function that smoothly scales with warpFactor (0.0 - 1.0)
void hapticFeedback(float warpFactor) {
  static unsigned long lastPatternTime = 0;
  static unsigned long patternDuration = 0;
  static bool pulseState = false;
  static int substate = 0; // For turbulence/double-pulse
  unsigned long now = millis();

  // If warpFactor is very low, turn off vibration
  if (warpFactor < 0.02f) {
    digitalWrite(VIBRATION_PIN, LOW);
    pulseState = false;
    substate = 0;
    return;
  }

  // Interpolate ON/OFF durations based on warpFactor
  // At low warp: ON 20-40ms, OFF 400-300ms
  // At high warp: ON 200-500ms, OFF 40-20ms
  int onMin = map(warpFactor * 1000, 0, 1000, 20, 200);
  int onMax = map(warpFactor * 1000, 0, 1000, 40, 500);
  int offMin = map(warpFactor * 1000, 0, 1000, 400, 40);
  int offMax = map(warpFactor * 1000, 0, 1000, 300, 20);

  // As warpFactor increases, add more turbulence and surges
  float turbulenceChance = 0.0f;
  if (warpFactor > 0.3f) turbulenceChance = (warpFactor - 0.3f) * 1.1f; // up to ~0.8 at max

  if (now - lastPatternTime > patternDuration) {
    if (substate == 1 && !pulseState) {
      // Double pulse turbulence
      pulseState = true;
      patternDuration = random(onMin, onMax / 2);
      digitalWrite(VIBRATION_PIN, HIGH);
      substate = 0;
    } else {
      pulseState = !pulseState;
      if (pulseState) {
        // ON duration
        patternDuration = random(onMin, onMax);
        // At high warp, mostly ON with brief surges
        if (warpFactor > 0.7f && random(100) < 70) {
          // Long ON, simulate surge
          patternDuration += random(60, 160);
        }
        // Occasionally do a double pulse for turbulence
        if (random(1000) < (turbulenceChance * 1000)) {
          substate = 1;
        }
      } else {
        // OFF duration
        patternDuration = random(offMin, offMax);
        // At high warp, OFF is very brief
        if (warpFactor > 0.7f) patternDuration = random(20, 60);
      }
      digitalWrite(VIBRATION_PIN, pulseState ? HIGH : LOW);
    }
    lastPatternTime = now;
  }

}

// Frame timing
constexpr unsigned long TARGET_FRAME_MS = 33; // ~30 FPS

// State management
enum class State {
  NORMAL,
  WARP,
  DISCOVERY,
  MENU,
  QUIZ,
  STORY
};
State currentState = State::MENU; // Changed initial state to MENU

// Celestial objects
enum class CelestialObject {
  STAR,
  PLANET,
  NEBULA,
  GALAXY,
  SOLAR_SYSTEM,
  ASTEROID_FIELD,
  BLACK_HOLE,
  PULSAR,
  SUPERNOVA,
  COMET,     // Added Comet
  BINARY_STAR, // Added Binary Star System
  SPACE_STATION, // Added Space Station
  NUM_TYPES  // Keep this last
};
CelestialObject currentObject;
unsigned long discoveryStartTime;

// Object position and scale
int objectX, objectY;
float objectScale;

// Flag for state transitions and celestial object display
bool prevShouldWarp = false;
bool showingCelestialObject = false;

// Using structs and variables defined in blackhole.h
uint16_t prevPhotonRingColor;

// Utility function for easing
float easeInOutCubic(float t) {
  return t < 0.5f ? 4.0f * t * t * t : 1.0f - powf(-2.0f * t + 2.0f, 3) / 2.0f;
}

// Add this to the global variables section after the celestial object enum
bool objectsShown[static_cast<int>(CelestialObject::NUM_TYPES)] = {false}; // Track which objects have been shown
int objectsRemaining = static_cast<int>(CelestialObject::NUM_TYPES); // Count of objects not yet shown

// Menu system
#define MENU_ITEMS 3
struct MenuItem {
  const char* name;
  State state;
};

MenuItem menuItems[MENU_ITEMS] = {
  {"Discovery", State::DISCOVERY},
  {"Quiz", State::QUIZ},
  {"Story", State::STORY}
};
int currentMenuItem = 0;

// Forward declarations for functions that are used before they're defined
void typewriterText(const char* text, int delayMs);
void initializeAccretionParticle(int index, int centerX, int centerY);

// Forward declarations for LED functions
void setLedModeMenu(int currentSelection, int numItems);
void setLedModeQuiz(bool answerCorrect, bool waitingForAnswer);
void setLedModeOff();
void setLedModeStory();
void setLedModeWarp();

// Add new variable to track power state
bool isPoweredOn = true;  // Start powered on

// Add these after the display dimensions
constexpr float BASE_WIDTH = 240.0f;  // Base design width
constexpr float BASE_HEIGHT = 320.0f; // Base design height

// Global scaling factors
float scaleX = 1.0f;
float scaleY = 1.0f;
float scaleFactor = 1.0f;

// Global menu metrics for responsive drawing
int g_boxY = 0;
int g_boxWidth = 0;
int g_boxHeight = 0;
int g_menuTextSize = 0;
int g_charWidth = 0;

// Pulsar global variable definition
float prevAngle = 0.0f;

// Supernova global variable definition
int supernovaPhase = 0;

// Function to initialize scaling factors
void initializeScaling() {
  scaleX = SCREEN_WIDTH / BASE_WIDTH;
  scaleY = SCREEN_HEIGHT / BASE_HEIGHT;
  scaleFactor = std::min(scaleX, scaleY); // Use minimum to maintain aspect ratio
  
  // Pre-calculate some common scaled values for rendering
  int menuTextBufferSize = SCREEN_WIDTH / 6; // Approx number of chars that fit
  
  // Initialize global menu buffers based on screen size
  g_boxY = SCREEN_HEIGHT * 0.25;
  g_boxWidth = SCREEN_WIDTH * 0.4;
  g_boxHeight = SCREEN_HEIGHT * 0.08;
  
  float menuScale = SCREEN_WIDTH / 240.0f;
  g_menuTextSize = static_cast<int>(std::max(1.0f, round(menuScale)));
  g_charWidth = 6 * g_menuTextSize;
}

// Helper functions for scaling
inline int scaleX_i(int x) { return round(x * scaleX); }
inline int scaleY_i(int y) { return round(y * scaleY); }
inline float scaleX_f(float x) { return x * scaleX; }
inline float scaleY_f(float y) { return y * scaleY; }
inline int scale_i(int v) { return round(v * scaleFactor); }
inline float scale_f(float v) { return v * scaleFactor; }

// ---- START CHANGE ----
// Replace TFT_eSprite globals with SpriteHandles
// TFT_eSprite stationSprite = TFT_eSprite(&tft);  // Sprite for space station
// bool stationSpriteCreated = false;  // Flag to track if sprite is created
SpriteHandle stationHandle = {0}; // Initialize with invalid ID 0

// TFT_eSprite binaryStarSprite = TFT_eSprite(&tft);
// bool binaryStarSpriteCreated = false;
SpriteHandle binaryStarHandle = {0};

// Definitions for Quiz Mode sprites (were extern in quiz_mode.h)
// TFT_eSprite quizSprite = TFT_eSprite(&tft); // Definition for quizSprite
// bool quizSpriteCreated = false;            // Definition for quizSpriteCreated
SpriteHandle quizHandle = {0};
// ---- END CHANGE ----

// Create an instance of the StoryMode class
StoryMode storyModeManager; // Default constructor will be called

void setup() {
  pinMode(VIBRATION_PIN, OUTPUT);
  digitalWrite(VIBRATION_PIN, LOW); // Ensure vibration is off initially
  Serial.begin(9600);  // Move Serial.begin to top for debugging
  
  Serial.println("--- PSRAM CHECK START ---");
  #if defined(ESP32) && defined(BOARD_HAS_PSRAM)
  if (psramInit()) {
    Serial.printf("PSRAM initialized successfully. Total PSRAM: %u, Free PSRAM: %u\n", ESP.getPsramSize(), ESP.getFreePsram());
  } else {
    Serial.println("PSRAM initialization FAILED.");
  }
  #elif defined(ESP32)
  Serial.printf("Board might have PSRAM, but psramInit() not explicitly called or BOARD_HAS_PSRAM not defined. Free PSRAM: %u\n", ESP.getFreePsram());
  #else
  Serial.println("Not an ESP32 or PSRAM check not configured for this board.");
  #endif
  Serial.println("--- PSRAM CHECK END ---");

  // Configure pins
  pinMode(TFT_LED, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  
  // Check if this is a wake from deep sleep
  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
  
  if (wakeup_reason == ESP_SLEEP_WAKEUP_EXT0) {
    // We woke up from button press
    Serial.println("Waking from deep sleep");
    isPoweredOn = true;
    
    // Initialize display
    digitalWrite(TFT_LED, HIGH);
    tft.init();
    tft.setRotation(0);
    initColors();
    tft.fillScreen(COLOR_BG);

  
    // Show wake-up message
    tft.setTextColor(COLOR_GREEN);
    tft.setTextSize(1);
    tft.setCursor((SCREEN_WIDTH - tft.textWidth("POWERING ON...")) / 2, SCREEN_HEIGHT/2);
    tft.print("POWERING ON...");
    delay(1000);
    
    // Continue with normal initialization
    initializeSystem();
  } else {
    // Normal power-on
    Serial.println("Normal power-on");
    isPoweredOn = true;
    digitalWrite(TFT_LED, HIGH);
    initializeSystem();
  }

}

// Add this new function to handle system initialization
void initializeSystem() {
  initializeScaling(); // Initialize scaling factors first
  // tft.setAttribute(PSRAM_ENABLE, true); // <<<< Temporarily commented out for testing PSRAM issue
  Serial.printf("Inside initializeSystem, (global tft PSRAM_ENABLE is OFF for this test) - Free PSRAM: %u, Free Heap: %u\n", ESP.getFreePsram(), ESP.getFreeHeap());
  
  // Reset menu buffer data when system initializes
  g_boxY = SCREEN_HEIGHT * 0.25;
  g_boxWidth = SCREEN_WIDTH * 0.4;
  g_boxHeight = SCREEN_HEIGHT * 0.08;
  float menuScale = SCREEN_WIDTH / 240.0f;
  g_menuTextSize = static_cast<int>(std::max(1.0f, round(menuScale)));
  g_charWidth = 6 * g_menuTextSize;
  
  // Rest of initialization
  pinMode(VIBRATION_PIN, OUTPUT);
  digitalWrite(VIBRATION_PIN, LOW); // Ensure vibration is off at system init
  tft.init();
  tft.setRotation(0);
  initColors();
  tft.fillScreen(COLOR_BG);
  
  // Initialize LEDs
  setupLeds();
  
  // ---- START CHANGE ----
  // Initialize the new SpriteManager
  SpriteManager::begin();
  // ---- END CHANGE ----
  
  // Initialize potentiometer
  pinMode(POT_PIN, INPUT);
  randomSeed(analogRead(POT_PIN));
  
  // Initialize stars for background - Updated to use screen dimensions
  for (int i = 0; i < STAR_COUNT; i++) {
    stars[i].x = random(0, SCREEN_WIDTH);
    stars[i].y = random(0, SCREEN_HEIGHT);
    stars[i].realX = static_cast<float>(stars[i].x);
    stars[i].realY = static_cast<float>(stars[i].y);
    stars[i].brightness = random(150, 256);
    stars[i].increasing = random(0, 2);
    stars[i].streakLength = 0;
    drawStar(stars[i]);
  }
  
  // Initialize shooting stars
  initShootingStars();
  
  // Draw menu screen
  drawMenu();
  setLedModeMenu(currentMenuItem, MENU_ITEMS); // Initialize LED for menu
  
  // Initialize object tracking
  memset(objectsShown, false, sizeof(objectsShown));
  objectsRemaining = static_cast<int>(CelestialObject::NUM_TYPES);
  
  // Initialize tracking variables
  prevNebulaCount = 0;
  prevGalaxyCoreRadius = 0;
  for (int arm = 0; arm < MAX_GALAXY_ARMS; arm++) {
    prevGalaxyPointCount[arm] = 0;
  }
}

void loop() {
  // Haptic feedback: override for special events (supernova, black hole)
  if (hapticOverrideActive && millis() < hapticOverrideEndTime) {
    hapticFeedback(hapticOverrideValue);
  } else {
    hapticOverrideActive = false;
    if (currentState == State::WARP) {
      hapticFeedback(warpFactor);
    } else {
      hapticFeedback(0.0f);
    }
  }

  // Only process if powered on
  if (isPoweredOn) {
    checkPowerButton();
    
    if (powerOffRequested) {
      powerOff();
      return;
    }
    
    unsigned long frameStart = millis();
    
    readPotentiometer();
    
    // Update LED effects continuously
    updateLedEffects();
    
    // Handle different states
    switch (currentState) {
      case State::MENU:
        // Process menu navigation and selection
        processMenuInput();
        // Update background stars
        updateStars();
        // setLedModeMenu(currentMenuItem, MENU_ITEMS); // Already set when menu is drawn
        break;
        
      case State::QUIZ:
        // Handle quiz mode
        updateQuizMode();
        // LED mode for quiz is set within updateQuizMode
        break;
        
      case State::STORY:
        // Handle story mode
        {
            bool storyShouldExit = storyModeManager.update(potValue, digitalRead(BUTTON_PIN) == LOW);
            if (storyShouldExit) {
                // storyModeManager.exit() was already called internally by the update/processInput methods
                currentState = State::MENU;
                setLedModeMenu(currentMenuItem, MENU_ITEMS); // Set LED for menu
                tft.fillScreen(BG_COLOR); // Ensure screen is cleared before drawing menu
                drawMenu();
            } else {
                // Only update warpFactor if not exiting
                warpFactor = storyModeManager.getWarpFactor();
                setLedModeStory(); // Set LED for story mode
            }
        }
        break;
        
      case State::WARP:
        // Process warp input for WARP state
        processInput();
        // Update warp stars
        updateWarpStars();
        setLedModeWarp(); // Set LED for warp mode
        break;
             
             
      case State::NORMAL:
      
        // Process input for NORMAL and DISCOVERY states
        processInput();
        updateStars();

        break;


      case State::DISCOVERY:
      
        // Process input for NORMAL and DISCOVERY states
        processInput();
        
   
     
          updateStars(); // Ensure stars twinkle in discovery mode
   
        
        // Update shooting stars every frame as they are important for visual appeal
        updateShootingStars();
        
        // Only draw celestial objects if in discovery mode and object should be shown
        if (currentState == State::DISCOVERY && showingCelestialObject) {
            // Set LED mode for discovery with the current object name
            switch (currentObject) {
                case CelestialObject::STAR:
                    setLedModeDiscovery("Star");
                    break;
                case CelestialObject::PLANET:
                    setLedModeDiscovery("Planet");
                    break;
                case CelestialObject::NEBULA:
                    setLedModeDiscovery("Nebula");
                    break;
                case CelestialObject::GALAXY:
                    setLedModeDiscovery("Galaxy");
                    break;
                case CelestialObject::SOLAR_SYSTEM:
                    setLedModeDiscovery("Solar System");
                    break;
                case CelestialObject::ASTEROID_FIELD:
                    setLedModeDiscovery("Asteroid Field");
                    break;
                case CelestialObject::BLACK_HOLE:
                    setLedModeDiscovery("Black Hole");
                    break;
                case CelestialObject::PULSAR:
                    setLedModeDiscovery("Pulsar");
                    break;
                case CelestialObject::SUPERNOVA:
                    setLedModeDiscovery("Supernova");
                    break;
                case CelestialObject::COMET:
                    setLedModeDiscovery("Comet");
                    break;
                case CelestialObject::BINARY_STAR:
                    setLedModeDiscovery("Binary Star");
                    break;
                case CelestialObject::SPACE_STATION:
                    setLedModeDiscovery("Space Station");
                    break;
                default:
                    setLedModeDiscovery("");
                    break;
            }
            drawCelestialObject();
        } else if (currentState == State::DISCOVERY && !showingCelestialObject) {
            setLedModeDiscovery("None"); // Subtle breathing for empty space
        }
        

        break;
    }
    
    // Dynamic frame timing based on current state
    unsigned long frameTime = millis() - frameStart;
    unsigned long targetFrameTime;
    
    // Adjust target frame time based on current state to optimize performance
    if (currentState == State::WARP) {
      targetFrameTime = TARGET_FRAME_MS;  // Full framerate for warp effect
    } else if (currentState == State::DISCOVERY && showingCelestialObject) {
      targetFrameTime = TARGET_FRAME_MS + 5;  // Slightly lower framerate for complex objects
    } else {
      targetFrameTime = TARGET_FRAME_MS + 10; // Lower framerate for standard starfield
    }
    
    if (frameTime < targetFrameTime) {
      delay(targetFrameTime - frameTime);
    }
  } else {
    // When powered off, only check for button press
    checkPowerButton();
    delay(50);  // Prevent tight loop when off
  }
}

void readPotentiometer() {
  // Take multiple readings for stability
  int rawValue = 0;
  const int numReadings = 4;
  
  for (int i = 0; i < numReadings; i++) {
    // Invert the reading (4095 - value) to reverse the potentiometer direction
    rawValue += 4095 - analogRead(POT_PIN);
  }
  
  // Average the readings and store in potValue (0-4095 range for 12-bit ADC)
  potValue = rawValue / numReadings;
  

}

void processInput() {
  // Scale from 0-4095 to 0-1.0 for 12-bit ADC
  float rawWarpFactor = static_cast<float>(potValue) / 4095.0f;
  warpFactor = easeInOutCubic(rawWarpFactor);
  // Use a more precise threshold for 12-bit ADC (about 2.5% of full scale)
  bool shouldWarp = (potValue > 100);

  // Check for button press in DISCOVERY state to return to menu
  if (currentState == State::DISCOVERY) {
    static unsigned long lastButtonTime = 0;
    static bool buttonPressed = false;
    
    if (digitalRead(BUTTON_PIN) == LOW && !buttonPressed) {
      unsigned long currentTime = millis();
      if (currentTime - lastButtonTime > 300) { // Debounce
        buttonPressed = true;
        lastButtonTime = currentTime;
        
        // If showing a celestial object, erase it and clean up its sprite
        if (showingCelestialObject) {
          // Special handling for binary star
          if (currentObject == CelestialObject::BINARY_STAR) {
            #ifdef ESP32
            Serial.println("Extra cleanup for binary star before menu");
            #endif
            // Extra clear for binary star to remove any artifacts
            int clearSize = 160; // Large enough to cover any binary star
            tft.fillRect(objectX - clearSize/2, objectY - clearSize/2, clearSize, clearSize, BG_COLOR);
          }
          
          eraseCelestialObject(); // This will call deleteSprite for the specific object
          showingCelestialObject = false;
          
          // Force another cleanup and screen redraw
          cleanupAllCelestialObjectSprites();
          delay(10); // Small delay to allow memory management
        }
        
        // Return to menu with screen clear
        currentState = State::MENU;
        setLedModeMenu(currentMenuItem, MENU_ITEMS); // Set LED for menu
        tft.fillScreen(BG_COLOR);
        delay(10); // Small delay to ensure clean screen
        drawMenu();
        return;
      }
    } else if (digitalRead(BUTTON_PIN) == HIGH) {
      buttonPressed = false;
    }
  }

  // Fix transition logic for WARP mode
  if (shouldWarp && !prevShouldWarp) {
    // We're entering warp mode from another state
    Serial.println("Entering WARP mode");
    
    // If a celestial object was being shown in DISCOVERY, erase it and clean up sprites
    if (currentState == State::DISCOVERY && showingCelestialObject) {
      // Important: First erase and cleanup the current celestial object
      #ifdef ESP32
      Serial.println("Cleaning up before entering warp");
      #endif
      
      // In case of binary star, explicitly clean it up first with extra clearing
      if (currentObject == CelestialObject::BINARY_STAR && binaryStarHandle.id != 0) {
          #ifdef ESP32
          Serial.println("[Refactor] Extra screen clear for binary star before warp");
          #endif
          int clearSize = 160; // Large enough to cover any binary star
          tft.fillRect(objectX - clearSize/2, objectY - clearSize/2, clearSize, clearSize, BG_COLOR);
          // Sprite destruction handled by eraseCelestialObject -> cleanupAllCelestialObjectSprites below
      }
      
      // Now do the standard cleanup for all objects
      eraseCelestialObject(); // This should now use SpriteManager::destroy via cleanupAllCelestialObjectSprites
      showingCelestialObject = false;
      
      // Do one final check for any lingering sprites
      // cleanupAllCelestialObjectSprites(); // This is now called inside eraseCelestialObject potentially, or needs adjustment
      // Let's rely on eraseCelestialObject to handle its specific sprite.
      // A broader cleanup might be needed if eraseCelestialObject doesn't cover everything.
      // For now, assume eraseCelestialObject handles its own sprite.
      
      // Allow some time for memory operations to complete
      delay(20);
    }

    // Clean up any active shooting stars before entering warp mode
    for (int i = 0; i < MAX_SHOOTING_STARS; i++) {
      if (shootingStars[i].active) {
        // Erase the shooting star trail
        float oldX = shootingStars[i].x;
        float oldY = shootingStars[i].y;
        for (int j = 0; j < shootingStars[i].length; j++) {
          float trailX = oldX - j * shootingStars[i].vx / 2;
          float trailY = oldY - j * shootingStars[i].vy / 2;
          if (trailX >= 0 && trailX < SCREEN_WIDTH && trailY >= 0 && trailY < SCREEN_HEIGHT) {
            tft.drawPixel(trailX, trailY, BG_COLOR);
          }
        }
        // Deactivate the shooting star
        shootingStars[i].active = false;
      }
    }

    // Set the state to WARP with a clean screen
    currentState = State::WARP;
    tft.fillScreen(BG_COLOR); // Clear screen for warp effect
    delay(10); // Small delay to ensure screen is fully cleared
    
    // Initialize stars for proper warp effect
    // const float centerX = SCREEN_WIDTH / 2.0f; // No longer needed here
    // const float centerY = SCREEN_HEIGHT / 2.0f; // No longer needed here
    
    // [REMOVED] Loop that re-initialized all stars to radiate from center.
    // Stars will now continue from their current positions.
    // updateWarpStars() will handle their radial movement.

    // Clear any streak history for all stars, as they are entering warp
    for (int i = 0; i < STAR_COUNT; i++) {
      stars[i].streakLength = 0; // Reset streak length
      for (int j = 0; j <= MAX_STREAK_LENGTH; j++) {
        prevX[i][j] = SCREEN_WIDTH;  // Mark as invalid/off-screen
        prevY[i][j] = SCREEN_HEIGHT; // Mark as invalid/off-screen
      }
    }
  } 
  else if (!shouldWarp && prevShouldWarp) {
    // We're exiting warp mode
    Serial.println("Exiting WARP mode to DISCOVERY");
    
    setLedModeOff(); // Turn off LEDs or set to a discovery mode effect if desired
    // Transition from WARP to DISCOVERY
    currentState = State::DISCOVERY;

    // --- Clear warp streak buffers to prevent ghost artifacts ---
    for (int i = 0; i < STAR_COUNT; i++) {
      for (int j = 0; j <= MAX_STREAK_LENGTH; j++) {
        prevX[i][j] = SCREEN_WIDTH;  // Mark as invalid/off-screen
        prevY[i][j] = SCREEN_HEIGHT; // Mark as invalid/off-screen
      }
    }
    
    // --- Clear the entire screen to remove warp streaks cleanly ---
    tft.fillScreen(BG_COLOR);

    // Allow time for screen clearing operation
    delay(20); // Increased delay for better screen clearing
    
    // 4/5 probability to show celestial object (high chance)
    showingCelestialObject = (random(5) < 4);

    if (showingCelestialObject) {
      // IMPORTANT: Clean up any sprites from a previous discovery cycle BEFORE selecting/drawing a new one.
      // ---- START CHANGE ----
      // Use the new manager's destroyAll or specific destroy calls
      // cleanupAllCelestialObjectSprites(); 
      // Let's destroy specific known handles. A more robust system might track all discovery handles.
      SpriteManager::destroy(stationHandle); stationHandle = {0};
      SpriteManager::destroy(binaryStarHandle); binaryStarHandle = {0};
      // Add destroys for other sprites if they exist (nebula, planet etc)
      #ifdef ESP32
      Serial.println("[Refactor] Explicitly destroying known celestial handles before new discovery.");
      SpriteManager::dumpReport(); // See memory state before creating new
      #endif
      // ---- END CHANGE ----

      // Check if we need to reset our tracking (all objects have been shown)
      if (objectsRemaining == 0) {
        // Reset tracking for a new cycle
        memset(objectsShown, false, sizeof(objectsShown));
        objectsRemaining = static_cast<int>(CelestialObject::NUM_TYPES);
      }

      // Select an object that hasn't been shown yet
      int objectIndex;
      if (objectsRemaining > 0) {
        // Find an object that hasn't been shown yet
        do {
          objectIndex = random(0, static_cast<int>(CelestialObject::NUM_TYPES));
        } while (objectsShown[objectIndex]);

        // Mark this object as shown
        objectsShown[objectIndex] = true;
        objectsRemaining--;
      } else {
        // Fallback (shouldn't happen due to the reset above)
        objectIndex = random(0, static_cast<int>(CelestialObject::NUM_TYPES));
      }

      // Set the current object
      currentObject = static_cast<CelestialObject>(objectIndex);
      discoveryStartTime = millis();

      // Position celestial objects with appropriate location and scale
      if (currentObject == CelestialObject::BLACK_HOLE) {
        // Set position near the center with a small random offset
        int centerOffsetX = random(-80, 90); // Offset from -8 to +8 pixels
        int centerOffsetY = random(-80, 90); // Offset from -8 to +8 pixels
        objectX = SCREEN_WIDTH / 2 + centerOffsetX;
        objectY = SCREEN_HEIGHT / 2 + centerOffsetY;
        // Optional: You might want a slightly larger scale for black holes
        objectScale = random(150, 280) / 100.0f; 
        Serial.printf("Black Hole selected! Position: (%d, %d), Scale: %.2f\n", objectX, objectY, objectScale);
      } 
      else if (currentObject == CelestialObject::BINARY_STAR) {
        // Special handling for binary star - smaller scale to reduce memory usage
        // Position near center with smaller offsets for better rendering
        objectX = SCREEN_WIDTH / 2 + random(-30, 31);
        objectY = SCREEN_HEIGHT / 2 + random(-30, 31);
        
        // Calculate optimal scale based on screen dimensions and memory
        // Smaller scale for lower-memory situations
        #ifdef ESP32
        uint32_t freeHeap = ESP.getFreeHeap();
        uint32_t freePsram = ESP.getFreePsram();
        
        // Adjust scale based on available memory - smaller if memory is tight
        float memoryAdjustment = 1.0f;
        if (freePsram > 100000 || freeHeap > 50000) {
          memoryAdjustment = 1.0f; // Full scale if plenty of memory
        } else if (freePsram > 50000 || freeHeap > 30000) {
          memoryAdjustment = 0.9f; // 90% scale if moderate memory
        } else {
          memoryAdjustment = 0.8f; // 80% scale if low memory
        }
        
        objectScale = (random(120, 180) / 100.0f) * memoryAdjustment;
        Serial.printf("Binary Star selected! Memory status: PSRAM: %u, Heap: %u, Adjustment: %.2f\n", 
                      freePsram, freeHeap, memoryAdjustment);
        #else
        objectScale = random(120, 180) / 100.0f; // Even smaller scale for binary stars
        #endif
        
        Serial.printf("Binary Star selected! Position: (%d, %d), Scale: %.2f\n", 
                     objectX, objectY, objectScale);
        
        // Extra clear for binary star area to ensure no lingering artifacts
        int clearSize = 160; // Large enough to cover any binary star
        tft.fillRect(objectX - clearSize/2, objectY - clearSize/2, clearSize, clearSize, BG_COLOR);
        delay(10); // Small delay to ensure screen is fully cleared
      }
      else {
        // Default random positioning for all other objects
        objectX = random(20, SCREEN_WIDTH - 20);
        objectY = random(20, SCREEN_HEIGHT - 20);
        // Use the standard scale range for other objects
        objectScale = random(240, 350) / 100.0f; 
        Serial.printf("Object %d selected. Position: (%d, %d), Scale: %.2f\n", (int)currentObject, objectX, objectY, objectScale);
      }
      // ---- START CHANGE ----
      // Optional: Add a memory report after selecting an object, before drawing it
      #ifdef ESP32
      Serial.println("[Refactor] Memory status after object selection, before drawing:");
      SpriteManager::dumpReport();
      #endif
      // ---- END CHANGE ----
    } else {
       // No object selected this time
       Serial.println("No celestial object selected this cycle.");
    }
  } else if (currentState == State::DISCOVERY && !showingCelestialObject) {
    // If we are in DISCOVERY state but not showing an object (e.g., due to probability roll),
    // transition back to NORMAL state after a short delay or immediately.
    // This prevents staying indefinitely in DISCOVERY without an object.
    // Optional: add a timer here if you want a brief "empty space" discovery.
    // For now, let's transition back to NORMAL if warp isn't engaged again soon.
    if (!shouldWarp) { // If pot is still low
        // You could add a delay here, or just go back to NORMAL
        // currentState = State::NORMAL; // Uncomment if you want to explicitly go back to NORMAL
    }
  }
  prevShouldWarp = shouldWarp;
}

/**
 * Updates and renders stars in normal mode (non-warp)
 * This creates a gentle twinkling effect by randomly adjusting star brightness
 */
void updateStars() {
  for (int i = 0; i < STAR_COUNT; i++) {
    if (stars[i].x >= 0 && stars[i].x < SCREEN_WIDTH && 
        stars[i].y >= 0 && stars[i].y < SCREEN_HEIGHT) {
      tft.drawPixel(stars[i].x, stars[i].y, BG_COLOR);
    }
    
    if (random(0, 20) == 0) {
      int delta = random(1, 3);
      if (stars[i].increasing) {
        stars[i].brightness = std::min(stars[i].brightness + delta, 255);
        if (stars[i].brightness == 255) stars[i].increasing = false;
      } else {
        stars[i].brightness = std::max(stars[i].brightness - delta, 150);
        if (stars[i].brightness == 150) stars[i].increasing = true;
      }
    }
    
    if (stars[i].x < 0 || stars[i].x >= SCREEN_WIDTH || 
        stars[i].y < 0 || stars[i].y >= SCREEN_HEIGHT) {
      stars[i].x = random(SCREEN_WIDTH);
      stars[i].y = random(SCREEN_HEIGHT);
      // Add missing realX/realY updates for consistency
      stars[i].realX = static_cast<float>(stars[i].x);
      stars[i].realY = static_cast<float>(stars[i].y);
      stars[i].brightness = random(150, 256);
      stars[i].increasing = random(0, 2);
    }
    drawStar(stars[i]);
  }
}

/**
 * Draws a single star with specified brightness
 */
// Implementation moved to star.h

/**
 * Updates and renders stars in warp mode
 * Creates the iconic Star Trek warp effect with stars stretching based on distance from center
 */
void updateWarpStars() {
  const float centerX = SCREEN_WIDTH / 2.0f;
  const float centerY = SCREEN_HEIGHT / 2.0f;

  // 1. Clear previous streaks (from the frame before this one)
  // Optimized for 240x320 screen - only clear pixels that were actually drawn
  for (int i = 0; i < STAR_COUNT; i++) {
    for (int j = 0; j <= MAX_STREAK_LENGTH; j++) {
      uint16_t px = prevX[i][j];
      uint16_t py = prevY[i][j];
      // Only clear pixels that were valid in previous frame
      if (px < SCREEN_WIDTH && py < SCREEN_HEIGHT) {
        // Only clear if this pixel was actually drawn (not just marked)
        if (px != SCREEN_WIDTH && py != SCREEN_HEIGHT) {
          tft.drawPixel(px, py, BG_COLOR);
        }
        // Mark as invalid for next frame
        prevX[i][j] = SCREEN_WIDTH;
        prevY[i][j] = SCREEN_HEIGHT;
      }
    }
  }

  // Calculate scaled streak parameters
  float baseStreakLength = scale_f(MAX_STREAK_LENGTH);
  float baseSpeed = scale_f(3.0f);
  float minSpeed = scale_f(MIN_WARP_SPEED * 5.0f);

  for (int i = 0; i < STAR_COUNT; i++) {
    float dx = stars[i].realX - centerX;
    float dy = stars[i].realY - centerY;
    float distance = sqrtf(dx * dx + dy * dy);
    if (distance < 1.0f) distance = 1.0f;
    
    float dirX = dx / distance;
    float dirY = dy / distance;
    
    // Scale streak length with screen size
    int streakLength = static_cast<int>(warpFactor * 
                      std::min(distance / 2.0f, baseStreakLength));
    stars[i].streakLength = streakLength;
    
    // Draw streak with optimized bounds checking for 240x320
    for (int j = 0; j <= streakLength; j++) {
      int streakX = roundf(stars[i].realX + dirX * j);
      int streakY = roundf(stars[i].realY + dirY * j);
      
      // Only store positions that are actually on screen
      if (streakX >= 0 && streakX < SCREEN_WIDTH && 
          streakY >= 0 && streakY < SCREEN_HEIGHT) {
        if (j <= MAX_STREAK_LENGTH) {
          prevX[i][j] = streakX;
          prevY[i][j] = streakY;
        }
        
        uint8_t intensity = (streakLength > 0) ? 
                          (stars[i].brightness * (streakLength - j) / streakLength) : 
                          stars[i].brightness;
        uint16_t color = tft.color565(intensity, intensity, intensity);
        tft.drawPixel(streakX, streakY, color);
      } else if (j <= MAX_STREAK_LENGTH) {
        // Mark off-screen positions as invalid
        prevX[i][j] = SCREEN_WIDTH;
        prevY[i][j] = SCREEN_HEIGHT;
      }
    }
     // Clear any remaining part of the previous streak storage beyond the current streak length
     // This ensures old longer streaks are fully cleared when the current streak is shorter.
    for (int j = streakLength + 1; j <= MAX_STREAK_LENGTH; ++j) {
         prevX[i][j] = SCREEN_WIDTH; // Mark as invalid/off-screen
         prevY[i][j] = SCREEN_HEIGHT;
    }
    
    // Update star position with scaled speed
    float speed = (distance / 10.0f + 1.0f) * warpFactor * baseSpeed;
    speed = std::max(speed, minSpeed * warpFactor);
    
    stars[i].realX += dirX * speed;
    stars[i].realY += dirY * speed;
    
    // Convert to integer positions
    int newX = roundf(stars[i].realX);
    int newY = roundf(stars[i].realY);
    
    // 3. Reset stars that move off screen
    if (newX < 0 || newX >= SCREEN_WIDTH ||
        newY < 0 || newY >= SCREEN_HEIGHT) {

      // Erase the streak that was just drawn *before* resetting the star
      // This ensures no pixels are left behind when a star goes off-screen
      for (int j = 0; j <= MAX_STREAK_LENGTH; j++) { // Iterate through stored points
          uint16_t px = prevX[i][j];
          uint16_t py = prevY[i][j];
          // Strict bounds checking to prevent drawing outside screen
          if (px < SCREEN_WIDTH && py < SCREEN_HEIGHT) {
              tft.drawPixel(px, py, BG_COLOR);
          }
      }

      // Now reset the star's position
      float angle = random(0, 360) * PI / 180.0f;
      float maxRadius = sqrt((SCREEN_WIDTH / 2.0f) * (SCREEN_WIDTH / 2.0f) + (SCREEN_HEIGHT / 2.0f) * (SCREEN_HEIGHT / 2.0f));
      float distance = random(10, maxRadius); // Respawn within screen bounds
      stars[i].realX = centerX + cos(angle) * distance;
      stars[i].realY = centerY + sin(angle) * distance;
      stars[i].x = roundf(stars[i].realX);
      stars[i].y = roundf(stars[i].realY);
      stars[i].brightness = random(150, 256);
      
      // Clear the previous position buffer for the reset star
      // This is critical to prevent stray pixels from appearing
      for (int j = 0; j <= MAX_STREAK_LENGTH; j++) {
          prevX[i][j] = SCREEN_WIDTH;  // Mark as invalid/off-screen
          prevY[i][j] = SCREEN_HEIGHT; // Mark as invalid/off-screen
      }

    } else {
      // Star is still on screen
      stars[i].x = newX;
      stars[i].y = newY;
    }
  } // End of STAR_COUNT loop
}

/**
 * Initializes the shooting stars system
 */
void initShootingStars() {
  for (int i = 0; i < MAX_SHOOTING_STARS; i++) {
    shootingStars[i].active = false;
  }
}

/**
 * Updates and renders shooting stars across the star field
 * Randomly creates new shooting stars and manages their lifespan
 */
void updateShootingStars() {
  unsigned long currentTime = millis();
  
  // Randomly create new shooting stars
  if (random(100) < 1) { // 2% chance per frame
    for (int i = 0; i < MAX_SHOOTING_STARS; i++) {
      if (!shootingStars[i].active) {
        shootingStars[i].active = true;
        shootingStars[i].startTime = currentTime;
        shootingStars[i].lifetime = random(500, 1500);
        
        // Randomly choose one of four screen edges to start from
        int edge = random(4);
        switch (edge) {
          case 0: // top
            shootingStars[i].x = random(SCREEN_WIDTH);
            shootingStars[i].y = 0;
            break;
          case 1: // right
            shootingStars[i].x = SCREEN_WIDTH - 1;
            shootingStars[i].y = random(SCREEN_HEIGHT);
            break;
          case 2: // bottom
            shootingStars[i].x = random(SCREEN_WIDTH);
            shootingStars[i].y = SCREEN_HEIGHT - 1;
            break;
          case 3: // left
            shootingStars[i].x = 0;
            shootingStars[i].y = random(SCREEN_HEIGHT);
            break;
        }
        
        // Set target to somewhere near center with randomness
        float targetX = SCREEN_WIDTH / 2 + random(-20, 21);
        float targetY = SCREEN_HEIGHT / 2 + random(-20, 21);
        float dx = targetX - shootingStars[i].x;
        float dy = targetY - shootingStars[i].y;
        float dist = sqrt(dx*dx + dy*dy);
        float speed = random(2, 5) + random(0, 100) / 100.0f;
        shootingStars[i].vx = dx / dist * speed;
        shootingStars[i].vy = dy / dist * speed;
        shootingStars[i].length = random(5, 15);
        
        break;
      }
    }
  }
  
  // Update and draw active shooting stars
  for (int i = 0; i < MAX_SHOOTING_STARS; i++) {
    if (shootingStars[i].active) {
      // Erase previous position
      float oldX = shootingStars[i].x;
      float oldY = shootingStars[i].y;
      for (int j = 0; j < shootingStars[i].length; j++) {
        float trailX = oldX - j * shootingStars[i].vx / 2;
        float trailY = oldY - j * shootingStars[i].vy / 2;
        if (trailX >= 0 && trailX < SCREEN_WIDTH && trailY >= 0 && trailY < SCREEN_HEIGHT) {
          tft.drawPixel(trailX, trailY, BG_COLOR);
        }
      }
      
      // Update position
      shootingStars[i].x += shootingStars[i].vx;
      shootingStars[i].y += shootingStars[i].vy;
      
      // Check if it's off screen or expired
      if (shootingStars[i].x < 0 || shootingStars[i].x >= SCREEN_WIDTH ||
          shootingStars[i].y < 0 || shootingStars[i].y >= SCREEN_HEIGHT ||
          currentTime - shootingStars[i].startTime > shootingStars[i].lifetime) {
        shootingStars[i].active = false;
        continue;
      }
      
      // Draw at new position
      drawShootingStar(i);
    }
  }
}

/**
 * Draws a shooting star with a fading tail
 */
void drawShootingStar(int index) {
  ShootingStar& star = shootingStars[index];
  
  // Draw trail with fading brightness
  for (int j = 0; j < star.length; j++) {
    float trailX = star.x - j * star.vx / 2;
    float trailY = star.y - j * star.vy / 2;
    if (trailX >= 0 && trailX < SCREEN_WIDTH && trailY >= 0 && trailY < SCREEN_HEIGHT) {
      uint8_t brightness = map(j, 0, star.length - 1, 255, 50);
      uint16_t color = tft.color565(brightness, brightness, brightness);
      tft.drawPixel(trailX, trailY, color);
    }
  }
}

/**
 * Selects and draws the appropriate celestial object based on current selection
 */
void drawCelestialObject() {
  switch (currentObject) {
    case CelestialObject::STAR:
      drawStar();
      displayObjectName("STAR");
      break;
    case CelestialObject::PLANET:
      drawPlanet();
      displayObjectName("PLANET");
      break;
    case CelestialObject::NEBULA:
      drawNebula();
      displayObjectName("NEBULA");
      break;
    case CelestialObject::GALAXY:
      drawGalaxy();
      displayObjectName("GALAXY");
      break;
    case CelestialObject::SOLAR_SYSTEM:
      drawSolarSystem();
      displayObjectName("SOLAR SYSTEM");
      break;
    case CelestialObject::ASTEROID_FIELD:
      drawAsteroidField();
      displayObjectName("ASTEROID FIELD");
      break;
    case CelestialObject::BLACK_HOLE:
      drawBlackHole();
      displayObjectName("BLACK HOLE");
      break;
    case CelestialObject::PULSAR:
      drawPulsar();
      displayObjectName("PULSAR");
      break;
    case CelestialObject::SUPERNOVA:
      drawSupernova();
      displayObjectName("SUPERNOVA");
      break;
    case CelestialObject::COMET:
      drawComet();
      displayObjectName("COMET");
      break;
    case CelestialObject::BINARY_STAR:
      drawBinaryStar();
      displayObjectName("BINARY STAR");
      break;
    case CelestialObject::SPACE_STATION:
      drawSpaceStation();
      displayObjectName("SPACE STATION");
      break;
    default:
      break;
  }
}

/**
 * Erases the current celestial object by clearing its components
 */
void eraseCelestialObject() {
  // Display a message indicating the object is being erased
  tft.fillRect(0, SCREEN_HEIGHT - 10, SCREEN_WIDTH, 10, BG_COLOR); // Clear previous message

  switch (currentObject) {
    case CelestialObject::STAR:
      eraseStar();
      break;
    case CelestialObject::PLANET:
      erasePlanet();
      break;
    case CelestialObject::NEBULA:
      eraseNebula();
      break;
    case CelestialObject::GALAXY:
      eraseGalaxy();
      break;
    case CelestialObject::SOLAR_SYSTEM:
      eraseSolarSystem();
      break;
    case CelestialObject::ASTEROID_FIELD:
      eraseAsteroidField();
      asteroidFieldInitialized = false;
      break;
    case CelestialObject::BLACK_HOLE:
      eraseBlackHole();
      break;
    case CelestialObject::PULSAR:
      erasePulsar();
      break;
    case CelestialObject::SUPERNOVA:
      eraseSupernova();
      break;
    case CelestialObject::COMET:
      eraseComet();
      break;
    case CelestialObject::BINARY_STAR:
      eraseBinaryStar();
      break;
    case CelestialObject::SPACE_STATION:
      eraseSpaceStation();
      break;
  }
}

/**
 * Displays the name of the celestial object at the bottom of the screen
 */
void displayObjectName(const char* name) {
  // Set text properties
  tft.setTextSize(1);
  tft.setTextColor(COLOR_GREEN);
  
  // Center the text
  int16_t x1, y1;
  uint16_t w, h;
  w = tft.textWidth(name); // Use textWidth instead of getTextBounds
  h = 8; // Manually set height based on font size (assuming font size 1)
  int x = (SCREEN_WIDTH - w) / 2;
  
  // Draw the text at the bottom of the screen
  tft.setCursor(x, SCREEN_HEIGHT - 10);
  tft.print(name);
}

/**
 * Draws a retro-style intro screen with animation and waits for user input
 * Uses the main stars array for a seamless transition
 */
void drawIntroScreen() {
  // We're using the main stars array that's already initialized
  // Draw scanline effect for CRT look
  for (int y = 0; y < SCREEN_HEIGHT; y += 2) {
    tft.drawFastHLine(0, y, SCREEN_WIDTH, COLOR_SCANLINE);
  }
  
  // Draw WARP DRIVE title with shadow for 3D effect
  const int titleY = 20;
  // Shadow first
  tft.setTextColor(tft.color565(0, 0, 80));
  tft.setTextSize(2);
  tft.setCursor(12, titleY+1);
  tft.print("Cosmic");
  tft.setCursor(22, titleY+17);
  tft.print("Knobulator");
  
  // Then main text
  tft.setTextColor(tft.color565(80, 200, 255)); // Bright cyan
  tft.setCursor(10, titleY);
  tft.print("Cosmic");
  tft.setCursor(20, titleY+16);
  tft.print("Knobulator");
  
  // Animate a spaceship
  drawAnimatedSpaceship();
  
  // Draw "ESP8266" subtitle with proper positioning
  tft.setTextSize(1);
  tft.setTextColor(COLOR_GREEN);
  tft.setCursor(28, titleY + 48);
  typewriterText("For Mahira <3", 40);
  
  // Draw interface instructions with proper positioning
  tft.setTextColor(COLOR_HIGHLIGHT);
  tft.setCursor(10, 85);
  typewriterText("TURN KNOB FOR WARP", 20);
  
  // Draw pixelated loading bar
  const int barWidth = 100;
  const int barHeight = 4;
  const int barX = (SCREEN_WIDTH - barWidth) / 2;
  const int barY = 100;
  
  // Bar outline
  tft.drawRect(barX-1, barY-1, barWidth+2, barHeight+2, tft.color565(80, 80, 80));
  
  // "READY" blink
  for (int i = 0; i < 2; i++) {
    tft.setTextColor(COLOR_STARFIELD);
    tft.setCursor(50, 110);
    tft.print("READY");
    delay(400);
    tft.setTextColor(BG_COLOR);
    tft.setCursor(50, 110);
    tft.print("READY");
    delay(200);
  }
  tft.setTextColor(COLOR_STARFIELD);
  tft.setCursor(50, 110);
  tft.print("READY");
  
  // Wait for user input before proceeding
  unsigned long lastStarUpdateTime = millis();
  bool inputDetected = false;
  
  // Loop until input is detected
  while (!inputDetected) {
    // Read potentiometer value
    int rawValue = 0;
    for (int i = 0; i < 4; i++) {
      rawValue += analogRead(POT_PIN);
    }
    int currentPotValue = rawValue / 8; // 0-1023
 
    // Check if potentiometer has been turned (value > threshold)
    if (currentPotValue < 1900) {
      inputDetected = true;
      // Set the global potValue for use in main loop
      potValue = currentPotValue;
      
    }
    
    // Update twinkling stars while waiting (similar to updateStars function)
    unsigned long currentTime = millis();
    if (currentTime - lastStarUpdateTime > 100) { // Update every 100ms
      lastStarUpdateTime = currentTime;
      
      // Use the main updateStars function to keep the starfield consistent
      updateStars();
    }
    
    // Small delay to prevent CPU hogging
    delay(10);
  }
  
  // Clear only the text and UI elements, leaving the starfield intact
  // Clear title area
  tft.fillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, BG_COLOR);

  
  // Set initial state to WARP to skip NORMAL state
  currentState = State::WARP;
}



/**
 * Draws an animated pixel art spaceship with thruster effect
 */
void drawAnimatedSpaceship() {
  const int shipWidth = 15;
  const int shipHeight = 8; 
  const int shipY = 55;
  
  // Draw animated ship
  for (int frame = 0; frame < 12; frame++) {
    // Calculate position - ship moves slightly in a wave pattern
    int offsetY = sin(frame * 0.5) * 2;
    int shipX = 20 + frame * 7;
    
    if (frame > 0) {
      // Erase previous ship position
      tft.fillRect(shipX - 7, shipY - 5 + offsetY, shipWidth + 8, shipHeight + 6, BG_COLOR);
    }
    
    // Draw ship body - a triangular shape
    uint16_t shipColor = tft.color565(200, 200, 255);
    tft.fillTriangle(
      shipX, shipY + offsetY,
      shipX + shipWidth, shipY + shipHeight/2 + offsetY,
      shipX, shipY + shipHeight + offsetY,
      shipColor
    );
    
    // Ship outline
    tft.drawLine(
      shipX, shipY + offsetY,
      shipX + shipWidth, shipY + shipHeight/2 + offsetY,
      COLOR_STARFIELD
    );
    tft.drawLine(
      shipX, shipY + shipHeight + offsetY,
      shipX + shipWidth, shipY + shipHeight/2 + offsetY,
      COLOR_STARFIELD
    );
    
    // Cockpit window
    tft.fillRect(
      shipX + shipWidth - 5, shipY + shipHeight/2 - 1 + offsetY,
      3, 2,
      COLOR_TEXT_ALT
    );
    
    // Draw engine glow - changes size and color to animate
    uint8_t thrusterSize = 2 + (frame % 3);
    uint8_t thrusterBrightness = 180 + random(-20, 50);
    
    tft.fillRect(
      shipX - thrusterSize, shipY + shipHeight/2 - thrusterSize/2 + offsetY,
      thrusterSize, thrusterSize,
      tft.color565(thrusterBrightness, thrusterBrightness/2, 0)
    );
    
    // Draw a small exhaust trail
    for (int i = 1; i <= 5; i++) {
      uint8_t exhaustBrightness = std::max(0, 150 - i * 30);
      tft.drawPixel(
        shipX - thrusterSize - i - random(0, 2), 
        shipY + shipHeight/2 + random(-1, 2) + offsetY,
        tft.color565(exhaustBrightness, exhaustBrightness/3, 0)
      );
    }
    
    delay(80);
  }
}

/**
 * Prints text with a typewriter effect
 */
void typewriterText(const char* text, int delayMs) {
  for (int i = 0; i < strlen(text); i++) {
    tft.print(text[i]);
    delay(delayMs);
  }
}

/**
 * Checks the power button state and handles long press for power off
 */
void checkPowerButton() {
  static bool buttonState = HIGH;
  static bool lastButtonState = HIGH;
  static unsigned long lastDebounceTime = 0;
  static unsigned long pressStartTime = 0;
  const unsigned long debounceDelay = 50;
  
  // Read button with debounce
  int reading = digitalRead(BUTTON_PIN);
  
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }
  
  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != buttonState) {
      buttonState = reading;
      
      // Button press started
      if (buttonState == LOW) {
        pressStartTime = millis();
        Serial.println("Button pressed");
      }
      // Button released
      else {
        unsigned long pressDuration = millis() - pressStartTime;
        Serial.printf("Button released after %lu ms\n", pressDuration);
        
        // Long press detected while powered on
        if (pressDuration >= LONG_PRESS_TIME && isPoweredOn) {
          Serial.println("Long press detected - powering off");
          isPoweredOn = false;
          powerOffRequested = true;
        }
        
        pressStartTime = 0;
      }
    }
  }
  
  lastButtonState = reading;
}

/**
 * Powers off the device by pulling the EN pin low
 */
void powerOff() {
  Serial.println("Entering deep sleep");
  
  // Show power off message
  tft.fillScreen(COLOR_BG);
  tft.setTextColor(COLOR_ERROR);
  tft.setTextSize(1);
  tft.setCursor((SCREEN_WIDTH - tft.textWidth("POWERING OFF...")) / 2, SCREEN_HEIGHT/2);
  tft.print("POWERING OFF...");
  delay(1000);
  
  // Turn off display
  digitalWrite(TFT_LED, LOW);
  
  // Configure wake-up source
  esp_sleep_enable_ext0_wakeup(static_cast<gpio_num_t>(BUTTON_PIN), LOW);
  
  // Make sure the button pin is configured for wake-up
  // This is crucial for ESP32
  rtc_gpio_pullup_en(static_cast<gpio_num_t>(BUTTON_PIN));
  rtc_gpio_pulldown_dis(static_cast<gpio_num_t>(BUTTON_PIN));
  
  // Wait for button to be released
  while (digitalRead(BUTTON_PIN) == LOW) {
    delay(10);
  }
  delay(100); // Additional debounce delay
  
  // Enter deep sleep
  esp_deep_sleep_start();
}

/**
 * Draws the main menu interface with the menu items
 */
void drawMenu() {
  tft.fillScreen(BG_COLOR);

  // Draw retro grid background
  const int gridSize = 16;
  for (int x = 0; x < SCREEN_WIDTH; x += gridSize) {
    for (int y = 0; y < SCREEN_HEIGHT; y += gridSize) {
      tft.drawRect(x, y, gridSize, gridSize, tft.color565(0, 20, 40));
    }
  }

  // Add scanline effect for CRT look
  for (int y = 0; y < SCREEN_HEIGHT; y += 3) {
    tft.drawFastHLine(0, y, SCREEN_WIDTH, tft.color565(0, 0, 20));
  }

  // Draw title with enhanced size and retro effects
  // First draw the decorative lines
  int titleY = SCREEN_HEIGHT * 0.15;
  int lineY1 = titleY - 40;
  int lineY2 = titleY + 45;
  
  // Draw horizontal decorative lines
  for(int i = 0; i < 3; i++) {
    tft.drawFastHLine(20, lineY1 + i, SCREEN_WIDTH - 40, tft.color565(0, 100 + i*50, 200 + i*20));
    tft.drawFastHLine(20, lineY2 + i, SCREEN_WIDTH - 40, tft.color565(0, 100 + i*50, 200 + i*20));
  }

  // Draw title shadow with multiple layers for glow effect
  tft.setTextSize(3);  // Increased from 2 to 3
  tft.setTextDatum(MC_DATUM);
  
  // Draw multiple shadow layers for glow effect
  for(int i = 3; i > 0; i--) {
    tft.setTextColor(tft.color565(0, 60 - i*15, 120 - i*20));
    tft.drawString("WARP", SCREEN_WIDTH/2 + i, titleY - 15 + i);
    tft.drawString("DRIVE", SCREEN_WIDTH/2 + i, titleY + 15 + i);
  }
  
  // Draw main title text
  tft.setTextColor(tft.color565(0, 200, 255));
  tft.drawString("WARP", SCREEN_WIDTH/2, titleY - 15);
  tft.drawString("DRIVE", SCREEN_WIDTH/2, titleY + 15);

  // Add pixel-style underline for title
  for(int i = 0; i < 2; i++) {
    tft.drawFastHLine(SCREEN_WIDTH/2 - 100 + i*2, titleY + 35 + i, 200 - i*4, tft.color565(0, 200 - i*50, 255 - i*50));
  }

  // Menu layout parameters - adjusted to be lower on screen
  int baseCardWidth = SCREEN_WIDTH * 0.75;
  int baseCardHeight = SCREEN_HEIGHT * 0.11;  // Slightly smaller
  int cardSpacing = SCREEN_HEIGHT * 0.05;    // Slightly reduced spacing
  int totalHeight = MENU_ITEMS * baseCardHeight + (MENU_ITEMS - 1) * cardSpacing;
  int startY = (SCREEN_HEIGHT - totalHeight) / 2 + 50;  // Moved down further

  // Draw each menu item with retro styling
  for (int i = 0; i < MENU_ITEMS; i++) {
    float scaleFactor = (i == currentMenuItem) ? 1.15f : 1.0f;
    int cardWidth = baseCardWidth * scaleFactor;
    int cardHeight = baseCardHeight * scaleFactor;
    int cardX = (SCREEN_WIDTH - cardWidth) / 2;
    int cardY = startY + i * (baseCardHeight + cardSpacing);
    
    if (i == currentMenuItem) {
      cardY = startY + i * (baseCardHeight + cardSpacing) - (cardHeight-baseCardHeight)/2;
      
      // Draw retro selection arrows
      int arrowSize = 10;
      for (int j = 0; j < 3; j++) {
        tft.fillTriangle(
          cardX - 15 - j*4, cardY + cardHeight/2,
          cardX - 5 - j*4, cardY + cardHeight/2 - arrowSize,
          cardX - 5 - j*4, cardY + cardHeight/2 + arrowSize,
          tft.color565(255, 255, j * 85)
        );
        
        tft.fillTriangle(
          cardX + cardWidth + 15 + j*4, cardY + cardHeight/2,
          cardX + cardWidth + 5 + j*4, cardY + cardHeight/2 - arrowSize,
          cardX + cardWidth + 5 + j*4, cardY + cardHeight/2 + arrowSize,
          tft.color565(255, 255, j * 85)
        );
      }

      // Selected item style
      tft.fillRect(cardX + 4, cardY + 4, cardWidth - 8, cardHeight - 8, tft.color565(0, 40, 80));
      tft.drawRect(cardX + 2, cardY + 2, cardWidth - 4, cardHeight - 4, tft.color565(0, 160, 255));
      tft.drawRect(cardX, cardY, cardWidth, cardHeight, COLOR_HIGHLIGHT);
    } else {
      // Unselected item style
      tft.fillRect(cardX, cardY, cardWidth, cardHeight, tft.color565(0, 20, 40));
      tft.drawRect(cardX, cardY, cardWidth, cardHeight, tft.color565(0, 80, 160));
    }

    // Draw 8-bit style icons
    int iconX = cardX + cardHeight / 2;
    int iconY = cardY + cardHeight / 2;
    int iconSize = cardHeight / 3;
    
    switch (i) {
      case 0: // Discovery - pixelated planet
        tft.fillCircle(iconX, iconY, iconSize, tft.color565(0, 200, 255));
        tft.fillCircle(iconX - iconSize/3, iconY - iconSize/3, iconSize/3, tft.color565(0, 255, 255));
        break;
      case 1: // Quiz - pixelated question mark
        tft.fillRect(iconX - iconSize/2, iconY - iconSize/2, iconSize, iconSize, tft.color565(255, 220, 0));
        tft.setTextColor(tft.color565(0, 0, 0));
        tft.setTextSize(2);
        tft.drawChar(iconX - iconSize/3, iconY - iconSize/2, '?', tft.color565(0, 0, 0), tft.color565(255, 220, 0), 1);
        break;
      case 2: // Story - pixelated book
        for(int j = 0; j < 3; j++) {
          tft.drawRect(iconX - iconSize/2 + j*2, iconY - iconSize/2 + j*2, 
                      iconSize - j*4, iconSize - j*2, 
                      tft.color565(255 - j*40, 100 - j*20, 100 - j*20));
        }
        break;
    }

    // Draw menu text with retro style
    tft.setTextDatum(MC_DATUM);
    if (i == currentMenuItem) {
      // Selected text with glow effect
      tft.setTextColor(tft.color565(100, 200, 255));
      tft.setTextSize(2);
      tft.drawString(menuItems[i].name, cardX + cardWidth/2 + 1, cardY + cardHeight/2 + 1);
      tft.setTextColor(COLOR_HIGHLIGHT);
      tft.drawString(menuItems[i].name, cardX + cardWidth/2, cardY + cardHeight/2);
    } else {
      // Unselected text
      tft.setTextColor(COLOR_TEXT);
      tft.setTextSize(2);
      tft.drawString(menuItems[i].name, cardX + cardWidth/2, cardY + cardHeight/2);
    }
  }

  // Draw retro footer with scanlines
  int footerHeight = SCREEN_HEIGHT * 0.08;
  int footerY = SCREEN_HEIGHT - footerHeight;
  
  // Footer background with scanlines
  tft.fillRect(0, footerY, SCREEN_WIDTH, footerHeight, tft.color565(0, 20, 40));
  for (int y = footerY; y < SCREEN_HEIGHT; y += 2) {
    tft.drawFastHLine(0, y, SCREEN_WIDTH, tft.color565(0, 30, 60));
  }

  // Footer text with pixel-style border
  tft.drawRect(0, footerY, SCREEN_WIDTH, footerHeight, tft.color565(0, 100, 200));
  tft.setTextColor(COLOR_GREEN);
  tft.setTextSize(1);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("KNOB: Select  |  BTN: Go", SCREEN_WIDTH/2, footerY + footerHeight/2);
}

/**
 * Handles menu navigation and selection
 */
void processMenuInput() {
  static int lastMenuItem = currentMenuItem;
  static unsigned long lastButtonTime = 0;
  static bool buttonPressed = false;
  
  // Map pot value (0-4095) directly to menu selection
  int potRange = 4095 / MENU_ITEMS;
  int mappedItem = potValue / potRange;
  mappedItem = constrain(mappedItem, 0, MENU_ITEMS - 1);
  
  // Only update if selection changed
  if (mappedItem != lastMenuItem) {
    currentMenuItem = mappedItem;
    setLedModeMenu(currentMenuItem, MENU_ITEMS); // Update LED for menu selection change
    
    // Haptic feedback for menu navigation
    extern bool hapticOverrideActive;
    extern float hapticOverrideValue;
    extern unsigned long hapticOverrideEndTime;
    hapticOverrideActive = true;
    hapticOverrideValue = 0.3f; // Menu nav intensity
    hapticOverrideEndTime = millis() + 100; // 100ms burst
    
    // --- NEW: Redraw the entire menu for correct highlight ---
    drawMenu();
    
    lastMenuItem = currentMenuItem;
  }
  
  // Check button press for selection
  if (digitalRead(BUTTON_PIN) == LOW && !buttonPressed) {
    unsigned long currentTime = millis();
    if (currentTime - lastButtonTime > 300) { // Debounce
      buttonPressed = true;
      lastButtonTime = currentTime;
      
      // Quick visual feedback using correct metrics
      tft.fillRect(SCREEN_WIDTH/2 - g_boxWidth/2, g_boxY, 
                  g_boxWidth, g_boxHeight, COLOR_STARFIELD);
      delay(80);
      tft.fillRect(SCREEN_WIDTH/2 - g_boxWidth/2, g_boxY, 
                  g_boxWidth, g_boxHeight, tft.color565(80, 0, 120));
      
      // Draw highlighted text with proper scaling
      tft.setTextSize(g_menuTextSize);
      tft.setTextColor(COLOR_HIGHLIGHT);
      
      int textLen = strlen(menuItems[currentMenuItem].name);
      int textPixelWidth = textLen * g_charWidth;
      int textX = SCREEN_WIDTH/2 - textPixelWidth/2;
      int textY = g_boxY + g_boxHeight/2 - 4*g_menuTextSize;
      
      tft.setCursor(textX, textY);
      tft.print(menuItems[currentMenuItem].name);
      delay(50);
      
      // Switch to the selected state
      State selectedState = menuItems[currentMenuItem].state;
      
      // Clear the screen
      tft.fillScreen(BG_COLOR);
      
      // Special handling for each state
      if (selectedState == State::DISCOVERY) {
        // For Discovery mode, transition to WARP first to trigger discovery
        currentState = State::WARP;
        setLedModeOff(); // Or a specific warp LED effect
        prevShouldWarp = true; // Set this so next update transitions to DISCOVERY
      } else if (selectedState == State::STORY) {
        storyModeManager.init(); // Initialize story mode using the class manager
        currentState = selectedState;
        setLedModeOff(); // Or a specific story LED effect
      } else if (selectedState == State::QUIZ) {
        currentState = selectedState;
        // LED mode for quiz will be set in updateQuizMode / startQuiz
        // setLedModeQuiz(false, true); // Initial state for quiz LEDs
      } else {
        currentState = selectedState;
        setLedModeOff(); // Default off for other states for now
      }
    }
  } else if (digitalRead(BUTTON_PIN) == HIGH) {
    buttonPressed = false;
  }
}

/**
 * Placeholder function for Quiz mode
 */
#include "quiz_mode.h"

// --- Quiz Mode State Enum ---
enum QuizPhase { QUIZ_QUESTION, QUIZ_RESULT_POPUP };
static QuizPhase quizPhase = QUIZ_QUESTION;
static bool quizInitialized = false;
static unsigned long lastButtonTime = 0;
static bool buttonPressed = false;

extern QuizState quizState;

void updateQuizMode() {
  // Initialize quiz when entering QUIZ mode
  if (!quizInitialized) {
    startQuiz();
    quizPhase = QUIZ_QUESTION;
    quizInitialized = true;
    setLedModeQuiz(false, true); // Initial state for quiz LEDs: no answer yet, waiting for answer
  }

  // Read button
  bool btn = (digitalRead(BUTTON_PIN) == LOW);
  bool btnEvent = false;
  unsigned long now = millis();
  if (btn && !buttonPressed && now - lastButtonTime > 300) {
    buttonPressed = true;
    lastButtonTime = now;
    btnEvent = true;
  } else if (!btn) {
    buttonPressed = false;
  }

  // --- Quiz Phase State Machine ---

  // 1. Handle pop-up if active (block all other input)
  if (quizPopupState.active) {
    int popupResult = processQuizPopupInput(btnEvent);
    if (popupResult == 1) { // Left button: Try Again or Next
      uiInitialized = false; // Reset UI flag for next question/redraw
      if (quizState.answeredCorrectly) {
        // Next Question
        setupQuizOptions();
        quizState.answeredCorrectly = false;
        quizState.showHint = false;
        setLedModeQuiz(false, true); // Reset for next question
        updateQuiz();
      } else {
        // Try Again: just redraw quiz
        quizState.answeredCorrectly = false;
        quizState.showHint = false;
        setLedModeQuiz(false, true); // Reset for try again
        updateQuiz();
      }
    } else if (popupResult == 2) { // Right button: Menu
      uiInitialized = false; // Reset UI flag before leaving quiz
      quizInitialized = false;
      quizPhase = QUIZ_QUESTION;
      quizPopupState.active = false;
      if (quizHandle.id != 0) { // Use handle to check validity
          SpriteManager::destroy(quizHandle);
          quizHandle = {0}; // Invalidate handle
          Serial.println("[Quiz] Quiz sprite destroyed (popup menu exit).");
      }
      tft.fillScreen(BG_COLOR);
      currentState = State::MENU;
      setLedModeMenu(currentMenuItem, MENU_ITEMS); // Set LED for menu
      drawMenu();
      return;
    }
    // If popup still active, redraw it
    if (quizPopupState.active) updateQuizPopup();
    return; // Block all other quiz logic while popup is up
  }

  // 2. If no pop-up, handle quiz input and display
  switch (quizPhase) {
    case QUIZ_QUESTION:
      processQuizInput(potValue, btnEvent);
      updateQuiz();
      updateStars();
      // If answered, show popup
      if (quizState.answeredCorrectly) {
        showQuizPopup(true); // Correct
        setLedModeQuiz(true, false); // Correct answer, not waiting
      } else if (quizState.showHint) {
        showQuizPopup(false); // Wrong
        setLedModeQuiz(false, false); // Incorrect answer, not waiting
      }
      break;
    case QUIZ_RESULT_POPUP:
      // (Unused, all handled in QUIZ_QUESTION for simplicity)
      break;
  }

  // 3. Only allow returning to menu if user long-presses button and pop-up is NOT active
  static unsigned long pressStart = 0;
  if (!quizPopupState.active && btn) {
    if (pressStart == 0) pressStart = now;
    if (now - pressStart > 1500) { // Long press
      quizInitialized = false;
      quizPhase = QUIZ_QUESTION;
      quizPopupState.active = false;
      if (quizHandle.id != 0) { // Use handle to check validity
          SpriteManager::destroy(quizHandle);
          quizHandle = {0}; // Invalidate handle
          Serial.println("[Quiz] Quiz sprite destroyed (long press exit).");
      }
      tft.fillScreen(BG_COLOR);
      currentState = State::MENU;
      setLedModeMenu(currentMenuItem, MENU_ITEMS); // Set LED for menu
      drawMenu();
      pressStart = 0;
      return;
    }
  } else if (!btn) {
    pressStart = 0;
  }
}

// Helper functions to extract 8-bit R, G, B from 16-bit TFT_eSPI color
uint8_t extract_red(uint16_t color) {
    return (color & 0xF800) >> 8; // Extract red component (5 bits shifted)
}
uint8_t extract_green(uint16_t color) {
    return (color & 0x07E0) >> 3; // Extract green component (6 bits shifted)
}
uint8_t extract_blue(uint16_t color) {
    return (color & 0x001F) << 3; // Extract blue component (5 bits shifted)
}

// Function to clean up all celestial object sprites
void cleanupAllCelestialObjectSprites() {
  #ifdef ESP32
  Serial.println("[Refactor] Cleaning up specific celestial object handles.");
  Serial.printf("Before cleanup - Heap: %u, PSRAM: %u\n", ESP.getFreeHeap(), ESP.getFreePsram());
  SpriteManager::dumpReport(); // Show detailed sprite status
  #endif
  
  // ---- START CHANGE ----
  // Use SpriteManager::destroy for known handles instead of flags
  /* Old logic:
  if (stationSpriteCreated) {
    SpriteManager::safeDeleteSprite(stationSprite, "SpaceStation");
    stationSpriteCreated = false;
    #ifdef ESP32
    Serial.println("Cleaned up station sprite.");
    #endif
  }
  if (binaryStarSpriteCreated) {
    SpriteManager::safeDeleteSprite(binaryStarSprite, "BinaryStar");
    binaryStarSpriteCreated = false;
    #ifdef ESP32
    Serial.println("Cleaned up binary star sprite.");
    #endif
  }
  */
  SpriteManager::destroy(stationHandle);
  stationHandle = {0};
  SpriteManager::destroy(binaryStarHandle);
  binaryStarHandle = {0};
  // Add similar SpriteManager::destroy calls for other celestial object handles if/when they are added
  // (e.g., nebulaHandle, planetHandle, etc.)
  // Alternatively, if you know all active sprites *should* be celestial objects,
  // you could consider using SpriteManager::destroyAll(), but be careful if other
  // non-celestial sprites might exist.
  
  // ---- END CHANGE ----
  
  #ifdef ESP32
  Serial.println("[Refactor] After cleanup - Memory Status:");
  SpriteManager::dumpReport(); // Show status after cleanup
  #endif
  
  // Log memory status but don't restart
  #ifdef ESP32
  // Check using SpriteManager's report or direct ESP calls
  // This check might be less necessary now with the manager handling memory better
  if (ESP.getFreeHeap() < 20000 || ESP.getFreePsram() < 100000) { 
    Serial.println("WARN: Memory low after cleanup - check SpriteManager report for details.");
    delay(50); // Small delay to allow Serial to complete
  }
  #endif
}

