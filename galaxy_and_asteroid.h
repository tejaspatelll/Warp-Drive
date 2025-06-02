/**
 * galaxy_and_asteroid.h
 * 
 * This file contains all code related to drawing and managing galaxy and asteroid field
 * objects in the Warp Drive visualization.
 */

#ifndef GALAXY_AND_ASTEROID_H
#define GALAXY_AND_ASTEROID_H

#include <TFT_eSPI.h>
#include <Arduino.h>
#include <cmath>
#include <algorithm>  // for std::min/max

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

// Remove extern and directly use the constants
#define MAX_GALAXY_ARMS 6
#define MAX_GALAXY_POINTS 50

// Forward declare functions from main sketch
extern int scale_i(int v);
extern float scale_f(float v);
extern void displayObjectName(const char* name);

// Define PI if not already defined
#ifndef PI
#define PI 3.14159265358979323846
#endif

// Galaxy globals
extern int prevGalaxyCenterX, prevGalaxyCenterY;
extern int prevGalaxyCoreRadius;
extern int prevGalaxyPoints[MAX_GALAXY_ARMS][MAX_GALAXY_POINTS][2]; // [arm][point][x,y]
extern int prevGalaxyPointCount[MAX_GALAXY_ARMS];

// Asteroid field globals
#define MAX_ASTEROIDS 15
struct Asteroid {
  float x, y;
  float vx, vy;
  int radius;
  int prevX, prevY;
};
extern Asteroid asteroids[MAX_ASTEROIDS];
extern bool asteroidFieldInitialized;

// Forward declarations
void eraseCelestialObject();
void eraseGalaxy();
void eraseAsteroidField();
void drawGalaxy();
void drawAsteroidField();

/**
 * Erases the galaxy by clearing its components
 */
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

/**
 * Erases the asteroid field by clearing all asteroids
 */
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

/**
 * Draws a spiral galaxy with a bright core and colored stars
 */
void drawGalaxy() {
  // Constants for spiral galaxy generation
  const int numArms = std::min(5, MAX_GALAXY_ARMS); // Use 5 arms or max available
  const float armSeparationDistance = 2 * PI / numArms;
  const float armOffsetMax = 0.5f;
  const float rotationFactor = 5;
  const float randomOffsetXY = scale_f(2.0f); // Scale random offset
  
  int centerX = objectX;
  int centerY = objectY;
  int coreRadius = scale_i(1) * objectScale;
  
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
  float galaxyScaleFactor = 25.0f * objectScale * scaleFactor; 
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
      armOffset = armOffset * (1 / std::max(distance, 0.1f));
      
      // Apply squared offset with sign preservation
      float squaredArmOffset = armOffset * abs(armOffset);
      
      // Apply rotation that increases with distance
      float rotation = distanceSquared * rotationFactor;
      
      // Calculate final angle with time-based variation
      float angle = arm * armSeparationDistance + squaredArmOffset + rotation;
      
      // Convert to cartesian coordinates
      float radius = galaxyScaleFactor * distance;
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

/**
 * Draws a dynamic asteroid field with moving, glowing asteroids
 */
void drawAsteroidField() {
  if (!asteroidFieldInitialized) {
    int fieldWidth = SCREEN_WIDTH;
    int fieldHeight = SCREEN_HEIGHT;
    int centerX = objectX;
    int centerY = objectY;
    
    // Initialize asteroids with more variety
    for (int i = 0; i < MAX_ASTEROIDS; i++) {
      asteroids[i].x = centerX + random(-fieldWidth, fieldWidth);
      asteroids[i].y = centerY + random(-fieldHeight, fieldHeight);
      
      // Scale speeds
      float speed = random(1, 4) * scaleFactor + random(0, 100) / 100.0f;
      float angle = random(0, 360) * PI / 180.0f;
      asteroids[i].vx = cos(angle) * speed;
      asteroids[i].vy = sin(angle) * speed;
      
      // Scale sizes
      asteroids[i].radius = scale_i(random(1, 4));
      asteroids[i].prevX = round(asteroids[i].x);
      asteroids[i].prevY = round(asteroids[i].y);
    }
    asteroidFieldInitialized = true;
  }

  // Scale field dimensions
  int fieldWidth = SCREEN_WIDTH;
  int fieldHeight = SCREEN_HEIGHT;
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

#endif // GALAXY_AND_ASTEROID_H
