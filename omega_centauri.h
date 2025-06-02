#ifndef OMEGA_CENTAURI_H
#define OMEGA_CENTAURI_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include <esp_heap_caps.h> // Include for heap_caps_malloc

// External references
extern TFT_eSPI tft;
extern const int SCREEN_WIDTH;
extern const int SCREEN_HEIGHT;
extern uint16_t BG_COLOR;
extern int objectX, objectY;
extern float objectScale;

// Omega Centauri parameters
#define MAX_OMEGA_STARS 900
#define MAX_CORE_STARS 150
#define MAX_STREAM_STARS 20

struct OmegaStar {
    float x, y;
    float originalX, originalY; // Original relative position
    float angle; // For orbital motion
    float distance; // Distance from center
    float orbitalSpeed;
    uint16_t color;
    float brightness;
    float twinklePhase;
    float twinkleSpeed;
    int size;
    int prevX, prevY;
    bool active;
    bool isCoreStar; // True for dense core stars
    bool isStreamStar; // True for streaming stars
    float streamPhase; // For streaming animation
};

// Global variables for Omega Centauri animation
// OmegaStar omegaStars[MAX_OMEGA_STARS]; // Removed static array
OmegaStar* omegaStars = nullptr; // Pointer for dynamic allocation in PSRAM
int numOmegaStars = 0; // To store the actual number of allocated stars
bool omegaCentauriInitialized = false;
unsigned long omegaCentauriLastUpdateTime = 0;
float globalRotation = 0.0f;

// Color palette for globular cluster (mostly yellow-white to red)
const uint16_t OMEGA_COLORS[] = {
    0xFFFF,  // White
    0xFFE0,  // Yellow
    0xFDA0,  // Light orange
    0xFB00,  // Orange
    0xF800,  // Red
    0xFEF5   // Pale yellow
};
const int NUM_OMEGA_COLORS = sizeof(OMEGA_COLORS) / sizeof(OMEGA_COLORS[0]);

