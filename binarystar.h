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
extern TFT_eSprite binaryStarSprite;
extern bool binaryStarSpriteCreated;

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
  
  #ifdef ESP32
  //Serial.printf("[BinaryStar] Scale: %.2f, Stars: %d/%d, Orbits: %d/%d, Sprite: %d\n", 
  //             scale, radius1, radius2, orbitRadius1, orbitRadius2, calculatedSpriteSize);
  #endif
  
  if (!binaryStarSpriteCreated) {
    // If memory is critically low, use direct drawing
    #ifdef ESP32
    if (ESP.getFreeHeap() < 8000 || ESP.getFreePsram() < 8000) {
      Serial.println("[BinaryStar] Critical memory - using direct drawing");
      binaryStarSpriteCreated = false;
      drawBinaryStarDirect();
      return;
    }
    
    Serial.printf("[BinaryStar] Creating sprite %dx%d. Heap: %u, PSRAM: %u\n", 
                 calculatedSpriteSize, calculatedSpriteSize, ESP.getFreeHeap(), ESP.getFreePsram());
    #endif
    
    // Clean up any previously created sprite first
    if (binaryStarSprite.width() > 0 || binaryStarSprite.height() > 0) {
      binaryStarSprite.deleteSprite();
    }
    
    // Create sprite if not already done or if size changed significantly
    if (!binaryStarSpriteCreated || abs(calculatedSpriteSize - binaryStarSprite.width()) > 4 ) {
        SpriteManager::safeDeleteSprite(binaryStarSprite, "BinaryStar");
        SpriteManager::createObjectSprite(binaryStarSprite, calculatedSpriteSize, "BinaryStar", true);
        binaryStarSpriteCreated = (binaryStarSprite.width() > 0 && binaryStarSprite.height() > 0);
        forceRedrawBinaryStar = true;
    }
    
    // Verify sprite creation success by checking dimensions
    if (binaryStarSprite.width() > 0 && binaryStarSprite.height() > 0) {
      binaryStarSpriteCreated = true;
      #ifdef ESP32
      Serial.printf("[BinaryStar] Sprite created: %dx%d\n", 
                   binaryStarSprite.width(), binaryStarSprite.height());
      #endif
    } else {
      binaryStarSpriteCreated = false;
      #ifdef ESP32
      Serial.println("[BinaryStar] Sprite creation FAILED");
      #endif
      
      drawBinaryStarDirect();
      return;
    }
  }
  
  if (!binaryStarSpriteCreated) {
    drawBinaryStarDirect(); // Fallback if sprite not created
    return;
  }
  
  // If we reach here, binaryStarSpriteCreated is true and sprite should be valid.
  // Double-check sprite validity one more time
  if (binaryStarSprite.width() <= 0 || binaryStarSprite.height() <= 0) {
    #ifdef ESP32
    Serial.println("[BinaryStar] Sprite invalid despite creation flag, using direct drawing");
    #endif
    binaryStarSpriteCreated = false;
    drawBinaryStarDirect();
    return;
  }
  
  int spriteWidth = binaryStarSprite.width();
  int spriteHeight = binaryStarSprite.height();
  int spriteOffsetX = centerX - spriteWidth / 2;
  int spriteOffsetY = centerY - spriteHeight / 2;
  int spriteCenterX = spriteWidth / 2;
  int spriteCenterY = spriteHeight / 2;
  
  // Clear with solid color - must happen before any drawing
  binaryStarSprite.fillSprite(BG_COLOR);
  
  // Draw minimal starfield background - just a few key stars
  // Even more selectively draw stars to prevent flickering
  for (int i = 0; i < STAR_COUNT; i += 4) { // Draw every 4th star
    if (stars[i].brightness > 200) { // Only draw the brightest stars
      int spriteStarX = stars[i].x - spriteOffsetX;
      int spriteStarY = stars[i].y - spriteOffsetY;
      if (spriteStarX >= 2 && spriteStarX < spriteWidth-2 && 
          spriteStarY >= 2 && spriteStarY < spriteHeight-2) {
        uint8_t brightness = stars[i].brightness;
        uint16_t color = binaryStarSprite.color565(brightness, brightness, brightness);
        binaryStarSprite.drawPixel(spriteStarX, spriteStarY, color);
      }
    }
  }
  
  // Binary star colors - these could be parameters if we want different types
  uint16_t color1_core = binaryStarSprite.color565(255, 210, 100);
  uint16_t color1_glow = binaryStarSprite.color565(255, 160, 40);
  uint16_t color2_core = binaryStarSprite.color565(160, 210, 255);
  uint16_t color2_glow = binaryStarSprite.color565(70, 150, 240);
  
  // Calculate star positions based on time and orbit
  float angularSpeed = 0.0012f;
  float time_val = currentTime * angularSpeed;
  
  int x1 = round(spriteCenterX + orbitRadius1 * cos(time_val));
  int y1 = round(spriteCenterY + orbitRadius1 * sin(time_val));
  int x2 = round(spriteCenterX - orbitRadius2 * cos(time_val));
  int y2 = round(spriteCenterY - orbitRadius2 * sin(time_val));
  
  // Draw orbital trails - simple circles for stability
  uint16_t trailColorBase = binaryStarSprite.color565(40, 40, 50);
  
  // Draw just the orbit circles - no individual points to avoid flickering
  binaryStarSprite.drawCircle(spriteCenterX, spriteCenterY, orbitRadius1, trailColorBase);
  binaryStarSprite.drawCircle(spriteCenterX, spriteCenterY, orbitRadius2, trailColorBase);
  
  // Draw interaction stream between stars if they're close enough
  float distanceBetween = sqrt(pow(x2 - x1, 2) + pow(y2 - y1, 2));
  float interactionDistance = (radius1 + radius2) * 3.5f;
  
  if (distanceBetween < interactionDistance && distanceBetween > 1.0f) {
    // Draw a simple connection line for efficiency - thicker line to avoid flickering
    uint16_t streamColor = binaryStarSprite.color565(200, 210, 240);
    
    // Draw a line with thickness=2 by drawing two adjacent lines
    binaryStarSprite.drawLine(x1, y1, x2, y2, streamColor);
    binaryStarSprite.drawLine(x1, y1+1, x2, y2+1, streamColor);
  }
  
  // Draw the stars with simplified approach to avoid flickering
  // Use filled circles instead of gradient approach for consistent rendering
  binaryStarSprite.fillCircle(x1, y1, radius1, color1_core);
  binaryStarSprite.drawCircle(x1, y1, radius1+1, color1_glow); // Add glow ring
  
  binaryStarSprite.fillCircle(x2, y2, radius2, color2_core);
  binaryStarSprite.drawCircle(x2, y2, radius2+1, color2_glow); // Add glow ring
  
  // Push the sprite to the screen with strict bounds checking
  if (binaryStarSpriteCreated && 
      spriteOffsetX >= -spriteWidth/2 && spriteOffsetX <= SCREEN_WIDTH - spriteWidth/2 &&
      spriteOffsetY >= -spriteHeight/2 && spriteOffsetY <= SCREEN_HEIGHT - spriteHeight/2) {
    
    #ifdef ESP32
    // Check memory before push
    if (ESP.getFreeHeap() < 5000) {
      Serial.println("[BinaryStar] Memory critically low before push, using direct draw");
      SpriteManager::safeDeleteSprite(binaryStarSprite, "BinaryStar");
      binaryStarSpriteCreated = false;
      drawBinaryStarDirect();
      return;
    }
    #endif
    
    // Push sprite with full clear of area before
    tft.fillRect(spriteOffsetX, spriteOffsetY, spriteWidth, spriteHeight, BG_COLOR);
    binaryStarSprite.pushSprite(spriteOffsetX, spriteOffsetY);
    
    #ifdef ESP32
    // Check if push caused memory issues
    if (ESP.getFreeHeap() < 4000) {
      Serial.println("[BinaryStar] Memory critically low after push, freeing resources");
      SpriteManager::safeDeleteSprite(binaryStarSprite, "BinaryStar");
      binaryStarSpriteCreated = false;
    }
    #endif
  } else {
    #ifdef ESP32
    Serial.println("[BinaryStar] Sprite position out of bounds, using direct draw");
    #endif
    binaryStarSprite.deleteSprite();
    binaryStarSpriteCreated = false;
    drawBinaryStarDirect();
  }
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
  // Store sprite state before deletion
  bool wasCreated = binaryStarSpriteCreated;
  
  #ifdef ESP32
  Serial.printf("[BinaryStar] Erasing binary star at %d,%d size %d\n", 
                prevBinaryStarX, prevBinaryStarY, prevBinaryStarSize);
  #endif
  
  // Delete the sprite using the safer method if it exists
  if (wasCreated) {
    // Use the new safe delete method from SpriteManager
    SpriteManager::safeDeleteSprite(binaryStarSprite, "BinaryStar");
    binaryStarSpriteCreated = false;
  } else {
    #ifdef ESP32
    Serial.println("[BinaryStar] Erase called, sprite not valid or created.");
    #endif
  }
  
  // Calculate area to clear using saved values
  int eraseSize = 0;
  
  // Use previous saved values if we have them
  if (prevBinaryStarSize > 0) {
    eraseSize = prevBinaryStarSize;
  } else {
    // Fallback calculation
    float scale_val = objectScale * scaleFactor;
    float orbitScaleVal_erase = scale_f(15.0f) * scale_val;
    int orbitRadius_erase = static_cast<int>(scale_i(orbitScaleVal_erase));
    eraseSize = std::max(100, orbitRadius_erase * 3);
  }
  
  // Add extra margin to eraseSize to catch flickering dots
  eraseSize = eraseSize * 1.2;
  
  // Safety bounds check to prevent drawing outside screen
  int left = prevBinaryStarX - eraseSize/2;
  int top = prevBinaryStarY - eraseSize/2;
  if (left < 0) left = 0;
  if (top < 0) top = 0;
  
  int right = left + eraseSize;
  int bottom = top + eraseSize;
  if (right > SCREEN_WIDTH) right = SCREEN_WIDTH;
  if (bottom > SCREEN_HEIGHT) bottom = SCREEN_HEIGHT;
  
  int width = right - left;
  int height = bottom - top;
  
  if (width > 0 && height > 0) {
    #ifdef ESP32
    Serial.printf("[BinaryStar] Clearing screen area %d,%d %dx%d\n", left, top, width, height);
    #endif
    
    // Clear the screen area - use multiple passes to ensure complete clearing
    tft.fillRect(left, top, width, height, BG_COLOR);
    
    // Add a second clear for extra safety in case of memory corruption
    delay(5); // Small delay to allow display to update
    tft.fillRect(left, top, width, height, BG_COLOR);
  } else {
    #ifdef ESP32
    Serial.println("[BinaryStar] Invalid clear region calculated");
    #endif
    
    // Fallback clear - use a minimal reasonable area around object position
    tft.fillRect(prevBinaryStarX - 50, prevBinaryStarY - 50, 100, 100, BG_COLOR);
    
    // Add a second clear for extra safety
    delay(5);
    tft.fillRect(prevBinaryStarX - 50, prevBinaryStarY - 50, 100, 100, BG_COLOR);
  }
  
  // Reset tracking variables
  prevBinaryStarSize = 0;
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
