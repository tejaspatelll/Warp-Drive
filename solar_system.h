/**
 * solar_system.h
 * 
 * This file contains all code related to drawing and managing a solar system
 * object in the Warp Drive visualization.
 */

#ifndef SOLAR_SYSTEM_H
#define SOLAR_SYSTEM_H

#include <TFT_eSPI.h>
#include <Arduino.h>
#include <cmath>
#include "star.h" // Include for Star struct

// Forward declarations from main sketch
extern TFT_eSPI tft;
extern const int SCREEN_WIDTH;
extern const int SCREEN_HEIGHT;
extern uint16_t BG_COLOR;
extern uint16_t COLOR_HIGHLIGHT;
extern uint16_t COLOR_BORDER;
extern const uint16_t COLOR_STAR;
extern int objectX;
extern int objectY;
extern float objectScale;
extern float scaleFactor;
extern float scaleX;
extern Star stars[];

// Forward declare functions
extern int scale_i(int v);

// Define PI if not already defined
#ifndef PI
#define PI 3.14159265358979323846
#endif

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
static FlareParticle flareParticles[MAX_FLARE_PARTICLES];
extern bool solarSystemInitialized; // Flag for initialization

// Previous positions - declarations
static int prevSunX, prevSunY, prevSunRadius;
static int prevOrbitRadii[4]; // Stores scaled orbit radii
static int prevPlanetX[4], prevPlanetY[4], prevPlanetRadius[4];

// Sprite buffering for solar system
static SpriteHandle sunHandle = {0};
static SpriteHandle planetHandles[4] = {{0}, {0}, {0}, {0}};
static bool solarSystemSpritesCreated = false;  // Flag to track if sprites are created
static bool forceRedrawSun = true; // Always draw sun on first frame
static bool forceRedrawPlanets = true; // Always draw planets on first frame

// Forward declarations
void drawSolarSystem();
void eraseSolarSystem();

