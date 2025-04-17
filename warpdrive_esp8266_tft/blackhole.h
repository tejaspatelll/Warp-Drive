#ifndef BLACKHOLE_H
#define BLACKHOLE_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <SPI.h>

// Color extraction functions (keep as they are)
inline int red(uint16_t color) { return ((color >> 11) & 0x1F) << 3; }
inline int green(uint16_t color) { return ((color >> 5) & 0x3F) << 2; }
inline int blue(uint16_t color) { return (color & 0x1F) << 3; }

// Constants
extern const int SCREEN_WIDTH;
extern const int SCREEN_HEIGHT;
#define MAX_ACCRETION_PARTICLES 450
#define MAX_FALLING_STARS 6

// Structs (keep as they are)
struct AccretionParticle {
  float angle;
  float distance;
  float speed;
  int brightness;
  int size; // Kept for compatibility, but not used in pixel drawing
  int prevX, prevY;
  uint16_t color;
  bool active;
  bool hasTrail;
  float relativistic_factor;  // Add this for relativistic effects
  float doppler_shift;       // Add this for color shifting
  unsigned long trailStartTime;
  unsigned long trailLifetime;
  int trailX[8]; // Increase array size from 5 to 8 for longer trails
  int trailY[8];
  uint16_t trailColors[8]; // Add colors for each trail segment
  int trailLength;
};

struct FallingStar {
  float x, y;
  float vx, vy;
  float distance;
  int brightness;
  int prevX, prevY;
  bool active;
  float spinFactor;
  unsigned long startTime;
  bool hasTrail;
  unsigned long trailLifetime;
};

// Function prototypes
void initializeAccretionParticle(int index, int centerX, int centerY);
void calculateTrailColors(AccretionParticle &particle);

// Global variables for black hole animation
extern TFT_eSPI tft;
extern uint16_t BG_COLOR; // Assuming this is your background color (e.g., black)

// Object position and scale
extern int objectX, objectY;
extern float objectScale;

// Black hole parameters
float blackHoleRadius;
float diskInnerRadius;
float diskOuterRadius;
int prevBlackHoleX = -1000, prevBlackHoleY = -1000; // Initialize off-screen
float previousEventHorizonRadius = 0; // Use float for radius comparison
bool blackHoleInitialized = false;
unsigned long blackHoleLastUpdateTime;

// Global arrays
AccretionParticle accretionDisk[MAX_ACCRETION_PARTICLES];
FallingStar fallingStars[MAX_FALLING_STARS];

// Lens tracking
int previousLensPoints[60][2];
// Falling star trails tracking (moved to global scope for persistence)
int prevTrailX[10][MAX_FALLING_STARS];
int prevTrailY[10][MAX_FALLING_STARS];
int trailLen[MAX_FALLING_STARS];
// Inner particle tracking (moved to global scope)
int prevInnerParticleX[4] = {-1, -1, -1, -1};
int prevInnerParticleY[4] = {-1, -1, -1, -1};


