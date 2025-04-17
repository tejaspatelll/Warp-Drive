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

#include <TFT_eSPI.h> // Replace Adafruit_GFX and Adafruit_ST7735
#include <SPI.h>
#include "blackhole.h"
#include "pulsar.h" // Include the pulsar header file
#include "supernova.h" // Include the supernova header file
#include "comet.h" // Include the comet header file
#include "star.h"
#include <esp_sleep.h>
#include <driver/rtc_io.h>

// Power management defines
#define BUTTON_PIN 15   // Make sure this matches your actual button pin
#define LONG_PRESS_TIME 3000 // Time in milliseconds for a long press to power off
#define SHORT_PRESS_TIME 50  // Minimum time for a valid short press

// Power management variables
bool powerOffRequested = false;
unsigned long buttonPressStartTime = 0;

// Change the TFT_LED pin definition
#define TFT_LED 19  // Changed from 16 to 19

// Potentiometer pin
#define POT_PIN   35  // ADC1_CHANNEL_7 for ESP32
#define ST7735_GRAY 0x8410  // Medium gray in RGB565 format
//#define ST7735_GREEN 0x07E0  // Green color in RGB565 format

// Previous positions for nebula
#define MAX_NEBULA_CIRCLES 15
int prevNebulaX[MAX_NEBULA_CIRCLES], prevNebulaY[MAX_NEBULA_CIRCLES];
int prevNebulaR[MAX_NEBULA_CIRCLES];
int prevNebulaColors[MAX_NEBULA_CIRCLES];
int prevNebulaCount = 0;

// Previous positions for galaxy
int prevGalaxyCenterX, prevGalaxyCenterY;
int prevGalaxyCoreRadius;
#define MAX_GALAXY_ARMS 4
#define MAX_GALAXY_POINTS 50
int prevGalaxyPoints[MAX_GALAXY_ARMS][MAX_GALAXY_POINTS][2]; // [arm][point][x,y]
int prevGalaxyPointCount[MAX_GALAXY_ARMS];

// Previous positions for solar system
int prevSunX, prevSunY, prevSunRadius;
int prevOrbitRadii[4];
int prevPlanetX[4], prevPlanetY[4], prevPlanetRadius[4];

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
struct Asteroid {
  float x, y;
  float vx, vy;
  int radius;
  int prevX, prevY;
};
Asteroid asteroids[MAX_ASTEROIDS];
bool asteroidFieldInitialized = false;

// Initialize TFT object
TFT_eSPI tft = TFT_eSPI();
// Display dimensions
constexpr int SCREEN_WIDTH  = 128;
constexpr int SCREEN_HEIGHT = 128;
int potValue = 0;  // Potentiometer value

// Starfield parameters
constexpr int STAR_COUNT = 60;
Star stars[STAR_COUNT];

// Previous positions for streak erasure in warp mode
constexpr int MAX_STREAK_LENGTH = 15;
uint8_t prevX[STAR_COUNT][MAX_STREAK_LENGTH + 1];
uint8_t prevY[STAR_COUNT][MAX_STREAK_LENGTH + 1];

// Colors
uint16_t BG_COLOR = TFT_BLACK;
constexpr uint16_t STAR_COLOR = TFT_WHITE;

// Warp drive state
float warpFactor = 0.0f;    // 0.0 (no warp) to 1.0 (full warp)
bool warpEnabled = false;   // Warp mode toggle
constexpr float MIN_WARP_SPEED = 0.5f; // Minimum speed to avoid static stars

// Frame timing
constexpr unsigned long TARGET_FRAME_MS = 33; // ~30 FPS

// State management
enum class State {
  NORMAL,
  WARP,
  DISCOVERY
};
State currentState = State::NORMAL;

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

// Supernova parameters - moved to supernova.h
// #define MAX_SUPERNOVA_PARTICLES 60
// struct SupernovaParticle {
//   float x, y;          // Position
//   float vx, vy;        // Velocity
//   int brightness;      // Brightness
//   uint16_t color;      // Color
//   int prevX, prevY;    // Previous position
//   bool active;         // Whether the particle is active
// };

// SupernovaParticle supernovaParticles[MAX_SUPERNOVA_PARTICLES];
// bool supernovaInitialized = false;
// unsigned long supernovaStartTime = 0;
// int supernovaPhase = 0;  // 0=initial, 1=expanding, 2=fading
// int supernovaRadius = 0;
// int prevSupernovaX, prevSupernovaY;

// Comet parameters - moved to comet.h
// #define MAX_COMET_TAIL 30
// struct CometParticle {
//   float x, y;           // Position
//   int brightness;       // Brightness (0-255)
//   unsigned long spawnTime;  // When this particle was created
// };

// bool cometInitialized = false;
// float cometX, cometY;       // Current comet position
// float cometVx, cometVy;     // Comet velocity
// int cometRadius;            // Comet nucleus radius
// int prevCometX, prevCometY; // Previous comet position for erasing
// CometParticle cometTail[MAX_COMET_TAIL];
// unsigned long cometLastParticleTime = 0;

// Utility function for easing
float easeInOutCubic(float t) {
  return t < 0.5f ? 4.0f * t * t * t : 1.0f - powf(-2.0f * t + 2.0f, 3) / 2.0f;
}

// Add this to the global variables section after the celestial object enum
bool objectsShown[static_cast<int>(CelestialObject::NUM_TYPES)] = {false}; // Track which objects have been shown
int objectsRemaining = static_cast<int>(CelestialObject::NUM_TYPES); // Count of objects not yet shown

// Forward declarations for functions that are used before they're defined
void typewriterText(const char* text, int delayMs);
void initializeAccretionParticle(int index, int centerX, int centerY);

// Add new variable to track power state
bool isPoweredOn = true;  // Start powered on

void setup() {
  Serial.begin(9600);  // Move Serial.begin to top for debugging
  
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
    tft.setRotation(2);
    tft.fillScreen(TFT_BLACK);
    
    // Show wake-up message
    tft.setTextColor(TFT_GREEN);
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
  tft.init();
  tft.setRotation(2);
  tft.fillScreen(TFT_BLACK);
  
  // Initialize potentiometer
  pinMode(POT_PIN, INPUT);
  randomSeed(analogRead(POT_PIN));
  
  // Draw intro screen
  drawIntroScreen();
  
  // Initialize object tracking
  memset(objectsShown, false, sizeof(objectsShown));
  objectsRemaining = static_cast<int>(CelestialObject::NUM_TYPES);
  
  // Initialize tracking variables
  prevNebulaCount = 0;
  prevGalaxyCoreRadius = 0;
  for (int arm = 0; arm < MAX_GALAXY_ARMS; arm++) {
    prevGalaxyPointCount[arm] = 0;
  }
  
  // Initialize stars
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
}

