/**
 * binarystar.h
 * 
 * This file contains all code related to drawing and managing binary star systems
 * in the Warp Drive visualization.
 */

#ifndef BINARYSTAR_H
#define BINARYSTAR_H

#include <TFT_eSPI.h>
#include <Arduino.h>
#include <cmath>
#include "sprite_manager.h" // Include the sprite manager
#include "star.h" // For Star struct definition

// Forward declarations from main sketch
extern TFT_eSPI tft;
extern const int SCREEN_WIDTH;
extern const int SCREEN_HEIGHT;
extern uint16_t BG_COLOR;
extern uint16_t COLOR_GREEN;
extern int objectX;
extern int objectY;
extern float objectScale;
extern float scaleFactor;
extern Star stars[];

// Declare sprite for binary star rendering
extern SpriteHandle binaryStarHandle; // Use handle from .ino

static bool forceRedrawBinaryStar = true; // Flag to force redraw of binary star sprite content

// Forward declare functions from main sketch
extern int scale_i(int v);
extern float scale_f(float v);
extern void displayObjectName(const char* name);
extern uint8_t extract_red(uint16_t color);
extern uint8_t extract_green(uint16_t color);
extern uint8_t extract_blue(uint16_t color);

// Define PI if not already defined
#ifndef PI
#define PI 3.14159265358979323846
#endif

const int MAX_STREAM_POINTS_BINARY = 15;
// Further reduce maximum sprite dimension to avoid memory issues
const int MAX_BINARYSTAR_SPRITE_DIM = 160; // Reduced from 200 to better manage memory

// Track previous screen area for proper cleanup
int prevBinaryStarX = 0;
int prevBinaryStarY = 0;
int prevBinaryStarSize = 0;

// Forward declarations for our own functions
void drawBinaryStarDirect();
void eraseBinaryStar();
void drawStarRealistic(TFT_eSprite& sprite, int x, int y, int radius, uint16_t coreColor, uint16_t glowColor);

/**
 * Calculates the optimal sprite size based on the binary star scale
 */
int calculateOptimalSpriteSize(float scale, int* pRadius1 = nullptr, int* pRadius2 = nullptr, 
                              int* pOrbitRadius1 = nullptr, int* pOrbitRadius2 = nullptr) {
  // Calculate actual star radii based on scale
  int radius1 = std::max(1, static_cast<int>(scale_i(7) * scale * 0.8f));
  int radius2 = std::max(1, static_cast<int>(scale_i(4) * scale * 0.8f));
  
  // Calculate orbit radii - this determines how far the stars are from center
  float orbitScaleVal = scale_f(15.0f) * scale * 0.9f;
  int orbitRadius1 = std::max(1, static_cast<int>(scale_i(orbitScaleVal * 0.33f)));
  int orbitRadius2 = std::max(1, static_cast<int>(scale_i(orbitScaleVal * 0.67f)));
  
  // Store calculated values if pointers provided
  if (pRadius1) *pRadius1 = radius1;
  if (pRadius2) *pRadius2 = radius2;
  if (pOrbitRadius1) *pOrbitRadius1 = orbitRadius1;
  if (pOrbitRadius2) *pOrbitRadius2 = orbitRadius2;
  
  // Calculate exact dimensions needed - maximum distance between stars plus their radii
  int maxDistance = orbitRadius1 + orbitRadius2; // Maximum distance between star centers
  int maxExtent = maxDistance + radius1 + radius2; // Add star radii for total extent
  
  // Add just enough margin for glow effects and anti-aliasing (5%)
  int calculatedSize = ceil(maxExtent * 1.05f);
  
  #ifdef ESP32
  // Check available memory and adjust size if needed
  uint32_t freeHeap = ESP.getFreeHeap();
  uint32_t freePsram = ESP.getFreePsram();
  uint32_t requiredMem = calculatedSize * calculatedSize;
  
  float memoryRatio = (freePsram > 10000) ? 
                     std::min((float)freePsram / (float)requiredMem, 10.0f) :
                     std::min((float)freeHeap / (float)requiredMem, 5.0f);
                     
  if (memoryRatio < 3.0f) {
    // If tight on memory, reduce size proportionally
    float adjustFactor = std::max(0.6f, memoryRatio / 3.0f);
    calculatedSize = ceil(calculatedSize * adjustFactor);
    
    Serial.printf("[BinaryStar] Memory tight (ratio: %.1f) - Reduced size to %d\n", 
                 memoryRatio, calculatedSize);
  }
  #endif
  
  // Ensure the size is even for better memory alignment
  if (calculatedSize % 2 != 0) {
    calculatedSize++;
  }
  
  // Cap at maximum size to prevent excessive memory usage
  calculatedSize = std::min(calculatedSize, MAX_BINARYSTAR_SPRITE_DIM);
  
  // Ensure minimum reasonable size
  calculatedSize = std::max(calculatedSize, 40);
  
  return calculatedSize;
}

