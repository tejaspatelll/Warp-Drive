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
#include "star.h"           // Include star.h for the Star struct definition
#include "story_mode.h"     // Include for STAR_COUNT definition
#include "sprite_manager.h" // <<< Added this include

// Forward declarations from main sketch
extern TFT_eSPI tft;
extern const int SCREEN_WIDTH;  // Changed to const to match declaration in quiz_popup.h
extern const int SCREEN_HEIGHT; // Changed to const to match declaration in quiz_popup.h
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
// extern TFT_eSprite stationSprite;
// extern bool stationSpriteCreated;
extern SpriteHandle stationHandle; // Use handle from .ino

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
void drawSpaceStation()
{
  int centerX = objectX;
  int centerY = objectY;
  float scale = objectScale * scaleFactor; // Apply global scaling
  unsigned long currentTime = millis();

  // Use sprite-based double buffering for flicker-free rendering
  // Calculate dimensions for the station parts
  int bodyWidth = scale_i(18) * scale;
  int bodyHeight = scale_i(7) * scale;
  int panelWidth = scale_i(5) * scale;
  int panelHeight = scale_i(12) * scale;

  // Calculate total width and height including panels
  int totalWidth = bodyWidth + 2 * panelWidth;
  int totalHeight = std::max(bodyHeight, panelHeight);

  // Calculate the maximum possible extent during rotation
  float diagonal = sqrt(totalWidth * totalWidth + totalHeight * totalHeight);

  // EXIT ANIMATION FIX: Account for maximum possible scale during exit animation
  // Exit animation can scale up to 3.0x, so we need a sprite large enough for that
  float maxPossibleScale = 3.0f;                            // Maximum scale during exit animation
  int spriteSize = ceil(diagonal * maxPossibleScale * 1.2); // Add safety margin for max scale

  // Create sprite if needed
  if (stationHandle.id == 0)
  {
    Serial.printf("[SpaceStation] Creating sprite size: %d (accounts for exit animation scaling)\n", spriteSize);
    SpriteAllocResult res = SpriteManager::create(spriteSize, spriteSize, true, stationHandle);

    if (res != SpriteAllocResult::Success &&
        res != SpriteAllocResult::SuccessShrunk &&
        res != SpriteAllocResult::FellBackToHeap)
    {
      Serial.printf("[SpaceStation] Sprite creation failed! Falling back to direct draw. Result: %d\n", (int)res);
      drawSpaceStationDirect();
      return;
    }
    Serial.println("[SpaceStation] Sprite created successfully");
  }

  // Get sprite reference
  TFT_eSprite *spritePtr = SpriteManager::getSpriteRef(stationHandle);
  if (!spritePtr)
  {
    Serial.println("[SpaceStation] Failed to get sprite reference, using direct draw");
    drawSpaceStationDirect();
    return;
  }

  // Get sprite dimensions
  int spriteWidth = spritePtr->width();
  int spriteHeight = spritePtr->height();

  // Calculate sprite offset to center station
  int spriteOffsetX = centerX - spriteWidth / 2;
  int spriteOffsetY = centerY - spriteHeight / 2;

  // Convert global coordinates to sprite coordinates
  int spriteCenterX = spriteWidth / 2;
  int spriteCenterY = spriteHeight / 2;

  // Clear the sprite buffer
  spritePtr->fillSprite(BG_COLOR);

  // Draw starfield background into sprite
  for (int i = 0; i < STAR_COUNT; i++)
  {
    int spriteStarX = stars[i].x - spriteOffsetX;
    int spriteStarY = stars[i].y - spriteOffsetY;

    if (spriteStarX >= 0 && spriteStarX < spriteWidth &&
        spriteStarY >= 0 && spriteStarY < spriteHeight)
    {
      uint8_t brightness = stars[i].brightness;
      uint16_t color = spritePtr->color565(brightness, brightness, brightness);
      spritePtr->drawPixel(spriteStarX, spriteStarY, color);
    }
  }

  // Scale all dimensions
  int moduleWidth = bodyWidth * 0.8;
  int moduleHeight = bodyHeight;
  int dishRadius = scale_i(2) * scale;
  int lightRadius = std::max(1, scale_i(1));

  // Calculate current rotation angle
  float stationAngle = (currentTime * 0.0002);
  float cosAngle = cos(stationAngle);
  float sinAngle = sin(stationAngle);

  // Rotation transform function using sprite coordinates
  auto rotatePoint = [spriteCenterX, spriteCenterY, cosAngle, sinAngle](int &x, int &y)
  {
    int relX = x - spriteCenterX;
    int relY = y - spriteCenterY;
    x = spriteCenterX + relX * cosAngle - relY * sinAngle;
    y = spriteCenterY + relX * sinAngle + relY * cosAngle;
  };

  // Main station body
  int x1 = spriteCenterX - moduleWidth / 2;
  int y1 = spriteCenterY - moduleHeight / 2;
  int x2 = spriteCenterX + moduleWidth / 2;
  int y2 = spriteCenterY + moduleHeight / 2;

  // Store and rotate the four corners
  int bodyCorners[4][2] = {
      {x1, y1}, {x2, y1}, {x2, y2}, {x1, y2}};

  for (int i = 0; i < 4; i++)
  {
    rotatePoint(bodyCorners[i][0], bodyCorners[i][1]);
  }

  // Draw a filled polygon for the station body
  uint16_t bodyColor = spritePtr->color565(180, 180, 180);
  spritePtr->fillTriangle(
      bodyCorners[0][0], bodyCorners[0][1],
      bodyCorners[1][0], bodyCorners[1][1],
      bodyCorners[2][0], bodyCorners[2][1],
      bodyColor);

  spritePtr->fillTriangle(
      bodyCorners[0][0], bodyCorners[0][1],
      bodyCorners[2][0], bodyCorners[2][1],
      bodyCorners[3][0], bodyCorners[3][1],
      bodyColor);

  // Draw solar panels
  for (int panel = 0; panel < 2; panel++)
  {
    int panelX = (panel == 0) ? spriteCenterX - bodyWidth / 2 - panelWidth / 2 : spriteCenterX + bodyWidth / 2 + panelWidth / 2;
    int panelY = spriteCenterY;

    for (int segment = 0; segment < 4; segment++)
    {
      int segmentHeight = panelHeight / 4;
      int segX1 = panelX - panelWidth / 2;
      int segY1 = panelY - panelHeight / 2 + segment * segmentHeight;
      int segX2 = panelX + panelWidth / 2;
      int segY2 = segY1 + segmentHeight - 1;

      // Store corners before rotation
      int corners[4][2] = {
          {segX1, segY1}, {segX2, segY1}, {segX2, segY2}, {segX1, segY2}};

      // Rotate all corners
      for (int c = 0; c < 4; c++)
      {
        rotatePoint(corners[c][0], corners[c][1]);
      }

      // Draw filled panel
      uint16_t panelColor = spritePtr->color565(40 + segment * 5, 45 + segment * 5, 80 + segment * 10);
      spritePtr->fillTriangle(
          corners[0][0], corners[0][1],
          corners[1][0], corners[1][1],
          corners[2][0], corners[2][1],
          panelColor);

      spritePtr->fillTriangle(
          corners[0][0], corners[0][1],
          corners[2][0], corners[2][1],
          corners[3][0], corners[3][1],
          panelColor);
    }
  }

  // Draw communication dish
  int dishX = spriteCenterX;
  int dishY = spriteCenterY - bodyHeight / 2 - scale_i(2);
  rotatePoint(dishX, dishY);
  spritePtr->fillCircle(dishX, dishY, dishRadius, spritePtr->color565(120, 120, 120));

  // Navigation lights
  // Red light (left)
  int redX = spriteCenterX - bodyWidth / 2;
  int redY = spriteCenterY;
  rotatePoint(redX, redY);
  bool redOn = ((currentTime / 500) % 2 == 0);
  if (redOn)
  {
    spritePtr->fillCircle(redX, redY, lightRadius, COLOR_ERROR);
  }

  // Green light (right)
  int greenX = spriteCenterX + bodyWidth / 2;
  int greenY = spriteCenterY;
  rotatePoint(greenX, greenY);
  bool greenOn = ((currentTime / 500) % 2 == 1);
  if (greenOn)
  {
    spritePtr->fillCircle(greenX, greenY, lightRadius, COLOR_GREEN);
  }

  // White strobe (top)
  int strobeX = spriteCenterX;
  int strobeY = spriteCenterY - bodyHeight / 2;
  rotatePoint(strobeX, strobeY);
  bool strobeOn = ((currentTime / 2000) % 4 == 0);
  if (strobeOn)
  {
    spritePtr->fillCircle(strobeX, strobeY, lightRadius, COLOR_STARFIELD);
  }

  // Draw windows with blinking lights
  for (int i = 0; i < 3; i++)
  {
    int winX = spriteCenterX - bodyWidth / 4 + i * (bodyWidth / 4);
    int winY = spriteCenterY;
    rotatePoint(winX, winY);

    // Window light effect
    bool windowOn = ((currentTime / 1000) + i) % 3 == 0;
    uint16_t windowColor = windowOn ? spritePtr->color565(255, 255, 150) : spritePtr->color565(100, 100, 80);

    spritePtr->drawPixel(winX, winY, windowColor);
    if (scale > 1.0)
    {
      spritePtr->drawPixel(winX, winY + 1, windowColor);
    }
  }

  // Communication beam
  bool beamActive = ((currentTime / 3000) % 2 == 0);
  if (beamActive)
  {
    int beamLength = scale_i(10);
    int beamEndX = dishX + cos(stationAngle + PI / 4) * beamLength;
    int beamEndY = dishY + sin(stationAngle + PI / 4) * beamLength;
    spritePtr->drawLine(dishX, dishY, beamEndX, beamEndY, spritePtr->color565(70, 70, 255));

    // Add beam animation
    if ((currentTime / 200) % 2 == 0)
    {
      int midX = dishX + cos(stationAngle + PI / 4) * beamLength * 0.5;
      int midY = dishY + sin(stationAngle + PI / 4) * beamLength * 0.5;
      spritePtr->drawPixel(midX, midY, spritePtr->color565(200, 200, 255));
    }
  }

  // Push the final sprite to the screen (double buffering complete!)
  SpriteManager::draw(stationHandle, spriteOffsetX, spriteOffsetY);
}