void drawBlackHole() {
    int centerX = objectX;
    int centerY = objectY;
    float scale = objectScale;
    unsigned long currentTime = millis();

    // Calculate radii based on scale for this frame
    blackHoleRadius = 14 * scale;  // Event horizon radius
    diskInnerRadius = blackHoleRadius * 1.2;
    diskOuterRadius = blackHoleRadius * 2.0; // Adjusted slightly

    // Initialize black hole system on first run
    if (!blackHoleInitialized) {
        // Initialize accretion disk particles
        for (int i = 0; i < MAX_ACCRETION_PARTICLES; i++) {
            initializeAccretionParticle(i, centerX, centerY);
        }

        // Initialize falling stars (inactive at first)
        for (int i = 0; i < MAX_FALLING_STARS; i++) {
            fallingStars[i].active = false;
            fallingStars[i].hasTrail = false;
            fallingStars[i].prevX = -1;
            fallingStars[i].prevY = -1;
            trailLen[i] = 0; // Initialize trail length
            for(int t=0; t<10; ++t) { // Initialize trail arrays
                prevTrailX[t][i] = -1;
                prevTrailY[t][i] = -1;
            }
        }

        // Initialize lens tracking array
        for (int i = 0; i < 60; i++) {
            previousLensPoints[i][0] = -1;
            previousLensPoints[i][1] = -1;
        }

        // Initialize inner particle tracking
        for(int i=0; i<4; ++i) {
            prevInnerParticleX[i] = -1;
            prevInnerParticleY[i] = -1;
        }

        // Set initial prev positions for erase logic on first frame after init
        prevBlackHoleX = centerX;
        prevBlackHoleY = centerY;
        previousEventHorizonRadius = blackHoleRadius;

        blackHoleLastUpdateTime = currentTime;
        blackHoleInitialized = true;
    }

    float deltaTime = (currentTime - blackHoleLastUpdateTime) / 1000.0f;
    if (deltaTime > 0.1f) deltaTime = 0.1f; // Cap to avoid huge jumps
    if (deltaTime <= 0) deltaTime = 0.016; // Prevent division by zero or negative time if millis() wraps or glitches
    blackHoleLastUpdateTime = currentTime;

    // --- Erasing Section ---

    // Erase old black hole center and lens points ONLY if it moved or size changed significantly
    bool blackHoleMovedOrResized = (centerX != prevBlackHoleX || centerY != prevBlackHoleY ||
                                   abs(blackHoleRadius - previousEventHorizonRadius) > 0.5); // Check float difference threshold

    if (blackHoleMovedOrResized && previousEventHorizonRadius > 0) {
        // Erase the old event horizon position and photon rings area
        // Make erase radius slightly larger to catch photon rings and potential artifacts
        float eraseRadius = previousEventHorizonRadius + 4;
        tft.fillCircle(prevBlackHoleX, prevBlackHoleY, eraseRadius, BG_COLOR);

        // Erase old lens points when black hole moves/resizes
        for (int i = 0; i < 60; i++) {
            if (previousLensPoints[i][0] >= 0) {
                tft.drawPixel(previousLensPoints[i][0], previousLensPoints[i][1], BG_COLOR);
                previousLensPoints[i][0] = -1; // Mark as erased
            }
        }
    }

    // Erase previous accretion disk particles (both halves) and trails
    for (int i = 0; i < MAX_ACCRETION_PARTICLES; i++) {
        if (accretionDisk[i].prevX >= 0) { // Check if it had a valid previous position
            tft.drawPixel(accretionDisk[i].prevX, accretionDisk[i].prevY, BG_COLOR);
        }
        // Erase previous trails regardless of prevX validity (for fading trails)
        for (int t = 0; t < accretionDisk[i].trailLength; t++) {
             if (accretionDisk[i].trailX[t] >= 0) {
                 tft.drawPixel(accretionDisk[i].trailX[t], accretionDisk[i].trailY[t], BG_COLOR);
             }
        }
        // Clear the trail positions for the next frame *after* erasing
        accretionDisk[i].trailLength = 0;
        for(int t=0; t<8; ++t) {
            accretionDisk[i].trailX[t] = -1;
            accretionDisk[i].trailY[t] = -1;
        }
    }


    // Erase previous falling stars and their trails
    for (int i = 0; i < MAX_FALLING_STARS; i++) {
        // Erase all previous trail points for this star (includes head at index 0 if trailLen > 0)
        for (int t = 0; t < trailLen[i]; t++) {
            if (prevTrailX[t][i] >= 0) {
                tft.drawPixel(prevTrailX[t][i], prevTrailY[t][i], BG_COLOR);
            }
             // Clear the specific trail point after erasing
            prevTrailX[t][i] = -1;
            prevTrailY[t][i] = -1;
        }
         trailLen[i] = 0; // Reset trail length after erasing all points
    }

    // Erase previous inner particles
    for (int i = 0; i < 4; i++) {
        if (prevInnerParticleX[i] >= 0) {
            tft.drawPixel(prevInnerParticleX[i], prevInnerParticleY[i], BG_COLOR);
             prevInnerParticleX[i] = -1; // Mark as erased
             prevInnerParticleY[i] = -1;
        }
    }
    // --- End Erasing Section ---


    // --- Update Section (Physics and Logic) ---

    // Update Accretion Disk particles
    for (int i = 0; i < MAX_ACCRETION_PARTICLES; i++) {
        // Store previous position *before* updating, if it was valid
        int lastValidX = accretionDisk[i].prevX;
        int lastValidY = accretionDisk[i].prevY;
        accretionDisk[i].prevX = -1; // Assume invalid until new position is calculated

        if (!accretionDisk[i].active) {
            // Handle fading trail for inactive particles
            if (accretionDisk[i].hasTrail) {
                if (millis() - accretionDisk[i].trailStartTime > accretionDisk[i].trailLifetime) {
                    accretionDisk[i].hasTrail = false;
                    // Reinitialize the particle once trail fades
                    initializeAccretionParticle(i, centerX, centerY);
                } else {
                    // Keep the last position for trail rendering (if needed, but erase handles cleanup)
                     accretionDisk[i].prevX = lastValidX;
                     accretionDisk[i].prevY = lastValidY;
                }
            } else { // Inactive and no trail
                 initializeAccretionParticle(i, centerX, centerY);
            }
            continue; // Skip active processing for inactive/fading particles
        }

        // Update angle (Keplerian motion)
        float currentDiskInnerRadius = max(1.0f, blackHoleRadius * 1.2f); // Ensure non-zero radius
        float spinFactor = sqrt(currentDiskInnerRadius / max(accretionDisk[i].distance, currentDiskInnerRadius * 0.5f)); // Prevent extreme speed/div by zero very close in
        accretionDisk[i].angle += accretionDisk[i].speed * spinFactor * deltaTime * 60; // Scale speed with deltaTime
        accretionDisk[i].angle = fmod(accretionDisk[i].angle, 2.0f * PI); // Keep angle in range [0, 2*PI)
        if (accretionDisk[i].angle < 0) accretionDisk[i].angle += 2.0f * PI;

        // Update distance (inward spiral) - COMMENTED OUT FOR ENDLESS ROTATION
        /*
        float currentDiskOuterRadius = max(currentDiskInnerRadius + 1.0f, blackHoleRadius * 2.0f);
        float distRange = currentDiskOuterRadius - currentDiskInnerRadius;
        float distanceRatio = (distRange > 0.1f) ? (accretionDisk[i].distance - currentDiskInnerRadius) / distRange : 0.0f;
        distanceRatio = constrain(distanceRatio, 0.0f, 1.0f);
        float inwardForce = 0.01f + (1.0f - distanceRatio) * 0.03f; // Slower base inward pull
        accretionDisk[i].distance *= (1.0f - inwardForce * deltaTime * 5); // Scale inward pull with deltaTime, reduced factor
        */

        // Ensure distance doesn't go below a minimum or too far out (can happen with large deltaTime steps)
        // Keep this check slightly, but maybe adjust the lower bound if not consuming
        accretionDisk[i].distance = max(accretionDisk[i].distance, currentDiskInnerRadius * 0.1f); // Prevent going too close if BH shrinks rapidly
        accretionDisk[i].distance = min(accretionDisk[i].distance, diskOuterRadius * 1.1f); // Use calculated outer radius

        // Calculate relativistic factor (near horizon particles move faster)
        float distRatio = max(0.1f, (accretionDisk[i].distance - diskInnerRadius) / max(1.0f, (diskOuterRadius - diskInnerRadius)));
        accretionDisk[i].relativistic_factor = 0.8f + (1.0f - distRatio) * 2.0f; // Higher near inner edge
        
        // Calculate new position...
        float angle = accretionDisk[i].angle;
        float distance = accretionDisk[i].distance;
        float verticalCompression = 0.5f - 0.3f * cos(angle); // Perspective effect
        float floatX = centerX + cos(angle) * distance;
        float floatY = centerY + sin(angle) * distance * verticalCompression;

        // Round coordinates for drawing and storing previous position
        int x = round(floatX);
        int y = round(floatY);

        // Trail calculation - shift existing trail points
        if (accretionDisk[i].active) {
            // Create a trail based on velocity
            if (accretionDisk[i].prevX >= 0) { // Has valid previous position
                // Only add new trail point if we've moved enough
                float dist = sqrt(sq(x - accretionDisk[i].prevX) + sq(y - accretionDisk[i].prevY));
                
                if (dist > (0.5f + accretionDisk[i].relativistic_factor * 0.5f)) {
                    // Shift existing trail points
                    for (int t = 7; t > 0; t--) {
                        accretionDisk[i].trailX[t] = accretionDisk[i].trailX[t-1];
                        accretionDisk[i].trailY[t] = accretionDisk[i].trailY[t-1];
                    }
                    
                    // Add new position to trail
                    accretionDisk[i].trailX[0] = accretionDisk[i].prevX;
                    accretionDisk[i].trailY[0] = accretionDisk[i].prevY;
                    
                    // Calculate colors for trail segments
                    calculateTrailColors(accretionDisk[i]);
                    
                    // Update trail length, capped at array size
                    accretionDisk[i].trailLength = min(8, accretionDisk[i].trailLength + 1);
                }
            }
        }
        
        // Store current position for next frame's erase & trail
        accretionDisk[i].prevX = x;
        accretionDisk[i].prevY = y;
    }


    // Update Falling Stars
    for (int i = 0; i < MAX_FALLING_STARS; i++) {
         // Store previous position *before* updating
        int lastValidX = fallingStars[i].prevX;
        int lastValidY = fallingStars[i].prevY;
        fallingStars[i].prevX = -1; // Assume invalid until new calculation

         if (!fallingStars[i].active) {
             if (fallingStars[i].hasTrail) {
                 if (millis() - fallingStars[i].startTime > fallingStars[i].trailLifetime) {
                     fallingStars[i].hasTrail = false; // Trail faded completely
                 } else {
                     // Keep last valid position if trail is still fading (for erase logic)
                     fallingStars[i].prevX = lastValidX;
                     fallingStars[i].prevY = lastValidY;
                 }
             }
             continue; // Skip inactive stars
         }

        float dx = centerX - fallingStars[i].x;
        float dy = centerY - fallingStars[i].y;
        float distSq = dx*dx + dy*dy;
        float dist = sqrt(distSq);
        fallingStars[i].distance = dist;

        if (dist > 0.1f) { // Avoid division by zero
            // Gravity (1/r^2)
            float gravityForce = (blackHoleRadius * blackHoleRadius * 150.0f) / max(distSq, blackHoleRadius * 0.5f); // Tuned gravity strength
            gravityForce = min(gravityForce, 50.0f); // Cap gravity force to prevent extreme acceleration
            float accX = (dx / dist) * gravityForce;
            float accY = (dy / dist) * gravityForce;

            // Frame dragging (tangential acceleration, stronger closer in)
            if (dist < blackHoleRadius * 8.0f) {
                float perpX = -dy / dist; // Normalized perpendicular vector
                float perpY = dx / dist;
                float spinRadius = blackHoleRadius * 4.0f;
                float effectiveDist = max(dist, spinRadius * 0.1f); // Avoid extreme strength very close
                float spinStrength = fallingStars[i].spinFactor * 1.5f * (spinRadius / effectiveDist); // Tuned spin strength
                spinStrength = min(spinStrength, 8.0f); // Cap spin strength
                accX += perpX * spinStrength; // Add tangential acceleration
                accY += perpY * spinStrength;
            }

             // Update velocity using acceleration and deltaTime
            fallingStars[i].vx += accX * deltaTime;
            fallingStars[i].vy += accY * deltaTime;

             // Optional: Limit maximum speed to prevent instability
             float speedSq = fallingStars[i].vx * fallingStars[i].vx + fallingStars[i].vy * fallingStars[i].vy;
             float maxSpeedSq = 400.0f; // Max speed squared (e.g., 20 pixels per frame equiv)
             if (speedSq > maxSpeedSq) {
                 float speedScale = sqrt(maxSpeedSq / speedSq);
                 fallingStars[i].vx *= speedScale;
                 fallingStars[i].vy *= speedScale;
             }

        }

        // Update position using velocity and deltaTime
        fallingStars[i].x += fallingStars[i].vx * deltaTime;
        fallingStars[i].y += fallingStars[i].vy * deltaTime;

        int x = round(fallingStars[i].x);
        int y = round(fallingStars[i].y);

        // Check for consumption or out of bounds
        // Use distance calculated from float position for accuracy
        if (dist <= blackHoleRadius || x < -10 || x >= SCREEN_WIDTH + 10 || y < -10 || y >= SCREEN_HEIGHT + 10) { // Wider bounds check
            fallingStars[i].active = false;

            // If star was consumed (close enough), trigger consumption effect and trail fade
             if (dist <= blackHoleRadius * 1.5f) { // Only flash if consumed near BH
                // Instantaneous flash effect (draw immediately)
                float consumptionAngle = atan2(fallingStars[i].y - centerY, fallingStars[i].x - centerX);
                for (int r = 0; r <= 2; r++) {
                    for (int j = 0; j < 8; j++) {
                         float angle = j * PI / 4.0f;
                         int flashX = round(centerX + cos(angle) * (blackHoleRadius + r));
                         int flashY = round(centerY + sin(angle) * (blackHoleRadius + r));
                         if (flashX >= 0 && flashX < SCREEN_WIDTH && flashY >= 0 && flashY < SCREEN_HEIGHT) {
                             tft.drawPixel(flashX, flashY, TFT_WHITE); // Simple white flash
                         }
                    }
                }
                // Start trail fade
                fallingStars[i].hasTrail = true;
                fallingStars[i].trailLifetime = 600;
                fallingStars[i].startTime = millis(); // Reset timer for trail fade
             } else {
                 // If star just went out of bounds far away, don't necessarily start a trail fade
                 fallingStars[i].hasTrail = false;
             }
             // Keep the last valid position for erase/trail logic
             fallingStars[i].prevX = lastValidX;
             fallingStars[i].prevY = lastValidY;
             continue; // Stop processing this star
        }

        // Store current calculated position for next frame's erase & this frame's draw
        fallingStars[i].prevX = x;
        fallingStars[i].prevY = y;
    }


    // Randomly create new falling stars
    if (random(100) < 4) { // Reduced frequency
        for (int i = 0; i < MAX_FALLING_STARS; i++) {
            if (!fallingStars[i].active && !fallingStars[i].hasTrail) { // Only activate if truly inactive
                fallingStars[i].active = true;
                fallingStars[i].hasTrail = false;
                fallingStars[i].startTime = currentTime;
                fallingStars[i].brightness = random(180, 256);
                fallingStars[i].spinFactor = random(50, 200) / 100.0f; // Reduced max spin slightly

                int edge = random(4);
                switch (edge) {
                     case 0: fallingStars[i].x = random(SCREEN_WIDTH); fallingStars[i].y = -5; break; // Start slightly off screen
                     case 1: fallingStars[i].x = SCREEN_WIDTH + 4; fallingStars[i].y = random(SCREEN_HEIGHT); break;
                     case 2: fallingStars[i].x = random(SCREEN_WIDTH); fallingStars[i].y = SCREEN_HEIGHT + 4; break;
                     case 3: fallingStars[i].x = -5; fallingStars[i].y = random(SCREEN_HEIGHT); break;
                }

                float dx = centerX - fallingStars[i].x;
                float dy = centerY - fallingStars[i].y;
                float dist = sqrt(dx*dx + dy*dy);
                fallingStars[i].distance = dist;
                float angle_to_center = atan2(dy, dx);
                float angle_offset = (random(-10, 10) * PI / 180.0f); // Smaller offset
                float initial_angle = angle_to_center + angle_offset;

                float initialSpeed = random(4, 10) / 10.0f; // Slower start speed
                fallingStars[i].vx = cos(initial_angle) * initialSpeed;
                fallingStars[i].vy = sin(initial_angle) * initialSpeed;
                fallingStars[i].prevX = -1; // Initialize prev position as invalid
                fallingStars[i].prevY = -1;
                trailLen[i] = 0; // Reset trail length for new star
                break; // Activate only one star per check
            }
        }
    }

    // --- End Update Section ---


    // --- Drawing Section (Reordered) ---

   // 1. Draw Back Half of Accretion Disk (Top half, sin(angle) <= 0)
for (int i = 0; i < MAX_ACCRETION_PARTICLES; i++) {
    if (!accretionDisk[i].active || accretionDisk[i].prevX < 0) continue; // Skip inactive or invalid position
    if (sin(accretionDisk[i].angle) > 0) continue; // Skip front half

    int x = accretionDisk[i].prevX; // Use the calculated position from the update phase
    int y = accretionDisk[i].prevY;

    // Check bounds again before drawing
    if (x >= 0 && x < SCREEN_WIDTH && y >= 0 && y < SCREEN_HEIGHT) {
        // *** Add check: Don't draw back-half pixels inside the event horizon ***
        float distSqFromCenter = sq(x - centerX) + sq(y - centerY);
        if (distSqFromCenter <= sq(round(blackHoleRadius))) { // Use sq(round(...)) for consistency with fillCircle
            continue; // Skip drawing this pixel, it's inside/on the horizon edge
        }

        // UNIFIED Visibility Factor calculated here (or could be calculated once before both loops if preferred)
        float visibilityFactor = 0.8f + 0.4f * sin(accretionDisk[i].angle); // Ranges from 0.4 (back) to 0.8 (sides)
        visibilityFactor = max(0.1f, visibilityFactor); // Ensure minimum visibility

        uint16_t baseColor = accretionDisk[i].color;
        int r = red(baseColor) * visibilityFactor;
        int g = green(baseColor) * visibilityFactor;
        int b = blue(baseColor) * visibilityFactor;

        // Maybe add a slight ambient minimum brightness if particles become too dim at the back
        r = max(r, 30); // Adjust minimum thresholds as needed
        g = max(g, 25);
        b = max(b, 20);

        uint16_t finalColor = tft.color565(r, g, b);
        tft.drawPixel(x, y, finalColor);

        // Draw the trail for this particle (no changes needed here)
        for (int t = 0; t < accretionDisk[i].trailLength; t++) {
            int trailX = accretionDisk[i].trailX[t];
            int trailY = accretionDisk[i].trailY[t];

            if (trailX < 0 || trailY < 0 || trailX >= SCREEN_WIDTH || trailY >= SCREEN_HEIGHT) continue;

            float trailDistSq = sq(trailX - centerX) + sq(trailY - centerY);
            if (trailDistSq <= sq(round(blackHoleRadius))) continue;

            tft.drawPixel(trailX, trailY, accretionDisk[i].trailColors[t]);
        }
    }
}


    // 2. Draw the Black Hole Event Horizon (Black Center)
    if (blackHoleRadius >= 0.5) { // Draw if radius is at least half a pixel
         // Use TFT_BLACK directly for the event horizon singularity
        tft.fillCircle(centerX, centerY, round(blackHoleRadius), TFT_BLACK);
    }

    // 3. Draw Inner swirling particles (on top of black hole, behind stars/front disk)
    for (int i = 0; i < 4; i++) {
        float innerAngle = fmod((currentTime / 90.0f) + i * (PI/2.0f), 2.0f * PI); // Slower swirl, ensure range
        float distanceFactor = 0.15f + 0.6f * (float)i/4.0f; // Start closer to center
        // Ensure distance factor doesn't go beyond 1 inside the radius
        distanceFactor = min(distanceFactor, 0.9f);
        int innerX = round(centerX + cos(innerAngle) * blackHoleRadius * distanceFactor);
        int innerY = round(centerY + sin(innerAngle) * blackHoleRadius * distanceFactor);

        // Check bounds before drawing
        if (innerX >= 0 && innerX < SCREEN_WIDTH && innerY >= 0 && innerY < SCREEN_HEIGHT) {
            int brightness = 50 - 12 * i; // Fainter overall
            brightness = max(10, brightness); // Minimum brightness
            uint16_t innerColor = tft.color565(brightness, brightness, brightness);
            tft.drawPixel(innerX, innerY, innerColor);
            prevInnerParticleX[i] = innerX; // Store for next erase
            prevInnerParticleY[i] = innerY;
        } else {
             // If off-screen, ensure previous position is marked invalid for erase
             prevInnerParticleX[i] = -1;
             prevInnerParticleY[i] = -1;
        }
    }

    // 4. Draw Photon Ring (on top of black hole, inner swirls)
    if (blackHoleRadius >= 0.5) {
        int r_bh = round(blackHoleRadius);
        // Primary ring (brightest)
        uint16_t photonRingColor = tft.color565(255, 230, 180);
        tft.drawCircle(centerX, centerY, r_bh, photonRingColor);
        // Highlight at the front (bottom side, appears brighter due to Doppler/viewing angle)
        for (float angle = PI * 0.75f; angle < PI * 1.25f; angle += 0.04f) { // Adjusted angle range/step
            int x = round(centerX + r_bh * cos(angle));
            int y = round(centerY + r_bh * sin(angle));
            if (x >= 0 && x < SCREEN_WIDTH && y >= 0 && y < SCREEN_HEIGHT) {
                tft.drawPixel(x, y, TFT_WHITE);
            }
        }
        // Secondary ring (fainter) - draw only if radius permits
        if (r_bh + 1 < min(SCREEN_WIDTH, SCREEN_HEIGHT) / 2) { // Basic check to avoid huge circles
             uint16_t secondRingColor = tft.color565(200, 180, 150);
             tft.drawCircle(centerX, centerY, r_bh + 1, secondRingColor);
        }
        // Tertiary ring (faintest) - draw only if radius permits
        if (r_bh + 2 < min(SCREEN_WIDTH, SCREEN_HEIGHT) / 2) {
             uint16_t thirdRingColor = tft.color565(150, 140, 120);
             tft.drawCircle(centerX, centerY, r_bh + 2, thirdRingColor);
        }
    }

    /** 5. Draw Gravitational Lensing (appears behind the front disk/stars but over back disk/horizon)
    float lensRadius = blackHoleRadius * 1.8; // Slightly reduced lens effect radius
    for (int i = 0; i < 60; i++) {
        float angle = i * 6 * PI / 180.0f;
        // Distortion effect - stronger lensing effect "behind" the black hole (top part)
        float distortion = 0.9f + 0.5f * (1.0f - sin(angle)) / 2.0f; // Sin range adjusted
        float adjustedRadius = lensRadius * distortion;
        int x = round(centerX + cos(angle) * adjustedRadius);
        int y = round(centerY + sin(angle) * adjustedRadius);

        // Check bounds before drawing
        if (x >= 0 && x < SCREEN_WIDTH && y >= 0 && y < SCREEN_HEIGHT) {
            // Color based on simulated Doppler shift (red/blue shift)
            float doppler = (1.0f + sin(angle)) / 2.0f; // 0=top(red), 1=bottom(blue)
            uint16_t color;
            if (doppler > 0.75f) color = tft.color565(180, 200, 255); // More subtle Blue shift
            else if (doppler < 0.25f) color = tft.color565(255, 180, 150); // More subtle Red shift
            else color = tft.color565(230, 200 + (doppler * 30), 200 + (doppler * 50)); // Smoother transition

            // Draw new lens pixel
            tft.drawPixel(x, y, color);
            // Store current position for next frame's erase
            previousLensPoints[i][0] = x;
            previousLensPoints[i][1] = y;
        } else {
            // Mark previous point as needing erase if it was on screen
            if(previousLensPoints[i][0] >= 0) {
               // Erase logic at top handles actual erasing if BH moved/resized
               // Marking invalid ensures it doesn't get drawn again if BH *didn't* move
               // but this point moved off screen.
               // Let erase logic handle it based on prev position.
               // No action needed here unless we want immediate erase.
            }
             previousLensPoints[i][0] = -1; // Mark current point as off-screen
             previousLensPoints[i][1] = -1;
        }
    }
**/
    // 6. Draw Falling Stars (and their spaghettification/trails)
    // Drawn after lensing but before the front disk half
    for (int i = 0; i < MAX_FALLING_STARS; i++) {
        // Draw active stars or fading trails
        if (!fallingStars[i].active && !fallingStars[i].hasTrail) continue;

        // Use the position calculated and stored in prevX/Y during the update phase
        int x = fallingStars[i].prevX;
        int y = fallingStars[i].prevY;

        // If only trail is fading, don't draw the head, just let erase handle cleanup
        if (!fallingStars[i].active && fallingStars[i].hasTrail) continue;

        // Check bounds before drawing star head
        if (x >= 0 && x < SCREEN_WIDTH && y >= 0 && y < SCREEN_HEIGHT) {
            // Recalculate distance/gravity for brightness/effects based on *current* float position
            float current_dx = centerX - fallingStars[i].x;
            float current_dy = centerY - fallingStars[i].y;
            float current_distSq = current_dx*current_dx + current_dy*current_dy;
            float current_dist = sqrt(current_distSq);

            float gravityFactor = min(3.0f, (float)(blackHoleRadius * 20.0f / max(current_distSq, 1.0f)));
            int starBrightness = min(255, fallingStars[i].brightness + (int)(200 * gravityFactor));
            starBrightness = max(20, starBrightness); // Ensure minimum brightness
            uint16_t starColor = tft.color565(starBrightness, starBrightness, starBrightness);

            // Draw the main star head
            tft.drawPixel(x, y, starColor);
            // Store head position as the start of the trail for erasing next frame
            if (trailLen[i] < 10) {
                prevTrailX[trailLen[i]][i] = x;
                prevTrailY[trailLen[i]][i] = y;
                trailLen[i]++;
            }

            // Draw Spaghettification/Tidal Effects if close enough
if (current_dist < blackHoleRadius * 2.0f) {
    float stretchAngle = atan2(current_dy, current_dx); // Angle from star TO center

    // Tidal forces ~ 1/r^3, more pronounced effect calculation
    float tidalForce = (blackHoleRadius * blackHoleRadius * blackHoleRadius * 50.0f) / max(current_distSq * current_dist, 1.0f) ;
    float stretchFactor = min(6.0f, tidalForce); // Cap the stretch factor

    int numStretchPoints = max(1, (int)stretchFactor);
    for (int j = 1; j <= numStretchPoints && trailLen[i] < 10; j++) {
        // Stretch points along the line connecting star and center
        float spacing = j * (0.4f + j * 0.05f); // Adjust spacing logic

        // Point towards BH (use rounded coords as base)
        int aheadX = round(x + cos(stretchAngle) * spacing);
        int aheadY = round(y + sin(stretchAngle) * spacing);

        // Point away from BH
        int behindX = round(x - cos(stretchAngle) * spacing * 1.1f); // Slightly shorter behind tail
        int behindY = round(y - sin(stretchAngle) * spacing * 1.1f);

        // Check bounds and ensure point is outside event horizon before drawing
        if (aheadX >= 0 && aheadX < SCREEN_WIDTH && aheadY >= 0 && aheadY < SCREEN_HEIGHT &&
            sqrt(sq(aheadX-centerX) + sq(aheadY-centerY)) > blackHoleRadius) {
            float intensityFactor = 1.0f / (j * 0.7f + 1.0f); // Fade further points more
            uint16_t aheadColor = tft.color565(
                min(255, (int)(starBrightness * 1.2f * intensityFactor)), // Increased brightness
                min(255, (int)(starBrightness * 1.1f * intensityFactor)), // Slightly increased brightness
                min(255, (int)(starBrightness * intensityFactor))
            );
            tft.drawPixel(aheadX, aheadY, aheadColor);
            prevTrailX[trailLen[i]][i] = aheadX;
            prevTrailY[trailLen[i]][i] = aheadY;
            trailLen[i]++;
        }

        if (behindX >= 0 && behindX < SCREEN_WIDTH && behindY >= 0 && behindY < SCREEN_HEIGHT && trailLen[i] < 10) {
            float tailFactor = 1.0f / (j * 1.0f + 1.0f); // Fade further points more
            uint16_t behindColor = tft.color565(
                min(255, (int)(starBrightness * 1.1f * tailFactor)), // Increased brightness
                min(255, (int)(starBrightness * 0.8f * tailFactor)), // Reduced green component for redder tint
                min(255, (int)(starBrightness * 0.6f * tailFactor)) // Reduced blue component for redder tint
            );
            tft.drawPixel(behindX, behindY, behindColor);
            prevTrailX[trailLen[i]][i] = behindX;
            prevTrailY[trailLen[i]][i] = behindY;
            trailLen[i]++;
        }
    }
}

        } else {
             // If star head is off screen, ensure its trail points are still potentially processed by erase logic
             // The trailLen reset in the erase section handles cleanup
        }
    }

  // 7. Draw Front Half of Accretion Disk (Bottom half, sin(angle) > 0)
// This part is drawn last, so it appears on top of everything else near the center
for (int i = 0; i < MAX_ACCRETION_PARTICLES; i++) {
    if (!accretionDisk[i].active || accretionDisk[i].prevX < 0) continue; // Skip inactive or invalid position
    if (sin(accretionDisk[i].angle) <= 0) continue; // Skip back half

    int x = accretionDisk[i].prevX; // Use position calculated in update phase
    int y = accretionDisk[i].prevY;

    // Check bounds again before drawing
    if (x >= 0 && x < SCREEN_WIDTH && y >= 0 && y < SCREEN_HEIGHT) {

         // UNIFIED Visibility Factor calculated using the SAME formula as the back half
        float visibilityFactor = 0.8f + 0.4f * sin(accretionDisk[i].angle); // Ranges from 0.8 (sides) to 1.2 (front)
        // No need for a max(0.1f, ...) here as sin() is positive, but keep it if you unify outside the loop

        uint16_t baseColor = accretionDisk[i].color;
        int r_base = red(baseColor);
        int g_base = green(baseColor);
        int b_base = blue(baseColor);

        // Calculate distance to center using drawn coordinates for effects
        float distToCenter = sqrt(sq(x - centerX) + sq(y - centerY));

        // Brighten particles very close to the event horizon (heating/energy)
        float boostFactor = 1.0f; // Initialize boost factor
        if (distToCenter < blackHoleRadius * 1.6f && blackHoleRadius > 0) {
            // Boost increases sharply near horizon
            boostFactor = 1.0f + max(0.0f, (blackHoleRadius * 1.6f - distToCenter) / (blackHoleRadius * 0.6f)) * 0.9f;
            // Apply boost BEFORE visibility factor for better effect control
            r_base = min(255, (int)(r_base * boostFactor));
            g_base = min(255, (int)(g_base * boostFactor));
            b_base = min(255, (int)(b_base * boostFactor));
        }

        // Apply base visibility factor
        r_base = constrain((int)(r_base * visibilityFactor), 0, 255);
        g_base = constrain((int)(g_base * visibilityFactor), 0, 255);
        b_base = constrain((int)(b_base * visibilityFactor), 0, 255);

        uint16_t finalColor = tft.color565(r_base, g_base, b_base);

        // Draw main particle
        tft.drawPixel(x, y, finalColor);

        // Draw the trail for this particle (no changes needed here)
        for (int t = 0; t < accretionDisk[i].trailLength; t++) {
            int trailX = accretionDisk[i].trailX[t];
            int trailY = accretionDisk[i].trailY[t];

            if (trailX < 0 || trailY < 0 || trailX >= SCREEN_WIDTH || trailY >= SCREEN_HEIGHT) continue;

            float trailDistSq = sq(trailX - centerX) + sq(trailY - centerY);
            if (trailDistSq <= sq(round(blackHoleRadius))) continue;

            tft.drawPixel(trailX, trailY, accretionDisk[i].trailColors[t]);
        }

        // Optional: Draw subtle bright trail for inner edge of front disk (Keep as is, or adjust brightness based on new r_base etc)
        if (distToCenter < blackHoleRadius * 1.4f && distToCenter > blackHoleRadius) {
             float trailAngle = atan2(y - centerY, x - centerX);
             for (int t = 1; t <= 1; t++) { // Simplified trail (1 point)
                 int trailX = round(x - t * cos(trailAngle) * 0.6f); // Closer trail point
                 int trailY = round(y - t * sin(trailAngle) * 0.6f);
                 if (trailX >= 0 && trailX < SCREEN_WIDTH && trailY >= 0 && trailY < SCREEN_HEIGHT) {
                     float trailFactor = 0.6f / t; // Fainter trail
                     // Use the calculated r_base, g_base, b_base for consistency
                     uint16_t trailColor = tft.color565(
                        min(255, (int)(r_base * trailFactor * 1.2f)), // Slightly brighter trail color base
                        min(255, (int)(g_base * trailFactor * 1.1f)),
                        min(255, (int)(b_base * trailFactor))
                     );
                     tft.drawPixel(trailX, trailY, trailColor);
                     // Store trail points (No change needed)
                     if (accretionDisk[i].trailLength < 8) {
                        accretionDisk[i].trailX[accretionDisk[i].trailLength] = trailX;
                        accretionDisk[i].trailY[accretionDisk[i].trailLength] = trailY;
                        // Trail color calculation might need adjustment here if calculateTrailColors used old logic
                        accretionDisk[i].trailColors[accretionDisk[i].trailLength] = trailColor; // Or recalculate faded color
                        accretionDisk[i].trailLength++;
                     }
                 }
             }
         }
    }
}


    // --- End Drawing Section ---


    // Store current black hole parameters for next frame's comparison/erase
    prevBlackHoleX = centerX;
    prevBlackHoleY = centerY;
    previousEventHorizonRadius = blackHoleRadius;
}