/**
 * Draws a binary star system using sprite buffering
 */
void drawBinaryStar() {
  int centerX = objectX;
  int centerY = objectY;
  float scale = objectScale * scaleFactor;
  unsigned long currentTime = millis();
  
  // Store position for proper cleanup later
  prevBinaryStarX = centerX;
  prevBinaryStarY = centerY;
  
  // Calculate star and orbit parameters once
  int radius1, radius2, orbitRadius1, orbitRadius2;
  
  // Calculate optimal sprite size based on object scale
  int calculatedSpriteSize = calculateOptimalSpriteSize(scale, &radius1, &radius2, 
                                                      &orbitRadius1, &orbitRadius2);
  
  // Save the size for proper cleanup later (add margin for cleanup)
  prevBinaryStarSize = calculatedSpriteSize * 1.2f;
  
  // Use handle to manage sprite lifecycle
  TFT_eSprite* spritePtr = nullptr;

  if (binaryStarHandle.id == 0) { // Create sprite if handle is invalid
    // If memory is critically low, use direct drawing
    #ifdef ESP32
    if (ESP.getFreeHeap() < 8000 || ESP.getFreePsram() < 8000) {
      Serial.println("[BinaryStar] Critical memory - using direct drawing");
      drawBinaryStarDirect();
      return;
    }
    Serial.printf("[BinaryStar] Creating sprite %dx%d. Heap: %u, PSRAM: %u\n", 
                 calculatedSpriteSize, calculatedSpriteSize, ESP.getFreeHeap(), ESP.getFreePsram());
    #endif
    
      // Create sprite using the manager
      SpriteAllocResult res = SpriteManager::create(calculatedSpriteSize, calculatedSpriteSize, true, binaryStarHandle); // Prefer PSRAM

      if (res == SpriteAllocResult::Success || res == SpriteAllocResult::SuccessShrunk || res == SpriteAllocResult::FellBackToHeap) {
          Serial.printf("[BinaryStar] Sprite created successfully via manager (ID: %u).\n", binaryStarHandle.id);
          spritePtr = SpriteManager::getSpriteRef(binaryStarHandle);
          if (!spritePtr) {
              Serial.println("[BinaryStar] ERROR: Failed to get sprite ref after creation!");
              SpriteManager::destroy(binaryStarHandle); // Clean up failed creation
              binaryStarHandle = {0};
              drawBinaryStarDirect(); // Fallback
              return;
          }
          forceRedrawBinaryStar = true; // Force redraw after creation
    } else {
          Serial.printf("[BinaryStar] ERROR: Failed to create sprite via manager! Result: %d\n", (int)res);
          binaryStarHandle = {0}; // Ensure handle is invalid
          drawBinaryStarDirect(); // Fallback
          return;
      }
  } else { // Handle is valid, try to get sprite reference
      spritePtr = SpriteManager::getSpriteRef(binaryStarHandle);
      if (!spritePtr) {
          Serial.println("[BinaryStar] ERROR: Handle was valid but failed to get sprite reference. Using direct draw.");
          binaryStarHandle = {0}; // Invalidate handle as it points to nothing usable
      drawBinaryStarDirect();
      return;
    }
      // Check if size needs significant update (optional)
      if (abs(calculatedSpriteSize - spritePtr->width()) > 10) { // If size differs by > 10 pixels
          Serial.println("[BinaryStar] INFO: Recreating sprite due to significant size change.");
          SpriteManager::destroy(binaryStarHandle); // Destroy old one
          binaryStarHandle = {0};
          drawBinaryStar(); // Recurse to recreate with new size (be careful with recursion depth)
    return;
  }
  }

  // If we reach here, spritePtr should be valid

  int spriteWidth = spritePtr->width();
  int spriteHeight = spritePtr->height();
  int spriteOffsetX = centerX - spriteWidth / 2;
  int spriteOffsetY = centerY - spriteHeight / 2;
  int spriteCenterX = spriteWidth / 2;
  int spriteCenterY = spriteHeight / 2;
  
  // Clear with solid color (use spritePtr)
  spritePtr->fillSprite(BG_COLOR);
  
  // Draw minimal starfield background (use spritePtr)
  for (int i = 0; i < STAR_COUNT; i += 4) {
    if (stars[i].brightness > 200) {
      int spriteStarX = stars[i].x - spriteOffsetX;
      int spriteStarY = stars[i].y - spriteOffsetY;
      if (spriteStarX >= 2 && spriteStarX < spriteWidth-2 && 
          spriteStarY >= 2 && spriteStarY < spriteHeight-2) {
        uint8_t brightness = stars[i].brightness;
        uint16_t color = spritePtr->color565(brightness, brightness, brightness);
        spritePtr->drawPixel(spriteStarX, spriteStarY, color);
      }
    }
  }
  
  // Binary star colors (use spritePtr for color565)
  uint16_t color1_core = spritePtr->color565(255, 210, 100);
  uint16_t color1_glow = spritePtr->color565(255, 160, 40);
  uint16_t color2_core = spritePtr->color565(160, 210, 255);
  uint16_t color2_glow = spritePtr->color565(70, 150, 240);
  
  // Calculate star positions based on time and orbit
  float angularSpeed = 0.0012f;
  float time_val = currentTime * angularSpeed;
  
  int x1 = round(spriteCenterX + orbitRadius1 * cos(time_val));
  int y1 = round(spriteCenterY + orbitRadius1 * sin(time_val));
  int x2 = round(spriteCenterX - orbitRadius2 * cos(time_val));
  int y2 = round(spriteCenterY - orbitRadius2 * sin(time_val));
  
  // Draw orbital trails - simple circles for stability
  uint16_t trailColorBase = spritePtr->color565(40, 40, 50);
  
  // Draw just the orbit circles - no individual points to avoid flickering
  spritePtr->drawCircle(spriteCenterX, spriteCenterY, orbitRadius1, trailColorBase);
  spritePtr->drawCircle(spriteCenterX, spriteCenterY, orbitRadius2, trailColorBase);
  
  // Draw interaction stream between stars if they're close enough
  float distanceBetween = sqrt(pow(x2 - x1, 2) + pow(y2 - y1, 2));
  float interactionDistance = (radius1 + radius2) * 3.5f;
  
  if (distanceBetween < interactionDistance && distanceBetween > 1.0f) {
    // Draw a simple connection line for efficiency - thicker line to avoid flickering
    uint16_t streamColor = spritePtr->color565(200, 210, 240);
    
    // Draw a line with thickness=2 by drawing two adjacent lines
    spritePtr->drawLine(x1, y1, x2, y2, streamColor);
    spritePtr->drawLine(x1, y1+1, x2, y2+1, streamColor);
  }
  
  // Draw the stars with simplified approach to avoid flickering
  // Use filled circles instead of gradient approach for consistent rendering
  drawStarRealistic(*spritePtr, x1, y1, radius1, color1_core, color1_glow);
  drawStarRealistic(*spritePtr, x2, y2, radius2, color2_core, color2_glow);
  
  // Push the sprite to the screen using the manager
  SpriteManager::draw(binaryStarHandle, spriteOffsetX, spriteOffsetY);
    
  forceRedrawBinaryStar = false;
}