void loop() {
  // Only process if powered on
  if (isPoweredOn) {
    checkPowerButton();
    
    if (powerOffRequested) {
      powerOff();
      return;
    }
    
    unsigned long frameStart = millis();
    
    readPotentiometer();
    processInput();
    
    if (currentState == State::WARP) {
      updateWarpStars();
    } else {
      // In NORMAL and DISCOVERY states
      
      // Static variable to stagger updates of different animations
      // to distribute processing load across multiple frames
      static int frameCounter = 0;
      frameCounter++;
      
      // Update stars every frame in DISCOVERY state
      if (currentState == State::DISCOVERY) {
        updateStars(); // Ensure stars twinkle in discovery mode
      } else if (frameCounter % 2 == 0) {
        updateStars(); // Only update stars on even frames in NORMAL mode
      }
      
      // Update shooting stars every frame as they are important for visual appeal
      updateShootingStars();
      
      // Only draw celestial objects if in discovery mode and object should be shown
      if (currentState == State::DISCOVERY && showingCelestialObject) {
        drawCelestialObject();
      }
      
      // Reset frame counter every 10 frames
      if (frameCounter >= 10) {
        frameCounter = 0;
      }
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
  
  // Debug output
  Serial.print("rawValue: ");
  Serial.print(rawValue);
  Serial.print(", potValue: ");
  Serial.println(potValue);
}

void processInput() {
  // Scale from 0-4095 to 0-1.0 for 12-bit ADC
  float rawWarpFactor = static_cast<float>(potValue) / 4095.0f;
  warpFactor = easeInOutCubic(rawWarpFactor);
  // Use a more precise threshold for 12-bit ADC (about 2.5% of full scale)
  bool shouldWarp = (potValue > 100);

  if (shouldWarp && !prevShouldWarp) {
    // Transition to WARP, erase celestial object if in DISCOVERY
    if (currentState == State::DISCOVERY && showingCelestialObject) {
      eraseCelestialObject();
      showingCelestialObject = false;
    }
    currentState = State::WARP;
  } else if (!shouldWarp && prevShouldWarp) {
    // Transition from WARP to DISCOVERY
    currentState = State::DISCOVERY;

    // 3/5 probability to show celestial object (increased chance)
    showingCelestialObject = (random(5) < 4);

    if (showingCelestialObject) {
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

      // *** MODIFICATION START ***
      // Check if the selected object is a Black Hole
      if (currentObject == CelestialObject::BLACK_HOLE) {
        // Set position near the center with a small random offset
        int centerOffsetX = random(-8, 9); // Offset from -8 to +8 pixels
        int centerOffsetY = random(-8, 9); // Offset from -8 to +8 pixels
        objectX = SCREEN_WIDTH / 2 + centerOffsetX;
        objectY = SCREEN_HEIGHT / 2 + centerOffsetY;
        // Optional: You might want a slightly larger scale for black holes
        objectScale = random(100, 180) / 100.0f; // Scale 1.8 to 2.8
        Serial.printf("Black Hole selected! Position: (%d, %d), Scale: %.2f\n", objectX, objectY, objectScale); // Debug print
      } else {
        // Default random positioning for all other objects
        objectX = random(20, SCREEN_WIDTH - 20);
        objectY = random(20, SCREEN_HEIGHT - 20);
        // Use the standard scale range for other objects
        objectScale = random(120, 240) / 100.0f; // Scale 1.2 to 2.4
        Serial.printf("Object %d selected. Position: (%d, %d), Scale: %.2f\n", (int)currentObject, objectX, objectY, objectScale); // Debug print
      }
      // *** MODIFICATION END ***

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
    if (random(0, 20) == 0) { // 20% chance to update per frame
      tft.drawPixel(stars[i].x, stars[i].y, BG_COLOR);
      int delta = random(1, 3);
      if (stars[i].increasing) {
        stars[i].brightness = min(stars[i].brightness + delta, 255);
        if (stars[i].brightness == 255) stars[i].increasing = false;
      } else {
        stars[i].brightness = max(stars[i].brightness - delta, 150);
        if (stars[i].brightness == 150) stars[i].increasing = true;
      }
      drawStar(stars[i]);
    }
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

  // First, clear previous streaks
  for (int i = 0; i < STAR_COUNT; i++) {
    for (int j = 0; j <= stars[i].streakLength; j++) {
      if (prevX[i][j] < SCREEN_WIDTH && prevY[i][j] < SCREEN_HEIGHT) {
        tft.drawPixel(prevX[i][j], prevY[i][j], BG_COLOR);
      }
    }
  }

  // Then draw new streaks and update positions
  for (int i = 0; i < STAR_COUNT; i++) {
    // Calculate direction vector from center
    float dx = stars[i].realX - centerX;
    float dy = stars[i].realY - centerY;
    float distance = sqrtf(dx * dx + dy * dy);
    if (distance < 1.0f) distance = 1.0f;

    float dirX = dx / distance;
    float dirY = dy / distance;

    // Calculate streak length based on warp factor and distance
    int streakLength = static_cast<int>(warpFactor * min(distance / 2.0f, static_cast<float>(MAX_STREAK_LENGTH)));
    stars[i].streakLength = streakLength;
    
    // Draw the streak
    for (int j = 0; j <= streakLength; j++) {
      int streakX = roundf(stars[i].realX + dirX * j);
      int streakY = roundf(stars[i].realY + dirY * j);
      if (j <= MAX_STREAK_LENGTH) {
        prevX[i][j] = streakX;
        prevY[i][j] = streakY;
      }
      if (streakX >= 0 && streakX < SCREEN_WIDTH && streakY >= 0 && streakY < SCREEN_HEIGHT) {
        // Fade intensity based on position in streak
        uint8_t intensity = (streakLength > 0) ? (stars[i].brightness * (streakLength - j) / streakLength) : stars[i].brightness;
        uint16_t color = tft.color565(intensity, intensity, intensity);
        tft.drawPixel(streakX, streakY, color);
      }
    }

    // Update star position - stars move faster when further from center
    float speed = (distance / 10.0f + 1.0f) * warpFactor * 3.0f;
    speed = max(speed, MIN_WARP_SPEED * warpFactor * 5.0f);

    stars[i].realX += dirX * speed;
    stars[i].realY += dirY * speed;

    // Convert to integer positions
    int newX = roundf(stars[i].realX);
    int newY = roundf(stars[i].realY);

    // Reset stars that move off screen back to a position near center
    if (newX < 0 || newX >= SCREEN_WIDTH || newY < 0 || newY >= SCREEN_HEIGHT) {
      stars[i].realX = centerX + random(-62, 63);
      stars[i].realY = centerY + random(-62, 63);
      stars[i].x = roundf(stars[i].realX);
      stars[i].y = roundf(stars[i].realY);
      stars[i].brightness = random(150, 256);
    } else {
      stars[i].x = newX;
      stars[i].y = newY;
    }
  }
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
 * Displays the name of the celestial object at the bottom of the screen
 */
void displayObjectName(const char* name) {
  // Clear the bottom text area first
 // tft.fillRect(0, SCREEN_HEIGHT - 10, SCREEN_WIDTH, 10, BG_COLOR);
  
  // Set text properties
  tft.setTextSize(1);
  tft.setTextColor(TFT_GREEN);
  
  // Center the text
  int16_t x1, y1;
  uint16_t w, h;
  // tft.getTextBounds(name, 0, 0, &x1, &y1, &w, &h); // Removed getTextBounds
  w = tft.textWidth(name); // Use textWidth instead
  h = 8; // Manually set height based on font size (assuming font size 1)
  int x = (SCREEN_WIDTH - w) / 2;
  
  // Draw the text at the bottom of the screen
  tft.setCursor(x, SCREEN_HEIGHT - 10);
  tft.print(name);
}

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

void eraseGalaxy() {
  if (prevGalaxyCoreRadius > 0) {
    // Clear the core with extra pixels to catch any glow effects
    tft.fillCircle(prevGalaxyCenterX, prevGalaxyCenterY, prevGalaxyCoreRadius + 1, BG_COLOR);
  }
  
  // Clear all star points in galaxy arms
  for (int arm = 0; arm < MAX_GALAXY_ARMS; arm++) {
    for (int i = 0; i < prevGalaxyPointCount[arm]; i++) {
      int x = prevGalaxyPoints[arm][i][0];
      int y = prevGalaxyPoints[arm][i][1];
      if (x >= 0 && x < SCREEN_WIDTH && y >= 0 && y < SCREEN_HEIGHT) {
        // Clear the pixel and surrounding pixels to catch any bloom effects
        tft.drawPixel(x, y, BG_COLOR);
        if (x > 0) tft.drawPixel(x-1, y, BG_COLOR);
        if (x < SCREEN_WIDTH-1) tft.drawPixel(x+1, y, BG_COLOR);
        if (y > 0) tft.drawPixel(x, y-1, BG_COLOR);
        if (y < SCREEN_HEIGHT-1) tft.drawPixel(x, y+1, BG_COLOR);
      }
    }
    prevGalaxyPointCount[arm] = 0;
  }
  prevGalaxyCoreRadius = 0;
}


// Define a struct to store star positions
struct Point {
  int x, y;
};

// Define a struct for solar flare particles
struct FlareParticle {
  int x, y;           // Current position
  float vx, vy;       // Velocity
  float life;         // Remaining lifetime (0-1)
  uint16_t color;     // Particle color
  bool active;        // Whether particle is active
};

#define MAX_FLARE_PARTICLES 15
FlareParticle flareParticles[MAX_FLARE_PARTICLES];
bool solarSystemInitialized = false; // Flag for initialization

void drawSolarSystem() {
    int centerX = objectX;
    int centerY = objectY;
    int sunRadius = 10 * objectScale;
    float t = millis() / 1000.0f;

    // Planet parameters
    float speeds[] = {0.5, 0.3, 0.2, 0.1};
    uint16_t planetColors[] = {
        tft.color565(0, 0, 255),     // Mercury - Blue
        tft.color565(255, 100, 0),   // Venus - Orange
        tft.color565(0, 255, 0),     // Earth - Green
        tft.color565(255, 0, 0)      // Mars - Red
    };
    int planetSizes[] = {3, 4, 3, 2};
    int orbitRadii[] = {20, 30, 40, 50};

    // Pulsing effect for sun
    float pulseFactor = (sin(t * 2.0f) + 1.0f) / 2.0f; // 0 to 1

    if (!solarSystemInitialized) {
        // Initialize flare particles
        for (int i = 0; i < MAX_FLARE_PARTICLES; i++) {
            flareParticles[i].active = false;
        }

        // Draw starfield (once during initialization)
        for (int i = 0; i < 20; i++) {
            int starX = random(SCREEN_WIDTH);
            int starY = random(SCREEN_HEIGHT);
            uint8_t brightness = random(50, 150);
            uint16_t starColor = tft.color565(brightness, brightness, brightness);
            tft.drawPixel(starX, starY, starColor);
            stars[i] = {starX, starY};
        }

        // Draw sun with corona
        for (int r = sunRadius + 2; r > sunRadius; r--) {
            uint8_t brightness = map(r, sunRadius, sunRadius + 2, 255, 100);
            uint16_t coronaColor = tft.color565(brightness, brightness, 0);
            tft.drawCircle(centerX, centerY, r, coronaColor);
        }
        tft.fillCircle(centerX, centerY, sunRadius, TFT_YELLOW);

        // Draw faint orbit paths (static)
        for (int i = 0; i < 4; i++) {
            int orbitRadius = orbitRadii[i] * objectScale;
            for (int j = 0; j < 360; j += 5) { // Increase step for performance
                float angle = j * PI / 180.0f;
                int x = centerX + orbitRadius * cos(angle);
                int y = centerY + orbitRadius * sin(angle);
                if (x >= 0 && x < SCREEN_WIDTH && y >= 0 && y < SCREEN_HEIGHT) {
                    tft.drawPixel(x, y, tft.color565(20, 20, 20));
                }
            }
            prevOrbitRadii[i] = orbitRadius;
        }

        // Draw initial planets, rings, and moon
        for (int i = 0; i < 4; i++) {
            int orbitRadius = orbitRadii[i] * objectScale;
            int planetX = centerX + orbitRadius * cos(t * speeds[i]);
            int planetY = centerY + orbitRadius * sin(t * speeds[i]);
            int planetRadius = planetSizes[i] * objectScale;

            // Planet glow
            for (int r = planetRadius + 1; r > planetRadius; r--) {
                uint8_t brightness = map(r, planetRadius, planetRadius + 1, 255, 100);
                uint16_t glowColor = tft.color565(
                    ((planetColors[i] >> 11) & 0x1F) * brightness / 255,
                    ((planetColors[i] >> 5) & 0x3F) * brightness / 255,
                    (planetColors[i] & 0x1F) * brightness / 255
                );
                tft.drawCircle(planetX, planetY, r, glowColor);
            }
            tft.fillCircle(planetX, planetY, planetRadius, planetColors[i]);

            // Rings for planet 2 (Saturn-like)
            if (i == 2) {
                tft.drawCircle(planetX, planetY, planetRadius + 2, tft.color565(150, 150, 150));
            }

            // Moon for planet 1 (Earth-like)
            if (i == 1) {
                float moonAngle = t * 2.0f; // Faster orbit
                int moonX = planetX + 5 * cos(moonAngle);
                int moonY = planetY + 5 * sin(moonAngle);
                tft.drawPixel(moonX, moonY, TFT_WHITE);
            }

            prevPlanetX[i] = planetX;
            prevPlanetY[i] = planetY;
            prevPlanetRadius[i] = planetRadius;
        }

        prevSunX = centerX;
        prevSunY = centerY;
        prevSunRadius = sunRadius;
        solarSystemInitialized = true;
    } else {
        // Erase previous planet positions (including rings and moon)
        for (int i = 0; i < 4; i++) {
            // Erase planet with extra radius to cover glow
            tft.fillCircle(prevPlanetX[i], prevPlanetY[i], prevPlanetRadius[i] + 3, BG_COLOR);
            
            // Special handling for ringed planet (i=2)
            if (i == 2) {
                // Erase ring area with larger radius
                tft.fillCircle(prevPlanetX[i], prevPlanetY[i], prevPlanetRadius[i] + 4, BG_COLOR);
            }
        }

        // Update and draw planets
        for (int i = 0; i < 4; i++) {
            int orbitRadius = orbitRadii[i] * objectScale;
            int planetX = centerX + orbitRadius * cos(t * speeds[i]);
            int planetY = centerY + orbitRadius * sin(t * speeds[i]);
            int planetRadius = planetSizes[i] * objectScale;

            // Planet glow
            for (int r = planetRadius + 1; r > planetRadius; r--) {
                uint8_t brightness = map(r, planetRadius, planetRadius + 1, 255, 100);
                uint16_t glowColor = tft.color565(
                    ((planetColors[i] >> 11) & 0x1F) * brightness / 255,
                    ((planetColors[i] >> 5) & 0x3F) * brightness / 255,
                    (planetColors[i] & 0x1F) * brightness / 255
                );
                tft.drawCircle(planetX, planetY, r, glowColor);
            }
            tft.fillCircle(planetX, planetY, planetRadius, planetColors[i]);

            // Rings for planet 2
            if (i == 2) {
                // Draw thicker ring with inner and outer circles
                tft.drawCircle(planetX, planetY, planetRadius + 1, tft.color565(150, 150, 150));
                tft.drawCircle(planetX, planetY, planetRadius + 2, tft.color565(150, 150, 150));
                tft.drawCircle(planetX, planetY, planetRadius + 3, tft.color565(150, 150, 150));
            }

            // Moon for planet 1
            if (i == 1) {
                float moonAngle = t * 2.0f;
                int moonX = planetX + 5 * cos(moonAngle);
                int moonY = planetY + 5 * sin(moonAngle);
                tft.drawPixel(moonX, moonY, TFT_WHITE);
            }

            prevPlanetX[i] = planetX;
            prevPlanetY[i] = planetY;
        }

       // Update existing flare particles
for (int i = 0; i < MAX_FLARE_PARTICLES; i++) {
    if (flareParticles[i].active) {
        // Erase previous position
        tft.drawPixel(flareParticles[i].x, flareParticles[i].y, BG_COLOR);
        
        // Update position with gravity effect (pulls back to sun)
        float dx = centerX - flareParticles[i].x;
        float dy = centerY - flareParticles[i].y;
        float distSq = dx * dx + dy * dy;
        float dist = sqrt(distSq);
        
        // Check if particle is inside the sun (has fallen back)
        if (dist < sunRadius) {
            flareParticles[i].active = false;
            continue;
        }
        
        // Gravitational force - stronger for most particles (realistic solar physics)
        float gravityFactor = 0.05 * (sunRadius * 2 / (dist * dist)); // Inverse square law
        
        // Apply gravity force
        flareParticles[i].vx += gravityFactor * dx / dist;
        flareParticles[i].vy += gravityFactor * dy / dist;
        
        // Apply velocity
        flareParticles[i].x += flareParticles[i].vx;
        flareParticles[i].y += flareParticles[i].vy;
        
        // Reduce life
        flareParticles[i].life -= 0.02; // Adjust for faster/slower decay
        
        // Draw new position if still active
        if (flareParticles[i].life > 0) {
            // Set particle color based on distance from sun (temperature gradient)
            uint8_t red = 255;
            uint8_t green = min(255, 180 + (int)(75 * (1.0 - dist / (sunRadius * 5))));
            uint8_t blue = min(200, (int)(80 * (1.0 - dist / (sunRadius * 3))));
            
            uint16_t currentColor = tft.color565(red, green, blue);
            tft.drawPixel(flareParticles[i].x, flareParticles[i].y, currentColor);
        } else {
            flareParticles[i].active = false;
        }
    }
}


        // Generate new flares (5% chance per frame)
        // Generate new flares (5% chance per frame)
if (random(100) < 5) {
    // Choose a random starting angle for the flare
    float flareAngle = random(360) * PI / 180.0f;
    float flareBaseX = centerX + sunRadius * cos(flareAngle);
    float flareBaseY = centerY + sunRadius * sin(flareAngle);
    
    // Redraw the sun when a particle leaves
    tft.fillCircle(centerX, centerY, sunRadius, TFT_YELLOW); // Redraw sun
    
    // Generate 8-16 particles for this flare (more particles for better visual effect)
    int particleCount = random(8, 17);
    float baseSpeed = random(8, 18) / 10.0f; // Base speed for this eruption
    
    // Create an arc-like eruption pattern with more variation
    for (int i = 0; i < particleCount; i++) {
        // Find an inactive particle slot
        for (int j = 0; j < MAX_FLARE_PARTICLES; j++) {
            if (!flareParticles[j].active) {
                // Initialize the particle
                flareParticles[j].x = flareBaseX;
                flareParticles[j].y = flareBaseY;
                
                // Create arc-like pattern with more variation
                float arcPosition = (float)i / particleCount; // 0.0 to 1.0 position in arc
                float arcFactor = 1.0 - fabs(arcPosition - 0.5) * 2.0; // 1.0 in middle, 0.0 at edges
                
                // Randomize the velocity direction in an arc pattern with more spread
                float angleVariation = (random(-30, 30) + (arcPosition - 0.5) * 60) * PI / 180.0f;
                float particleAngle = flareAngle + angleVariation;
                
                // Determine if this will be an escaping particle (only ~10-15% escape)
                bool willEscape = random(100) < 12;
                
                // Set velocity - particles in middle of arc move faster
                float speed;
                if (willEscape) {
                    // Escaping particles move faster
                    speed = baseSpeed * (1.4 + arcFactor * 1.0);
                } else {
                    // Non-escaping particles have more varied speeds
                    speed = baseSpeed * (0.44 + arcFactor * 0.84);
                }
                
                flareParticles[j].vx = speed * cos(particleAngle);
                flareParticles[j].vy = speed * sin(particleAngle);
                
                // Set random lifetime - escaping particles live longer
                flareParticles[j].life = random(70, 100) / 100.0f;
                if (willEscape) {
                    flareParticles[j].life += 0.4; // Longer life for escaping particles
                }
                
                // Set initial color based on temperature (yellow-white at base, redder for slower particles)
                if (willEscape) {
                    flareParticles[j].color = tft.color565(255, 200, 100); // Brighter orange for escaping
                } else {
                    flareParticles[j].color = tft.color565(255, 150, 50); // Darker orange for non-escaping
                }
                
                flareParticles[j].active = true;
                break;
            }
        }
    }
}

    }
}

void eraseSolarSystem() {
  if (prevSunRadius > 0) {
    // Clear sun and corona with larger buffer for complete clearing
    tft.fillCircle(prevSunX, prevSunY, prevSunRadius + 7, BG_COLOR);

    // Clear planets, rings, and moon
    for (int i = 0; i < 4; i++) {
      if (prevPlanetRadius[i] > 0) {
        tft.fillCircle(prevPlanetX[i], prevPlanetY[i], prevPlanetRadius[i] + 3, BG_COLOR);
      }
    }

    // Clear stars
    for (int i = 0; i < 20; i++) {
      tft.drawPixel(stars[i].x, stars[i].y, BG_COLOR);
    }
    
    // Clear flare particles
    for (int i = 0; i < MAX_FLARE_PARTICLES; i++) {
      if (flareParticles[i].active) {
        tft.drawPixel(flareParticles[i].x, flareParticles[i].y, BG_COLOR);
        flareParticles[i].active = false;
      }
    }

    // Reset tracking variables
  prevSunRadius = 0;
  for (int i = 0; i < 4; i++) {
    prevOrbitRadii[i] = 0;
    prevPlanetRadius[i] = 0;
    }
    solarSystemInitialized = false;
  }
  
  // Clear orbit paths
  for (int i = 0; i < 4; i++) {
    tft.fillCircle(objectX, objectY, prevOrbitRadii[i] + 1, BG_COLOR);
  }
}

void eraseAsteroidField() {
  for (int i = 0; i < MAX_ASTEROIDS; i++) {
    if (asteroids[i].radius > 0) {
      // Clear asteroid with expanded radius to ensure complete cleanup
      tft.fillCircle(asteroids[i].prevX, asteroids[i].prevY, 
                    asteroids[i].radius + 8, BG_COLOR); // Increased to +8 for more robust erasure
      
      // Also clear any potential artifacts in the movement path
      int midX = (asteroids[i].prevX + asteroids[i].x) / 2;
      int midY = (asteroids[i].prevY + asteroids[i].y) / 2;
      tft.fillCircle(midX, midY, asteroids[i].radius + 4, BG_COLOR);
    }
  }
}

// Enhanced nebula constants
#define MAX_NEBULA_PARTICLES 200  // Slightly increased for better detail
#define MAX_NEBULA_CORES 4      // Multiple cores for complex structure
#define MAX_DUST_LANES 3        // Dark dust lanes for realism

struct NebulaParticle {
    float x, y;
    float vx, vy;
    float density;       // Particle density affects brightness
    float temperature;   // Temperature affects color
    uint16_t color;
    int radius;
    int prevX, prevY;
    bool isDustLane;    // Whether this particle is part of a dark dust lane
};

struct NebulaCore {
    float x, y;         // Core position
    float temperature;  // Core temperature affects surrounding particles
    float intensity;    // Core brightness
    float radius;       // Influence radius
};

// Global variables
NebulaParticle nebulaParticles[MAX_NEBULA_PARTICLES];
NebulaCore nebulaCores[MAX_NEBULA_CORES];
bool nebulaInitialized = false;

// Color temperature mapping (Blackbody radiation approximation)
uint16_t getColorFromTemperature(float temp, float density) {
    int r, g, b;
    
    // Temperature ranges from 0 (cool) to 1 (hot)
    if (temp < 0.3) {
        // Cool reds and purples
        r = 255 * temp * 3;
        g = 0;
        b = 100 * temp;
    } else if (temp < 0.6) {
        // Transition to blues
        r = 150 - (temp - 0.3) * 200;
        g = (temp - 0.3) * 200;
        b = 150 + (temp - 0.3) * 200;
    } else {
        // Hot blues and whites
        r = (temp - 0.6) * 400;
        g = 120 + (temp - 0.6) * 300;
        b = 255;
    }

    // Apply density modulation
    float brightness = constrain(density * 1.2f, 0.2f, 1.0f);
    r = constrain((int)(r * brightness), 0, 255);
    g = constrain((int)(g * brightness), 0, 255);
    b = constrain((int)(b * brightness), 0, 255);

    return tft.color565(r, g, b);
}

void drawNebula() {
    int centerX = objectX;
    int centerY = objectY;
    float scale = objectScale;
    
    // Initialize nebula structure
    if (!nebulaInitialized) {
        // Initialize cores with varying properties
        for (int i = 0; i < MAX_NEBULA_CORES; i++) {
            nebulaCores[i].x = centerX + random(-30, 30) * scale;
            nebulaCores[i].y = centerY + random(-30, 30) * scale;
            nebulaCores[i].temperature = random(60, 100) / 100.0f;
            nebulaCores[i].intensity = random(70, 100) / 100.0f;
            nebulaCores[i].radius = random(15, 25) * scale;
        }

        // Initialize particles
        for (int i = 0; i < MAX_NEBULA_PARTICLES; i++) {
            // Randomly assign particle to a core's influence
            int coreIndex = random(MAX_NEBULA_CORES);
            float angle = random(360) * PI / 180.0;
            float dist = random(nebulaCores[coreIndex].radius * 1.5);
            
            nebulaParticles[i].x = nebulaCores[coreIndex].x + cos(angle) * dist;
            nebulaParticles[i].y = nebulaCores[coreIndex].y + sin(angle) * dist;
            
            // Initialize velocity (slow, swirling motion)
            float speed = random(2, 8) / 1000.0;
            nebulaParticles[i].vx = cos(angle + PI/2) * speed; // Tangential velocity
            nebulaParticles[i].vy = sin(angle + PI/2) * speed;
            
            // Set particle properties
            nebulaParticles[i].density = random(60, 100) / 100.0f;
            nebulaParticles[i].temperature = nebulaCores[coreIndex].temperature * 
                                           (0.7 + random(30) / 100.0f);
            nebulaParticles[i].radius = random(100) < 30 ? 2 : 1; // 30% larger particles
            nebulaParticles[i].isDustLane = random(100) < 15; // 15% dust lanes
            
            // Initialize previous position
            nebulaParticles[i].prevX = -1;
            nebulaParticles[i].prevY = -1;
        }
        nebulaInitialized = true;
    }

    // Animation timing
    static unsigned long lastUpdate = 0;
    unsigned long currentTime = millis();
    float deltaTime = (currentTime - lastUpdate) / 1000.0f;
    lastUpdate = currentTime;
    
    // Global nebula pulsing
    float globalPulse = (sin(currentTime / 3000.0f) + 1.0f) / 2.0f;

    // Process particles in batches for smooth animation
    static int startIndex = 0;
    int particlesToUpdate = min(40, MAX_NEBULA_PARTICLES);

    // Erase old positions
    for (int i = 0; i < particlesToUpdate; i++) {
        int index = (startIndex + i) % MAX_NEBULA_PARTICLES;
        if (nebulaParticles[index].prevX >= 0) {
            if (nebulaParticles[index].radius == 1) {
                tft.drawPixel(nebulaParticles[index].prevX, 
                            nebulaParticles[index].prevY, BG_COLOR);
            } else {
                tft.fillCircle(nebulaParticles[index].prevX,
                             nebulaParticles[index].prevY,
                             nebulaParticles[index].radius, BG_COLOR);
            }
        }
    }

    // Update and draw new positions
    for (int i = 0; i < particlesToUpdate; i++) {
        int index = (startIndex + i) % MAX_NEBULA_PARTICLES;
        NebulaParticle& particle = nebulaParticles[index];

        // Update position with smooth motion
        particle.x += particle.vx * deltaTime * 60;
        particle.y += particle.vy * deltaTime * 60;

        // Apply influence from nearest core
        float minDist = 1000;
        int nearestCore = 0;
        for (int c = 0; c < MAX_NEBULA_CORES; c++) {
            float dx = nebulaCores[c].x - particle.x;
            float dy = nebulaCores[c].y - particle.y;
            float dist = sqrt(dx*dx + dy*dy);
            if (dist < minDist) {
                minDist = dist;
                nearestCore = c;
            }
        }

        // Adjust velocity based on core influence
        if (minDist < nebulaCores[nearestCore].radius) {
            float angle = atan2(particle.y - nebulaCores[nearestCore].y,
                              particle.x - nebulaCores[nearestCore].x);
            particle.vx += cos(angle + PI/2) * 0.0001f;
            particle.vy += sin(angle + PI/2) * 0.0001f;
        }

        // Calculate final color
        float effectiveTemp = particle.temperature;
        float effectiveDensity = particle.density * 
                                (0.8 + 0.2 * globalPulse) * 
                                (particle.isDustLane ? 0.3 : 1.0);

        // Draw particle
        int x = round(particle.x);
        int y = round(particle.y);
        
        if (x >= 0 && x < SCREEN_WIDTH && y >= 0 && y < SCREEN_HEIGHT) {
            uint16_t color = getColorFromTemperature(effectiveTemp, effectiveDensity);
            
            if (particle.radius == 1) {
                tft.drawPixel(x, y, color);
            } else {
                tft.fillCircle(x, y, particle.radius, color);
            }
            
            particle.prevX = x;
            particle.prevY = y;
        }
    }

    startIndex = (startIndex + particlesToUpdate) % MAX_NEBULA_PARTICLES;
}

void eraseNebula() {
  if (nebulaInitialized) {
    for (int i = 0; i < MAX_NEBULA_PARTICLES; i++) {
      if (nebulaParticles[i].prevX >= 0 && nebulaParticles[i].prevX < SCREEN_WIDTH && 
          nebulaParticles[i].prevY >= 0 && nebulaParticles[i].prevY < SCREEN_HEIGHT) {
        if (nebulaParticles[i].radius == 1) {
          tft.drawPixel(nebulaParticles[i].prevX, nebulaParticles[i].prevY, BG_COLOR);
        } else {
      tft.fillCircle(nebulaParticles[i].prevX, nebulaParticles[i].prevY, 
                         nebulaParticles[i].radius, BG_COLOR);
        }
      }
    }
  }
  nebulaInitialized = false;
}

void drawGalaxy() {
  // Constants for spiral galaxy generation
  const int numArms = min(5, MAX_GALAXY_ARMS); // Use 5 arms or max available
  const float armSeparationDistance = 2 * PI / numArms;
  const float armOffsetMax = 0.5f;
  const float rotationFactor = 5;
  const float randomOffsetXY = 2.0f; // Adjusted for pixel space
  
  int centerX = objectX;
  int centerY = objectY;
  int coreRadius = 1 * objectScale; // Slightly larger core
  
  // Add time-based effects
  float time = millis() / 1000.0f;
  float pulseFactor = (sin(time * 2.0f) + 1.0f) / 2.0f; // 0 to 1
  float rotationSpeed = 0.1f + 0.05f * sin(time * 0.5f); // Varying rotation speed
  
  // Erase previous core
  if (prevGalaxyCoreRadius > 0) {
    tft.fillCircle(prevGalaxyCenterX, prevGalaxyCenterY, prevGalaxyCoreRadius, BG_COLOR);
  }
  
  // Erase previous points
  for (int arm = 0; arm < MAX_GALAXY_ARMS; arm++) {
    for (int i = 0; i < prevGalaxyPointCount[arm]; i++) {
      int x = prevGalaxyPoints[arm][i][0];
      int y = prevGalaxyPoints[arm][i][1];
      if (x >= 0 && x < SCREEN_WIDTH && y >= 0 && y < SCREEN_HEIGHT) {
        tft.drawPixel(x, y, BG_COLOR);
      }
    }
    prevGalaxyPointCount[arm] = 0;
  }
  
  // Draw galaxy core with pulsing effect
  for (int r = coreRadius; r > 0; r--) {
    float brightness = map(r, 0, coreRadius, 255, 180);
    brightness *= (0.8f + 0.2f * pulseFactor); // Add pulsing to core
    uint16_t color = tft.color565(brightness, brightness, brightness);
    tft.drawCircle(centerX, centerY, r, color);
  }
  
  // Add a bright center with color variation
  uint8_t centerBrightness = 255 * (0.7f + 0.3f * pulseFactor);
  uint16_t centerColor = tft.color565(centerBrightness, centerBrightness, centerBrightness);
  tft.fillCircle(centerX, centerY, coreRadius / 2, centerColor);
  
  // Store current core position and radius
  prevGalaxyCenterX = centerX;
  prevGalaxyCenterY = centerY;
  prevGalaxyCoreRadius = coreRadius;
  
  // Scaling factor - adjusted to fit the display (smaller value = larger galaxy)
  float scaleFactor = 25.0f * objectScale; 
  float globalRotation = (millis() / 10000.0f) * rotationSpeed; // Variable rotation speed
  
  // Draw each arm with enhanced effects
  for (int arm = 0; arm < numArms; arm++) {
    int pointsDrawn = 0;
    
    // Number of points per arm - adjusted based on display size
    const int pointsPerArm = 100; 
    
    for (int i = 0; i < pointsPerArm; i++) {
      // Generate a distance from center (0-1)
      float distance = (float)i / pointsPerArm;
      
      // Square the distance to concentrate more stars near center
      float distanceSquared = distance * distance;
      
      // Apply density factor - more stars in inner galaxy
      float density = 1.0f - distanceSquared * 0.8f;
      
      // Add some randomness to star distribution
      if (random(100) > density * 90) continue;
      
      // Calculate arm offset that decreases with distance
      float armOffset = (random(1000) / 1000.0f) * armOffsetMax;
      armOffset = armOffset - armOffsetMax / 2;
      armOffset = armOffset * (1 / max(distance, 0.1f));
      
      // Apply squared offset with sign preservation
      float squaredArmOffset = armOffset * abs(armOffset);
      
      // Apply rotation that increases with distance
      float rotation = distanceSquared * rotationFactor;
      
      // Calculate final angle with time-based variation
      float angle = arm * armSeparationDistance + squaredArmOffset + rotation + globalRotation;
      
      // Convert to cartesian coordinates
      float radius = scaleFactor * distance;
      float baseX = centerX + radius * cos(angle);
      float baseY = centerY + radius * sin(angle);
      
      // Add small random offset - more offset farther from center
      int x = round(baseX + (random(1000) / 1000.0f - 0.5f) * randomOffsetXY * distance);
      int y = round(baseY + (random(1000) / 1000.0f - 0.5f) * randomOffsetXY * distance);
      
      // Check if within screen bounds
      if (x >= 0 && x < SCREEN_WIDTH && y >= 0 && y < SCREEN_HEIGHT && 
          pointsDrawn < MAX_GALAXY_POINTS) {
        // Store position for next frame's erasing
        prevGalaxyPoints[arm][pointsDrawn][0] = x;
        prevGalaxyPoints[arm][pointsDrawn][1] = y;
        pointsDrawn++;
        
        // Enhanced brightness calculation with distance and time effects
        float brightness = (1.0f - distance * 0.5f) * 255.0f;
        brightness *= (0.8f + 0.2f * sin(time * 3.0f + distance * 10.0f)); // Add twinkling
        brightness = constrain(brightness, 150, 255);
        
        uint16_t color = tft.color565(brightness, brightness, brightness);
        
        // Add colored stars with more variety
        if (random(15) == 0) { // Increased chance for colored stars
          if (distance > 0.7f) {
            color = tft.color565(100, 100, 255); // Blue for outer arms
          } else if (distance > 0.4f) {
            color = tft.color565(255, 255, 100); // Yellow for middle arms
          } else {
            color = tft.color565(255, 100, 100); // Red for inner arms
          }
        }
        
        tft.drawPixel(x, y, color);
      }
    }
    
    prevGalaxyPointCount[arm] = pointsDrawn;
  }
  
  // Clear remaining arms if we reduced the number
  for (int arm = numArms; arm < MAX_GALAXY_ARMS; arm++) {
    prevGalaxyPointCount[arm] = 0;
  }
}

void drawAsteroidField() {
  if (!asteroidFieldInitialized) {
    int fieldWidth = 128;
    int fieldHeight = 128;
    int centerX = objectX;
    int centerY = objectY;
    
    // Initialize asteroids with more variety
    for (int i = 0; i < MAX_ASTEROIDS; i++) {
      asteroids[i].x = centerX + random(-fieldWidth, fieldWidth);
      asteroids[i].y = centerY + random(-fieldHeight, fieldHeight);
      
      // Vary asteroid speeds and directions
      float speed = random(1, 4) + random(0, 100) / 100.0f;
      float angle = random(0, 360) * PI / 180.0f;
      asteroids[i].vx = cos(angle) * speed;
      asteroids[i].vy = sin(angle) * speed;
      
      // Vary asteroid sizes
      asteroids[i].radius = random(1, 4);
      asteroids[i].prevX = round(asteroids[i].x);
      asteroids[i].prevY = round(asteroids[i].y);
    }
    asteroidFieldInitialized = true;
  }

  int fieldWidth = 50 * objectScale;
  int fieldHeight = 50 * objectScale;
  int centerX = objectX;
  int centerY = objectY;

  // Add time-based effects - calculate once per frame
  float time = millis() / 1000.0f;
  float pulseFactor = (sin(time * 2.0f) + 1.0f) / 2.0f;
  
  // Calculate max distance once
  float maxDistance = sqrt(fieldWidth*fieldWidth + fieldHeight*fieldHeight);

  // Process asteroids in smaller batches for smoother animation
  static int startAsteroid = 0;
  int asteroidsToUpdate = min(5, MAX_ASTEROIDS); // Update 1/3 of asteroids per frame
  
  for (int i = 0; i < asteroidsToUpdate; i++) {
    int index = (startAsteroid + i) % MAX_ASTEROIDS;
    
    // Erase the asteroid at its previous position with a larger radius to prevent artifacts
    tft.fillCircle(asteroids[index].prevX, asteroids[index].prevY, 
                   asteroids[index].radius + 3, BG_COLOR);
    
    // Also clear the path between previous and current position to eliminate trails
    int midX = (asteroids[index].prevX + round(asteroids[index].x)) / 2;
    int midY = (asteroids[index].prevY + round(asteroids[index].y)) / 2;
    tft.fillCircle(midX, midY, asteroids[index].radius + 2, BG_COLOR);

    // Update the asteroid's position with some randomness
    asteroids[index].x += asteroids[index].vx + (random(100) - 50) / 1000.0f;
    asteroids[index].y += asteroids[index].vy + (random(100) - 50) / 1000.0f;

    // Bounce the asteroid off the edges of the field with energy loss
    if (asteroids[index].x < centerX - fieldWidth || asteroids[index].x > centerX + fieldWidth) {
      asteroids[index].vx = -asteroids[index].vx * 0.95f; // 5% energy loss
      
      // Nudge asteroid away from boundary to prevent sticking
      if (asteroids[index].x < centerX - fieldWidth) {
        asteroids[index].x = centerX - fieldWidth + 1;
      } else {
        asteroids[index].x = centerX + fieldWidth - 1;
      }
      
      // Add some randomness to the bounce
      asteroids[index].vy += (random(100) - 50) / 100.0f;
    }
    
    if (asteroids[index].y < centerY - fieldHeight || asteroids[index].y > centerY + fieldHeight) {
      asteroids[index].vy = -asteroids[index].vy * 0.95f; // 5% energy loss
      
      // Nudge asteroid away from boundary to prevent sticking
      if (asteroids[index].y < centerY - fieldHeight) {
        asteroids[index].y = centerY - fieldHeight + 1;
      } else {
        asteroids[index].y = centerY + fieldHeight - 1;
      }
      
      // Add some randomness to the bounce
      asteroids[index].vx += (random(100) - 50) / 100.0f;
    }

    // Draw the asteroid with glow effect
    int x = round(asteroids[index].x);
    int y = round(asteroids[index].y);
    
    // Calculate asteroid brightness based on distance from center
    float dx = x - centerX;
    float dy = y - centerY;
    float distance = sqrt(dx*dx + dy*dy);
    float brightness = 1.0f - (distance / maxDistance);
    brightness = constrain(brightness, 0.3f, 1.0f);
    
    // Add pulsing effect
    brightness *= (0.8f + 0.2f * pulseFactor);
    
    // Draw asteroid glow
    for (int r = asteroids[index].radius + 1; r > asteroids[index].radius; r--) {
      uint8_t glowBrightness = map(r, asteroids[index].radius, asteroids[index].radius + 1, 255 * brightness, 100);
      uint16_t glowColor = tft.color565(glowBrightness, glowBrightness, glowBrightness);
      tft.drawCircle(x, y, r, glowColor);
    }
    
    // Draw main asteroid
    uint8_t asteroidBrightness = 255 * brightness;
    uint16_t asteroidColor = tft.color565(asteroidBrightness, asteroidBrightness, asteroidBrightness);
    tft.fillCircle(x, y, asteroids[index].radius, asteroidColor);

    // Update the previous position
    asteroids[index].prevX = x;
    asteroids[index].prevY = y;
  }
  
  // Update starting asteroid for next frame
  startAsteroid = (startAsteroid + asteroidsToUpdate) % MAX_ASTEROIDS;
}

/**
 * Draws a black hole with realistic accretion disk and gravitational effects
 * The black hole has a proper thermally colored accretion disk and stars being captured
 */


/**
 * Draws a pulsar - a rapidly rotating neutron star that emits beams of radiation
 */
// Implementation moved to pulsar.h

/**
 * Draws a supernova - an exploding star with expanding shock wave and debris
 */
// Implementation moved to supernova.h

/**
 * Erases the supernova and its particles
 */
// Implementation moved to supernova.h

/**
 * Draws a comet with a glowing head and particle tail
 */
// Implementation moved to comet.h

/**
 * Erases the comet and its tail
 */
// Implementation moved to comet.h

/**
 * Draws a star with flares and light variations
 */
// Implementation moved to star.h

/**
 * Erases a star
 */
// Implementation moved to star.h

/**
 * Draws a planet with atmosphere and surface details
 * Modified to cycle through planet types
 */
// Static variables to hold the current planet's generated configuration
static bool planetConfigured = false;
static uint32_t planetSeed = 0; // Seed for procedural generation
static uint16_t planetBaseColor1 = TFT_BLACK;
static uint16_t planetBaseColor2 = TFT_BLACK;
static uint16_t planetFeatureColor = TFT_BLACK; // For clouds/details
static uint16_t planetAtmosColor = TFT_BLACK;
static float lightAngle = 0.0f; // Angle from which light is coming (radians)
static int planetType = 0;      // 0=Rocky, 1=Gas, 2=Earth-like, 3=Ice
static int baseRadius = 0;      // Store base radius used for configuration

// Simple pseudo-random noise function (replace with Perlin/Simplex if available/performant)
// Uses integer coordinates and a seed for deterministic noise
float simpleNoise(int x, int y, uint32_t seed) {
    // Simple hash-like function - basic but gives variation
    uint32_t hash = seed;
    hash = (hash ^ 61) ^ (x * 31);
    hash = hash + (y * 53);
    hash = hash ^ (hash >> 16);
    hash = hash * 0xDEADBEEF; // Mix it up
    hash = hash ^ (hash >> 13);
    return (hash & 0xFFFF) / 65535.0f; // Normalize to 0.0 - 1.0
}

// Helper to blend two colors
uint16_t blendColor(uint16_t color1, uint16_t color2, float ratio) {
    ratio = constrain(ratio, 0.0f, 1.0f);
    uint8_t r1 = ((color1 >> 11) & 0x1F);
    uint8_t g1 = ((color1 >> 5) & 0x3F);
    uint8_t b1 = (color1 & 0x1F);

    uint8_t r2 = ((color2 >> 11) & 0x1F);
    uint8_t g2 = ((color2 >> 5) & 0x3F);
    uint8_t b2 = (color2 & 0x1F);

    uint8_t r = r1 * (1.0 - ratio) + r2 * ratio;
    uint8_t g = g1 * (1.0 - ratio) + g2 * ratio;
    uint8_t b = b1 * (1.0 - ratio) + b2 * ratio;

    return tft.color565(r << 3, g << 2, b << 3); // Re-encode to 565 directly might lose precision, better use full RGB if possible
    // Or re-encode using the TFT function if it handles 8-bit RGB input:
    // return tft.color565( (uint8_t)((r1 * (1.0 - ratio) + r2 * ratio) * 8.1),  // Approximate scaling back to 8-bit range
    //                      (uint8_t)((g1 * (1.0 - ratio) + g2 * ratio) * 4.05),
    //                      (uint8_t)((b1 * (1.0 - ratio) + b2 * ratio) * 8.1) );
}


void drawPlanet() {
    int centerX = objectX;
    int centerY = objectY;
    float scale = max(0.1f, objectScale); // Ensure scale is positive
    int currentRadius = round(15 * scale);  // Slightly larger base radius

    // If planet not configured OR the base radius changed significantly (needs regeneration)
    if (!planetConfigured || abs(currentRadius - baseRadius) > 2) {
        planetSeed = random(0xFFFFFFFF); // New seed for this planet
        planetType = random(0, 4);       // 0=Rocky, 1=Gas, 2=Earth-like, 3=Ice
        lightAngle = random(0, 360) * DEG_TO_RAD; // Random light direction

        // Define color palettes based on type
        switch (planetType) {
            case 0: // Rocky (Mars/Desert like)
                planetBaseColor1 = tft.color565(110, 70, 50);   // Dark Brown/Red
                planetBaseColor2 = tft.color565(210, 140, 90);  // Lighter Tan/Orange
                planetFeatureColor = tft.color565(180, 170, 160); // Wispy clouds/dust
                planetAtmosColor = tft.color565(230, 180, 150); // Thin, dusty atmosphere
                break;
            case 1: // Gas Giant (Jupiter/Saturn like)
                planetBaseColor1 = tft.color565(160, 140, 110); // Beige/Brown band
                planetBaseColor2 = tft.color565(220, 200, 170); // Lighter Cream band
                planetFeatureColor = tft.color565(240, 230, 220); // Bright Storms/swirls
                planetAtmosColor = tft.color565(210, 200, 180); // Hazy atmosphere
                break;
            case 2: // Earth-like
                planetBaseColor1 = tft.color565(20, 80, 160);   // Deep Ocean Blue
                planetBaseColor2 = tft.color565(50, 140, 70);   // Land Green
                planetFeatureColor = tft.color565(250, 250, 250); // White Clouds
                planetAtmosColor = tft.color565(180, 210, 240); // Blue sky atmosphere
                break;
            case 3: // Ice World
                planetBaseColor1 = tft.color565(150, 180, 210); // Shadowed Ice Blue
                planetBaseColor2 = tft.color565(220, 235, 255); // Bright Ice/Snow White
                planetFeatureColor = tft.color565(190, 210, 230); // Cracks / Light Blue features
                planetAtmosColor = tft.color565(210, 225, 245); // Very thin, bright atmosphere
                break;
        }
        planetConfigured = true;
        baseRadius = currentRadius; // Store the radius used for this configuration
    }

    // --- Drawing ---
    // Calculate square of radius for faster distance check
    float radiusSq = (float)currentRadius * currentRadius;
    // Calculate light vector components once
    float lightVecX = cos(lightAngle);
    float lightVecY = sin(lightAngle);

    tft.startWrite(); // Optimize drawing speed

    for (int y = -currentRadius; y <= currentRadius; y++) {
        for (int x = -currentRadius; x <= currentRadius; x++) {
            float distSq = (float)x * x + (float)y * y;

            // Is the pixel inside the planet's circle?
            if (distSq <= radiusSq) {
                float dist = sqrt(distSq);
                float currentAbsX = centerX + x;
                float currentAbsY = centerY + y;

                // Check screen bounds (optional but good practice)
                if (currentAbsX < 0 || currentAbsX >= SCREEN_WIDTH || currentAbsY < 0 || currentAbsY >= SCREEN_HEIGHT) {
                    continue;
                }

                // --- Calculate Base Color using Noise ---
                // Use multiple noise layers for more detail (adjust frequencies/amplitudes)
                float noiseVal = simpleNoise(x / 2, y / 2, planetSeed) * 0.6f; // Base layer
                noiseVal += simpleNoise(x * 2, y * 2, planetSeed + 1) * 0.3f; // Detail layer
                noiseVal += simpleNoise(y / 4, x/ 4, planetSeed + 2) * 0.1f; // Subtle large features (gas giant bands?)
                noiseVal = constrain(noiseVal, 0.0f, 1.0f);

                // Map noise to color gradient
                uint16_t baseSurfaceColor;
                if (planetType == 2) { // Special case for Earth: bias towards water
                   baseSurfaceColor = blendColor(planetBaseColor1, planetBaseColor2, constrain(noiseVal * 1.5f - 0.3f, 0.0f, 1.0f)); // More water
                } else {
                   baseSurfaceColor = blendColor(planetBaseColor1, planetBaseColor2, noiseVal);
                }

                 // --- Cloud/Feature Layer (optional based on type) ---
                float featureNoise = simpleNoise(x * 3 + 50, y * 3, planetSeed + 3); // Different noise for features
                float featureThreshold;
                switch(planetType) {
                    case 1: featureThreshold = 0.65f; break; // More prominent storms/bands
                    case 2: featureThreshold = 0.7f; break; // Clouds
                    default: featureThreshold = 0.9f; break; // Less frequent features
                }
                if (featureNoise > featureThreshold) {
                    float featureIntensity = (featureNoise - featureThreshold) / (1.0f - featureThreshold); // How "strong" is the feature
                    baseSurfaceColor = blendColor(baseSurfaceColor, planetFeatureColor, constrain(featureIntensity * 0.8f, 0.0f, 0.8f)); // Blend feature color in
                }


                // --- Calculate Lighting ---
                // Normalize pixel vector relative to center
                float pixelVecX = x / (float)currentRadius;
                float pixelVecY = y / (float)currentRadius;
                // Dot product between light vector and pixel normal vector (approximated by pixelVec)
                float dotProd = pixelVecX * lightVecX + pixelVecY * lightVecY;
                // Intensity based on angle (cosine relationship), clamp negative values (shadow)
                float lightIntensity = max(0.0f, dotProd);
                // Add ambient light so shadow side isn't pitch black
                lightIntensity = 0.15f + lightIntensity * 0.85f; // Adjust ambient(0.15) vs directional(0.85) ratio

                // Apply lighting to the surface color
                uint8_t r = red(baseSurfaceColor);
                uint8_t g = green(baseSurfaceColor);
                uint8_t b = blue(baseSurfaceColor);
                r = constrain((int)(r * lightIntensity), 0, 255);
                g = constrain((int)(g * lightIntensity), 0, 255);
                b = constrain((int)(b * lightIntensity), 0, 255);
                uint16_t litColor = tft.color565(r, g, b);

                // --- Atmosphere Haze near edge ---
                float edgeFactor = dist / (float)currentRadius; // 0 at center, 1 at edge
                float hazeAmount = pow(edgeFactor, 4.0f); // Make haze stronger near edge (power > 1)
                hazeAmount = constrain(hazeAmount * 0.4f, 0.0f, 0.4f); // Control max haze effect
                uint16_t finalColor = blendColor(litColor, planetAtmosColor, hazeAmount);


                // --- Draw the pixel ---
                tft.drawPixel(currentAbsX, currentAbsY, finalColor);
            }
        }
    }

    // --- Draw Outer Atmosphere Glow (Softer version) ---
    int glowRadiusStart = currentRadius + 1;
    int glowRadiusEnd = currentRadius + max(2, (int)(6 * scale)); // Glow thickness scales
    for (int r = glowRadiusEnd; r >= glowRadiusStart; r--) {
        float progress = (float)(r - glowRadiusStart) / (float)(glowRadiusEnd - glowRadiusStart + 1); // 1.0 near planet, 0.0 far out
        float alpha = (1.0 - progress) * 0.5f; // Fade out, max alpha less than 1.0

        // Blend atmosphere color with background based on alpha
        uint16_t glowColor = blendColor(BG_COLOR, planetAtmosColor, alpha);

        // Draw circle - might be slow, consider drawing arcs or points if needed
        tft.drawCircle(centerX, centerY, r, glowColor);
    }

    tft.endWrite(); // End optimized drawing
}

// --- Updated Erase Function ---

/**
 * Erases the planet and resets the configuration flag.
 */
void erasePlanet() {
    int centerX = objectX;
    int centerY = objectY;
    float scale = max(0.1f, objectScale);
    int currentRadius = round(15 * scale); // Match radius calculation in drawPlanet
    int glowThickness = max(2, (int)(6 * scale)); // Match glow thickness

    // Erase a circle slightly larger than the planet + atmosphere glow
    int eraseRadius = currentRadius + glowThickness + 2; // Add buffer
    tft.fillCircle(centerX, centerY, eraseRadius, BG_COLOR);

    // Signal that the planet needs to be re-configured on the next draw call
    planetConfigured = false;
}

#include <vector> // Include for std::vector

// --- Static variables for Binary Star state and erasing ---
static const int MAX_TRAIL_POINTS_BINARY = 10; // Increased for denser trail
static const int MAX_STREAM_POINTS_BINARY = 15;

// Previous positions and radii for precise erasing
static int b_prevCenterX = -1000, b_prevCenterY = -1000;
static float b_prevScale = -1.0f;
static int b_prevX1 = -1, b_prevY1 = -1, b_prevRadius1_eff = -1; // Store effective radius (base+glow)
static int b_prevX2 = -1, b_prevY2 = -1, b_prevRadius2_eff = -1;

// Store previous trail points
static std::vector<std::pair<int, int>> b_prevTrail1(MAX_TRAIL_POINTS_BINARY);
static std::vector<std::pair<int, int>> b_prevTrail2(MAX_TRAIL_POINTS_BINARY);
static int b_prevNumTrailPoints = 0;

// Store previous stream points
static std::vector<std::pair<int, int>> b_prevStream(MAX_STREAM_POINTS_BINARY);
static int b_prevNumStreamPoints = 0;

static bool b_wasDrawn = false; // Flag if the system was drawn in the previous frame

/**
 * Helper function to draw a star with soft glow and limb darkening
 */
void drawStarRealistic(int x, int y, int radius, uint16_t coreColor, uint16_t glowColor) {
    if (radius < 1) return; // Don't draw if too small

    uint8_t core_r = red(coreColor);
    uint8_t core_g = green(coreColor);
    uint8_t core_b = blue(coreColor);

    uint8_t glow_r = red(glowColor);
    uint8_t glow_g = green(glowColor);
    uint8_t glow_b = blue(glowColor);

    int maxGlowRadius = radius * 1.7; // Adjust glow extent

    tft.startWrite(); // Optimize drawing

    // Draw glow layers
    for (int r = maxGlowRadius; r > radius; r--) {
        float progress = (float)(r - radius) / (float)(maxGlowRadius - radius); // 0.0 at edge, 1.0 at max glow
        float alpha = (1.0 - progress * progress) * 0.4f; // Fade out non-linearly, control intensity

        // Blend glow color with background (approximate)
        // A true alpha blend needs reading background pixel, which is slow.
        // We fake it by blending with black (BG_COLOR assumed black here)
        uint8_t blended_r = glow_r * alpha;
        uint8_t blended_g = glow_g * alpha;
        uint8_t blended_b = glow_b * alpha;
        uint16_t blendedColor = tft.color565(blended_r, blended_g, blended_b);

        if (blended_r > 5 || blended_g > 5 || blended_b > 5) { // Only draw if color is visible
             tft.drawCircle(x, y, r, blendedColor);
        }
    }

    // Draw the star body with limb darkening
    for (int py = -radius; py <= radius; py++) {
        for (int px = -radius; px <= radius; px++) {
            float distSq = px * px + py * py;
            if (distSq <= radius * radius) {
                float dist = sqrt(distSq);
                float limbFactor = 1.0 - (dist / radius) * 0.35; // Darken by up to 35% at the edge

                uint8_t r = constrain((int)(core_r * limbFactor), 0, 255);
                uint8_t g = constrain((int)(core_g * limbFactor), 0, 255);
                uint8_t b = constrain((int)(core_b * limbFactor), 0, 255);
                tft.drawPixel(x + px, y + py, tft.color565(r, g, b));
            }
        }
    }
    tft.endWrite(); // End optimized drawing
}


// ---------------------------------------------------------


/**
 * Draws a binary star system with two stars orbiting each other, improved graphics
 */
void drawBinaryStar() {
    // ---- Erase Previous Frame ---
    eraseBinaryStar(); // Call erase first using stored previous state

    // ---- Current Frame Calculation ---
    int centerX = objectX;
    int centerY = objectY;
    float scale = max(0.1f, objectScale); // Ensure positive scale

    // Star base radii (adjust these for desired appearance)
    int radius1 = max(1, (int)(7 * scale)); // Main star (larger, G-type)
    int radius2 = max(1, (int)(4 * scale)); // Companion star (smaller, B-type)

    // More distinct colors
    uint16_t color1_core = tft.color565(255, 210, 100); // Brighter yellow-orange core
    uint16_t color1_glow = tft.color565(255, 160, 40);  // Deeper orange glow
    uint16_t color2_core = tft.color565(160, 210, 255); // Bright blue-white core
    uint16_t color2_glow = tft.color565(70, 150, 240);  // Deeper blue glow

    // Orbit mechanics (m1*r1 = m2*r2 -> r2/r1 = m1/m2)
    // Let's assume Star 1 (larger radius) is more massive.
    // Ratio of radii is ~7/4 = 1.75. Let mass ratio m1/m2 be ~2.
    // Then orbit radius ratio r2/r1 should be ~2.
    float orbitScale = 15.0f * scale; // Overall size of the system orbit
    int orbitRadius1 = max(1, (int)(orbitScale * 0.33f)); // Closer orbit for massive star
    int orbitRadius2 = max(1, (int)(orbitScale * 0.67f)); // Further orbit for less massive star

    float angularSpeed = 0.0012f; // Slightly slower speed
    float t = millis() * angularSpeed; // Current angle

    // Calculate positions
    int x1 = round(centerX + orbitRadius1 * cos(t));
    int y1 = round(centerY + orbitRadius1 * sin(t));
    int x2 = round(centerX - orbitRadius2 * cos(t)); // Opposite side
    int y2 = round(centerY - orbitRadius2 * sin(t));

    // Clear current trail/stream point lists for this frame's drawing
    b_prevNumTrailPoints = 0;
    b_prevNumStreamPoints = 0;

    // --- Draw Orbital Trails ---
    uint16_t trailColorBase = tft.color565(40, 40, 50); // Faint bluish-grey trail
    float trailLengthAngle = 1.5 * PI; // How much of the orbit trail to show (radians)
    float angleStep = 0.08; // Smaller step for denser trail

    tft.startWrite();
    for (float angleOffset = angleStep; angleOffset <= trailLengthAngle; angleOffset += angleStep) {
        float currentAngle = t - angleOffset; // Go backwards in time for trail

        // Trail point for star 1
        int tx1 = round(centerX + orbitRadius1 * cos(currentAngle));
        int ty1 = round(centerY + orbitRadius1 * sin(currentAngle));

        // Trail point for star 2
        int tx2 = round(centerX - orbitRadius2 * cos(currentAngle));
        int ty2 = round(centerY - orbitRadius2 * sin(currentAngle));

        // Fade the trail based on how far back in time (angleOffset)
        float fade = 1.0 - (angleOffset / trailLengthAngle); // Linear fade 1.0 (near star) -> 0.0 (end of trail)
        fade = fade * fade; // Sharper fade out (optional)

        uint8_t tr = red(trailColorBase) * fade;
        uint8_t tg = green(trailColorBase) * fade;
        uint8_t tb = blue(trailColorBase) * fade;

        if ((tr > 3 || tg > 3 || tb > 3) && b_prevNumTrailPoints < MAX_TRAIL_POINTS_BINARY) { // Only draw/store if visible enough
            uint16_t fadedTrailColor = tft.color565(tr, tg, tb);
            if (tx1 != b_prevX1 || ty1 != b_prevY1) { // Avoid overdrawing same pixel
                tft.drawPixel(tx1, ty1, fadedTrailColor);
                b_prevTrail1[b_prevNumTrailPoints].first = tx1;
                b_prevTrail1[b_prevNumTrailPoints].second = ty1;
            }
            if (tx2 != b_prevX2 || ty2 != b_prevY2) {
                tft.drawPixel(tx2, ty2, fadedTrailColor);
                b_prevTrail2[b_prevNumTrailPoints].first = tx2;
                b_prevTrail2[b_prevNumTrailPoints].second = ty2;
            }
            b_prevNumTrailPoints++;
        }
    }
    tft.endWrite();


    // --- Draw Interaction Stream (if stars are close enough) ---
    float distanceBetween = sqrt(sq(x2 - x1) + sq(y2 - y1));
    float interactionDistance = (radius1 + radius2) * 3.5f; // How close for stream to appear
    float streamPullOffDistance = radius1 * 1.5f; // Where stream starts from star 1 surface

    if (distanceBetween < interactionDistance && distanceBetween > 1.0f) {
        // Stream goes from larger Star 1 towards smaller Star 2

        // Calculate direction vector from 1 to 2
        float dirX = (x2 - x1) / distanceBetween;
        float dirY = (y2 - y1) / distanceBetween;

        // Calculate stream start point near surface of Star 1
        float startX = x1 + dirX * streamPullOffDistance;
        float startY = y1 + dirY * streamPullOffDistance;

        // Calculate stream end point near surface of Star 2 (L1 point approximation)
        // Simple approach: aim towards center of star 2
        float endX = x2 - dirX * (radius2 * 1.2f);
        float endY = y2 - dirY * (radius2 * 1.2f);

        // Introduce curvature due to orbital motion (Coriolis-like effect)
        // Perpendicular vector to direction
        float perpX = -dirY;
        float perpY = dirX;
        float curveFactor = 7.0 * scale * (1.0 - distanceBetween / interactionDistance); // Stronger curve when closer

        uint16_t streamColor = tft.color565(200, 210, 240); // Faint hot gas color

        tft.startWrite();
        for (int i = 0; i < MAX_STREAM_POINTS_BINARY; ++i) {
            float t_stream = (float)i / (MAX_STREAM_POINTS_BINARY - 1); // Progress along stream (0 to 1)

            // Linear interpolation base
            float baseX = startX + (endX - startX) * t_stream;
            float baseY = startY + (endY - startY) * t_stream;

            // Add curve offset (parabolic shape)
            float curveOffset = curveFactor * t_stream * (1.0 - t_stream);
            int streamX = round(baseX + perpX * curveOffset);
            int streamY = round(baseY + perpY * curveOffset);

            // Fade stream towards the end point
            float fade = 1.0 - t_stream * 0.5; // Fade out
            uint8_t sr = red(streamColor) * fade;
            uint8_t sg = green(streamColor) * fade;
            uint8_t sb = blue(streamColor) * fade;

            if (sr > 3 || sg > 3 || sb > 3) {
                tft.drawPixel(streamX, streamY, tft.color565(sr, sg, sb));
                // Store for erasing
                b_prevStream[b_prevNumStreamPoints].first = streamX;
                b_prevStream[b_prevNumStreamPoints].second = streamY;
                b_prevNumStreamPoints++;
            }
        }
        tft.endWrite();
    }

    // --- Draw Stars (on top of trails/streams) ---
    drawStarRealistic(x1, y1, radius1, color1_core, color1_glow);
    drawStarRealistic(x2, y2, radius2, color2_core, color2_glow);

    // --- Store state for next erase ---
    b_prevCenterX = centerX;
    b_prevCenterY = centerY;
    b_prevScale = scale;
    b_prevX1 = x1;
    b_prevY1 = y1;
    b_prevRadius1_eff = radius1 * 2.5; // Store effective erase radius (base + glow)
    b_prevX2 = x2;
    b_prevY2 = y2;
    b_prevRadius2_eff = radius2 * 2.5;
    b_wasDrawn = true; // Mark that we drew something
}


// ---------------------------------------------------------


/**
 * Erases the previously drawn binary star system using stored state
 */
void eraseBinaryStar() {
    if (!b_wasDrawn) return; // Nothing to erase if nothing was drawn

    tft.startWrite(); // Use transaction for faster erasing

    // Erase previous Stream
    for (int i = 0; i < b_prevNumStreamPoints; ++i) {
        tft.drawPixel(b_prevStream[i].first, b_prevStream[i].second, BG_COLOR);
    }

    // Erase previous Trails
    for (int i = 0; i < b_prevNumTrailPoints; ++i) {
        tft.drawPixel(b_prevTrail1[i].first, b_prevTrail1[i].second, BG_COLOR);
        tft.drawPixel(b_prevTrail2[i].first, b_prevTrail2[i].second, BG_COLOR);
    }

    // Erase previous Star positions (using stored effective radius including glow)
    if (b_prevRadius1_eff > 0) {
        // Add a small buffer to the erase radius just in case
        tft.fillCircle(b_prevX1, b_prevY1, b_prevRadius1_eff + 2, BG_COLOR);
    }
    if (b_prevRadius2_eff > 0) {
        tft.fillCircle(b_prevX2, b_prevY2, b_prevRadius2_eff + 2, BG_COLOR);
    }

    tft.endWrite(); // Finish transaction

    // Reset state variables and flag
    b_wasDrawn = false;
    b_prevNumTrailPoints = 0; // Clear counts for safety
    b_prevNumStreamPoints = 0;
    // Keep prevX/Y etc. as they might be needed if erase is called multiple times before draw
}


/**
 * Draws a space station with improved blinking lights and realistic details
 * Using true pixel-based updates to eliminate flickering
 */
void drawSpaceStation() {
  static float prevStationAngle = -100; // Initial invalid value to force first draw
  static int prevBodyPoints[4][2] = {{0}}; // Store previous body corner positions
  static int prevPanelPoints[2][4][4][2] = {{{{0}}}}; // [panel][segment][corner][x,y]
  static int prevPanelBorders[2][4][5][2] = {{{{0}}}}; // [panel][segment][border_point][x,y] - Added for border tracking
  static int prevDishPos[2] = {0}; // Previous dish position
  static int prevLightPos[3][2] = {{0}}; // Previous light positions [red,green,strobe][x,y]
  static int prevBeamPoints[2][2] = {{0}}; // Previous beam endpoints
  static bool prevLightState[3] = {false}; // Previous light states [red,green,strobe]
  static bool prevBeamActive = false; // Was beam active in previous frame
  static int prevWindowPos[3][2] = {{0}}; // Previous window positions
  static bool prevWindowState[3] = {false}; // Previous window states
  
  int centerX = objectX;
  int centerY = objectY;
  float scale = objectScale;
  unsigned long currentTime = millis();
  
  // Station dimensions
  int bodyWidth = 18 * scale;
  int bodyHeight = 7 * scale;
  int moduleWidth = bodyWidth * 0.8;
  int moduleHeight = bodyHeight;
  int panelWidth = 5 * scale;
  int panelHeight = 12 * scale;
  int dishRadius = 2 * scale;
  int lightRadius = max(1, (int)(scale + 0.5));
  
  // Calculate current rotation angle
  float stationAngle = (currentTime * 0.0002);
  float cosAngle = cos(stationAngle);
  float sinAngle = sin(stationAngle);
  
  // Rotation transform function
  auto rotatePoint = [centerX, centerY, cosAngle, sinAngle](int& x, int& y) {
    int relX = x - centerX;
    int relY = y - centerY;
    x = centerX + relX * cosAngle - relY * sinAngle;
    y = centerY + relX * sinAngle + relY * cosAngle;
  };
  
  // For the first draw or if rotation has changed significantly, redraw everything
  bool firstDraw = (prevStationAngle < -99);
  bool majorRotationChange = abs(stationAngle - prevStationAngle) > 0.05;
  
  // First, erase previous elements if this isn't the first draw
  if (!firstDraw) {
    // Clear a bounding box around the entire previous station to ensure no artifacts
    int minX = SCREEN_WIDTH, maxX = 0, minY = SCREEN_HEIGHT, maxY = 0;
    
    // Calculate bounds from all previous positions
    // Include body points
    for (int i = 0; i < 4; i++) {
      minX = min(minX, prevBodyPoints[i][0]);
      maxX = max(maxX, prevBodyPoints[i][0]);
      minY = min(minY, prevBodyPoints[i][1]);
      maxY = max(maxY, prevBodyPoints[i][1]);
    }
    
    // Include panel points and borders
    for (int panel = 0; panel < 2; panel++) {
      for (int segment = 0; segment < 4; segment++) {
        // Panel corners
        for (int corner = 0; corner < 4; corner++) {
          minX = min(minX, prevPanelPoints[panel][segment][corner][0]);
          maxX = max(maxX, prevPanelPoints[panel][segment][corner][0]);
          minY = min(minY, prevPanelPoints[panel][segment][corner][1]);
          maxY = max(maxY, prevPanelPoints[panel][segment][corner][1]);
        }
        // Panel borders
        for (int border = 0; border < 5; border++) {
          minX = min(minX, prevPanelBorders[panel][segment][border][0]);
          maxX = max(maxX, prevPanelBorders[panel][segment][border][0]);
          minY = min(minY, prevPanelBorders[panel][segment][border][1]);
          maxY = max(maxY, prevPanelBorders[panel][segment][border][1]);
        }
      }
    }
    
    // Expand bounds by 15% to ensure complete coverage
    int width = maxX - minX;
    int height = maxY - minY;
    minX = max(0, minX - width/6);
    maxX = min(SCREEN_WIDTH-1, maxX + width/6);
    minY = max(0, minY - height/6);
    maxY = min(SCREEN_HEIGHT-1, maxY + height/6);
    
    // Clear the expanded bounding area
    tft.fillRect(minX, minY, maxX - minX + 1, maxY - minY + 1, BG_COLOR);
    
    // Additional precise clearing of panel borders
    for (int panel = 0; panel < 2; panel++) {
      for (int segment = 0; segment < 4; segment++) {
        for (int border = 0; border < 4; border++) {
          int nextBorder = (border + 1) % 4;
          tft.drawLine(
            prevPanelBorders[panel][segment][border][0],
            prevPanelBorders[panel][segment][border][1],
            prevPanelBorders[panel][segment][nextBorder][0],
            prevPanelBorders[panel][segment][nextBorder][1],
            BG_COLOR
          );
        }
      }
    }
  }
  
  // Main station body - ALWAYS redraw to ensure rotation
  int x1 = centerX - moduleWidth/2;
  int y1 = centerY - moduleHeight/2;
  int x2 = centerX + moduleWidth/2;
  int y2 = centerY + moduleHeight/2;
  
  // Store and rotate the four corners
  int bodyCorners[4][2] = {
    {x1, y1}, {x2, y1}, {x2, y2}, {x1, y2}
  };
  
  for (int i = 0; i < 4; i++) {
    rotatePoint(bodyCorners[i][0], bodyCorners[i][1]);
  }
  
  // Draw a filled polygon for the station body
  uint16_t bodyColor = tft.color565(180, 180, 180);
  tft.fillTriangle(
    bodyCorners[0][0], bodyCorners[0][1],
    bodyCorners[1][0], bodyCorners[1][1],
    bodyCorners[2][0], bodyCorners[2][1],
    bodyColor
  );
  
  tft.fillTriangle(
    bodyCorners[0][0], bodyCorners[0][1],
    bodyCorners[2][0], bodyCorners[2][1],
    bodyCorners[3][0], bodyCorners[3][1],
    bodyColor
  );
  
  // Store body corners for next frame
  memcpy(prevBodyPoints, bodyCorners, sizeof(bodyCorners));
  
  // Draw solar panels with improved border tracking
  for (int panel = 0; panel < 2; panel++) {
    int panelX = (panel == 0) ? 
      centerX - bodyWidth/2 - panelWidth/2 : 
      centerX + bodyWidth/2 + panelWidth/2;
    int panelY = centerY;
    
    for (int segment = 0; segment < 4; segment++) {
      int segmentHeight = panelHeight / 4;
      int segX1 = panelX - panelWidth/2;
      int segY1 = panelY - panelHeight/2 + segment * segmentHeight;
      int segX2 = panelX + panelWidth/2;
      int segY2 = segY1 + segmentHeight - 1;
      
      // Store corners before rotation
      int corners[4][2] = {
        {segX1, segY1}, {segX2, segY1}, 
        {segX2, segY2}, {segX1, segY2}
      };
      
      // Rotate all corners
      for (int c = 0; c < 4; c++) {
        rotatePoint(corners[c][0], corners[c][1]);
        // Store rotated corners for border tracking
        prevPanelBorders[panel][segment][c][0] = corners[c][0];
        prevPanelBorders[panel][segment][c][1] = corners[c][1];
      }
      
      // Store for next frame's panel corners
      memcpy(prevPanelPoints[panel][segment], corners, sizeof(corners));
      
      // Draw filled panel
      uint16_t panelColor = tft.color565(40 + segment*5, 45 + segment*5, 80 + segment*10);
      tft.fillTriangle(
        corners[0][0], corners[0][1],
        corners[1][0], corners[1][1],
        corners[2][0], corners[2][1],
        panelColor
      );
      
      tft.fillTriangle(
        corners[0][0], corners[0][1],
        corners[2][0], corners[2][1],
        corners[3][0], corners[3][1],
        panelColor
      );
      
      // Draw and store panel borders with extra precision
      uint16_t panelOutlineColor = tft.color565(30 + segment*5, 35 + segment*5, 70 + segment*10);
      for (int c = 0; c < 4; c++) {
        int next = (c + 1) % 4;
        tft.drawLine(
          corners[c][0], corners[c][1],
          corners[next][0], corners[next][1],
          panelOutlineColor
        );
        
        // Store border points with slight offset for better coverage
        float dx = corners[next][0] - corners[c][0];
        float dy = corners[next][1] - corners[c][1];
        float len = sqrt(dx*dx + dy*dy);
        if (len > 0) {
          float nx = -dy/len; // Normal vector
          float ny = dx/len;
          // Store slightly offset border point for better cleanup
          prevPanelBorders[panel][segment][4][0] = corners[c][0] + nx;
          prevPanelBorders[panel][segment][4][1] = corners[c][1] + ny;
        }
      }
    }
  }

  // Draw communication dish
  int dishX = centerX;
  int dishY = centerY - bodyHeight/2 - 2 * scale;
  rotatePoint(dishX, dishY);
  tft.fillCircle(dishX, dishY, dishRadius, tft.color565(120, 120, 120));
  prevDishPos[0] = dishX;
  prevDishPos[1] = dishY;
  
  // Navigation lights
  // Red light (left)
  int redX = centerX - bodyWidth/2;
  int redY = centerY;
  rotatePoint(redX, redY);
  bool redOn = ((currentTime / 500) % 2 == 0);
  if (redOn) {
    tft.fillCircle(redX, redY, lightRadius, tft.color565(255, 0, 0));
  }
  prevLightPos[0][0] = redX;
  prevLightPos[0][1] = redY;
  prevLightState[0] = redOn;
  
  // Green light (right)
  int greenX = centerX + bodyWidth/2;
  int greenY = centerY;
  rotatePoint(greenX, greenY);
  bool greenOn = ((currentTime / 500) % 2 == 1);
  if (greenOn) {
    tft.fillCircle(greenX, greenY, lightRadius, tft.color565(0, 255, 0));
  }
  prevLightPos[1][0] = greenX;
  prevLightPos[1][1] = greenY;
  prevLightState[1] = greenOn;
  
  // White strobe (top)
  int strobeX = centerX;
  int strobeY = centerY - bodyHeight/2;
  rotatePoint(strobeX, strobeY);
  bool strobeOn = ((currentTime / 2000) % 4 == 0);
  if (strobeOn) {
    tft.fillCircle(strobeX, strobeY, lightRadius, tft.color565(255, 255, 255));
  }
  prevLightPos[2][0] = strobeX;
  prevLightPos[2][1] = strobeY;
  prevLightState[2] = strobeOn;
  
  // Draw windows with blinking lights
  for (int i = 0; i < 3; i++) {
    int winX = centerX - bodyWidth/4 + i * (bodyWidth/4);
    int winY = centerY;
    rotatePoint(winX, winY);
    
    // Window light effect
    bool windowOn = ((currentTime / 1000) + i) % 3 == 0;
    uint16_t windowColor = windowOn ? 
      tft.color565(255, 255, 150) : tft.color565(100, 100, 80);
    
    tft.drawPixel(winX, winY, windowColor);
    if (scale > 1.0) {
      tft.drawPixel(winX, winY+1, windowColor);
    }
    
    prevWindowPos[i][0] = winX;
    prevWindowPos[i][1] = winY;
    prevWindowState[i] = windowOn;
  }
  
  // Communication beam
  bool beamActive = ((currentTime / 3000) % 2 == 0);
  if (beamActive) {
    int beamLength = 10 * scale;
    int beamEndX = dishX + cos(stationAngle + PI/4) * beamLength;
    int beamEndY = dishY + sin(stationAngle + PI/4) * beamLength;
    tft.drawLine(dishX, dishY, beamEndX, beamEndY, tft.color565(70, 70, 255));
    
    prevBeamPoints[0][0] = dishX;
    prevBeamPoints[0][1] = dishY;
    prevBeamPoints[1][0] = beamEndX;
    prevBeamPoints[1][1] = beamEndY;
    
    // Add beam animation
    if ((currentTime / 200) % 2 == 0) {
      int midX = dishX + cos(stationAngle + PI/4) * beamLength * 0.5;
      int midY = dishY + sin(stationAngle + PI/4) * beamLength * 0.5;
      tft.drawPixel(midX, midY, tft.color565(200, 200, 255));
    }
  }
  prevBeamActive = beamActive;
  
  // Store angle for next frame
  prevStationAngle = stationAngle;
}

// Erase function is kept empty as erasing is handled in the drawing function
void eraseSpaceStation() {
  // Calculate the maximum possible extent of the station based on its dimensions
  float scale = objectScale;
  int bodyWidth = 18 * scale;
  int bodyHeight = 7 * scale;
  int panelWidth = 5 * scale;
  int panelHeight = 12 * scale;
  
  // Calculate a larger bounding box that encompasses the entire station with rotation
  int maxExtent = max(max(bodyWidth, panelHeight), max(bodyHeight, panelWidth)) + 5; // Increased margin
  
  // Clear the entire station area with a rectangle that encompasses all parts
  tft.fillRect(objectX - maxExtent, objectY - maxExtent, maxExtent * 2, maxExtent * 2, BG_COLOR);
  
  // Use multiple overlapping circles for more thorough cleaning of rotated elements
  int maxRadius = maxExtent + 10; // Slightly larger than the bounding box
  
  // Clear with multiple overlapping circles at different positions
  for (int offset = 0; offset <= 10; offset += 2) {
    // Center circle
    tft.fillCircle(objectX, objectY, maxRadius - offset, BG_COLOR);
    
    // Offset circles in cardinal directions
    tft.fillCircle(objectX + offset, objectY, maxRadius - offset, BG_COLOR);
    tft.fillCircle(objectX - offset, objectY, maxRadius - offset, BG_COLOR);
    tft.fillCircle(objectX, objectY + offset, maxRadius - offset, BG_COLOR);
    tft.fillCircle(objectX, objectY - offset, maxRadius - offset, BG_COLOR);
    
    // Diagonal offset circles
    tft.fillCircle(objectX + offset, objectY + offset, maxRadius - offset, BG_COLOR);
    tft.fillCircle(objectX - offset, objectY + offset, maxRadius - offset, BG_COLOR);
    tft.fillCircle(objectX + offset, objectY - offset, maxRadius - offset, BG_COLOR);
    tft.fillCircle(objectX - offset, objectY - offset, maxRadius - offset, BG_COLOR);
  }
  
  // Clear any potential beam artifacts with a wider line
  int beamLength = 10 * scale;
  for (float angle = 0; angle < 2 * PI; angle += PI / 8) {
    int endX = objectX + cos(angle) * beamLength;
    int endY = objectY + sin(angle) * beamLength;
    tft.drawLine(objectX, objectY, endX, endY, BG_COLOR);
  }
  
  // Additional cleanup for any remaining artifacts
  for (int i = 0; i < 4; i++) {
    float angle = i * PI / 2;
    int x = objectX + cos(angle) * maxRadius;
    int y = objectY + sin(angle) * maxRadius;
    tft.fillCircle(x, y, 5, BG_COLOR);
  }
}

/**
 * Draws a retro-style intro screen with animation and waits for user input
 * Uses the main stars array for a seamless transition
 */
void drawIntroScreen() {
  // We're using the main stars array that's already initialized
  // Draw scanline effect for CRT look
  for (int y = 0; y < SCREEN_HEIGHT; y += 2) {
    tft.drawFastHLine(0, y, SCREEN_WIDTH, tft.color565(30, 30, 30));
  }
  
  // Draw WARP DRIVE title with shadow for 3D effect
  const int titleY = 20;
  // Shadow first
  tft.setTextColor(tft.color565(0, 0, 80));
  tft.setTextSize(2);
  tft.setCursor(12, titleY+1);
  tft.print("WARP");
  tft.setCursor(22, titleY+17);
  tft.print("DRIVE");
  
  // Then main text
  tft.setTextColor(tft.color565(80, 200, 255)); // Bright cyan
  tft.setCursor(10, titleY);
  tft.print("WARP");
  tft.setCursor(20, titleY+16);
  tft.print("DRIVE");
  
  // Animate a spaceship
  drawAnimatedSpaceship();
  
  // Draw "ESP8266" subtitle with proper positioning
  tft.setTextSize(1);
  tft.setTextColor(tft.color565(0, 255, 0));
  tft.setCursor(28, titleY + 48);
  typewriterText("For Mahira <3", 40);
  
  // Draw interface instructions with proper positioning
  tft.setTextColor(tft.color565(255, 255, 0));
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
    tft.setTextColor(tft.color565(255, 255, 255));
    tft.setCursor(50, 110);
    tft.print("READY");
    delay(400);
    tft.setTextColor(BG_COLOR);
    tft.setCursor(50, 110);
    tft.print("READY");
    delay(200);
  }
  tft.setTextColor(tft.color565(255, 255, 255));
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
 * Placeholder for warpTransitionEffect - removed as per requirements
 * This function is kept as a stub to maintain compatibility with any code that might call it
 */
void warpTransitionEffect() {
  // Function intentionally left empty - transition effect removed
  // No screen clearing or effects to preserve the starfield
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
      tft.color565(255, 255, 255)
    );
    tft.drawLine(
      shipX, shipY + shipHeight + offsetY,
      shipX + shipWidth, shipY + shipHeight/2 + offsetY,
      tft.color565(255, 255, 255)
    );
    
    // Cockpit window
    tft.fillRect(
      shipX + shipWidth - 5, shipY + shipHeight/2 - 1 + offsetY,
      3, 2,
      tft.color565(100, 200, 255)
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
      uint8_t exhaustBrightness = max(0, 150 - i * 30);
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
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_RED);
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