void drawSolarSystem() {
    int centerX = objectX;
    int centerY = objectY;
    float scale = objectScale * scaleFactor; // Apply global scaling
    
    // Sun properties
    int sunRadius = round(10 * scale);
    // Maximum planet sizes for buffer allocation
    int maxPlanetRadius = scale_i(4) * objectScale + 4; // Add some margin for glow

    // Planet and orbit properties (baseOrbitRadii are scaled by global scaleFactor via scale_i)
    float speeds[] = {0.5f * scaleX, 0.3f * scaleX, 0.2f * scaleX, 0.1f * scaleX};
    int planetSizes[] = { scale_i(3), scale_i(4), scale_i(3), scale_i(2) }; // Scaled by global scaleFactor
    int baseOrbitRadii[] = { scale_i(20), scale_i(30), scale_i(40), scale_i(50) }; // Scaled by global scaleFactor

    uint16_t planetColors[4] = {
        tft.color565(180, 180, 180), // Mercury: gray
        tft.color565(255, 200, 80),  // Venus: yellowish
        tft.color565(80, 180, 255),  // Earth: blue
        tft.color565(255, 100, 80)   // Mars: red
    };
    
    float t = millis() / 1000.0f;

    // Initialization block (runs once per solar system display)
    if (!solarSystemInitialized) {
        // Initialize flare particles
        for (int i = 0; i < MAX_FLARE_PARTICLES; i++) {
            flareParticles[i].active = false;
        }

        // Initialize prevSunRadius to 0 to prevent erasure before first draw
        prevSunRadius = 0;
        // Initialize prevPlanetRadius and prevOrbitRadii to 0
        for(int i=0; i<4; ++i) {
            prevPlanetRadius[i] = 0;
            prevOrbitRadii[i] = 0; // Orbits will be set on first full draw pass
        }

        // Create sprites for double buffering
        if (sunHandle.id == 0) { // Check if sun sprite needs creation
            int sunSpriteSize = sunRadius * 2 + 8; // Padding for corona
            Serial.printf("[SolarSystem] Creating sun sprite: %dx%d\n", sunSpriteSize, sunSpriteSize);
            SpriteAllocResult sunRes = SpriteManager::create(sunSpriteSize, sunSpriteSize, true, sunHandle); // Prefer PSRAM
            if (sunRes == SpriteAllocResult::Success || sunRes == SpriteAllocResult::SuccessShrunk || sunRes == SpriteAllocResult::FellBackToHeap) {
                Serial.println("[SolarSystem] Sun sprite created.");
                forceRedrawSun = true;
            } else {
                Serial.printf("[SolarSystem] ERROR: Failed to create sun sprite! Result: %d\n", (int)sunRes);
                sunHandle = {0};
            }
        }

        // Create planet sprites
        for (int i = 0; i < 4; i++) {
            if (planetHandles[i].id == 0) { // Check if this planet sprite needs creation
                int planetSpriteSize = maxPlanetRadius * 2 + 4; // Padding for glow
                Serial.printf("[SolarSystem] Creating planet %d sprite: %dx%d\n", i, planetSpriteSize, planetSpriteSize);
                SpriteAllocResult planetRes = SpriteManager::create(planetSpriteSize, planetSpriteSize, true, planetHandles[i]); // Prefer PSRAM
                if (planetRes == SpriteAllocResult::Success || planetRes == SpriteAllocResult::SuccessShrunk || planetRes == SpriteAllocResult::FellBackToHeap) {
                    Serial.printf("[SolarSystem] Planet %d sprite created.\n", i);
                    forceRedrawPlanets = true; // Force redraw for all if one is created
                } else {
                     Serial.printf("[SolarSystem] ERROR: Failed to create planet %d sprite! Result: %d\n", i, (int)planetRes);
                     planetHandles[i] = {0};
                }
            }
        }

        // The starfield drawing here (for 20 stars) was in the original init block.
        // It modifies the global `stars` array. If these are "local" stars, this is fine.
        // This part is kept as per original logic, assuming it's intended.
        for (int i = 0; i < 20; i++) {
            int starX = random(SCREEN_WIDTH);
            int starY = random(SCREEN_HEIGHT);
            uint8_t brightness = random(50, 150);
            
            Star starData;
            starData.x = starX;
            starData.y = starY;
            starData.realX = static_cast<float>(starX);
            starData.realY = static_cast<float>(starY);
            starData.brightness = brightness;
            starData.increasing = random(0, 2);
            stars[i] = starData; // Overwrites global stars[0] to stars[19]
        }
        solarSystemInitialized = true;
    }

    // --- ERASURE OF PREVIOUS FRAME'S DYNAMIC ELEMENTS ---
    // We'll skip direct erasure since we'll redraw with sprites.
    // Only erase what's absolutely necessary (mainly flare particles)

    // --- DRAWING CURRENT FRAME ELEMENTS ---

    // Draw Sun in sprite buffer
    static bool flareActive = false;
    static unsigned long lastFlareTime = 0;
    
    // Check for new flare activation (5% chance per frame)
    if (random(100) < 5) {
        flareActive = true;
        lastFlareTime = millis();
        forceRedrawSun = true; // Force redraw for new flare
    }
    
    // Deactivate flare after 1 second
    if (millis() - lastFlareTime > 1000 && flareActive) {
        flareActive = false;
        forceRedrawSun = true; // Force redraw when flare ends
    }
    
    // Always draw sun sprite - sun should always be visible
    // Only recreate the buffer contents when needed
    if (forceRedrawSun) {
        // Get sprite reference
        TFT_eSprite* sunSpritePtr = SpriteManager::getSpriteRef(sunHandle);
        if (sunSpritePtr) { // Check if sprite is valid before using it
            int spriteWidth = sunSpritePtr->width();
            int spriteHeight = sunSpritePtr->height();
            int spriteCenter = spriteWidth / 2; // Assuming square sprite

            // Clear sprite 
            sunSpritePtr->fillSprite(BG_COLOR); // Use BG_COLOR for clearing
            
            float pulseFactor = (sin(t * 2.0f) + 1.0f) / 2.0f;
            
            // Draw corona in sprite (use spritePtr)
            for (int r_sun_corona = sunRadius + 2; r_sun_corona > sunRadius; r_sun_corona--) {
                uint8_t brightness = map(r_sun_corona, sunRadius, sunRadius + 2, 255, 100);
                if (flareActive) {
                    brightness = min(255, brightness + 20);
                }
                uint16_t coronaColor = sunSpritePtr->color565(brightness, brightness, 0);
                sunSpritePtr->drawCircle(spriteCenter, spriteCenter, r_sun_corona, coronaColor);
            }
            
            // Draw sun body in sprite (use spritePtr)
            uint16_t sunColor = sunSpritePtr->color565(255, 255, 0); 
            if (flareActive) {
                sunColor = sunSpritePtr->color565(255, 255, 100);
            }
            sunSpritePtr->fillCircle(spriteCenter, spriteCenter, sunRadius, sunColor);
            
            forceRedrawSun = false; // Reset flag
        } else {
            // Sprite creation failed or handle invalid, draw directly to screen as fallback
            Serial.println("[SolarSystem] ERROR: Sun sprite handle invalid or ref failed - drawing directly.");
            tft.fillCircle(centerX, centerY, sunRadius, tft.color565(255, 255, 0));
            forceRedrawSun = false; // Still reset flag
        }
    }
    
    // Push sprite to screen at the correct position using handle
    if (sunHandle.id != 0) {
        int16_t sunSpriteW = SpriteManager::getWidth(sunHandle);
        if (sunSpriteW > 0) { // Check if width is valid
             int sunSpriteCenter = sunSpriteW / 2; // Use actual sprite width
             SpriteManager::draw(sunHandle, centerX - sunSpriteCenter, centerY - sunSpriteCenter);
        } else {
            // Handle case where entry not found but handle was valid (shouldn't happen)
            tft.fillCircle(centerX, centerY, sunRadius, tft.color565(255, 255, 0));
            Serial.println("[SolarSystem] ERROR: Sun sprite width invalid despite valid handle - drawing directly.");
        }
    } else {
        // If sprite handle is invalid, draw directly to screen
        tft.fillCircle(centerX, centerY, sunRadius, tft.color565(255, 255, 0));
        #ifdef ESP32
        Serial.println("[SolarSystem] ERROR: Sun sprite handle invalid during push - drawing directly.");
        #endif
    }
    
    prevSunX = centerX;
    prevSunY = centerY;
    prevSunRadius = sunRadius; // Store radius for next frame's erase

    // Draw faint orbit paths (EVERY FRAME)
    for (int i = 0; i < 4; i++) {
        int currentOrbitDrawRadius = baseOrbitRadii[i] * objectScale; // Apply object-specific scale
        for (int j = 0; j < 360; j += 5) { // Draw orbit as series of pixels
            float angle_rad = j * PI / 180.0f;
            int px = centerX + round(currentOrbitDrawRadius * cos(angle_rad));
            int py = centerY + round(currentOrbitDrawRadius * sin(angle_rad));
            if (px >= 0 && px < SCREEN_WIDTH && py >= 0 && py < SCREEN_HEIGHT) {
                tft.drawPixel(px, py, tft.color565(20, 20, 20)); // Faint orbit color
            }
        }
        prevOrbitRadii[i] = currentOrbitDrawRadius; // Store for eraseSolarSystem()
    }

    // Update and draw planets using sprite buffers
    // Always redraw planets since they're moving
    for (int i = 0; i < 4; i++) {
        int currentPlanetOrbitRadius = baseOrbitRadii[i] * objectScale; // Actual orbit radius for positioning
        int planetX = centerX + round(currentPlanetOrbitRadius * cos(t * speeds[i]));
        int planetY = centerY + round(currentPlanetOrbitRadius * sin(t * speeds[i]));
        int planetDrawRadius = planetSizes[i] * objectScale; // Visual radius of the planet
        
        // Draw planet in sprite buffer if needed
        if (forceRedrawPlanets) {
            TFT_eSprite* planetSpritePtr = SpriteManager::getSpriteRef(planetHandles[i]);
            if (planetSpritePtr) { // Check sprite validity
                int planetSpriteW = planetSpritePtr->width();
                int planetSpriteCenter = planetSpriteW / 2;
                planetSpritePtr->fillSprite(BG_COLOR); // Clear sprite
                
                // Draw planet features
                planetSpritePtr->fillCircle(planetSpriteCenter, planetSpriteCenter, planetDrawRadius, planetColors[i]);
                
                // Simple shading/highlight
                //uint16_t highlightColor = tft.color565(255, 255, 255); // Use global tft for color565
                //uint16_t shadowColor = tft.color565(0, 0, 0);
                //planetSpritePtr->fillCircle(planetSpriteCenter - planetDrawRadius/3, planetSpriteCenter - planetDrawRadius/3, planetDrawRadius/3, highlightColor);
                //planetSpritePtr->fillCircle(planetSpriteCenter + planetDrawRadius/3, planetSpriteCenter + planetDrawRadius/4, planetDrawRadius/4, shadowColor);
                
            } else {
                 Serial.printf("[SolarSystem] ERROR: Planet %d sprite handle invalid or ref failed - drawing directly.\n", i);
                 tft.fillCircle(planetX, planetY, planetDrawRadius, planetColors[i]); // Fallback direct draw
            }
        }
        
        // Push planet sprite using handle
        if (planetHandles[i].id != 0) {
            int16_t planetSpriteW = SpriteManager::getWidth(planetHandles[i]);
            if (planetSpriteW > 0) { // Check width validity
                 int planetSpriteCenter = planetSpriteW / 2; // Use actual sprite width
                 SpriteManager::draw(planetHandles[i], planetX - planetSpriteCenter, planetY - planetSpriteCenter);
            } else {
                 tft.fillCircle(planetX, planetY, planetDrawRadius, planetColors[i]); // Fallback direct draw
                 Serial.printf("[SolarSystem] ERROR: Planet %d sprite width invalid - drawing directly.\n", i);
            }
        } else {
             tft.fillCircle(planetX, planetY, planetDrawRadius, planetColors[i]); // Fallback direct draw
             #ifdef ESP32
             Serial.printf("[SolarSystem] ERROR: Planet %d sprite handle invalid during push - drawing directly.\n", i);
             #endif
        }

        // Draw moon for planet at index 1 directly on screen (small, no need for sprite)
        if (i == 1) {
            static int prevMoonX = -1, prevMoonY = -1;
            float moonAngle = t * 2.0f;
            int moonOrbitRadius = round(5 * scaleFactor * objectScale);
            int moonX = planetX + round(moonOrbitRadius * cos(moonAngle));
            int moonY = planetY + round(moonOrbitRadius * sin(moonAngle));
            
            // Clear previous moon position
            if (prevMoonX != -1 && prevMoonX >= 0 && prevMoonX < SCREEN_WIDTH && 
                prevMoonY >= 0 && prevMoonY < SCREEN_HEIGHT) {
                tft.drawPixel(prevMoonX, prevMoonY, BG_COLOR);
            }
            
            // Draw new moon position
            if (moonX >= 0 && moonX < SCREEN_WIDTH && moonY >= 0 && moonY < SCREEN_HEIGHT) {
                tft.drawPixel(moonX, moonY, COLOR_STAR);
                
                // Store current position for next frame
                prevMoonX = moonX;
                prevMoonY = moonY;
            }
        }

        prevPlanetX[i] = planetX;
        prevPlanetY[i] = planetY;
        prevPlanetRadius[i] = planetDrawRadius; // Store for next frame's erase
    }

    // Flare particles remain drawn directly on screen
    // First erase old positions
    for (int i = 0; i < MAX_FLARE_PARTICLES; i++) {
        if (flareParticles[i].active) {
            // Only erase if within screen bounds
            if (flareParticles[i].x >= 0 && flareParticles[i].x < SCREEN_WIDTH &&
                flareParticles[i].y >= 0 && flareParticles[i].y < SCREEN_HEIGHT) {
                tft.drawPixel(flareParticles[i].x, flareParticles[i].y, BG_COLOR);
            }
            
            // Update position with gravity effect (pulls back to sun)
            float dx_flare = centerX - flareParticles[i].x;
            float dy_flare = centerY - flareParticles[i].y;
            float distSq_flare = dx_flare * dx_flare + dy_flare * dy_flare;
            float dist_flare = sqrt(distSq_flare);
            
            if (dist_flare < sunRadius) { // Check if particle is inside the sun
                flareParticles[i].active = false;
                continue;
            }
            
            float gravityFactor = 0.05 * (sunRadius * 2 / (dist_flare * dist_flare));
            flareParticles[i].vx += gravityFactor * dx_flare / dist_flare;
            flareParticles[i].vy += gravityFactor * dy_flare / dist_flare;
            
            flareParticles[i].x += flareParticles[i].vx;
            flareParticles[i].y += flareParticles[i].vy;
            flareParticles[i].life -= 0.02f; // Adjust for decay
            
            if (flareParticles[i].life > 0 && 
                flareParticles[i].x >= 0 && flareParticles[i].x < SCREEN_WIDTH &&
                flareParticles[i].y >= 0 && flareParticles[i].y < SCREEN_HEIGHT) {
                
                uint8_t red_f = 255;
                uint8_t green_f = min(255, 180 + (int)(75 * (1.0 - dist_flare / (sunRadius * 5))));
                uint8_t blue_f = min(200, (int)(80 * (1.0 - dist_flare / (sunRadius * 3))));
                uint16_t currentColor = tft.color565(red_f, green_f, blue_f);
                tft.drawPixel(flareParticles[i].x, flareParticles[i].y, currentColor);
            } else {
                flareParticles[i].active = false;
            }
        }
    }

    // Generate new flares
    if (random(100) < 5) { // 5% chance per frame
        float flareAngle = random(360) * PI / 180.0f;
        float flareBaseX = centerX + sunRadius * cos(flareAngle);
        float flareBaseY = centerY + sunRadius * sin(flareAngle);
        
        // Set flareActive to redraw sun next frame
        flareActive = true;
        lastFlareTime = millis();
        forceRedrawSun = true; // Sun needs to be redrawn
        
        int particleCount = random(8, 17);
        float baseSpeed = random(8, 18) / 10.0f;
        
        for (int i = 0; i < particleCount; i++) {
            for (int j = 0; j < MAX_FLARE_PARTICLES; j++) {
                if (!flareParticles[j].active) {
                    flareParticles[j].x = flareBaseX;
                    flareParticles[j].y = flareBaseY;
                    
                    float arcPosition = (float)i / particleCount;
                    float arcFactor = 1.0 - fabs(arcPosition - 0.5) * 2.0;
                    float angleVariation = (random(-30, 30) + (arcPosition - 0.5) * 60) * PI / 180.0f;
                    float particleAngle = flareAngle + angleVariation;
                    bool willEscape = random(100) < 12;
                    float speed;
                    if (willEscape) {
                        speed = baseSpeed * (1.4f + arcFactor * 1.0f);
                    } else {
                        speed = baseSpeed * (0.44f + arcFactor * 0.84f);
                    }
                    flareParticles[j].vx = speed * cos(particleAngle);
                    flareParticles[j].vy = speed * sin(particleAngle);
                    flareParticles[j].life = random(70, 100) / 100.0f;
                    if (willEscape) {
                        flareParticles[j].life += 0.4f;
                    }
                    if (willEscape) {
                        flareParticles[j].color = tft.color565(255, 200, 100);
                    } else {
                        flareParticles[j].color = tft.color565(255, 150, 50);
                    }
                    flareParticles[j].active = true;
                    break;
                }
            }
        }
    }
}

