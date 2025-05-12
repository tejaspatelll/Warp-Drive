/**
 * spacestation.h
 * 
 * This file contains all code related to drawing and managing a space station
 * object in the Warp Drive visualization.
 */

#ifndef SPACESTATION_H
#define SPACESTATION_H

#include <TFT_eSPI.h>
#include <Arduino.h>
#include <cmath>
#include "star.h" // Include star.h for the Star struct definition
#include "story_mode.h" // Include for STAR_COUNT definition
#include "sprite_manager.h" // <<< Added this include

// Forward declarations from main sketch
extern TFT_eSPI tft;
extern const int SCREEN_WIDTH;   // Changed to const to match declaration in quiz_popup.h
extern const int SCREEN_HEIGHT;  // Changed to const to match declaration in quiz_popup.h
extern uint16_t BG_COLOR;
extern uint16_t COLOR_ERROR;
extern uint16_t COLOR_GREEN;
extern uint16_t COLOR_STARFIELD;
extern float scaleX;
extern float scaleY;
extern float scaleFactor;

// Object position and parameters shared with main sketch
extern int objectX;
extern int objectY;
extern float objectScale;

// Star data from main sketch - STAR_COUNT is defined in story_mode.h
extern Star stars[];

// Declare sprite for space station rendering
extern TFT_eSprite stationSprite;
extern bool stationSpriteCreated;

static bool forceRedrawStation = true; // Flag to force redraw of station sprite content

// Helper function declarations
extern int scale_i(int v);

// Function to convert angle to radians (in case not available)
#ifndef PI
#define PI 3.14159265358979323846
#endif

// Forward declarations for our own functions
void drawSpaceStationDirect();
void eraseSpaceStation();

/**
 * Draws a space station with improved blinking lights and realistic details
 * Using double buffering approach with sprites for smoother animation
 */