// Implement direct drawing fallback using the same parameters
void drawBinaryStarDirect() {
  int centerX = objectX;
  int centerY = objectY;
  float scale = objectScale * scaleFactor;
  unsigned long currentTime = millis();
  
  // Store position for proper cleanup later
  prevBinaryStarX = centerX;
  prevBinaryStarY = centerY;
  
  // Calculate parameters using the same function for consistency
  int radius1, radius2, orbitRadius1, orbitRadius2;
  int calculatedSize = calculateOptimalSpriteSize(scale, &radius1, &radius2, 
                                                &orbitRadius1, &orbitRadius2);
  
  // Save the maximum extent for cleanup
  prevBinaryStarSize = calculatedSize;
  
  // Clear the area first to remove any previous artifacts
  int clearSize = calculatedSize;
  tft.fillRect(centerX - clearSize/2, centerY - clearSize/2, clearSize, clearSize, BG_COLOR);
  
  uint16_t color1_core = tft.color565(255, 210, 100);
  uint16_t color1_glow = tft.color565(255, 160, 40);
  uint16_t color2_core = tft.color565(160, 210, 255);
  uint16_t color2_glow = tft.color565(70, 150, 240);
  
  float angularSpeed = 0.0012f;
  float time_val = currentTime * angularSpeed;
  
  int x1 = round(centerX + orbitRadius1 * cos(time_val));
  int y1 = round(centerY + orbitRadius1 * sin(time_val));
  int x2 = round(centerX - orbitRadius2 * cos(time_val));
  int y2 = round(centerY - orbitRadius2 * sin(time_val));
  
  // Draw simplified orbital trails
  uint16_t trailColor = tft.color565(40, 40, 50);
  tft.drawCircle(centerX, centerY, orbitRadius1, trailColor);
  tft.drawCircle(centerX, centerY, orbitRadius2, trailColor);
  
  // Draw simple solid stars
  // First star - warmer yellow/orange
  tft.fillCircle(x1, y1, radius1, color1_core);
  tft.drawCircle(x1, y1, radius1 + 1, color1_glow);
  
  // Second star - cooler blue
  tft.fillCircle(x2, y2, radius2, color2_core);
  tft.drawCircle(x2, y2, radius2 + 1, color2_glow);
  
  // Draw a simple connection line between stars if they're close enough
  float distance = sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1));
  if (distance < (radius1 + radius2) * 3.5f) {
    // Draw a thicker line by using two adjacent lines
    uint16_t streamColor = tft.color565(200, 210, 240);
    tft.drawLine(x1, y1, x2, y2, streamColor);
    tft.drawLine(x1, y1+1, x2, y2+1, streamColor);
  }
  
  #ifdef ESP32
  Serial.println("[BinaryStar] Using direct drawing (fallback)");
  #endif
}