void initializeOmegaCentauri() {
    if (omegaCentauriInitialized) return;
    
    // Determine how many stars to allocate based on available PSRAM
    // Let's target a high number, but be prepared for allocation failure.
    const int TARGET_MAX_OMEGA_STARS = 5000; // Target count for Omega Centauri (much denser)
    size_t starSize = sizeof(OmegaStar);
    size_t availablePSRAM = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    
    numOmegaStars = TARGET_MAX_OMEGA_STARS;
    size_t requiredSize = numOmegaStars * starSize;
    
    // Reduce target star count if PSRAM is insufficient
    if (requiredSize > availablePSRAM) {
        numOmegaStars = availablePSRAM / starSize;
        if (numOmegaStars < 100) numOmegaStars = 100; // Ensure a minimum
        requiredSize = numOmegaStars * starSize;
        Serial.printf("WARN: Not enough PSRAM for %d Omega Centauri stars, reducing to %d. Available: %u bytes\n", 
                      TARGET_MAX_OMEGA_STARS, numOmegaStars, availablePSRAM);
    }
    
    // Allocate memory in PSRAM (SPIRAM)
    omegaStars = (OmegaStar*)heap_caps_malloc(requiredSize, MALLOC_CAP_SPIRAM);
    
    if (omegaStars == nullptr) {
        Serial.println("ERROR: Failed to allocate PSRAM for Omega Centauri stars!");
        numOmegaStars = 0; // Allocation failed
        omegaCentauriInitialized = true; // Mark as initialized but with no stars
        omegaCentauriLastUpdateTime = millis();
        return; // Exit initialize if allocation failed
    } else {
         Serial.printf("Allocated %u bytes for %d Omega Centauri stars in PSRAM\n", requiredSize, numOmegaStars);
    }

    float centerX = 0; // Relative to object center
    float centerY = 0;
    float coreRadius = 12.0f; // Dense core radius
    float clusterRadius = 35.0f; // Full cluster radius
    
    // Calculate proportional counts based on allocated total
    int numCoreStars = numOmegaStars * 0.5; // 50% in core
    int numStreamStars = numOmegaStars * 0.05; // 5% streaming
    int numOuterStars = numOmegaStars - numCoreStars - numStreamStars; // Remaining in outer halo

    int starIndex = 0;
    
    // Initialize all stars with a smooth density gradient concentrated towards the center
    // and classify them into core/stream/outer for animation purposes
    for (int i = 0; i < numOmegaStars; i++) {
        omegaStars[i].active = true;
        omegaStars[i].size = 1; // All stars are point particles
        omegaStars[i].prevX = -1;
        omegaStars[i].prevY = -1;
        omegaStars[i].streamPhase = 0;
        
        // Use inverse transform sampling for a centrally concentrated distribution
        // f(r) ~ r^(-alpha) where alpha > 1 (e.g., alpha=2 for roughly inverse square law)
        // CDF is F(r) = (r^(2-alpha) - R_min^(2-alpha)) / (R_max^(2-alpha) - R_min^(2-alpha))
        // Inverse CDF R(u) = (u * (R_max^(2-alpha) - R_min^(2-alpha)) + R_min^(2-alpha))^(1/(2-alpha))
        // For alpha=2 (simplified): R(u) = R_min * exp(u * ln(R_max/R_min))
        // Let's use a simpler power law distribution R(u) = R_max * u^(1/gamma) where gamma > 0
        // Using R(u) = R_max * sqrt(u) or R_max * u is too linear. Let's try R(u) = R_max * u^(1/3) or u^(1/4)
        // A simpler approach: R(u) = R_max * sqrt(1-u^2) favors the core
        float u = random(1000) / 1000.0f; // Uniform random [0,1]
        // This distribution (sqrt(1-u^2)) puts more points near R_max, not center. Let's flip.
        // How about distribution proportional to 1/r? CDF is proportional to ln(r). R(u) = exp(u * ln(R_max) + (1-u) * ln(R_min))
        // Let's simplify and use a distribution that samples distance `d` from 0 to clusterRadius,
        // but with higher probability for smaller `d`. We can pick `d` randomly, but then accept/reject
        // based on a probability proportional to 1/d^p where p > 0.
        // A simpler way: sample angle uniformly, sample radius with decreasing probability.
        // Let's use `distance = clusterRadius * sqrt(random(1000)/1000.0f)` which concentrates them towards the edge... flip that.
        // `distance = clusterRadius * (1.0f - sqrt(random(1000)/1000.0f))` concentrates towards center.
        // Let's use `distance = clusterRadius * pow(random(1000)/1000.0f, power)` where power < 1.
        // Power = 0.25 gives strong concentration
        float r_dist_u = random(1000) / 1000.0f; // Uniform random [0,1]
        float distance = clusterRadius * pow(r_dist_u, 2.0f); // Concentrates towards outer edge (inverse gradient)

        float angle = random(360) * PI / 180.0f;
        
        omegaStars[i].distance = distance;
        omegaStars[i].angle = angle;
        omegaStars[i].originalX = centerX + cos(angle) * distance;
        omegaStars[i].originalY = centerY + sin(angle) * distance;
        
        // Classify stars based on their generated distance for animation/brightness
        float coreBoundary = 10.0f; // Define core region boundary based on distance
        float streamBoundary = 25.0f; // Define stream region boundary

        if (distance < coreBoundary) {
            omegaStars[i].isCoreStar = true;
            omegaStars[i].isStreamStar = false;
            omegaStars[i].brightness = 0.8f + random(20) / 100.0f; // Brighter in core
            omegaStars[i].orbitalSpeed = 0.005f + random(5) / 1000.0f; // Faster orbital speed in core
            // Core stars are mostly white, some rare colored ones
            if (random(100) < 5) omegaStars[i].color = OMEGA_COLORS[random(NUM_OMEGA_COLORS)];
            else omegaStars[i].color = 0xFFFF;
            omegaStars[i].twinkleSpeed = 0.015f + random(10) / 1000.0f; // Faster twinkling in core
        } else if (distance < streamBoundary && random(100) < 15) { // Some stars in mid-range are streaming
            omegaStars[i].isCoreStar = false;
            omegaStars[i].isStreamStar = true;
            omegaStars[i].brightness = 0.4f + random(30) / 100.0f;
            omegaStars[i].orbitalSpeed = 0.01f + random(10) / 1000.0f; // Faster orbital speed for streaming
            // Stream stars mostly white, some rare colored ones
            if (random(100) < 8) omegaStars[i].color = OMEGA_COLORS[random(NUM_OMEGA_COLORS)];
            else omegaStars[i].color = 0xFFFF;
            omegaStars[i].twinkleSpeed = 0.02f + random(10) / 1000.0f; // Faster twinkling
            omegaStars[i].streamPhase = random(100) / 100.0f * 2 * PI;
        } else {
            omegaStars[i].isCoreStar = false;
            omegaStars[i].isStreamStar = false;
            omegaStars[i].brightness = 0.2f + random(40) / 100.0f; // Dimmer in outer halo
            omegaStars[i].orbitalSpeed = 0.003f + random(3) / 1000.0f; // Slower orbital speed in outer halo
            // Outer stars mostly white, very rare colored ones
            if (random(100) < 3) omegaStars[i].color = OMEGA_COLORS[random(NUM_OMEGA_COLORS)];
            else omegaStars[i].color = 0xFFFF;
            omegaStars[i].twinkleSpeed = 0.01f + random(5) / 1000.0f; // Slower twinkling
        }
         omegaStars[i].twinklePhase = random(360) * PI / 180.0f;
    }
    
    omegaCentauriInitialized = true;
    omegaCentauriLastUpdateTime = millis();
}

