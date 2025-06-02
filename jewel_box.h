#ifndef JEWEL_BOX_H
#define JEWEL_BOX_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <SPI.h>

// External references
extern TFT_eSPI tft;
extern const int SCREEN_WIDTH;
extern const int SCREEN_HEIGHT;
extern uint16_t BG_COLOR;
extern int objectX, objectY;
extern float objectScale;

// Jewel Box Cluster parameters
#define MAX_JEWEL_STARS 55
#define MAX_BRIGHT_STARS 20

struct JewelStar {
    float x, y;
    float originalX, originalY; // Original relative position
    uint16_t color;
    float brightness;
    float twinklePhase;
    float twinkleSpeed;
    int size;
    int prevX, prevY;
    bool active;
    bool isBrightStar; // True for the main colored/bright stars
};

// Global variables for Jewel Box animation
JewelStar jewelStars[MAX_JEWEL_STARS];
bool jewelBoxInitialized = false;
unsigned long jewelBoxLastUpdateTime = 0;

// Star colors for the Jewel Box (blue, yellow, orange, red, white)
const uint16_t JEWEL_COLORS[] = {
    0x1E7F,  // Deep blue
    0x07FF,  // Cyan-blue  
    0xFFE0,  // Yellow
    0xFD20,  // Orange
    0xF800,  // Red
    0xFFFF   // White - keep white in palette for random assignment possibility
};
const int NUM_JEWEL_COLORS = sizeof(JEWEL_COLORS) / sizeof(JEWEL_COLORS[0]);

void initializeJewelBox() {
    if (jewelBoxInitialized) return;
    
    float centerX = 0; // Relative to object center
    float centerY = 0;
    float clusterRadius = 15.0f; // Adjusted cluster radius
    
    // Initialize all stars with radial distribution
    for (int i = 0; i < MAX_JEWEL_STARS; i++) {
        jewelStars[i].active = true;
        jewelStars[i].prevX = -1;
        jewelStars[i].prevY = -1;

        // Use a distribution that concentrates stars towards the center
        float angle = random(360) * PI / 180.0f;
        // Cube the random distance for higher density near center
        float u = random(1000) / 1000.0f; // Uniform random [0,1]
        float distance = clusterRadius * pow(u, 0.3); // Adjust exponent for distribution

        jewelStars[i].originalX = centerX + cos(angle) * distance;
        jewelStars[i].originalY = centerY + sin(angle) * distance;
        
        // Assign bright/colored status and properties based on index or probability
        if (i < MAX_BRIGHT_STARS) {
            // These are the prominent, brighter stars
            jewelStars[i].isBrightStar = true;
            jewelStars[i].size = random(2, 4); // Larger size
            jewelStars[i].brightness = 0.8f + random(20) / 100.0f; // Brighter initial brightness
            jewelStars[i].twinkleSpeed = 0.02f + random(10) / 500.0f;
            // Assign colors from palette - more likely to get distinct colors
            jewelStars[i].color = JEWEL_COLORS[random(NUM_JEWEL_COLORS -1 )]; // Exclude pure white for bright ones initially
             if (random(100) < 10) { // Small chance for a white bright star
                 jewelStars[i].color = 0xFFFF;
             }
        } else {
            // These are dimmer background stars
            jewelStars[i].isBrightStar = false;
            jewelStars[i].size = 1; // Single pixel
            jewelStars[i].brightness = 0.3f + random(30) / 100.0f; // Dimmer initial brightness
            jewelStars[i].twinkleSpeed = 0.01f + random(5) / 500.0f;
            jewelStars[i].color = 0xFFFF; // Mostly white background stars
            if (random(100) < 5) { // Very small chance for a colored dimmer star
                jewelStars[i].color = JEWEL_COLORS[random(NUM_JEWEL_COLORS -1 )];
            }
        }
        jewelStars[i].twinklePhase = random(360) * PI / 180.0f; // Random starting phase
    }
    
    jewelBoxInitialized = true;
    jewelBoxLastUpdateTime = millis();
}