void drawSpaceStation() {
  int centerX = objectX;
  int centerY = objectY;
  float scale = objectScale * scaleFactor; // Apply global scaling
  unsigned long currentTime = millis();
  
  // Create sprite if needed
  if (!stationSpriteCreated) {
    // Calculate dimensions for the station parts
    int bodyWidth = scale_i(18) * scale;
    int bodyHeight = scale_i(7) * scale;
    int panelWidth = scale_i(5) * scale;
    int panelHeight = scale_i(12) * scale;
    
    // Calculate total width and height including panels
    int totalWidth = bodyWidth + 2 * panelWidth; // Main body + both solar panels
    int totalHeight = std::max(bodyHeight, panelHeight); // Whichever is taller
    
    // Calculate the maximum possible extent during rotation
    // For a rotating object, we need a square buffer that can contain the
    // diagonal of the object at any rotation angle
    float diagonal = sqrt(totalWidth * totalWidth + totalHeight * totalHeight);
    
    // Add a safety margin (20% extra)
    int spriteSize = ceil(diagonal * 1.2);
    
    // Log the calculations for debugging
    Serial.printf("Station dimensions - Body: %dx%d, Panels: %dx%d\n", 
                  bodyWidth, bodyHeight, panelWidth, panelHeight);
    Serial.printf("Total size: %dx%d, Diagonal: %.1f, Final sprite size: %d\n", 
                  totalWidth, totalHeight, diagonal, spriteSize);
    
    // Create sprite if not already done (or if it needs recreation due to size change)
    if (!stationSpriteCreated) {
        SpriteManager::safeDeleteSprite(stationSprite, "SpaceStation"); // Delete before creating new
        SpriteManager::createObjectSprite(stationSprite, spriteSize, "SpaceStation", true);
        stationSpriteCreated = (stationSprite.width() > 0 && stationSprite.height() > 0);
        forceRedrawStation = true; // Force redraw after creation
    }
  }
  
  // If sprite creation failed or sprite was deleted, draw directly to screen
  if (!stationSpriteCreated) {
    // Fall back to drawing directly on the screen - clear previous drawing first
    eraseSpaceStation();
    drawSpaceStationDirect();
    return;
  }
  
  // Get sprite dimensions
  int spriteWidth = stationSprite.width();
  int spriteHeight = stationSprite.height();
  
  // Calculate sprite offset to center station
  int spriteOffsetX = centerX - spriteWidth / 2;
  int spriteOffsetY = centerY - spriteHeight / 2;
  
  // Convert global coordinates to sprite coordinates
  int spriteCenterX = spriteWidth / 2;
  int spriteCenterY = spriteHeight / 2;
  
  // Clear the sprite buffer
  stationSprite.fillSprite(BG_COLOR);
  
  // Draw starfield background from main stars array to maintain continuity
  for (int i = 0; i < STAR_COUNT; i++) {
    int spriteStarX = stars[i].x - spriteOffsetX;
    int spriteStarY = stars[i].y - spriteOffsetY;
    
    // Only draw stars that would appear within sprite bounds
    if (spriteStarX >= 0 && spriteStarX < spriteWidth && 
        spriteStarY >= 0 && spriteStarY < spriteHeight) {
      uint8_t brightness = stars[i].brightness;
      uint16_t color = stationSprite.color565(brightness, brightness, brightness);
      stationSprite.drawPixel(spriteStarX, spriteStarY, color);
    }
  }
  
  // Scale all dimensions
  int bodyWidth = scale_i(18) * scale;
  int bodyHeight = scale_i(7) * scale;
  int moduleWidth = bodyWidth * 0.8;
  int moduleHeight = bodyHeight;
  int panelWidth = scale_i(5) * scale;
  int panelHeight = scale_i(12) * scale;
  int dishRadius = scale_i(2) * scale;
  int lightRadius = std::max(1, scale_i(1));
  
  // Calculate current rotation angle (make rotation speed configurable)
  float stationAngle = (currentTime * 0.0002);
  float cosAngle = cos(stationAngle);
  float sinAngle = sin(stationAngle);
  
  // Rotation transform function using sprite coordinates
  auto rotatePoint = [spriteCenterX, spriteCenterY, cosAngle, sinAngle](int& x, int& y) {
    int relX = x - spriteCenterX;
    int relY = y - spriteCenterY;
    x = spriteCenterX + relX * cosAngle - relY * sinAngle;
    y = spriteCenterY + relX * sinAngle + relY * cosAngle;
  };
  
  // Main station body
  int x1 = spriteCenterX - moduleWidth/2;
  int y1 = spriteCenterY - moduleHeight/2;
  int x2 = spriteCenterX + moduleWidth/2;
  int y2 = spriteCenterY + moduleHeight/2;
  
  // Store and rotate the four corners
  int bodyCorners[4][2] = {
    {x1, y1}, {x2, y1}, {x2, y2}, {x1, y2}
  };
  
  for (int i = 0; i < 4; i++) {
    rotatePoint(bodyCorners[i][0], bodyCorners[i][1]);
  }
  
  // Draw a filled polygon for the station body
  uint16_t bodyColor = stationSprite.color565(180, 180, 180);
  stationSprite.fillTriangle(
    bodyCorners[0][0], bodyCorners[0][1],
    bodyCorners[1][0], bodyCorners[1][1],
    bodyCorners[2][0], bodyCorners[2][1],
    bodyColor
  );
  
  stationSprite.fillTriangle(
    bodyCorners[0][0], bodyCorners[0][1],
    bodyCorners[2][0], bodyCorners[2][1],
    bodyCorners[3][0], bodyCorners[3][1],
    bodyColor
  );
  
  // Draw solar panels
  for (int panel = 0; panel < 2; panel++) {
    int panelX = (panel == 0) ? 
      spriteCenterX - bodyWidth/2 - panelWidth/2 : 
      spriteCenterX + bodyWidth/2 + panelWidth/2;
    int panelY = spriteCenterY;
    
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
      }
      
      // Draw filled panel
      uint16_t panelColor = stationSprite.color565(40 + segment*5, 45 + segment*5, 80 + segment*10);
      stationSprite.fillTriangle(
        corners[0][0], corners[0][1],
        corners[1][0], corners[1][1],
        corners[2][0], corners[2][1],
        panelColor
      );
      
      stationSprite.fillTriangle(
        corners[0][0], corners[0][1],
        corners[2][0], corners[2][1],
        corners[3][0], corners[3][1],
        panelColor
      );
      
      // Draw panel borders
      uint16_t panelOutlineColor = stationSprite.color565(30 + segment*5, 35 + segment*5, 70 + segment*10);
      for (int c = 0; c < 4; c++) {
        int next = (c + 1) % 4;
        stationSprite.drawLine(
          corners[c][0], corners[c][1],
          corners[next][0], corners[next][1],
          panelOutlineColor
        );
      }
    }
  }

  // Draw communication dish
  int dishX = spriteCenterX;
  int dishY = spriteCenterY - bodyHeight/2 - scale_i(2);
  rotatePoint(dishX, dishY);
  stationSprite.fillCircle(dishX, dishY, dishRadius, stationSprite.color565(120, 120, 120));
  
  // Navigation lights
  // Red light (left)
  int redX = spriteCenterX - bodyWidth/2;
  int redY = spriteCenterY;
  rotatePoint(redX, redY);
  bool redOn = ((currentTime / 500) % 2 == 0);
  if (redOn) {
    stationSprite.fillCircle(redX, redY, lightRadius, COLOR_ERROR);
  }
  
  // Green light (right)
  int greenX = spriteCenterX + bodyWidth/2;
  int greenY = spriteCenterY;
  rotatePoint(greenX, greenY);
  bool greenOn = ((currentTime / 500) % 2 == 1);
  if (greenOn) {
    stationSprite.fillCircle(greenX, greenY, lightRadius, COLOR_GREEN);
  }
  
  // White strobe (top)
  int strobeX = spriteCenterX;
  int strobeY = spriteCenterY - bodyHeight/2;
  rotatePoint(strobeX, strobeY);
  bool strobeOn = ((currentTime / 2000) % 4 == 0);
  if (strobeOn) {
    stationSprite.fillCircle(strobeX, strobeY, lightRadius, COLOR_STARFIELD);
  }
  
  // Draw windows with blinking lights
  for (int i = 0; i < 3; i++) {
    int winX = spriteCenterX - bodyWidth/4 + i * (bodyWidth/4);
    int winY = spriteCenterY;
    rotatePoint(winX, winY);
    
    // Window light effect
    bool windowOn = ((currentTime / 1000) + i) % 3 == 0;
    uint16_t windowColor = windowOn ? 
      stationSprite.color565(255, 255, 150) : stationSprite.color565(100, 100, 80);
    
    stationSprite.drawPixel(winX, winY, windowColor);
    if (scale > 1.0) {
      stationSprite.drawPixel(winX, winY+1, windowColor);
    }
  }
  
  // Communication beam
  bool beamActive = ((currentTime / 3000) % 2 == 0);
  if (beamActive) {
    int beamLength = scale_i(10);
    int beamEndX = dishX + cos(stationAngle + PI/4) * beamLength;
    int beamEndY = dishY + sin(stationAngle + PI/4) * beamLength;
    stationSprite.drawLine(dishX, dishY, beamEndX, beamEndY, stationSprite.color565(70, 70, 255));
    
    // Add beam animation
    if ((currentTime / 200) % 2 == 0) {
      int midX = dishX + cos(stationAngle + PI/4) * beamLength * 0.5;
      int midY = dishY + sin(stationAngle + PI/4) * beamLength * 0.5;
      stationSprite.drawPixel(midX, midY, stationSprite.color565(200, 200, 255));
    }
  }
  
  // Push the sprite to the screen - error handling for memory issues
  #ifdef ESP32
  if (stationSpriteCreated) {
    stationSprite.pushSprite(spriteOffsetX, spriteOffsetY);
    
    // Check if we've run into low memory issues
    int freeHeap = ESP.getFreeHeap();
  //  Serial.printf("Free heap after drawing: %d bytes\n", freeHeap);
    if (freeHeap < 8000) {  // Use a higher threshold to be safe
      Serial.println("Low memory detected, freeing sprite resources");
      SpriteManager::safeDeleteSprite(stationSprite, "SpaceStation");
      stationSpriteCreated = false;
    }
  } else {
    // If sprite wasn't created, use direct drawing 
    drawSpaceStationDirect();
  }
  #else
  if (stationSpriteCreated) {
    stationSprite.pushSprite(spriteOffsetX, spriteOffsetY);
  } else {
    drawSpaceStationDirect();
  }
  #endif
  
  // Check if we've run into low memory issues (simplified check)
  #ifdef ESP32
  if (ESP.getFreeHeap() < 4000) {  // Use a reasonable threshold
    Serial.println("Low memory detected, freeing sprite resources");
    // Free sprite and try direct drawing next time
    if (stationSpriteCreated) {
      SpriteManager::safeDeleteSprite(stationSprite, "SpaceStation");
      stationSpriteCreated = false;
    }
  }
  #endif
}