// Update eraseSolarSystem to free sprites
void eraseSolarSystem() {
    // Destroy sprites using handles
    Serial.println("[SolarSystem] Erasing using SpriteManager::destroy");
    if (sunHandle.id != 0) {
        int16_t sunW = SpriteManager::getWidth(sunHandle);
        int16_t sunH = SpriteManager::getHeight(sunHandle);
        if (sunW > 0 && sunH > 0) {
            tft.fillRect(prevSunX - sunW / 2, prevSunY - sunH / 2, sunW, sunH, BG_COLOR);
        }
        SpriteManager::destroy(sunHandle);
        sunHandle = {0};
    }
    for (int i = 0; i < 4; i++) {
        if (planetHandles[i].id != 0) {
             int16_t planetW = SpriteManager::getWidth(planetHandles[i]);
             int16_t planetH = SpriteManager::getHeight(planetHandles[i]);
             if (planetW > 0 && planetH > 0) {
                 tft.fillRect(prevPlanetX[i] - planetW / 2, prevPlanetY[i] - planetH / 2, planetW, planetH, BG_COLOR);
             }
            SpriteManager::destroy(planetHandles[i]);
            planetHandles[i] = {0};
        }
    }
    
    // Clear orbit paths (can be slow, consider optimizing if needed)
    for (int i = 0; i < 4; i++) {
        if (prevOrbitRadii[i] > 0) {
            tft.fillCircle(objectX, objectY, prevOrbitRadii[i] + 1, BG_COLOR);
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
        // Clear orbit paths
        tft.fillCircle(objectX, objectY, prevOrbitRadii[i] + 1, BG_COLOR);
        prevOrbitRadii[i] = 0;
        prevPlanetRadius[i] = 0;
    }
    
    // Check memory status after cleanup
    #ifdef ESP32
    uint32_t freeHeap = ESP.getFreeHeap();
    uint32_t freePsram = ESP.getFreePsram();
    Serial.printf("Solar System cleanup - Heap: %u, PSRAM: %u\n", freeHeap, freePsram);
    
    // If memory is critically low after cleanup, we might have a leak
    if (freeHeap < 5000 || freePsram < 5000) {
        Serial.println("CRITICAL: Memory very low after solar system cleanup!");
        delay(100); // Give serial time to send
    }
    #endif
    
    solarSystemInitialized = false;
}

#endif // SOLAR_SYSTEM_H