// Helper function to initialize or reinitialize an accretion disk particle
// (Make sure this uses the *current* blackHoleRadius, diskInnerRadius, etc.)
void initializeAccretionParticle(int index, int centerX, int centerY) {
    float currentDiskInnerRadius = max(1.0f, blackHoleRadius * 1.2f);
    float currentDiskOuterRadius = max(currentDiskInnerRadius + 1.0f, blackHoleRadius * 2.5f); // Slightly larger disk
    float diskWidth = currentDiskOuterRadius - currentDiskInnerRadius;

    accretionDisk[index].angle = random(0, 3600) * PI / 1800.0f;

    // More realistic particle distribution using Shakura-Sunyaev model
    float randFactor = random(0, 1000) / 1000.0f;
    float distanceFactor = pow(randFactor, 2.0f); // Steeper power law for density
    accretionDisk[index].distance = currentDiskInnerRadius + (distanceFactor * diskWidth);

    // Relativistic orbital velocity (simplified)
    float orbital_velocity = sqrt(blackHoleRadius / accretionDisk[index].distance);
    accretionDisk[index].relativistic_factor = min(orbital_velocity, 0.9f); // Cap at 0.9c

    // Calculate Doppler shift including relativistic beaming
    float sin_angle = sin(accretionDisk[index].angle);
    float cos_angle = cos(accretionDisk[index].angle);
    float doppler = 1.0f / (1.0f - accretionDisk[index].relativistic_factor * sin_angle);
    accretionDisk[index].doppler_shift = doppler;

    // Temperature based on distance (T ~ r^(-3/4) for thin accretion disks)
    float temp_factor = pow(currentDiskInnerRadius / accretionDisk[index].distance, 0.75f);
    
    // Base color calculation using blackbody approximation
    float temp_ratio = temp_factor * doppler; // Include Doppler effect on temperature
    
    // Calculate RGB based on temperature (simplified blackbody)
    int r, g, b;
    if (temp_ratio > 1.5f) {
        // Very hot - more vibrant blue/white
        r = 255; g = 255; b = 255; // Pure white for hottest regions
    } else if (temp_ratio > 1.2f) {
        // Hot - bright white with blue tint
        r = 255; g = 255; b = 255;
    } else if (temp_ratio > 0.8f) {
        // Medium - bright white-yellow
        r = 255; g = 255; b = 220;
    } else if (temp_ratio > 0.6f) {
        // Cool - vibrant yellow
        r = 255; g = 240; b = 150;
    } else {
        // Coolest - bright orange
        r = 255; g = 200; b = 100;
    }

    // Apply relativistic beaming and Doppler shift
    float intensity = pow(doppler, 4.0f); // Relativistic beaming factor
    intensity = constrain(intensity, 0.1f, 3.0f);
    
    r = constrain((int)(r * intensity), 0, 255);
    g = constrain((int)(g * intensity), 0, 255);
    b = constrain((int)(b * intensity), 0, 255);

    accretionDisk[index].color = tft.color565(r, g, b);
    accretionDisk[index].brightness = constrain((int)(255 * intensity), 50, 255);

    // Initialize other properties
    accretionDisk[index].prevX = -1;
    accretionDisk[index].prevY = -1;
    accretionDisk[index].active = true;
    accretionDisk[index].hasTrail = true;
    accretionDisk[index].trailLength = 0;
    for (int t = 0; t < 8; t++) {
        accretionDisk[index].trailX[t] = -1;
        accretionDisk[index].trailY[t] = -1;
    }
    
    // Calculate Keplerian orbital speed
    float orbitRatio = sqrt(currentDiskInnerRadius / accretionDisk[index].distance);
    accretionDisk[index].speed = 0.04f * orbitRatio;

    // Calculate initial trail colors in case they're needed immediately
    calculateTrailColors(accretionDisk[index]);
}