void drawJewelBox() {
    if (!jewelBoxInitialized) {
        initializeJewelBox();
    }
    
    unsigned long currentTime = millis();
    float deltaTime = (currentTime - jewelBoxLastUpdateTime) / 1000.0f;
    if (deltaTime > 0.1f) deltaTime = 0.1f; // Cap deltaTime
    jewelBoxLastUpdateTime = currentTime;
    
    int centerX = objectX;
    int centerY = objectY;
    float scale = objectScale;
    
    // Update and draw each star
    for (int i = 0; i < MAX_JEWEL_STARS; i++) {
        if (!jewelStars[i].active) continue;
        
        // Calculate new position based on scale and center
        // Add a very subtle random drift to the original position
        float driftX = sin(currentTime / 5000.0f + i) * 0.2f; // Slow horizontal drift
        float driftY = cos(currentTime / 6000.0f + i*2) * 0.2f; // Slow vertical drift

        jewelStars[i].x = centerX + (jewelStars[i].originalX + driftX) * scale;
        jewelStars[i].y = centerY + (jewelStars[i].originalY + driftY) * scale;
        
        // Only erase if the position has actually changed enough
        int drawX = (int)jewelStars[i].x;
        int drawY = (int)jewelStars[i].y;

        if (jewelStars[i].prevX != drawX || jewelStars[i].prevY != drawY) {
             // Erase previous position with a slightly larger area to clean up completely
            int eraseSize = jewelStars[i].size + 2; // Increased erase size
            for (int ex = -eraseSize; ex <= eraseSize; ex++) {
                for (int ey = -eraseSize; ey <= eraseSize; ey++) {
                    int px = jewelStars[i].prevX + ex;
                    int py = jewelStars[i].prevY + ey;
                    if (px >= 0 && px < SCREEN_WIDTH && py >= 0 && py < SCREEN_HEIGHT) {
                        tft.drawPixel(px, py, BG_COLOR);
                    } else {
                        // If prev pos was off screen, just mark as cleared (no drawing needed)
                    }
                }
            }
        }

        // Update twinkling
        jewelStars[i].twinklePhase += jewelStars[i].twinkleSpeed * deltaTime; // Scale speed by deltaTime
        if (jewelStars[i].twinklePhase > 2 * PI) {
            jewelStars[i].twinklePhase -= 2 * PI;
        }
        
        // Calculate brightness with twinkling effect - slightly stronger twinkle for bright stars
        float twinkleFactor = jewelStars[i].isBrightStar ? (0.6f + 0.4f * sin(jewelStars[i].twinklePhase)) : (0.8f + 0.2f * sin(jewelStars[i].twinklePhase));
        float currentBrightness = jewelStars[i].brightness * twinkleFactor;
        currentBrightness = constrain(currentBrightness, 0.1f, 1.0f); // Ensure minimum brightness
        
        // Apply brightness to color (optimized)
        uint16_t drawColor;
        if (currentBrightness >= 1.0f) {
             drawColor = jewelStars[i].color;
        } else {
             // Manually apply brightness to RGB565
            uint8_t r5 = (jewelStars[i].color >> 11) & 0x1F;
            uint8_t g6 = (jewelStars[i].color >> 5) & 0x3F;
            uint8_t b5 = jewelStars[i].color & 0x1F;
            
            uint8_t r_scaled = (uint8_t)(r5 * currentBrightness);
            uint8_t g_scaled = (uint8_t)(g6 * currentBrightness);
            uint8_t b_scaled = (uint8_t)(b5 * currentBrightness);
            
            drawColor = (r_scaled << 11) | (g_scaled << 5) | b_scaled;
        }

        // Draw the star
        if (drawX >= 0 && drawX < SCREEN_WIDTH && drawY >= 0 && drawY < SCREEN_HEIGHT) {
            if (jewelStars[i].size == 1) {
                tft.drawPixel(drawX, drawY, drawColor);
            } else {
                // Draw larger stars as filled circles
                int circleRadius = jewelStars[i].size / 2;
                tft.fillCircle(drawX, drawY, circleRadius, drawColor);
                
                // Add sparkle effect for bright colored stars (small halo/glow)
                if (jewelStars[i].isBrightStar && currentBrightness > 0.8f) { // Sparkle when bright
                    int glowRadius = circleRadius + 1;
                    uint8_t glowAlpha = map(currentBrightness, 0.8f, 1.0f, 50, 100); // Glow intensity based on brightness
                    uint16_t glowColor = tft.color565(glowAlpha, glowAlpha, glowAlpha); // White glow
                    tft.drawCircle(drawX, drawY, glowRadius, glowColor);
                }
            }
        }
        
        // Store current position for next frame's erase
        jewelStars[i].prevX = drawX;
        jewelStars[i].prevY = drawY;
    }
}

void eraseJewelBox() {
    if (!jewelBoxInitialized) return;
    
    // Erase all star positions
    for (int i = 0; i < MAX_JEWEL_STARS; i++) {
        // Only erase if the previous position was on screen
        if (jewelStars[i].prevX >= 0 && jewelStars[i].prevX < SCREEN_WIDTH &&
            jewelStars[i].prevY >= 0 && jewelStars[i].prevY < SCREEN_HEIGHT) {
            
            int eraseSize = jewelStars[i].size + 3; // Ensure full cleanup
            for (int ex = -eraseSize; ex <= eraseSize; ex++) {
                for (int ey = -eraseSize; ey <= eraseSize; ey++) {
                    int px = jewelStars[i].prevX + ex;
                    int py = jewelStars[i].prevY + ey;
                    if (px >= 0 && px < SCREEN_WIDTH && py >= 0 && py < SCREEN_HEIGHT) {
                        tft.drawPixel(px, py, BG_COLOR);
                    }
                }
            }
        }
        jewelStars[i].prevX = -1;
        jewelStars[i].prevY = -1; // Mark as erased/invalid
    }
    
    jewelBoxInitialized = false; // Reset initialization flag
}

#endif // JEWEL_BOX_H 