// Fallback direct drawing function when sprite fails
void drawSpaceStationDirect() {
  static float prevStationAngle = -100; // Initial invalid value to force first draw
  int centerX = objectX;
  int centerY = objectY;
  float scale = objectScale * scaleFactor; // Apply global scaling
  unsigned long currentTime = millis();
  
  // Scale all dimensions
  int bodyWidth = scale_i(18) * scale;
  int bodyHeight = scale_i(7) * scale;
  int moduleWidth = bodyWidth * 0.8;
  int moduleHeight = bodyHeight;
  int panelWidth = scale_i(5) * scale;
  int panelHeight = scale_i(12) * scale;
  int dishRadius = scale_i(2) * scale;
  int lightRadius = std::max(1, scale_i(1));
  
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
  
  // Main station body
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
  
  // Draw solar panels
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
      }
      
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
    }
  }
  
  // Navigation lights and minimal details
  // Communication dish
  int dishX = centerX;
  int dishY = centerY - bodyHeight/2 - scale_i(2);
  rotatePoint(dishX, dishY);
  tft.fillCircle(dishX, dishY, dishRadius, tft.color565(120, 120, 120));
  
  // Only draw lights without saving previous state
  int redX = centerX - bodyWidth/2;
  int redY = centerY;
  rotatePoint(redX, redY);
  if ((currentTime / 500) % 2 == 0) {
    tft.fillCircle(redX, redY, lightRadius, COLOR_ERROR);
  }
  
  int greenX = centerX + bodyWidth/2;
  int greenY = centerY;
  rotatePoint(greenX, greenY);
  if ((currentTime / 500) % 2 == 1) {
    tft.fillCircle(greenX, greenY, lightRadius, COLOR_GREEN);
  }
  
  prevStationAngle = stationAngle;
}