// Fallback direct drawing function when sprite fails
void drawSpaceStationDirect()
{
  static float prevStationAngle = -100;                // Initial invalid value to force first draw
  static int prevCenterX = -1000, prevCenterY = -1000; // Track previous position
  static int prevMaxRadius = 0;                        // Track previous size for erasing

  int centerX = objectX;
  int centerY = objectY;
  float scale = objectScale * scaleFactor; // Apply global scaling
  unsigned long currentTime = millis();

  // Calculate maximum radius for erasing (including rotation)
  int maxRadius = scale_i(25) * scale; // Large enough to cover rotated station

  // Always erase previous frame because station rotates and lights blink
  // This prevents streaking artifacts
  if (prevMaxRadius > 0)
  {
    tft.fillCircle(prevCenterX, prevCenterY, prevMaxRadius + 2, BG_COLOR);

    // Redraw stars in the erased area to maintain starfield
    for (int i = 0; i < STAR_COUNT; i++)
    {
      int dx = stars[i].x - prevCenterX;
      int dy = stars[i].y - prevCenterY;
      int distSq = dx * dx + dy * dy;
      int radiusSq = (prevMaxRadius + 2) * (prevMaxRadius + 2);

      if (distSq <= radiusSq)
      {
        uint8_t brightness = stars[i].brightness;
        uint16_t color = tft.color565(brightness, brightness, brightness);
        tft.drawPixel(stars[i].x, stars[i].y, color);
      }
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

  // Calculate current rotation angle
  float stationAngle = (currentTime * 0.0002);
  float cosAngle = cos(stationAngle);
  float sinAngle = sin(stationAngle);

  // Rotation transform function
  auto rotatePoint = [centerX, centerY, cosAngle, sinAngle](int &x, int &y)
  {
    int relX = x - centerX;
    int relY = y - centerY;
    x = centerX + relX * cosAngle - relY * sinAngle;
    y = centerY + relX * sinAngle + relY * cosAngle;
  };

  // Main station body
  int x1 = centerX - moduleWidth / 2;
  int y1 = centerY - moduleHeight / 2;
  int x2 = centerX + moduleWidth / 2;
  int y2 = centerY + moduleHeight / 2;

  // Store and rotate the four corners
  int bodyCorners[4][2] = {
      {x1, y1}, {x2, y1}, {x2, y2}, {x1, y2}};

  for (int i = 0; i < 4; i++)
  {
    rotatePoint(bodyCorners[i][0], bodyCorners[i][1]);
  }

  // Draw a filled polygon for the station body
  uint16_t bodyColor = tft.color565(180, 180, 180);
  tft.fillTriangle(
      bodyCorners[0][0], bodyCorners[0][1],
      bodyCorners[1][0], bodyCorners[1][1],
      bodyCorners[2][0], bodyCorners[2][1],
      bodyColor);

  tft.fillTriangle(
      bodyCorners[0][0], bodyCorners[0][1],
      bodyCorners[2][0], bodyCorners[2][1],
      bodyCorners[3][0], bodyCorners[3][1],
      bodyColor);

  // Draw solar panels
  for (int panel = 0; panel < 2; panel++)
  {
    int panelX = (panel == 0) ? centerX - bodyWidth / 2 - panelWidth / 2 : centerX + bodyWidth / 2 + panelWidth / 2;
    int panelY = centerY;

    for (int segment = 0; segment < 4; segment++)
    {
      int segmentHeight = panelHeight / 4;
      int segX1 = panelX - panelWidth / 2;
      int segY1 = panelY - panelHeight / 2 + segment * segmentHeight;
      int segX2 = panelX + panelWidth / 2;
      int segY2 = segY1 + segmentHeight - 1;

      // Store corners before rotation
      int corners[4][2] = {
          {segX1, segY1}, {segX2, segY1}, {segX2, segY2}, {segX1, segY2}};

      // Rotate all corners
      for (int c = 0; c < 4; c++)
      {
        rotatePoint(corners[c][0], corners[c][1]);
      }

      // Draw filled panel
      uint16_t panelColor = tft.color565(40 + segment * 5, 45 + segment * 5, 80 + segment * 10);
      tft.fillTriangle(
          corners[0][0], corners[0][1],
          corners[1][0], corners[1][1],
          corners[2][0], corners[2][1],
          panelColor);

      tft.fillTriangle(
          corners[0][0], corners[0][1],
          corners[2][0], corners[2][1],
          corners[3][0], corners[3][1],
          panelColor);
    }
  }

  // Navigation lights and minimal details
  // Communication dish
  int dishX = centerX;
  int dishY = centerY - bodyHeight / 2 - scale_i(2);
  rotatePoint(dishX, dishY);
  tft.fillCircle(dishX, dishY, dishRadius, tft.color565(120, 120, 120));

  // Only draw lights without saving previous state
  int redX = centerX - bodyWidth / 2;
  int redY = centerY;
  rotatePoint(redX, redY);
  if ((currentTime / 500) % 2 == 0)
  {
    tft.fillCircle(redX, redY, lightRadius, COLOR_ERROR);
  }

  int greenX = centerX + bodyWidth / 2;
  int greenY = centerY;
  rotatePoint(greenX, greenY);
  if ((currentTime / 500) % 2 == 1)
  {
    tft.fillCircle(greenX, greenY, lightRadius, COLOR_GREEN);
  }

  prevStationAngle = stationAngle;

  // Store current values for next frame erasing
  prevCenterX = centerX;
  prevCenterY = centerY;
  prevMaxRadius = maxRadius;
}

void eraseSpaceStation()
{
  // Use handle and SpriteManager::destroy
  if (stationHandle.id != 0)
  {
    Serial.println("[SpaceStation] Erasing using SpriteManager::destroy");
    // Find the entry to get dimensions for clearing the screen area
    int16_t spriteW = SpriteManager::getWidth(stationHandle);
    int16_t spriteH = SpriteManager::getHeight(stationHandle);
    if (spriteW > 0 && spriteH > 0)
    { // Check if dimensions are valid
      // Calculate screen coords based on where it *was* drawn
      int spriteOffsetX = objectX - spriteW / 2;
      int spriteOffsetY = objectY - spriteH / 2;
      tft.fillRect(spriteOffsetX, spriteOffsetY, spriteW, spriteH, BG_COLOR);
    }
    else
    {
      // Fallback: guess a size to clear if dimensions couldn't be retrieved
      Serial.println("[SpaceStation] Warning: Could not get sprite dimensions for clearing. Guessing size.");
      int clearSize = 120; // Guess size
      tft.fillRect(objectX - clearSize / 2, objectY - clearSize / 2, clearSize, clearSize, BG_COLOR);
    }
    SpriteManager::destroy(stationHandle);
    stationHandle = {0}; // Invalidate handle
  }
  else
  {
    // If no sprite handle, try to erase based on direct drawing
    // This requires knowing the size used in drawSpaceStationDirect
    Serial.println("[SpaceStation] Erasing direct draw area (estimated).");
    float scale = objectScale * scaleFactor;
    int bodyWidth = scale_i(18) * scale;
    int bodyHeight = scale_i(7) * scale;
    int panelWidth = scale_i(5) * scale;
    int panelHeight = scale_i(12) * scale;
    int totalWidth = bodyWidth + 2 * panelWidth;
    int totalHeight = std::max(bodyHeight, panelHeight);
    float diagonal = sqrt(totalWidth * totalWidth + totalHeight * totalHeight);

    // EXIT ANIMATION FIX: Account for maximum possible scale during exit animation
    float maxPossibleScale = 3.0f;                           // Maximum scale during exit animation
    int clearSize = ceil(diagonal * maxPossibleScale * 1.2); // Match sprite size calculation
    tft.fillRect(objectX - clearSize / 2, objectY - clearSize / 2, clearSize, clearSize, BG_COLOR);
  }
}

#endif // SPACESTATION_H
