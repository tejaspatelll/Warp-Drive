#ifndef DOUBLE_CLUSTER_H
#define DOUBLE_CLUSTER_H

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

// Double Cluster parameters - Enhanced for realism
#define MAX_CLUSTER_STARS 75
#define MAX_NGC869_STARS 38  // Chi Persei (younger, more compact)
#define MAX_NGC884_STARS 37  // h Persei (older, more spread out)
#define MAX_BACKGROUND_STARS 30
#define MAX_SUPERGIANTS 8    // Bright red supergiants in the clusters

struct ClusterStar {
    float x, y;
    float originalX, originalY;
    uint16_t color;
    float brightness;
    float twinklePhase;
    float twinkleSpeed;
    float temperature; // Stellar temperature
    float mass; // Stellar mass
    float age; // Stellar age affects color and size
    float variability; // Some stars are variable
    int size;
    int prevX, prevY;
    bool active;
    int clusterId; // 0 = NGC 869, 1 = NGC 884, -1 = background
    bool isBrightStar; // True for prominent cluster stars
    bool isSupergiant; // True for evolved massive stars
    bool isRedSupergiant; // Specifically red supergiants
    int stellarClass; // 0=O, 1=B, 2=A, 3=F, 4=G, 5=K, 6=M
};

// Global variables for Double Cluster animation
ClusterStar doubleClusterStars[MAX_CLUSTER_STARS + MAX_BACKGROUND_STARS];
bool doubleClusterInitialized = false;
unsigned long doubleClusterLastUpdateTime = 0;
float stellarEvolutionPhase = 0.0f;

// Enhanced color palette for different stellar types
const uint16_t O_TYPE_COLORS[] = {
    0x07FF,  // Cyan-blue (very hot)
    0x1E7F,  // Deep blue
    0x3F7F   // Blue-white
};
const int NUM_O_TYPE_COLORS = sizeof(O_TYPE_COLORS) / sizeof(O_TYPE_COLORS[0]);

const uint16_t B_TYPE_COLORS[] = {
    0x07FF,  // Cyan-blue
    0x1E7F,  // Deep blue
    0x3F7F,  // Blue-white
    0xFFFF   // White-blue
};
const int NUM_B_TYPE_COLORS = sizeof(B_TYPE_COLORS) / sizeof(B_TYPE_COLORS[0]);

const uint16_t A_TYPE_COLORS[] = {
    0xFFFF,  // White
    0xF7FF,  // Slightly blue-white
    0xFFFE   // Pure white
};
const int NUM_A_TYPE_COLORS = sizeof(A_TYPE_COLORS) / sizeof(A_TYPE_COLORS[0]);

const uint16_t SUPERGIANT_COLORS[] = {
    0xF800,  // Red (M-type supergiant)
    0xFD20,  // Orange (K-type supergiant)
    0xFFE0,  // Yellow (G-type supergiant)
    0xFD40   // Orange-red
};
const int NUM_SUPERGIANT_COLORS = sizeof(SUPERGIANT_COLORS) / sizeof(SUPERGIANT_COLORS[0]);

// NGC 869 (Chi Persei) properties
const float NGC869_AGE = 5.0f; // Million years (very young)
const float NGC869_CENTER_X = -12.5f;
const float NGC869_CENTER_Y = 0.0f;
const float NGC869_RADIUS = 8.0f; // More compact

// NGC 884 (h Persei) properties  
const float NGC884_AGE = 13.0f; // Million years (older)
const float NGC884_CENTER_X = 12.5f;
const float NGC884_CENTER_Y = 0.0f;
const float NGC884_RADIUS = 10.0f; // More spread out