void drawOmegaCentauri() {
    if (!omegaCentauriInitialized || omegaStars == nullptr) { // Added check for nullptr
        initializeOmegaCentauri();
        if (omegaStars == nullptr) return; // Exit if initialization failed
    }
    
    unsigned long currentTime = millis();
    float deltaTime = (currentTime - omegaCentauriLastUpdateTime) / 1000.0f;
    if (deltaTime > 0.1f) deltaTime = 0.1f;
    omegaCentauriLastUpdateTime = currentTime;
    
    int centerX = objectX;
    int centerY = objectY;
    float scale = objectScale;
    
    // Update global rotation
    globalRotation += 0.001f; // Slower rotation
    if (globalRotation > 2 * PI) globalRotation -= 2 * PI;
    
    // Update and draw each star
    for (int i = 0; i < numOmegaStars; i++) {
        if (!omegaStars[i].active) continue;
        
        // Erase previous position
        if (omegaStars[i].prevX >= 0 && omegaStars[i].prevY >= 0) {
            // Erase the main pixel position
            if (omegaStars[i].prevX >= 0 && omegaStars[i].prevX < SCREEN_WIDTH &&
                omegaStars[i].prevY >= 0 && omegaStars[i].prevY < SCREEN_HEIGHT) {
                tft.drawPixel(omegaStars[i].prevX, omegaStars[i].prevY, BG_COLOR);
            }
            
            // Erase the streaming trail if it was drawn
            if (omegaStars[i].isStreamStar && omegaStars[i].brightness * (0.6f + 0.4f * sin(omegaStars[i].twinklePhase)) > 0.5f) { // Check if trail was likely drawn
                 // Recompute trail position based on previous state (using originalX, originalY + offsets)
                float prevStreamOffset = sin(omegaStars[i].streamPhase - 0.03f * deltaTime) * 5.0f; // Approximate previous offset
                float prevDistance = omegaStars[i].distance + prevStreamOffset;
                float prevAngle = omegaStars[i].angle - (omegaStars[i].orbitalSpeed + globalRotation * 0.1f); // Approximate previous angle

                float prevX_orig = cos(prevAngle) * prevDistance;
                float prevY_orig = sin(prevAngle) * prevDistance;

                int prevTrailX = (int)(objectX + prevX_orig * objectScale + cos(prevAngle + PI) * 2 * objectScale); // Scale trail offset too
                int prevTrailY = (int)(objectY + prevY_orig * objectScale + sin(prevAngle + PI) * 2 * objectScale);
                
                 // Erase the trail pixel
                if (prevTrailX >= 0 && prevTrailX < SCREEN_WIDTH && prevTrailY >= 0 && prevTrailY < SCREEN_HEIGHT) {
                    tft.drawPixel(prevTrailX, prevTrailY, BG_COLOR);
                }
            }
        }
        
        // Update orbital position
        omegaStars[i].angle += omegaStars[i].orbitalSpeed + globalRotation * 0.1f;
        if (omegaStars[i].angle > 2 * PI) omegaStars[i].angle -= 2 * PI;
        
        // Calculate new position with orbital motion
        float currentDistance = omegaStars[i].distance;
        
        // Add streaming effect for stream stars
        if (omegaStars[i].isStreamStar) {
            omegaStars[i].streamPhase += 0.03f;
            if (omegaStars[i].streamPhase > 2 * PI) omegaStars[i].streamPhase -= 2 * PI;
            
            // Stars appear to stream outward from core
            float streamOffset = sin(omegaStars[i].streamPhase) * 5.0f;
            currentDistance += streamOffset;
        }
        
        float x = cos(omegaStars[i].angle) * currentDistance;
        float y = sin(omegaStars[i].angle) * currentDistance;
        
        omegaStars[i].x = centerX + x * scale;
        omegaStars[i].y = centerY + y * scale;
        
        // Update twinkling
        omegaStars[i].twinklePhase += omegaStars[i].twinkleSpeed;
        if (omegaStars[i].twinklePhase > 2 * PI) {
            omegaStars[i].twinklePhase -= 2 * PI;
        }
        
        // Calculate brightness with twinkling effect
        float twinkleFactor = 0.6f + 0.4f * sin(omegaStars[i].twinklePhase);
        float currentBrightness = omegaStars[i].brightness * twinkleFactor;
        
        // Core stars are brighter
        if (omegaStars[i].isCoreStar) {
            currentBrightness *= 1.5f;
        }
        
        // Apply brightness to color
        uint16_t drawColor = omegaStars[i].color;
        if (currentBrightness < 1.0f) {
            int r = ((drawColor >> 11) & 0x1F) * currentBrightness;
            int g = ((drawColor >> 5) & 0x3F) * currentBrightness;
            int b = (drawColor & 0x1F) * currentBrightness;
            drawColor = (r << 11) | (g << 5) | b;
        }
        
        // Draw the star as a single pixel
        int drawX = (int)omegaStars[i].x;
        int drawY = (int)omegaStars[i].y;
        
        if (drawX >= 0 && drawX < SCREEN_WIDTH && drawY >= 0 && drawY < SCREEN_HEIGHT) {
            tft.drawPixel(drawX, drawY, drawColor);
            
            // Add streaming trail for stream stars
            if (omegaStars[i].isStreamStar && currentBrightness > 0.5f) {
                // Draw a faint trail behind streaming stars
                float trailAngle = omegaStars[i].angle + PI; // Opposite direction
                // Calculate trail position relative to current scaled position
                int trailX = drawX + (int)(cos(trailAngle) * 2 * scale); // Scale trail length
                int trailY = drawY + (int)(sin(trailAngle) * 2 * scale); // Scale trail length
                
                if (trailX >= 0 && trailX < SCREEN_WIDTH && trailY >= 0 && trailY < SCREEN_HEIGHT) {
                    // Dimmer color for trail (use 8-bit brightness calculation)
                    uint16_t baseColor = omegaStars[i].color;
                    float trailBrightness = currentBrightness * 0.3f; // Trail is dimmer
                    uint8_t r5 = (baseColor >> 11) & 0x1F;
                    uint8_t g6 = (baseColor >> 5) & 0x3F;
                    uint8_t b5 = baseColor & 0x1F;
                    
                    uint8_t r_scaled = (uint8_t)(r5 * trailBrightness);
                    uint8_t g_scaled = (uint8_t)(g6 * trailBrightness);
                    uint8_t b_scaled = (uint8_t)(b5 * trailBrightness);
                    
                    uint16_t trailColor = (r_scaled << 11) | (g_scaled << 5) | b_scaled;
                    
                    tft.drawPixel(trailX, trailY, trailColor);
                }
            }
        }
        
        // Store current position for next frame's erase
        omegaStars[i].prevX = drawX;
        omegaStars[i].prevY = drawY;
    }
}

void eraseOmegaCentauri() {
    if (!omegaCentauriInitialized) return;
    
    // Full screen clear for faster erase
    tft.fillScreen(BG_COLOR);

    // Erase core glow
    int centerX = objectX;
    int centerY = objectY;
    float scale = objectScale;
    int glowRadius = (int)(10 * scale);
    tft.fillCircle(centerX, centerY, glowRadius, BG_COLOR);
    
    // Erase the name
    tft.setTextSize(1);
    tft.setTextColor(BG_COLOR);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("Omega Centauri", SCREEN_WIDTH / 2, SCREEN_HEIGHT - 15);
    
    // Free allocated memory if it exists
    if (omegaStars != nullptr) {
        heap_caps_free(omegaStars);
        omegaStars = nullptr;
        numOmegaStars = 0; // Reset count
        Serial.println("Freed PSRAM for Omega Centauri stars.");
    }

    omegaCentauriInitialized = false;
}

#endif // OMEGA_CENTAURI_H 