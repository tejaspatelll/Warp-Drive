#ifndef STAR_H
#define STAR_H

#include <TFT_eSPI.h>

// Forward declarations of external variables
extern TFT_eSPI tft;
extern uint16_t BG_COLOR;
extern int objectX;
extern int objectY;
extern float objectScale;

// Star structure and related constants
struct Star {
  float realX;          // Actual X position (float for smooth warp movement)
  float realY;          // Actual Y position
  uint8_t x;            // Integer X position for drawing
  uint8_t y;            // Integer Y position for drawing
  uint8_t brightness;   // Brightness level (150-255)
  bool increasing;      // Brightness direction flag
  uint8_t streakLength; // Length of streak in warp mode
};

/**
 * Draws a single star with specified brightness
 */
void drawStar(const Star& star) {
  uint16_t color = tft.color565(star.brightness, star.brightness, star.brightness);
  tft.drawPixel(star.x, star.y, color);
}

/**
 * Draws a star with flares and light variations
 */
void drawStar() {
  int centerX = objectX;
  int centerY = objectY;
  float scale = objectScale;
  
  // Base size
  int radius = 8 * scale;
  uint16_t starColor = TFT_WHITE;
  
  // Draw core with glow
  for (int r = radius; r > 0; r--) {
    float intensity = map(r, 0, radius, 255, 50) / 255.0;
    uint16_t color = tft.color565(
      255 * intensity,
      255 * intensity,
      240 * intensity
    );
    tft.drawCircle(centerX, centerY, r, color);
  }
  tft.fillCircle(centerX, centerY, radius/2, starColor);
  
  // Draw starflares
  int flareLength = radius * 1.5;
  for (int i = 0; i < 4; i++) {
    float angle = i * PI / 2.0;
    int endX = centerX + flareLength * cos(angle);
    int endY = centerY + flareLength * sin(angle);
    
    // Draw flare with gradient
    for (int j = 0; j < flareLength; j++) {
      int x = centerX + j * cos(angle);
      int y = centerY + j * sin(angle);
      
      // Apply gradient
      float brightness = 1.0 - (float)j / flareLength;
      uint16_t flareColor = tft.color565(
        255 * brightness,
        255 * brightness,
        240 * brightness
      );
      
      tft.drawPixel(x, y, flareColor);
    }
  }
}

/**
 * Erases a star
 */
void eraseStar() {
  int centerX = objectX;
  int centerY = objectY;
  float scale = objectScale;
  
  // Erase with a circle slightly larger than the star + flares
  int eraseRadius = 8 * scale * 1.6;
  tft.fillCircle(centerX, centerY, eraseRadius, BG_COLOR);
}

/**
 * Draws a star with a glow effect and subtle color variations
 */
void drawStarWithGlow(int x, int y, int radius, uint16_t baseColor) {
  // Draw glow
  for (int r = radius + 2; r > radius; r--) {
    uint8_t brightness = map(r, radius, radius + 2, 200, 100);
    uint16_t glowColor = tft.color565(
      ((baseColor >> 11) & 0x1F) * brightness / 255,
      ((baseColor >> 5) & 0x3F) * brightness / 255,
      (baseColor & 0x1F) * brightness / 255
    );
    tft.drawCircle(x, y, r, glowColor);
  }

  // Draw core with subtle color variations
  for (int r = radius; r > 0; r--) {
    float intensity = map(r, 0, radius, 255, 150) / 255.0;
    uint8_t variation = random(-20, 21); // Subtle color variation
    
    // Calculate RGB components with proper type handling
    float rComponent = ((baseColor >> 11) & 0x1F) * intensity * 8 + variation;
    float gComponent = ((baseColor >> 5) & 0x3F) * intensity * 4 + variation;
    float bComponent = (baseColor & 0x1F) * intensity * 8 + variation;
    
    // Use proper type for min/max operations (renamed variables to avoid conflict)
    uint8_t red = constrain(rComponent, 0, 255);
    uint8_t green = constrain(gComponent, 0, 255);
    uint8_t blue = constrain(bComponent, 0, 255);
    
    uint16_t color = tft.color565(red, green, blue);
    tft.drawCircle(x, y, r, color);
  }
}

#endif // STAR_H 