void initializeDoubleCluster() {
    if (doubleClusterInitialized) return;
    
    int starIndex = 0;
    
    // Initialize NGC 869 (Chi Persei) - younger, more compact cluster
    for (int i = 0; i < MAX_NGC869_STARS && starIndex < MAX_CLUSTER_STARS; i++, starIndex++) {
        doubleClusterStars[starIndex].active = true;
        doubleClusterStars[starIndex].clusterId = 0; // NGC 869
        doubleClusterStars[starIndex].age = NGC869_AGE;
        doubleClusterStars[starIndex].prevX = -1;
        doubleClusterStars[starIndex].prevY = -1;
        
        // Create realistic cluster distribution (King profile approximation)
        float angle = random(360) * PI / 180.0f;
        float radius = sqrt(random(1000) / 1000.0f) * NGC869_RADIUS;
        
        // Add some core concentration
        if (random(100) < 40) { // 40% in dense core
            radius *= 0.6f;
        }
        
        // Add asymmetry and structure
        float asymmetryX = (random(1000) / 1000.0f - 0.5f) * 2.0f;
        float asymmetryY = (random(1000) / 1000.0f - 0.5f) * 2.0f;
        
        doubleClusterStars[starIndex].originalX = NGC869_CENTER_X + cos(angle) * radius + asymmetryX;
        doubleClusterStars[starIndex].originalY = NGC869_CENTER_Y + sin(angle) * radius + asymmetryY;
        
        // Determine stellar type based on cluster age and mass function
        int stellarTypeRand = random(100);
        if (stellarTypeRand < 8 && i < 6) { // 8% O-type stars (only in core)
            doubleClusterStars[starIndex].stellarClass = 0; // O-type
            doubleClusterStars[starIndex].mass = 15.0f + random(200) / 10.0f; // 15-35 solar masses
            doubleClusterStars[starIndex].temperature = 30000.0f + random(15000);
            doubleClusterStars[starIndex].color = O_TYPE_COLORS[random(NUM_O_TYPE_COLORS)];
            doubleClusterStars[starIndex].size = 3 + random(2);
            doubleClusterStars[starIndex].isBrightStar = true;
            doubleClusterStars[starIndex].isSupergiant = false; // Too young to evolve
        } else if (stellarTypeRand < 35) { // 27% B-type stars (main component)
            doubleClusterStars[starIndex].stellarClass = 1; // B-type
            doubleClusterStars[starIndex].mass = 3.0f + random(120) / 10.0f; // 3-15 solar masses
            doubleClusterStars[starIndex].temperature = 10000.0f + random(20000);
            doubleClusterStars[starIndex].color = B_TYPE_COLORS[random(NUM_B_TYPE_COLORS)];
            doubleClusterStars[starIndex].size = 2 + random(2);
            doubleClusterStars[starIndex].isBrightStar = i < 12;
            doubleClusterStars[starIndex].isSupergiant = false;
        } else if (stellarTypeRand < 60) { // 25% A-type stars
            doubleClusterStars[starIndex].stellarClass = 2; // A-type
            doubleClusterStars[starIndex].mass = 1.5f + random(15) / 10.0f; // 1.5-3.0 solar masses
            doubleClusterStars[starIndex].temperature = 7500.0f + random(2500);
            doubleClusterStars[starIndex].color = A_TYPE_COLORS[random(NUM_A_TYPE_COLORS)];
            doubleClusterStars[starIndex].size = 1 + random(2);
            doubleClusterStars[starIndex].isBrightStar = false;
            doubleClusterStars[starIndex].isSupergiant = false;
        } else { // 40% F and later types
            doubleClusterStars[starIndex].stellarClass = 3 + random(2); // F or G type
            doubleClusterStars[starIndex].mass = 0.8f + random(10) / 10.0f; // 0.8-1.8 solar masses
            doubleClusterStars[starIndex].temperature = 5000.0f + random(3000);
            doubleClusterStars[starIndex].color = 0xFFE0; // Yellow-white
            doubleClusterStars[starIndex].size = 1;
            doubleClusterStars[starIndex].isBrightStar = false;
            doubleClusterStars[starIndex].isSupergiant = false;
        }
        
        doubleClusterStars[starIndex].isRedSupergiant = false; // NGC 869 too young for red supergiants
        doubleClusterStars[starIndex].brightness = 0.4f + doubleClusterStars[starIndex].mass / 35.0f * 0.5f;
        doubleClusterStars[starIndex].variability = random(100) < 10 ? 0.1f + random(15) / 100.0f : 0.0f;
        doubleClusterStars[starIndex].twinklePhase = random(360) * PI / 180.0f;
        doubleClusterStars[starIndex].twinkleSpeed = 0.015f + random(10) / 1000.0f;
    }
    
    // Initialize NGC 884 (h Persei) - older, more evolved cluster
    for (int i = 0; i < MAX_NGC884_STARS && starIndex < MAX_CLUSTER_STARS; i++, starIndex++) {
        doubleClusterStars[starIndex].active = true;
        doubleClusterStars[starIndex].clusterId = 1; // NGC 884
        doubleClusterStars[starIndex].age = NGC884_AGE;
        doubleClusterStars[starIndex].prevX = -1;
        doubleClusterStars[starIndex].prevY = -1;
        
        // Create cluster distribution (more spread out than NGC 869)
        float angle = random(360) * PI / 180.0f;
        float radius = sqrt(random(1000) / 1000.0f) * NGC884_RADIUS;
        
        // Less core concentration (more evolved)
        if (random(100) < 25) { // 25% in core
            radius *= 0.7f;
        }
        
        float asymmetryX = (random(1000) / 1000.0f - 0.5f) * 3.0f;
        float asymmetryY = (random(1000) / 1000.0f - 0.5f) * 3.0f;
        
        doubleClusterStars[starIndex].originalX = NGC884_CENTER_X + cos(angle) * radius + asymmetryX;
        doubleClusterStars[starIndex].originalY = NGC884_CENTER_Y + sin(angle) * radius + asymmetryY;
        
        // Stellar population for older cluster (some evolved stars)
        int stellarTypeRand = random(100);
        if (stellarTypeRand < 3 && i < 3) { // Few remaining O-type stars
            doubleClusterStars[starIndex].stellarClass = 0; // O-type
            doubleClusterStars[starIndex].mass = 18.0f + random(120) / 10.0f;
            doubleClusterStars[starIndex].temperature = 32000.0f + random(12000);
            doubleClusterStars[starIndex].color = O_TYPE_COLORS[random(NUM_O_TYPE_COLORS)];
            doubleClusterStars[starIndex].size = 3 + random(2);
            doubleClusterStars[starIndex].isBrightStar = true;
            doubleClusterStars[starIndex].isSupergiant = random(100) < 30; // Some evolved
        } else if (stellarTypeRand < 25) { // 22% B-type stars
            doubleClusterStars[starIndex].stellarClass = 1; // B-type
            doubleClusterStars[starIndex].mass = 4.0f + random(100) / 10.0f;
            doubleClusterStars[starIndex].temperature = 12000.0f + random(18000);
            doubleClusterStars[starIndex].color = B_TYPE_COLORS[random(NUM_B_TYPE_COLORS)];
            doubleClusterStars[starIndex].size = 2 + random(2);
            doubleClusterStars[starIndex].isBrightStar = i < 8;
            doubleClusterStars[starIndex].isSupergiant = random(100) < 20; // Some supergiants
        } else if (stellarTypeRand < 40) { // A-type stars
            doubleClusterStars[starIndex].stellarClass = 2; // A-type
            doubleClusterStars[starIndex].mass = 1.8f + random(12) / 10.0f;
            doubleClusterStars[starIndex].temperature = 8000.0f + random(2000);
            doubleClusterStars[starIndex].color = A_TYPE_COLORS[random(NUM_A_TYPE_COLORS)];
            doubleClusterStars[starIndex].size = 1 + random(2);
            doubleClusterStars[starIndex].isBrightStar = false;
            doubleClusterStars[starIndex].isSupergiant = false;
        } else if (stellarTypeRand < 90) { // F, G types
            doubleClusterStars[starIndex].stellarClass = 3 + random(2);
            doubleClusterStars[starIndex].mass = 0.9f + random(8) / 10.0f;
            doubleClusterStars[starIndex].temperature = 5200.0f + random(2800);
            doubleClusterStars[starIndex].color = 0xFFE0;
            doubleClusterStars[starIndex].size = 1;
            doubleClusterStars[starIndex].isBrightStar = false;
            doubleClusterStars[starIndex].isSupergiant = false;
        } else { // 10% Red supergiants (evolved massive stars)
            doubleClusterStars[starIndex].stellarClass = 6; // M-type supergiant
            doubleClusterStars[starIndex].mass = 12.0f + random(80) / 10.0f; // Originally massive
            doubleClusterStars[starIndex].temperature = 3500.0f + random(1500);
            doubleClusterStars[starIndex].color = SUPERGIANT_COLORS[random(NUM_SUPERGIANT_COLORS)];
            doubleClusterStars[starIndex].size = 4 + random(2); // Large evolved stars
            doubleClusterStars[starIndex].isBrightStar = true;
            doubleClusterStars[starIndex].isSupergiant = true;
            doubleClusterStars[starIndex].isRedSupergiant = true;
        }
        
        doubleClusterStars[starIndex].brightness = 0.4f + doubleClusterStars[starIndex].mass / 35.0f * 0.5f;
        if (doubleClusterStars[starIndex].isRedSupergiant) {
            doubleClusterStars[starIndex].brightness *= 1.5f; // Red supergiants are very luminous
        }
        doubleClusterStars[starIndex].variability = doubleClusterStars[starIndex].isRedSupergiant ? 
                                                    0.2f + random(20) / 100.0f : // Red supergiants are variable
                                                    (random(100) < 5 ? 0.1f + random(10) / 100.0f : 0.0f);
        doubleClusterStars[starIndex].twinklePhase = random(360) * PI / 180.0f;
        doubleClusterStars[starIndex].twinkleSpeed = 0.015f + random(10) / 1000.0f;
    }
    
    // Initialize background field stars
    for (int i = 0; i < MAX_BACKGROUND_STARS && starIndex < MAX_CLUSTER_STARS + MAX_BACKGROUND_STARS; i++, starIndex++) {
        doubleClusterStars[starIndex].active = true;
        doubleClusterStars[starIndex].clusterId = -1; // Background
        doubleClusterStars[starIndex].age = 1000.0f + random(9000); // Various ages
        doubleClusterStars[starIndex].isBrightStar = false;
        doubleClusterStars[starIndex].isSupergiant = false;
        doubleClusterStars[starIndex].isRedSupergiant = false;
        doubleClusterStars[starIndex].prevX = -1;
        doubleClusterStars[starIndex].prevY = -1;
        
        // Scatter around the field
        float angle = random(360) * PI / 180.0f;
        float radius;
        
        if (random(100) < 20) {
            radius = random(100) / 10.0f; // Some close stars
        } else {
            radius = 30.0f + random(300) / 10.0f; // Most are distant
        }
        
        doubleClusterStars[starIndex].originalX = cos(angle) * radius;
        doubleClusterStars[starIndex].originalY = sin(angle) * radius;
        
        // Random field star properties
        doubleClusterStars[starIndex].stellarClass = random(7);
        doubleClusterStars[starIndex].mass = 0.5f + random(30) / 10.0f;
        doubleClusterStars[starIndex].temperature = 3000.0f + random(7000);
        doubleClusterStars[starIndex].size = 1 + random(2);
        doubleClusterStars[starIndex].brightness = 0.3f + random(40) / 100.0f;
        doubleClusterStars[starIndex].variability = random(100) < 3 ? 0.1f + random(10) / 100.0f : 0.0f;
        doubleClusterStars[starIndex].twinklePhase = random(360) * PI / 180.0f;
        doubleClusterStars[starIndex].twinkleSpeed = 0.01f + random(10) / 1000.0f;
        
        // Color based on temperature
        if (doubleClusterStars[starIndex].temperature > 10000) {
            doubleClusterStars[starIndex].color = 0xFFFF; // White/blue
        } else if (doubleClusterStars[starIndex].temperature > 7000) {
            doubleClusterStars[starIndex].color = 0xFFE0; // Yellow-white
        } else if (doubleClusterStars[starIndex].temperature > 5000) {
            doubleClusterStars[starIndex].color = 0xFFE0; // Yellow
        } else {
            doubleClusterStars[starIndex].color = 0xFD20; // Orange-red
        }
    }
    
    doubleClusterInitialized = true;
    doubleClusterLastUpdateTime = millis();
}

