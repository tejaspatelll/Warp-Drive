#ifndef COMET_H
#define COMET_H

#include <TFT_eSPI.h>

// Forward declarations of external variables and constants
extern TFT_eSPI tft;
extern uint16_t BG_COLOR;
extern const int SCREEN_WIDTH;
extern const int SCREEN_HEIGHT;
extern int objectX, objectY;
extern float objectScale;

// Comet parameters
#define MAX_COMET_TAIL 500 // Increased number of particles

// Struct for comet tail particles with velocity
struct CometParticle {
  float x, y;           // Position
  float vx, vy;         // Velocity
  int brightness;       // Brightness (0-255)
  unsigned long spawnTime;  // When this particle was created
};

// Module-private variables
namespace {
  bool cometInitialized = false;
  float cometX = 0, cometY = 0;       // Current comet position
  float cometVx = 0, cometVy = 0;     // Comet velocity
  int cometRadius = 0;                // Comet nucleus radius
  int prevCometX = 0, prevCometY = 0; // Previous comet position for erasing
  CometParticle cometTail[MAX_COMET_TAIL];
  unsigned long cometLastParticleTime = 0;
}

/**
 * Draws a comet with a glowing head and realistic particle tail
 */
void drawComet() {
  int centerX = objectX;
  int centerY = objectY;
  float scale = objectScale;
  unsigned long currentTime = millis();

  if (!cometInitialized) {
    // Initialize comet at a random edge
    int side = random(4); // 0=top, 1=right, 2=bottom, 3=left
    switch (side) {
      case 0: // Top
        cometX = random(SCREEN_WIDTH);
        cometY = 0;
        break;
      case 1: // Right
        cometX = SCREEN_WIDTH - 1;
        cometY = random(SCREEN_HEIGHT);
        break;
      case 2: // Bottom
        cometX = random(SCREEN_WIDTH);
        cometY = SCREEN_HEIGHT - 1;
        break;
      case 3: // Left
        cometX = 0;
        cometY = random(SCREEN_HEIGHT);
        break;
    }

    // Set velocity toward center with randomness
    float targetX = centerX + random(-20, 21);
    float targetY = centerY + random(-20, 21);
    float dx = targetX - cometX;
    float dy = targetY - cometY;
    float dist = sqrt(dx * dx + dy * dy);
    float speed = (0.3f + random(0, 100) / 500.0f) * scale;
    cometVx = dx / dist * speed;
    cometVy = dy / dist * speed;

    // Set comet size
    cometRadius = 2 * scale;

    // Initialize tail particles (inactive)
    for (int i = 0; i < MAX_COMET_TAIL; i++) {
      cometTail[i].brightness = 0;
      cometTail[i].x = cometX;
      cometTail[i].y = cometY;
      cometTail[i].vx = 0;
      cometTail[i].vy = 0;
      cometTail[i].spawnTime = 0;
    }

    prevCometX = round(cometX);
    prevCometY = round(cometY);
    cometLastParticleTime = currentTime;
    cometInitialized = true;
  }

  // Update comet position
  cometX += cometVx;
  cometY += cometVy;

  // Draw comet nucleus
  int x = round(cometX);
  int y = round(cometY);

  // Erase previous nucleus
  if (prevCometX >= 0 && prevCometX < SCREEN_WIDTH &&
      prevCometY >= 0 && prevCometY < SCREEN_HEIGHT) {
    tft.fillCircle(prevCometX, prevCometY, cometRadius + 1, BG_COLOR);
  }

  // Draw nucleus with glow
  if (x >= 0 && x < SCREEN_WIDTH && y >= 0 && y < SCREEN_HEIGHT) {
    for (int r = cometRadius; r > 0; r--) {
      uint8_t brightness = map(r, 0, cometRadius, 100, 255);
      uint16_t glowColor = tft.color565(brightness, brightness, brightness * 0.8);
      tft.drawCircle(x, y, r, glowColor);
    }
    tft.fillCircle(x, y, cometRadius / 2, TFT_WHITE);
    prevCometX = x;
    prevCometY = y;
  }

  // Spawn new tail particles at a faster rate
  if (currentTime - cometLastParticleTime > 5) {
    for (int i = 0; i < MAX_COMET_TAIL; i++) {
      if (cometTail[i].brightness == 0) {
        cometTail[i].x = cometX + random(-1, 2);
        cometTail[i].y = cometY + random(-1, 2);
        float angle = atan2(-cometVy, -cometVx);
        float angleDeviation = random(-30, 30) * PI / 180.0f;
        float speedFactor = 0.05f + random(0, 100) / 500.0f;
        cometTail[i].vx = cos(angle + angleDeviation) * speedFactor * scale;
        cometTail[i].vy = sin(angle + angleDeviation) * speedFactor * scale;
        cometTail[i].brightness = 150 + random(0, 106);
        cometTail[i].spawnTime = currentTime;
        cometLastParticleTime = currentTime;
        break;
      }
    }
  }

  // Update and draw tail particles
  for (int i = 0; i < MAX_COMET_TAIL; i++) {
    if (cometTail[i].brightness > 0) {
      int prevParticleX = round(cometTail[i].x);
      int prevParticleY = round(cometTail[i].y);

      // Update position with velocity and slightly accelerate
      cometTail[i].x += cometTail[i].vx;
      cometTail[i].y += cometTail[i].vy;
      cometTail[i].vx *= 1.001; // slightly accelerate in the initial direction
      cometTail[i].vy *= 1.001;

      int particleX = round(cometTail[i].x);
      int particleY = round(cometTail[i].y);

      // Erase only if position changed
      if (prevParticleX != particleX || prevParticleY != particleY) {
        if (prevParticleX >= 0 && prevParticleX < SCREEN_WIDTH &&
            prevParticleY >= 0 && prevParticleY < SCREEN_HEIGHT) {
          tft.drawPixel(prevParticleX, prevParticleY, BG_COLOR);
        }
      }

      // Age and fade particle
      unsigned long particleAge = currentTime - cometTail[i].spawnTime;
      if (particleAge > 2000) {
        cometTail[i].brightness = 0;
      } else {
        float fadeFactor = 1.0f - (float)particleAge / 2000.0f;
        int newBrightness = cometTail[i].brightness * fadeFactor;
        if (particleX >= 0 && particleX < SCREEN_WIDTH &&
            particleY >= 0 && particleY < SCREEN_HEIGHT) {
          uint16_t tailColor = tft.color565(
            newBrightness * 0.5,  
            newBrightness * 0.8,
            newBrightness
          );
          tft.drawPixel(particleX, particleY, tailColor);
        }
      }
    }
  }

  // Handle comet exiting screen
  if (x < -cometRadius || x > SCREEN_WIDTH + cometRadius ||
      y < -cometRadius || y > SCREEN_HEIGHT + cometRadius) {
    // Erase nucleus
    if (prevCometX >= 0 && prevCometX < SCREEN_WIDTH &&
        prevCometY >= 0 && prevCometY < SCREEN_HEIGHT) {
      tft.fillCircle(prevCometX, prevCometY, cometRadius + 1, BG_COLOR);
    }
    // Erase tail
    for (int i = 0; i < MAX_COMET_TAIL; i++) {
      if (cometTail[i].brightness > 0) {
        int particleX = round(cometTail[i].x);
        int particleY = round(cometTail[i].y);
        if (particleX >= 0 && particleX < SCREEN_WIDTH &&
            particleY >= 0 && particleY < SCREEN_HEIGHT) {
          tft.drawPixel(particleX, particleY, BG_COLOR);
        }
        cometTail[i].brightness = 0;
      }
    }
    cometInitialized = false;
  }
}

// Erase function remains the same
void eraseComet() {
  if (cometInitialized) {
    // Erase nucleus
    if (prevCometX >= 0 && prevCometX < SCREEN_WIDTH &&
        prevCometY >= 0 && prevCometY < SCREEN_HEIGHT) {
      tft.fillCircle(prevCometX, prevCometY, cometRadius + 1, BG_COLOR);
    }
    // Erase tail
    for (int i = 0; i < MAX_COMET_TAIL; i++) {
      if (cometTail[i].brightness > 0) {
        int particleX = round(cometTail[i].x);
        int particleY = round(cometTail[i].y);
        if (particleX >= 0 && particleX < SCREEN_WIDTH &&
            particleY >= 0 && particleY < SCREEN_HEIGHT) {
          tft.drawPixel(particleX, particleY, BG_COLOR);
        }
      }
    }
    cometInitialized = false;
  }
}

#endif // COMET_H
