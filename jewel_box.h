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
#define MAX_JEWEL_STARS 80
#define MAX_BRIGHT_STARS 30

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

// Star colors for the Jewel Box (blue, yellow, orange, red, white, green, gold)
const uint16_t JEWEL_COLORS[] = {
    0x001F,  // Saturated Blue
    0x07FF,  // Cyan-blue
    0xFFE0,  // Yellow
    0xFD20,  // Orange
    0xF800,  // Red
    0xFFFF,   // White
    0x07E0,  // Green
    0xFDC0,   // Gold/Orange-Yellow
    0xAFE0,  // Bright Green
    0xFF00,  // Pure Red
    0x00FF   // Pure Blue
};
const int NUM_JEWEL_COLORS = sizeof(JEWEL_COLORS) / sizeof(JEWEL_COLORS[0]);

void initializeJewelBox() {
    if (jewelBoxInitialized) return;
    
    float centerX = 0; // Relative to object center
    float centerY = 0;
    float clusterRadius = 18.0f; // Slightly increased cluster radius
    
    // Initialize all stars with radial distribution
    for (int i = 0; i < MAX_JEWEL_STARS; i++) {
        jewelStars[i].active = true;
        jewelStars[i].prevX = -1;
        jewelStars[i].prevY = -1;

        // Use a distribution that concentrates stars towards the center
        float angle = random(360) * PI / 180.0f;
        // Use a power law for radius distribution to concentrate towards center
        float u = random(1000) / 1000.0f; // Uniform random [0,1]
        float distance = clusterRadius * pow(u, 0.5); // Adjusted exponent for more central concentration

        jewelStars[i].originalX = centerX + cos(angle) * distance;
        jewelStars[i].originalY = centerY + sin(angle) * distance;
        
        // Assign bright/colored status and properties based on index or probability
        if (i < MAX_BRIGHT_STARS) {
            // These are the prominent, brighter stars
            jewelStars[i].isBrightStar = true;
            jewelStars[i].size = random(2, 5); // Larger size range
            jewelStars[i].brightness = 0.95f + random(5) / 100.0f; // Increased initial brightness
            jewelStars[i].twinkleSpeed = 0.04f + random(10) / 500.0f; // Faster, more noticeable twinkle
            // Assign colors from expanded palette - more likely to get distinct colors
            jewelStars[i].color = JEWEL_COLORS[random(NUM_JEWEL_COLORS)]; // Use full palette for bright ones
             if (random(100) < 25) { // Increased chance for a white bright star
                 jewelStars[i].color = 0xFFFF;
             }
        } else {
            // These are dimmer background stars
            jewelStars[i].isBrightStar = false;
            jewelStars[i].size = 1; // Single pixel
            jewelStars[i].brightness = 0.5f + random(30) / 100.0f; // Increased initial brightness for dimmer stars
            jewelStars[i].twinkleSpeed = 0.02f + random(5) / 500.0f; // Slightly faster twinkling
            jewelStars[i].color = 0xFFFF; // Mostly white background stars
            if (random(100) < 15) { // Increased chance for a colored dimmer star
                jewelStars[i].color = JEWEL_COLORS[random(NUM_JEWEL_COLORS - 2)]; // Exclude Green and Gold for dimmer ones mostly
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
        float driftX = sin(currentTime / 4000.0f + i) * 0.3f; // Slightly faster and wider horizontal drift
        float driftY = cos(currentTime / 5000.0f + i*2) * 0.3f; // Slightly faster and wider vertical drift

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
        float twinkleFactor = jewelStars[i].isBrightStar ? (0.5f + 0.5f * sin(jewelStars[i].twinklePhase)) : (0.7f + 0.3f * sin(jewelStars[i].twinklePhase)); // Stronger twinkle overall, more for bright stars
        float currentBrightness = jewelStars[i].brightness * twinkleFactor;
        currentBrightness = constrain(currentBrightness, 0.3f, 1.0f); // Increased minimum brightness during twinkle cycle
        
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
                // Draw larger stars as '+' symbols
                int halfSize = jewelStars[i].size / 2;
                tft.drawLine(drawX - halfSize, drawY, drawX + halfSize, drawY, drawColor); // Horizontal line
                tft.drawLine(drawX, drawY - halfSize, drawX, drawY + halfSize, drawColor); // Vertical line
                
                // Add sparkle effect for bright colored stars (small halo/glow and diffraction spikes)
                if (jewelStars[i].isBrightStar && currentBrightness > 0.7f) { // Sparkle when bright
                    int glowRadius = halfSize + 1;
                    uint16_t glowColor = jewelStars[i].color; // Use star's color for glow
                     // Reduce brightness for glow to make it subtle
                    uint8_t r5 = (glowColor >> 11) & 0x1F;
                    uint8_t g6 = (glowColor >> 5) & 0x3F;
                    uint8_t b5 = glowColor & 0x1F;
                    
                    uint8_t r_scaled = (uint8_t)(r5 * 0.5f); // Slightly brighter glow
                    uint8_t g_scaled = (uint8_t)(g6 * 0.5f);
                    uint8_t b_scaled = (uint8_t)(b5 * 0.5f);
                    
                    uint16_t dimGlowColor = (r_scaled << 11) | (g_scaled << 5) | b_scaled;
                    
                    tft.drawCircle(drawX, drawY, glowRadius, dimGlowColor);
                    
                    // Add subtle diffraction spikes for the brightest stars
                    if (currentBrightness > 0.9f && jewelStars[i].size > 2) { // Add spikes for very bright, larger stars
                        int spikeLength = halfSize + 2;
                        uint16_t spikeColor = jewelStars[i].color;
                         // Dimmer spikes
                        uint8_t spike_r5 = (spikeColor >> 11) & 0x1F;
                        uint8_t spike_g6 = (spikeColor >> 5) & 0x3F;
                        uint8_t spike_b5 = spikeColor & 0x1F;
                        
                        uint8_t spike_r_scaled = (uint8_t)(spike_r5 * 0.7f); // Dimmer spikes
                        uint8_t spike_g_scaled = (uint8_t)(spike_g6 * 0.7f);
                        uint8_t spike_b_scaled = (uint8_t)(spike_b5 * 0.7f);
                        
                        uint16_t dimSpikeColor = (spike_r_scaled << 11) | (spike_g_scaled << 5) | spike_b_scaled;

                        tft.drawLine(drawX - spikeLength, drawY, drawX + spikeLength, drawY, dimSpikeColor);
                        tft.drawLine(drawX, drawY - spikeLength, drawX, drawY + spikeLength, dimSpikeColor);
                    }
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
    
    // Full screen clear for faster erase
    tft.fillScreen(BG_COLOR);
    
    // No allocated memory to free for this object
    
    jewelBoxInitialized = false; // Reset initialization flag
}

#endif // JEWEL_BOX_H 