// Add this helper function
void calculateTrailColors(AccretionParticle &particle) {
    uint16_t baseColor = particle.color;
    int r = red(baseColor);
    int g = green(baseColor);
    int b = blue(baseColor);
    
    // Store particle colors with gradual fade
    for (int i = 0; i < 8; i++) {
        float fadeRatio = 1.0f - (i * 0.12f); // Fade out along trail
        particle.trailColors[i] = tft.color565(
            max(0, (int)(r * fadeRatio)),
            max(0, (int)(g * fadeRatio)),
            max(0, (int)(b * fadeRatio))
        );
    }
}

/**
 * Erases the black hole and its accretion disk - Simplified
 */
void eraseBlackHole() {
    if (blackHoleInitialized) {
        // Erase a sufficiently large area around the last known position
        // Calculate based on outer disk radius for safety
        float eraseRadius = (previousEventHorizonRadius > 0)
                            ? max(previousEventHorizonRadius * 2.5f, diskOuterRadius * 1.1f) + 5.0f
                            : 60.0f; // Default if no radius known
        tft.fillCircle(prevBlackHoleX, prevBlackHoleY, round(eraseRadius), BG_COLOR);

        // Reset state variables
        blackHoleInitialized = false;
        prevBlackHoleX = -1000; // Move off-screen to prevent ghost erase
        prevBlackHoleY = -1000;
        previousEventHorizonRadius = 0;
        // Also clear potentially persistent arrays if needed
         for (int i = 0; i < 60; i++) previousLensPoints[i][0] = -1;
         for (int i = 0; i < MAX_FALLING_STARS; i++) trailLen[i] = 0;
         for (int i = 0; i < 4; i++) prevInnerParticleX[i] = -1;
    }
}

#endif // BLACKHOLE_H