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
#define MAX_CLUSTER_STARS 150
#define MAX_NGC869_STARS 80  // Chi Persei (younger, more compact)
#define MAX_NGC884_STARS 70  // h Persei (older, more spread out)
#define MAX_BACKGROUND_STARS 50
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
    for (int i = 0; i < MAX_NGC869_STARS && starIndex < MAX_CLUSTER_STARS + MAX_BACKGROUND_STARS; i++, starIndex++) {
        doubleClusterStars[starIndex].active = true;
        doubleClusterStars[starIndex].clusterId = 0; // NGC 869
        doubleClusterStars[starIndex].age = NGC869_AGE;
        doubleClusterStars[starIndex].prevX = -1;
        doubleClusterStars[starIndex].prevY = -1;
        
        // Create realistic cluster distribution (King profile approximation)
        float angle = random(360) * PI / 180.0f;
        float u = random(1000) / 1000.0f; // Uniform random [0,1]
        float radius = NGC869_RADIUS * pow(u, 0.4); // Adjusted exponent for concentration
        
        // Add some core concentration
        if (random(100) < 50) { // 50% in dense core
            radius *= 0.5f; // Make core denser
        }
        
        // Add asymmetry and structure
        float asymmetryX = (random(1000) / 1000.0f - 0.5f) * 1.5f; // Reduced asymmetry for younger cluster
        float asymmetryY = (random(1000) / 1000.0f - 0.5f) * 1.5f;
        
        doubleClusterStars[starIndex].originalX = NGC869_CENTER_X + cos(angle) * radius + asymmetryX;
        doubleClusterStars[starIndex].originalY = NGC869_CENTER_Y + sin(angle) * radius + asymmetryY;
        
        // Determine stellar type based on cluster age and mass function
        int stellarTypeRand = random(100);
        if (stellarTypeRand < 10 && i < 8) { // Increased O-type stars (only in core)
            doubleClusterStars[starIndex].stellarClass = 0; // O-type
            doubleClusterStars[starIndex].mass = 18.0f + random(220) / 10.0f; // Higher mass range
            doubleClusterStars[starIndex].temperature = 35000.0f + random(10000);
            doubleClusterStars[starIndex].color = O_TYPE_COLORS[random(NUM_O_TYPE_COLORS)];
            doubleClusterStars[starIndex].size = 3 + random(3); // Larger size
            doubleClusterStars[starIndex].isBrightStar = true;
            doubleClusterStars[starIndex].isSupergiant = false; // Too young to evolve
        } else if (stellarTypeRand < 40) { // 30% B-type stars
            doubleClusterStars[starIndex].stellarClass = 1; // B-type
            doubleClusterStars[starIndex].mass = 4.0f + random(140) / 10.0f; // 4-18 solar masses
            doubleClusterStars[starIndex].temperature = 12000.0f + random(18000);
            doubleClusterStars[starIndex].color = B_TYPE_COLORS[random(NUM_B_TYPE_COLORS)];
            doubleClusterStars[starIndex].size = 2 + random(2);
            doubleClusterStars[starIndex].isBrightStar = i < 15; // More bright B stars
            doubleClusterStars[starIndex].isSupergiant = false;
        } else if (stellarTypeRand < 65) { // 25% A-type stars
            doubleClusterStars[starIndex].stellarClass = 2; // A-type
            doubleClusterStars[starIndex].mass = 2.0f + random(10) / 10.0f; // 2.0-3.0 solar masses
            doubleClusterStars[starIndex].temperature = 8000.0f + random(2000);
            doubleClusterStars[starIndex].color = A_TYPE_COLORS[random(NUM_A_TYPE_COLORS)];
            doubleClusterStars[starIndex].size = 1 + random(2);
            doubleClusterStars[starIndex].isBrightStar = false;
            doubleClusterStars[starIndex].isSupergiant = false;
        } else { // 35% F and later types
            doubleClusterStars[starIndex].stellarClass = 3 + random(3); // F, G, or K type
            doubleClusterStars[starIndex].mass = 0.7f + random(11) / 10.0f; // 0.7-1.8 solar masses
            doubleClusterStars[starIndex].temperature = 4500.0f + random(3500);
            doubleClusterStars[starIndex].color = 0xFFE0; // Yellow-white, some yellow/orange
            if (doubleClusterStars[starIndex].stellarClass == 5) doubleClusterStars[starIndex].color = 0xFD20; // Orange
            doubleClusterStars[starIndex].size = 1;
            doubleClusterStars[starIndex].isBrightStar = false;
            doubleClusterStars[starIndex].isSupergiant = false;
        }
        
        doubleClusterStars[starIndex].isRedSupergiant = false; // NGC 869 too young for red supergiants
        doubleClusterStars[starIndex].brightness = 0.3f + doubleClusterStars[starIndex].mass / 40.0f * 0.6f; // Adjusted brightness scale
        if (doubleClusterStars[starIndex].isBrightStar) doubleClusterStars[starIndex].brightness *= 1.2f; // Make designated bright stars brighter
        doubleClusterStars[starIndex].variability = random(100) < 8 ? 0.1f + random(10) / 100.0f : 0.0f; // Slight variability
        doubleClusterStars[starIndex].twinklePhase = random(360) * PI / 180.0f;
        doubleClusterStars[starIndex].twinkleSpeed = 0.02f + random(15) / 1000.0f; // Faster twinkling
    }
    
    // Initialize NGC 884 (h Persei) - older, more evolved cluster
    for (int i = 0; i < MAX_NGC884_STARS && starIndex < MAX_CLUSTER_STARS + MAX_BACKGROUND_STARS; i++, starIndex++) {
        doubleClusterStars[starIndex].active = true;
        doubleClusterStars[starIndex].clusterId = 1; // NGC 884
        doubleClusterStars[starIndex].age = NGC884_AGE;
        doubleClusterStars[starIndex].prevX = -1;
        doubleClusterStars[starIndex].prevY = -1;
        
        // Create cluster distribution (more spread out than NGC 869)
        float angle = random(360) * PI / 180.0f;
        float u = random(1000) / 1000.0f; // Uniform random [0,1]
        float radius = NGC884_RADIUS * pow(u, 0.6); // Less concentrated exponent
        
        // Less core concentration (more evolved)
        if (random(100) < 35) { // 35% in core
             radius *= 0.6f; // Less dense core than NGC 869
        }
        
        float asymmetryX = (random(1000) / 1000.0f - 0.5f) * 2.5f; // More asymmetry for older cluster
        float asymmetryY = (random(1000) / 1000.0f - 0.5f) * 2.5f;
        
        doubleClusterStars[starIndex].originalX = NGC884_CENTER_X + cos(angle) * radius + asymmetryX;
        doubleClusterStars[starIndex].originalY = NGC884_CENTER_Y + sin(angle) * radius + asymmetryY;
        
        // Stellar population for older cluster (some evolved stars)
        int stellarTypeRand = random(100);
        if (stellarTypeRand < 5 && i < 4) { // Fewer remaining O-type stars
            doubleClusterStars[starIndex].stellarClass = 0; // O-type
            doubleClusterStars[starIndex].mass = 20.0f + random(150) / 10.0f;
            doubleClusterStars[starIndex].temperature = 38000.0f + random(7000);
            doubleClusterStars[starIndex].color = O_TYPE_COLORS[random(NUM_O_TYPE_COLORS)];
            doubleClusterStars[starIndex].size = 3 + random(3);
            doubleClusterStars[starIndex].isBrightStar = true;
            doubleClusterStars[starIndex].isSupergiant = random(100) < 40; // More evolved
        } else if (stellarTypeRand < 30) { // 25% B-type stars
            doubleClusterStars[starIndex].stellarClass = 1; // B-type
            doubleClusterStars[starIndex].mass = 5.0f + random(80) / 10.0f;
            doubleClusterStars[starIndex].temperature = 15000.0f + random(15000);
            doubleClusterStars[starIndex].color = B_TYPE_COLORS[random(NUM_B_TYPE_COLORS)];
            doubleClusterStars[starIndex].size = 2 + random(2);
            doubleClusterStars[starIndex].isBrightStar = i < 10;
            doubleClusterStars[starIndex].isSupergiant = random(100) < 30; // More supergiants
        } else if (stellarTypeRand < 50) { // A-type stars
            doubleClusterStars[starIndex].stellarClass = 2; // A-type
            doubleClusterStars[starIndex].mass = 2.5f + random(7) / 10.0f;
            doubleClusterStars[starIndex].temperature = 8500.0f + random(1500);
            doubleClusterStars[starIndex].color = A_TYPE_COLORS[random(NUM_A_TYPE_COLORS)];
            doubleClusterStars[starIndex].size = 1 + random(2);
            doubleClusterStars[starIndex].isBrightStar = false;
            doubleClusterStars[starIndex].isSupergiant = false;
        } else if (stellarTypeRand < 85) { // F, G types
            doubleClusterStars[starIndex].stellarClass = 3 + random(3); // F, G, or K type
            doubleClusterStars[starIndex].mass = 0.8f + random(10) / 10.0f;
            doubleClusterStars[starIndex].temperature = 5000.0f + random(3000);
            doubleClusterStars[starIndex].color = 0xFFE0; // Yellow-white, some yellow/orange
            if (doubleClusterStars[starIndex].stellarClass == 5) doubleClusterStars[starIndex].color = 0xFD20; // Orange
            doubleClusterStars[starIndex].size = 1;
            doubleClusterStars[starIndex].isBrightStar = false;
            doubleClusterStars[starIndex].isSupergiant = false;
        } else { // 15% Red supergiants (increased chance)
            doubleClusterStars[starIndex].stellarClass = 6; // M-type supergiant
            doubleClusterStars[starIndex].mass = 15.0f + random(100) / 10.0f; // Originally more massive
            doubleClusterStars[starIndex].temperature = 3000.0f + random(1000);
            doubleClusterStars[starIndex].color = SUPERGIANT_COLORS[random(NUM_SUPERGIANT_COLORS)];
            doubleClusterStars[starIndex].size = 4 + random(3); // Larger evolved stars
            doubleClusterStars[starIndex].isBrightStar = true;
            doubleClusterStars[starIndex].isSupergiant = true;
            doubleClusterStars[starIndex].isRedSupergiant = true;
        }
        
        doubleClusterStars[starIndex].brightness = 0.3f + doubleClusterStars[starIndex].mass / 45.0f * 0.7f; // Adjusted brightness scale
        if (doubleClusterStars[starIndex].isBrightStar) doubleClusterStars[starIndex].brightness *= 1.3f; // Make designated bright stars brighter
        if (doubleClusterStars[starIndex].isRedSupergiant) {
            doubleClusterStars[starIndex].brightness *= 1.6f; // Red supergiants are very luminous
        }
        doubleClusterStars[starIndex].variability = doubleClusterStars[starIndex].isRedSupergiant ? 
                                                    0.25f + random(25) / 100.0f : // More variability for red supergiants
                                                    (random(100) < 12 ? 0.12f + random(12) / 100.0f : 0.0f); // Slight chance for others
        doubleClusterStars[starIndex].twinklePhase = random(360) * PI / 180.0f;
        doubleClusterStars[starIndex].twinkleSpeed = 0.025f + random(15) / 1000.0f; // Faster twinkling
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
        
        doubleClusterStars[starIndex].originalX = (random(1000) / 1000.0f - 0.5f) * 60.0f; // Wider distribution
        doubleClusterStars[starIndex].originalY = (random(1000) / 1000.0f - 0.5f) * 60.0f;
        
        // Random field star properties
        doubleClusterStars[starIndex].size = 1;
        doubleClusterStars[starIndex].brightness = 0.15f + random(35) / 100.0f; // Dimmer background stars
        doubleClusterStars[starIndex].temperature = 3000.0f + random(7000);
        doubleClusterStars[starIndex].variability = random(100) < 3 ? 0.1f + random(10) / 100.0f : 0.0f;
        doubleClusterStars[starIndex].twinklePhase = random(360) * PI / 180.0f;
        doubleClusterStars[starIndex].twinkleSpeed = 0.008f + random(8) / 1000.0f; // Slower twinkling
        
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
        
        // Calculate brightness with twinkling and variability
        float twinkleFactor = 0.6f + 0.4f * sin(doubleClusterStars[i].twinklePhase); // Standard twinkle
        float variabilityFactor = 1.0f + (sin(stellarEvolutionPhase * 5.0f + i) * doubleClusterStars[i].variability); // Apply variability
        
        float currentBrightness = doubleClusterStars[i].brightness * twinkleFactor * variabilityFactor;
        
        // Brighter stars and supergiants twinkle/vary more noticeably
        if (doubleClusterStars[i].isBrightStar || doubleClusterStars[i].isSupergiant) {
            twinkleFactor = 0.5f + 0.5f * sin(doubleClusterStars[i].twinklePhase); // Stronger twinkle
            variabilityFactor = 1.0f + (sin(stellarEvolutionPhase * 8.0f + i*2) * doubleClusterStars[i].variability * 1.5f); // More pronounced variability
             currentBrightness = doubleClusterStars[i].brightness * twinkleFactor * variabilityFactor;
        }
        
        currentBrightness = constrain(currentBrightness, 0.05f, 1.0f); // Ensure minimum visibility
        
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
                
                // Add a subtle glow for brighter stars and supergiants
                if ((doubleClusterStars[i].isBrightStar || doubleClusterStars[i].isSupergiant) && currentBrightness > 0.7f) {
                    int glowRadius = radius + 1;
                    uint16_t glowColor = doubleClusterStars[i].color; // Use star's color for glow
                     // Reduce brightness for glow to make it subtle
                    uint8_t r5 = (glowColor >> 11) & 0x1F;
                    uint8_t g6 = (glowColor >> 5) & 0x3F;
                    uint8_t b5 = glowColor & 0x1F;
                    
                    uint8_t r_scaled = (uint8_t)(r5 * 0.4f); // Dimmer glow
                    uint8_t g_scaled = (uint8_t)(g6 * 0.4f);
                    uint8_t b_scaled = (uint8_t)(b5 * 0.4f);
                    
                    uint16_t dimGlowColor = (r_scaled << 11) | (g_scaled << 5) | b_scaled;
                    
                    tft.drawCircle(drawX, drawY, glowRadius, dimGlowColor);
                }
                
                // Add diffraction spikes for very bright O/B type stars and Red Supergiants
                if ((doubleClusterStars[i].stellarClass <= 1 || doubleClusterStars[i].isRedSupergiant) && currentBrightness > 0.85f && doubleClusterStars[i].size > 3) {
                    int spikeLength = radius + 2 + (doubleClusterStars[i].size - 3) * 2; // Longer spikes for larger stars
                    uint16_t spikeColor = doubleClusterStars[i].color; // Use star's color for spikes
                    
                    // Dimmer spikes
                     uint8_t r5 = (spikeColor >> 11) & 0x1F;
                    uint8_t g6 = (spikeColor >> 5) & 0x3F;
                    uint8_t b5 = spikeColor & 0x1F;
                    
                    uint8_t r_scaled = (uint8_t)(r5 * 0.6f); // Dimmer spikes
                    uint8_t g_scaled = (uint8_t)(g6 * 0.6f);
                    uint8_t b_scaled = (uint8_t)(b5 * 0.6f);
                    
                    uint16_t dimSpikeColor = (r_scaled << 11) | (g_scaled << 5) | b_scaled;
                    
                    // Draw spikes
                    tft.drawLine(drawX - spikeLength, drawY, drawX + spikeLength, drawY, dimSpikeColor);
                    tft.drawLine(drawX, drawY - spikeLength, drawX, drawY + spikeLength, dimSpikeColor);
                     // Add diagonal spikes for largest stars
                    if (doubleClusterStars[i].size >= 5) {
                        int diagSpike = spikeLength * 0.7f;
                        tft.drawLine(drawX - diagSpike, drawY - diagSpike, drawX + diagSpike, drawY + diagSpike, dimSpikeColor);
                        tft.drawLine(drawX - diagSpike, drawY + diagSpike, drawX + diagSpike, drawY - diagSpike, dimSpikeColor);
                    }
                }
            }
        }
        
        doubleClusterStars[i].prevX = drawX;
        doubleClusterStars[i].prevY = drawY;
    }
}

void eraseDoubleCluster() {
    if (!doubleClusterInitialized) return;
    
    // Full screen clear for faster erase
    tft.fillScreen(BG_COLOR);
    
    // No allocated memory to free for this object
    doubleClusterInitialized = false;
}

#endif // DOUBLE_CLUSTER_H