void eraseSpaceStation() {
  if (stationSpriteCreated) {
    // Free sprite resources
    SpriteManager::safeDeleteSprite(stationSprite, "SpaceStation");
    stationSpriteCreated = false;
    #ifdef ESP32
    Serial.printf("Free heap after sprite deletion: %d bytes\n", ESP.getFreeHeap());
    #endif
  }
  
  // We still need to clear the area on screen where the space station was drawn
  // Calculate the maximum possible extent of the station based on its dimensions
  float scale = objectScale * scaleFactor;
  int bodyWidth = scale_i(18) * scale;
  int bodyHeight = scale_i(7) * scale;
  int panelWidth = scale_i(5) * scale;
  int panelHeight = scale_i(12) * scale;
  
  // Calculate total width and height including panels
  int totalWidth = bodyWidth + 2 * panelWidth; // Main body + both solar panels
  int totalHeight = std::max(bodyHeight, panelHeight); // Whichever is taller
  
  // Calculate the diagonal extent for rotation
  float diagonal = sqrt(totalWidth * totalWidth + totalHeight * totalHeight);
  
  // Add a safety margin (20% extra) and round up
  int eraseSize = ceil(diagonal * 1.2);
  
  // Clear the entire station area with the calculated size
  tft.fillRect(objectX - eraseSize/2, objectY - eraseSize/2, eraseSize, eraseSize, BG_COLOR);
}

#endif // SPACESTATION_H