// Implement cleanup with extra clearing
void eraseBinaryStar() {
  // Use handle and SpriteManager::destroy
  if (binaryStarHandle.id != 0) {
    Serial.println("[BinaryStar] Erasing using SpriteManager::destroy");
    int16_t spriteW = SpriteManager::getWidth(binaryStarHandle);
    int16_t spriteH = SpriteManager::getHeight(binaryStarHandle);
    if (spriteW > 0 && spriteH > 0) {
        int spriteOffsetX = prevBinaryStarX - spriteW / 2;
        int spriteOffsetY = prevBinaryStarY - spriteH / 2;
        tft.fillRect(spriteOffsetX, spriteOffsetY, spriteW, spriteH, BG_COLOR);
  } else {
        // Fallback clear if dimensions couldn't be retrieved
        Serial.println("[BinaryStar] Warning: Could not get sprite dimensions for clearing. Using previous size.");
        tft.fillRect(prevBinaryStarX - prevBinaryStarSize/2, prevBinaryStarY - prevBinaryStarSize/2, 
                     prevBinaryStarSize, prevBinaryStarSize, BG_COLOR);
    }
    SpriteManager::destroy(binaryStarHandle);
    binaryStarHandle = {0};
  } else {
    // If no handle, clear based on previous size used for direct drawing
    Serial.println("[BinaryStar] Erasing direct draw area.");
    tft.fillRect(prevBinaryStarX - prevBinaryStarSize/2, prevBinaryStarY - prevBinaryStarSize/2, 
                 prevBinaryStarSize, prevBinaryStarSize, BG_COLOR);
  }
  
  prevBinaryStarX = 0; prevBinaryStarY = 0; prevBinaryStarSize = 0; // Reset position info
}

/**
 * Helper function to draw a star with soft glow and limb darkening
 * Simplified for rendering stability
 */
void drawStarRealistic(TFT_eSprite& sprite, int x, int y, int radius, uint16_t coreColor, uint16_t glowColor) {
  if (radius < 1) return;
  
  // Just draw a simple filled circle with outline for stability
  sprite.fillCircle(x, y, radius, coreColor);
  sprite.drawCircle(x, y, radius+1, glowColor);
  
  // Add highlight only for larger stars
  if (radius > 4) {
    int innerRadius = radius * 0.6;
    uint8_t r = extract_red(coreColor);
    uint8_t g = extract_green(coreColor);
    uint8_t b = extract_blue(coreColor);
    
    // Make the inner part slightly brighter
    r = std::min(255, (int)(r * 1.1));
    g = std::min(255, (int)(g * 1.1));
    b = std::min(255, (int)(b * 1.1));
    
    uint16_t innerColor = sprite.color565(r, g, b);
    sprite.fillCircle(x, y, innerRadius, innerColor);
  }
}

#endif // BINARYSTAR_H
