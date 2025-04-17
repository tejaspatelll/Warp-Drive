#ifndef SUPERNOVA_H
#define SUPERNOVA_H

#include <TFT_eSPI.h>

// Forward declarations of external variables and constants
extern TFT_eSPI tft;
extern uint16_t BG_COLOR;
extern const int SCREEN_WIDTH;
extern const int SCREEN_HEIGHT;
extern int objectX, objectY;
extern float objectScale;

// Supernova parameters
#define MAX_SUPERNOVA_PARTICLES 60

// Struct for supernova particles
struct SupernovaParticle {
  float x, y;          // Position
  float vx, vy;        // Velocity
  int brightness;      // Brightness
  uint16_t color;      // Color
  int prevX, prevY;    // Previous position
  bool active;         // Whether the particle is active
};

// These variables are visible only within this module
namespace {
  SupernovaParticle supernovaParticles[MAX_SUPERNOVA_PARTICLES];
  bool supernovaInitialized = false;
  unsigned long supernovaStartTime = 0;
  int supernovaPhase = 0;  // 0=initial, 1=expanding, 2=fading
  int supernovaRadius = 0;
  int prevSupernovaX = 0, prevSupernovaY = 0;
}

/**
 * Draws a supernova - an exploding star with expanding shock wave and debris
 */
void drawSupernova() {
  int centerX = objectX;
  int centerY = objectY;
  float scale = objectScale;
  unsigned long currentTime = millis();
  
  if (!supernovaInitialized) {
    prevSupernovaX = centerX;
    prevSupernovaY = centerY;
    supernovaRadius = 5 * scale;
    supernovaStartTime = currentTime;
    supernovaPhase = 0;
    
    // Initialize explosion particles (initially inactive)
    for (int i = 0; i < MAX_SUPERNOVA_PARTICLES; i++) {
      supernovaParticles[i].active = false;
      supernovaParticles[i].x = centerX;
      supernovaParticles[i].y = centerY;
      
      // Pre-assign colors for later use
      int colorChoice = random(4);
      if (colorChoice == 0) {
        supernovaParticles[i].color = tft.color565(255, 255, 200); // White-yellow
      } else if (colorChoice == 1) {
        supernovaParticles[i].color = tft.color565(255, 150, 50);  // Orange
      } else if (colorChoice == 2) {
        supernovaParticles[i].color = tft.color565(255, 50, 50);   // Red
      } else {
        supernovaParticles[i].color = tft.color565(200, 200, 255); // Blue-white
      }
      
      // Store initial position for erasing
      supernovaParticles[i].prevX = centerX;
      supernovaParticles[i].prevY = centerY;
    }
    
    // Draw initial star
    uint16_t starColor = tft.color565(255, 200, 100); // Yellow-orange
    tft.fillCircle(centerX, centerY, supernovaRadius, starColor);
    
    supernovaInitialized = true;
  }
  
  // Time since supernova started
  unsigned long elapsedTime = currentTime - supernovaStartTime;
  
  // Phase transitions
  if (supernovaPhase == 0 && elapsedTime > 1000) {
    supernovaPhase = 1; // Start expansion
    
    // Activate all particles
    for (int i = 0; i < MAX_SUPERNOVA_PARTICLES; i++) {
      supernovaParticles[i].active = true;
      supernovaParticles[i].brightness = 255;
      
      // Random velocity in all directions
      float angle = random(0, 360) * PI / 180.0f;
      float speed = random(10, 20) / 10.0f * scale;
      supernovaParticles[i].vx = cos(angle) * speed;
      supernovaParticles[i].vy = sin(angle) * speed;
    }
  } else if (supernovaPhase == 1 && elapsedTime > 3000) {
    supernovaPhase = 2; // Start fading
  }
  
  // Draw supernova based on current phase
  if (supernovaPhase == 0) {
    // Pre-explosion: star grows brighter and pulses
    float time = currentTime / 200.0f;
    float pulseFactor = 0.8f + 0.2f * sin(time);
    
    // Gradually increase brightness
    float brightness = min(1.0f, (float)elapsedTime / 1000.0f) * pulseFactor;
    
    uint16_t starColor = tft.color565(
      255 * brightness, 
      200 * brightness, 
      100 * brightness
    );
    
    // Erase and redraw star with new brightness
    tft.fillCircle(centerX, centerY, supernovaRadius, BG_COLOR);
    tft.fillCircle(centerX, centerY, supernovaRadius, starColor);
    
  } else if (supernovaPhase == 1 || supernovaPhase == 2) {
    // Clear the center as the star has exploded
    tft.fillCircle(centerX, centerY, supernovaRadius, BG_COLOR);
    
    // Draw shockwave (expanding ring)
    int waveRadius = (int)(5 + (elapsedTime - 1000) / 100.0f * scale);
    int waveWidth = 3 * scale;
    
    // Only draw shockwave during expansion and early fading
    if (elapsedTime < 5000) {
      float waveBrightness = supernovaPhase == 1 ? 1.0f : max(0.0f, 1.0f - (elapsedTime - 3000.0f) / 2000.0f);
      
      for (int w = 0; w < waveWidth; w++) {
        float ringBrightness = waveBrightness * (1.0f - (float)w / waveWidth);
        uint16_t ringColor = tft.color565(
          255 * ringBrightness,
          200 * ringBrightness,
          150 * ringBrightness
        );
        
        tft.drawCircle(centerX, centerY, waveRadius + w, ringColor);
      }
    }
    
    // Update and draw particles
    for (int i = 0; i < MAX_SUPERNOVA_PARTICLES; i++) {
      if (supernovaParticles[i].active) {
        // Erase old position
        tft.drawPixel(supernovaParticles[i].prevX, supernovaParticles[i].prevY, BG_COLOR);
        
        // Update position
        supernovaParticles[i].x += supernovaParticles[i].vx;
        supernovaParticles[i].y += supernovaParticles[i].vy;
        
        // Fade particles in phase 2
        if (supernovaPhase == 2) {
          supernovaParticles[i].brightness = max(0, (int)(supernovaParticles[i].brightness - 2));
          
          // Deactivate completely faded particles
          if (supernovaParticles[i].brightness <= 10) {
            supernovaParticles[i].active = false;
            continue;
          }
        }
        
        // Calculate final color with applied brightness
        float brightnessFactor = supernovaParticles[i].brightness / 255.0f;
        uint16_t baseColor = supernovaParticles[i].color;
        uint8_t r = ((baseColor >> 11) & 0x1F) * brightnessFactor * 8;
        uint8_t g = ((baseColor >> 5) & 0x3F) * brightnessFactor * 4;
        uint8_t b = (baseColor & 0x1F) * brightnessFactor * 8;
        
        uint16_t finalColor = tft.color565(r, g, b);
        
        // Draw at new position
        int x = round(supernovaParticles[i].x);
        int y = round(supernovaParticles[i].y);
        
        if (x >= 0 && x < SCREEN_WIDTH && y >= 0 && y < SCREEN_HEIGHT) {
          tft.drawPixel(x, y, finalColor);
          
          // Store position for next erase
          supernovaParticles[i].prevX = x;
          supernovaParticles[i].prevY = y;
        } else {
          // Deactivate particles that leave the screen
          supernovaParticles[i].active = false;
        }
      }
    }
  }
}