void drawDoubleCluster() {
    if (!doubleClusterInitialized) {
        initializeDoubleCluster();
    }
    
    unsigned long currentTime = millis();
    float deltaTime = (currentTime - doubleClusterLastUpdateTime) / 1000.0f;
    if (deltaTime > 0.1f) deltaTime = 0.1f;
    doubleClusterLastUpdateTime = currentTime;
    
    int centerX = objectX;
    int centerY = objectY;
    float scale = objectScale;
    
    // Draw all stars
    for (int i = 0; i < MAX_CLUSTER_STARS + MAX_BACKGROUND_STARS; i++) {
        if (!doubleClusterStars[i].active) continue;
        
        doubleClusterStars[i].x = centerX + doubleClusterStars[i].originalX * scale;
        doubleClusterStars[i].y = centerY + doubleClusterStars[i].originalY * scale;
        
        int drawX = (int)doubleClusterStars[i].x;
        int drawY = (int)doubleClusterStars[i].y;
        
        // Erase previous position
        if (doubleClusterStars[i].prevX != drawX || doubleClusterStars[i].prevY != drawY) {
            if (doubleClusterStars[i].prevX >= 0 && doubleClusterStars[i].prevX < SCREEN_WIDTH &&
                doubleClusterStars[i].prevY >= 0 && doubleClusterStars[i].prevY < SCREEN_HEIGHT) {
                int eraseSize = doubleClusterStars[i].size + 1;
                for (int ex = -eraseSize; ex <= eraseSize; ex++) {
                    for (int ey = -eraseSize; ey <= eraseSize; ey++) {
                        int px = doubleClusterStars[i].prevX + ex;
                        int py = doubleClusterStars[i].prevY + ey;
                        if (px >= 0 && px < SCREEN_WIDTH && py >= 0 && py < SCREEN_HEIGHT) {
                            tft.drawPixel(px, py, BG_COLOR);
                        }
                    }
                }
            }
        }
        
        // Update twinkling
        doubleClusterStars[i].twinklePhase += doubleClusterStars[i].twinkleSpeed * deltaTime;
        if (doubleClusterStars[i].twinklePhase > 2 * PI) {
            doubleClusterStars[i].twinklePhase -= 2 * PI;
        }
        
        // Calculate brightness
        float twinkleFactor = 0.7f + 0.3f * sin(doubleClusterStars[i].twinklePhase);
        float currentBrightness = doubleClusterStars[i].brightness * twinkleFactor;
        
        // Bright cluster stars are more prominent
        if (doubleClusterStars[i].isBrightStar && doubleClusterStars[i].clusterId >= 0) {
            currentBrightness *= 1.3f;
        }
        
        currentBrightness = constrain(currentBrightness, 0.1f, 1.0f);
        
        // Apply brightness to color
        uint16_t drawColor;
        if (currentBrightness >= 1.0f) {
            drawColor = doubleClusterStars[i].color;
        } else {
            uint8_t r5 = (doubleClusterStars[i].color >> 11) & 0x1F;
            uint8_t g6 = (doubleClusterStars[i].color >> 5) & 0x3F;
            uint8_t b5 = doubleClusterStars[i].color & 0x1F;
            
            uint8_t r_scaled = (uint8_t)(r5 * currentBrightness);
            uint8_t g_scaled = (uint8_t)(g6 * currentBrightness);
            uint8_t b_scaled = (uint8_t)(b5 * currentBrightness);
            
            drawColor = (r_scaled << 11) | (g_scaled << 5) | b_scaled;
        }
        
        // Draw the star
        if (drawX >= 0 && drawX < SCREEN_WIDTH && drawY >= 0 && drawY < SCREEN_HEIGHT) {
            if (doubleClusterStars[i].size == 1) {
                tft.drawPixel(drawX, drawY, drawColor);
            } else {
                int radius = doubleClusterStars[i].size / 2;
                tft.fillCircle(drawX, drawY, radius, drawColor);
                
                // Add subtle cross pattern for bright cluster stars
                if (doubleClusterStars[i].isBrightStar && doubleClusterStars[i].clusterId >= 0 && currentBrightness > 0.8f) {
                    int crossLength = radius + 1;
                    uint16_t crossColor = drawColor; // Same color but could be dimmed
                    tft.drawLine(drawX - crossLength, drawY, drawX + crossLength, drawY, crossColor);
                    tft.drawLine(drawX, drawY - crossLength, drawX, drawY + crossLength, crossColor);
                }
            }
        }
        
        doubleClusterStars[i].prevX = drawX;
        doubleClusterStars[i].prevY = drawY;
    }
}

void eraseDoubleCluster() {
    if (!doubleClusterInitialized) return;
    
    // Erase all stars
    for (int i = 0; i < MAX_CLUSTER_STARS + MAX_BACKGROUND_STARS; i++) {
        if (doubleClusterStars[i].prevX >= 0 && doubleClusterStars[i].prevX < SCREEN_WIDTH &&
            doubleClusterStars[i].prevY >= 0 && doubleClusterStars[i].prevY < SCREEN_HEIGHT) {
            
            int eraseSize = doubleClusterStars[i].size + 3; // Extra size for cross pattern
            for (int ex = -eraseSize; ex <= eraseSize; ex++) {
                for (int ey = -eraseSize; ey <= eraseSize; ey++) {
                    int px = doubleClusterStars[i].prevX + ex;
                    int py = doubleClusterStars[i].prevY + ey;
                    if (px >= 0 && px < SCREEN_WIDTH && py >= 0 && py < SCREEN_HEIGHT) {
                        tft.drawPixel(px, py, BG_COLOR);
                    }
                }
            }
        }
        doubleClusterStars[i].prevX = -1;
        doubleClusterStars[i].prevY = -1;
    }
    
    doubleClusterInitialized = false;
}

#endif // DOUBLE_CLUSTER_H