/**
 * Erases the supernova and its particles
 */
void eraseSupernova() {
  if (supernovaInitialized) {
    // Erase the main supernova
    if (prevSupernovaX >= 0 && prevSupernovaX < SCREEN_WIDTH && 
        prevSupernovaY >= 0 && prevSupernovaY < SCREEN_HEIGHT) {
      // Use a larger radius to ensure we clear all visual effects
      int clearRadius = max(supernovaRadius + 5, 30);
      tft.fillCircle(prevSupernovaX, prevSupernovaY, clearRadius, BG_COLOR);
    }
    
    // Erase all particles with a bit of padding
    for (int i = 0; i < MAX_SUPERNOVA_PARTICLES; i++) {
      if (supernovaParticles[i].active) {
        int particleX = round(supernovaParticles[i].x);
        int particleY = round(supernovaParticles[i].y);
        
        if (particleX >= 0 && particleX < SCREEN_WIDTH && 
            particleY >= 0 && particleY < SCREEN_HEIGHT) {
          // Clear with slightly larger area for particles that might have visual blur
          tft.fillCircle(particleX, particleY, 2, BG_COLOR);
        }
      }
    }
    
    // Clear the full shockwave area just to be safe
    int maxShockwaveRadius = 40 * objectScale;
    tft.fillCircle(objectX, objectY, maxShockwaveRadius, BG_COLOR);
    
    supernovaInitialized = false;
  }
}

#endif // SUPERNOVA_H 