#ifndef PLEIADES_H
#define PLEIADES_H

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

// Pleiades parameters - Enhanced for realism
#define MAX_PLEIADES_STARS 55
#define MAX_SEVEN_SISTERS 9 // Include the two dimmer visible sisters
#define MAX_BACKGROUND_STARS 46
#define MAX_NEBULA_WISPS 40 // Enhanced reflection nebula
#define MAX_HOT_STAR_FEATURES 12 // Stellar winds and features

struct PleiadesNebulaWisp {
    float x, y;
    float originalX, originalY;
    uint16_t color;
    float brightness;
    float driftPhase;
    float driftSpeed;
    float illuminationFactor; // How much stellar light affects this wisp
    float density; // Dust density affects scattering
    int size;
    int prevX, prevY;
    bool active;
    int sourceStarIndex; // Which sister star illuminates this wisp
};

struct PleiadesStar {
    float x, y;
    float originalX, originalY;
    uint16_t color;
    float brightness;
    float twinklePhase;
    float twinkleSpeed;
    float temperature; // Stellar temperature affects color and wind
    float stellarWind; // Hot stars have strong winds
    float luminosity; // Affects nebula illumination
    int size;
    int prevX, prevY;
    bool active;
    bool isSister; // True for the main bright stars
    int sisterIndex; // Which of the sisters (0-8, or -1 for background)
    bool isHotStar; // B-type hot blue stars
};

struct HotStarFeature {
    float x, y;
    float originalX, originalY;
    float featureAngle;
    float pulsationPhase;
    float pulsationSpeed;
    uint16_t featureColor;
    float brightness;
    bool active;
    int parentStarIndex;
    int featureType; // 0=stellar wind, 1=rotation feature, 2=pulsation
};

// Global variables for Pleiades animation
PleiadesStar pleiadesStars[MAX_PLEIADES_STARS];
PleiadesNebulaWisp pleiadesNebula[MAX_NEBULA_WISPS];
HotStarFeature hotFeatures[MAX_HOT_STAR_FEATURES];
bool pleiadesInitialized = false;
unsigned long pleiadesLastUpdateTime = 0;
float globalDrift = 0.0f;
extern float stellarWindPhase; // Declare as extern

// Enhanced Seven Sisters data with authentic astronomical details
struct SisterData {
    const char* name;
    float x, y;
    uint16_t color;
    int size;
    float temperature; // Kelvin
    float luminosity; // Solar luminosities
    float mass; // Solar masses
};

const SisterData ENHANCED_SISTERS[MAX_SEVEN_SISTERS] = {
    // Main Seven Sisters (brightest)
    {"Alcyone", 0.0f, 0.0f, 0x1E7F, 5, 12500.0f, 2400.0f, 6.1f},     // Brightest B-type giant
    {"Atlas", -8.5f, 6.2f, 0x07FF, 4, 11000.0f, 1800.0f, 5.8f},      // B-type star
    {"Electra", 5.2f, -3.1f, 0x1E7F, 4, 13200.0f, 1200.0f, 5.2f},    // Hot B-type
    {"Maia", -3.2f, -8.1f, 0x07FF, 4, 11500.0f, 660.0f, 4.5f},       // B-type
    {"Merope", 7.1f, 4.3f, 0x1E7F, 4, 14100.0f, 630.0f, 4.2f},       // Very hot B-type
    {"Taygeta", -6.1f, -2.2f, 0x07FF, 3, 11800.0f, 300.0f, 4.3f},    // B-type
    {"Pleione", -9.2f, 7.8f, 0x3F7F, 3, 12000.0f, 190.0f, 3.2f},     // Be star (emission)
    // Two additional visible sisters
    {"Celaeno", 2.8f, 8.1f, 0x07FF, 3, 11200.0f, 60.0f, 4.0f},       // Dimmer sister
    {"Sterope", -4.5f, 5.9f, 0x1E7F, 2, 11600.0f, 40.0f, 3.8f}       // Dimmest visible
};

// Enhanced color palette for reflection nebula (blue scattering of starlight)
const uint16_t REFLECTION_COLORS[] = {
    0x1E7F,  // Deep blue (primary scattering)
    0x07FF,  // Cyan-blue (scattered starlight)
    0x3F7F,  // Blue-white (direct reflection)
    0x039F,  // Dark cyan (distant scattering)
    0x05FF,  // Aqua blue (gas component)
    0x1F7F   // Medium blue
};
const int NUM_REFLECTION_COLORS = sizeof(REFLECTION_COLORS) / sizeof(REFLECTION_COLORS[0]);

// Hot star colors (B-type main sequence)
const uint16_t HOT_STAR_COLORS[] = {
    0x07FF,  // Cyan-blue (B0-B3)
    0x1E7F,  // Deep blue (B5-B7)
    0x3F7F,  // Blue-white (B8-B9)
    0xFFFF   // White-blue (hottest)
};
const int NUM_HOT_STAR_COLORS = sizeof(HOT_STAR_COLORS) / sizeof(HOT_STAR_COLORS[0]);

void initializePleiades() {
    if (pleiadesInitialized) return;
    
    // Initialize the Enhanced Seven Sisters plus two dimmer ones
    for (int i = 0; i < MAX_SEVEN_SISTERS; i++) {
        pleiadesStars[i].active = true;
        pleiadesStars[i].isSister = true;
        pleiadesStars[i].sisterIndex = i;
        pleiadesStars[i].isHotStar = true; // All sisters are hot B-type stars
        
        pleiadesStars[i].originalX = ENHANCED_SISTERS[i].x;
        pleiadesStars[i].originalY = ENHANCED_SISTERS[i].y;
        pleiadesStars[i].color = ENHANCED_SISTERS[i].color;
        pleiadesStars[i].size = ENHANCED_SISTERS[i].size;
        pleiadesStars[i].temperature = ENHANCED_SISTERS[i].temperature;
        pleiadesStars[i].luminosity = ENHANCED_SISTERS[i].luminosity;
        pleiadesStars[i].stellarWind = ENHANCED_SISTERS[i].mass * 0.3f; // Hot stars have strong winds
        
        // Brightness based on luminosity
        pleiadesStars[i].brightness = 0.7f + (ENHANCED_SISTERS[i].luminosity / 2400.0f) * 0.3f;
        pleiadesStars[i].brightness = constrain(pleiadesStars[i].brightness, 0.7f, 1.0f);
        
        pleiadesStars[i].twinklePhase = random(360) * PI / 180.0f;
        pleiadesStars[i].twinkleSpeed = 0.018f + random(8) / 1000.0f;
        pleiadesStars[i].prevX = -1;
        pleiadesStars[i].prevY = -1;
    }
    
    // Initialize background cluster stars (mostly A and F type, some B)
    for (int i = MAX_SEVEN_SISTERS; i < MAX_PLEIADES_STARS; i++) {
        pleiadesStars[i].active = true;
        pleiadesStars[i].isSister = false;
        pleiadesStars[i].sisterIndex = -1;
        
        // Scatter around the cluster core with realistic distribution
        float angle = random(360) * PI / 180.0f;
        float radius;
        
        // Create core concentration with extended halo
        if (random(100) < 60) {
            // Core stars (concentrated)
            radius = sqrt(random(1000) / 1000.0f) * 15.0f;
        } else {
            // Halo stars (more scattered)
            radius = 15.0f + sqrt(random(1000) / 1000.0f) * 25.0f;
        }
        
        pleiadesStars[i].originalX = cos(angle) * radius;
        pleiadesStars[i].originalY = sin(angle) * radius;
        
        // Determine star type
        int starType = random(100);
        if (starType < 15) { // 15% hot B-type stars
            pleiadesStars[i].isHotStar = true;
            pleiadesStars[i].color = HOT_STAR_COLORS[random(NUM_HOT_STAR_COLORS)];
            pleiadesStars[i].temperature = 8000.0f + random(6000); // B-type range
            pleiadesStars[i].stellarWind = 0.5f + random(15) / 10.0f;
            pleiadesStars[i].size = 2 + random(2);
            pleiadesStars[i].brightness = 0.6f + random(30) / 100.0f;
        } else if (starType < 50) { // 35% A-type stars (white)
            pleiadesStars[i].isHotStar = false;
            pleiadesStars[i].color = 0xFFFF; // White
            pleiadesStars[i].temperature = 7000.0f + random(2000); // A-type range
            pleiadesStars[i].stellarWind = 0.2f + random(8) / 10.0f;
            pleiadesStars[i].size = 1 + random(2);
            pleiadesStars[i].brightness = 0.4f + random(40) / 100.0f;
        } else { // 50% F-type and cooler (yellow-white)
            pleiadesStars[i].isHotStar = false;
            pleiadesStars[i].color = 0xFFE0; // Yellow-white
            pleiadesStars[i].temperature = 5000.0f + random(2500); // F-type range
            pleiadesStars[i].stellarWind = 0.1f + random(5) / 10.0f;
            pleiadesStars[i].size = 1 + random(2);
            pleiadesStars[i].brightness = 0.3f + random(40) / 100.0f;
        }
        
        pleiadesStars[i].luminosity = pow(pleiadesStars[i].temperature / 5778.0f, 4) * 
                                     pow(pleiadesStars[i].brightness, 2); // Stefan-Boltzmann approx
        
        pleiadesStars[i].twinklePhase = random(360) * PI / 180.0f;
        pleiadesStars[i].twinkleSpeed = 0.012f + random(8) / 1000.0f;
        pleiadesStars[i].prevX = -1;
        pleiadesStars[i].prevY = -1;
    }
    
    // Initialize enhanced reflection nebula wisps
    for (int i = 0; i < MAX_NEBULA_WISPS; i++) {
        pleiadesNebula[i].active = true;
        pleiadesNebula[i].prevX = -1;
        pleiadesNebula[i].prevY = -1;
        
        // Create realistic dust distribution around bright stars
        int sourceSisterIndex = random(MAX_SEVEN_SISTERS);
        
        // Wisps form streams and filaments around the brightest stars
        float baseAngle = random(360) * PI / 180.0f;
        float streamOffset = (random(1000) / 1000.0f - 0.5f) * 20.0f;
        float crossOffset = (random(1000) / 1000.0f - 0.5f) * 8.0f;
        
        pleiadesNebula[i].originalX = ENHANCED_SISTERS[sourceSisterIndex].x + 
                                     cos(baseAngle) * streamOffset + 
                                     cos(baseAngle + PI/2) * crossOffset;
        pleiadesNebula[i].originalY = ENHANCED_SISTERS[sourceSisterIndex].y + 
                                     sin(baseAngle) * streamOffset + 
                                     sin(baseAngle + PI/2) * crossOffset;
        
        pleiadesNebula[i].sourceStarIndex = sourceSisterIndex;
        pleiadesNebula[i].density = 0.3f + random(70) / 100.0f;
        pleiadesNebula[i].illuminationFactor = ENHANCED_SISTERS[sourceSisterIndex].luminosity / 2400.0f;
        
        pleiadesNebula[i].size = 1 + random(3);
        pleiadesNebula[i].brightness = 0.15f + pleiadesNebula[i].density * 0.4f;
        pleiadesNebula[i].driftPhase = random(360) * PI / 180.0f;
        pleiadesNebula[i].driftSpeed = 0.002f + random(4) / 2000.0f;
        
        // Reflection nebula shows blue scattered light
        pleiadesNebula[i].color = REFLECTION_COLORS[random(NUM_REFLECTION_COLORS)];
        
        // Brighten wisps near more luminous stars
        if (pleiadesNebula[i].illuminationFactor > 0.5f) {
            pleiadesNebula[i].brightness *= 1.3f;
        }
    }
    
    // Initialize hot star features (stellar winds, rotation effects)
    for (int i = 0; i < MAX_HOT_STAR_FEATURES; i++) {
        hotFeatures[i].active = random(100) < 70; // 70% chance
        if (!hotFeatures[i].active) continue;
        
        // Attach to a hot star (Sisters or hot background stars)
        int parentIndex;
        if (random(100) < 60) {
            // Prefer sisters for features
            parentIndex = random(MAX_SEVEN_SISTERS);
        } else {
            // Find a hot background star
            do {
                parentIndex = MAX_SEVEN_SISTERS + random(MAX_BACKGROUND_STARS);
            } while (!pleiadesStars[parentIndex].isHotStar && random(100) < 80);
        }
        
        hotFeatures[i].parentStarIndex = parentIndex;
        hotFeatures[i].originalX = pleiadesStars[parentIndex].originalX;
        hotFeatures[i].originalY = pleiadesStars[parentIndex].originalY;
        
        hotFeatures[i].featureType = random(3); // 0=wind, 1=rotation, 2=pulsation
        hotFeatures[i].featureAngle = random(360) * PI / 180.0f;
        hotFeatures[i].pulsationPhase = random(360) * PI / 180.0f;
        hotFeatures[i].pulsationSpeed = 0.02f + random(15) / 1000.0f;
        
        if (hotFeatures[i].featureType == 0) { // Stellar wind
            hotFeatures[i].featureColor = 0x3F7F; // Blue-white wind
            hotFeatures[i].brightness = 0.3f + random(30) / 100.0f;
        } else if (hotFeatures[i].featureType == 1) { // Rotation feature
            hotFeatures[i].featureColor = 0x1E7F; // Deep blue rotation
            hotFeatures[i].brightness = 0.4f + random(20) / 100.0f;
        } else { // Pulsation
            hotFeatures[i].featureColor = 0x07FF; // Cyan pulsation
            hotFeatures[i].brightness = 0.5f + random(25) / 100.0f;
        }
    }
    
    pleiadesInitialized = true;
    pleiadesLastUpdateTime = millis();
}

void drawPleiades() {
    if (!pleiadesInitialized) {
        initializePleiades();
    }
    
    unsigned long currentTime = millis();
    float deltaTime = (currentTime - pleiadesLastUpdateTime) / 1000.0f;
    if (deltaTime > 0.1f) deltaTime = 0.1f;
    pleiadesLastUpdateTime = currentTime;
    
    int centerX = objectX;
    int centerY = objectY;
    float scale = objectScale;
    
    // Update global phases
    globalDrift += 0.001f * deltaTime; // Very slow drift
    if (globalDrift > 2 * PI) globalDrift -= 2 * PI;
    
    stellarWindPhase += 0.012f * deltaTime; // Stellar wind variations
    if (stellarWindPhase > 2 * PI) stellarWindPhase -= 2 * PI;
    
    // Draw enhanced reflection nebula first (background layer)
    for (int i = 0; i < MAX_NEBULA_WISPS; i++) {
        if (!pleiadesNebula[i].active) continue;
        
        // Enhanced nebula motion with stellar illumination effects
        pleiadesNebula[i].driftPhase += pleiadesNebula[i].driftSpeed * deltaTime;
        
        // Calculate stellar wind pressure from source star
        float windPressure = 0.0f;
        if (pleiadesNebula[i].sourceStarIndex < MAX_SEVEN_SISTERS) {
            float stellarWind = pleiadesStars[pleiadesNebula[i].sourceStarIndex].stellarWind;
            windPressure = stellarWind * sin(stellarWindPhase + pleiadesNebula[i].sourceStarIndex) * 0.3f;
        }
        
        // Nebula drifts with stellar winds and galactic motion
        float driftX = sin(pleiadesNebula[i].driftPhase + globalDrift) * 0.6f + windPressure;
        float driftY = cos(pleiadesNebula[i].driftPhase * 1.2f + globalDrift) * 0.4f;
        
        pleiadesNebula[i].x = centerX + (pleiadesNebula[i].originalX + driftX) * scale;
        pleiadesNebula[i].y = centerY + (pleiadesNebula[i].originalY + driftY) * scale;
        
        int drawX = (int)pleiadesNebula[i].x;
        int drawY = (int)pleiadesNebula[i].y;
        
        // Erase previous position
        if (pleiadesNebula[i].prevX != drawX || pleiadesNebula[i].prevY != drawY) {
            if (pleiadesNebula[i].prevX >= 0 && pleiadesNebula[i].prevX < SCREEN_WIDTH &&
                pleiadesNebula[i].prevY >= 0 && pleiadesNebula[i].prevY < SCREEN_HEIGHT) {
                int eraseSize = pleiadesNebula[i].size + 1;
                for (int ex = -eraseSize; ex <= eraseSize; ex++) {
                    for (int ey = -eraseSize; ey <= eraseSize; ey++) {
                        int px = pleiadesNebula[i].prevX + ex;
                        int py = pleiadesNebula[i].prevY + ey;
                        if (px >= 0 && px < SCREEN_WIDTH && py >= 0 && py < SCREEN_HEIGHT) {
                            tft.drawPixel(px, py, BG_COLOR);
                        }
                    }
                }
            }
        }
        
        // Enhanced brightness calculation with stellar illumination
        float baseBrightness = pleiadesNebula[i].brightness;
        float variation = 0.8f + 0.2f * sin(pleiadesNebula[i].driftPhase);
        
        // Calculate illumination from nearby hot stars
        float totalIllumination = 0.0f;
        for (int j = 0; j < MAX_SEVEN_SISTERS; j++) {
            if (!pleiadesStars[j].active) continue;
            float dx = pleiadesNebula[i].originalX - pleiadesStars[j].originalX;
            float dy = pleiadesNebula[i].originalY - pleiadesStars[j].originalY;
            float distance = sqrt(dx*dx + dy*dy);
            if (distance < 25.0f && distance > 0.1f) {
                float illuminationStrength = pleiadesStars[j].luminosity / (2400.0f * (1.0f + distance * 0.4f));
                totalIllumination += illuminationStrength;
            }
        }
        
        // Reflection nebula brightness depends on scattered starlight
        float currentBrightness = baseBrightness * variation * (1.0f + totalIllumination * 1.5f);
        
        // Add scattering effects based on dust density
        float scatteringEffect = pleiadesNebula[i].density * (0.9f + 0.1f * sin(stellarWindPhase));
        currentBrightness *= scatteringEffect;
        
        currentBrightness = constrain(currentBrightness, 0.05f, 0.9f); // Keep nebula dimmer than stars
        
        // Enhanced color with blue scattering
        uint16_t drawColor;
        uint8_t r5 = (pleiadesNebula[i].color >> 11) & 0x1F;
        uint8_t g6 = (pleiadesNebula[i].color >> 5) & 0x3F;
        uint8_t b5 = pleiadesNebula[i].color & 0x1F;
        
        // Apply Rayleigh scattering (blue light scattered more)
        float blueEnhancement = 1.0f + totalIllumination * 0.3f;
        uint8_t r_scaled = (uint8_t)(r5 * currentBrightness);
        uint8_t g_scaled = (uint8_t)(g6 * currentBrightness);
        uint8_t b_scaled = (uint8_t)(b5 * currentBrightness * blueEnhancement);
        
        // Ensure blue doesn't overflow
        b_scaled = constrain(b_scaled, 0, 31);
        
        drawColor = (r_scaled << 11) | (g_scaled << 5) | b_scaled;
        
        // Draw enhanced nebula wisp
        if (drawX >= 0 && drawX < SCREEN_WIDTH && drawY >= 0 && drawY < SCREEN_HEIGHT) {
            if (pleiadesNebula[i].size == 1) {
                tft.drawPixel(drawX, drawY, drawColor);
            } else {
                int radius = pleiadesNebula[i].size / 2;
                tft.fillCircle(drawX, drawY, radius, drawColor);
                
                // Add subtle glow for brighter wisps
                if (currentBrightness > 0.6f && totalIllumination > 0.3f) {
                    uint16_t glowColor = drawColor;
                    uint8_t glow_r = ((glowColor >> 11) & 0x1F) / 3;
                    uint8_t glow_g = ((glowColor >> 5) & 0x3F) / 3;
                    uint8_t glow_b = (glowColor & 0x1F) / 2; // Keep more blue in glow
                    uint16_t dimGlow = (glow_r << 11) | (glow_g << 5) | glow_b;
                    
                    tft.drawCircle(drawX, drawY, radius + 1, dimGlow);
                }
            }
        }
        
        pleiadesNebula[i].prevX = drawX;
        pleiadesNebula[i].prevY = drawY;
    }
    
    // Draw hot star features (middle layer)
    for (int i = 0; i < MAX_HOT_STAR_FEATURES; i++) {
        if (!hotFeatures[i].active) continue;
        
        // Update feature motion based on type
        hotFeatures[i].pulsationPhase += hotFeatures[i].pulsationSpeed * deltaTime;
        if (hotFeatures[i].pulsationPhase > 2 * PI) hotFeatures[i].pulsationPhase -= 2 * PI;
        
        // Get parent star position
        int parentIndex = hotFeatures[i].parentStarIndex;
        if (!pleiadesStars[parentIndex].active) continue;
        
        float featureX = centerX + pleiadesStars[parentIndex].originalX * scale;
        float featureY = centerY + pleiadesStars[parentIndex].originalY * scale;
        
        // Feature-specific effects
        if (hotFeatures[i].featureType == 0) { // Stellar wind
            // Draw wind as radiating pattern
            float windStrength = 1.0f + 0.3f * sin(hotFeatures[i].pulsationPhase + stellarWindPhase);
            float windRadius = 4.0f * windStrength * scale;
            
            for (int angle = 0; angle < 360; angle += 45) {
                float windX = featureX + cos(angle * PI / 180.0f) * windRadius;
                float windY = featureY + sin(angle * PI / 180.0f) * windRadius;
                
                if (windX >= 0 && windX < SCREEN_WIDTH && windY >= 0 && windY < SCREEN_HEIGHT) {
                    uint16_t windColor = hotFeatures[i].featureColor;
                    float windBrightness = hotFeatures[i].brightness * windStrength * 0.6f;
                    
                    uint8_t r5 = ((windColor >> 11) & 0x1F) * windBrightness;
                    uint8_t g6 = ((windColor >> 5) & 0x3F) * windBrightness;
                    uint8_t b5 = (windColor & 0x1F) * windBrightness;
                    uint16_t finalColor = (r5 << 11) | (g6 << 5) | b5;
                    
                    tft.drawPixel((int)windX, (int)windY, finalColor);
                }
            }
        } else if (hotFeatures[i].featureType == 1) { // Rotation feature
            // Draw rotation as elliptical pattern
            hotFeatures[i].featureAngle += 0.05f * deltaTime;
            if (hotFeatures[i].featureAngle > 2 * PI) hotFeatures[i].featureAngle -= 2 * PI;
            
            float rotRadius = 3.0f * scale;
            float rotX = featureX + cos(hotFeatures[i].featureAngle) * rotRadius;
            float rotY = featureY + sin(hotFeatures[i].featureAngle) * rotRadius * 0.6f;
            
            if (rotX >= 0 && rotX < SCREEN_WIDTH && rotY >= 0 && rotY < SCREEN_HEIGHT) {
                uint16_t rotColor = hotFeatures[i].featureColor;
                float rotBrightness = hotFeatures[i].brightness * (0.8f + 0.2f * sin(hotFeatures[i].pulsationPhase));
                
                uint8_t r5 = ((rotColor >> 11) & 0x1F) * rotBrightness;
                uint8_t g6 = ((rotColor >> 5) & 0x3F) * rotBrightness;
                uint8_t b5 = (rotColor & 0x1F) * rotBrightness;
                uint16_t finalColor = (r5 << 11) | (g6 << 5) | b5;
                
                tft.fillCircle((int)rotX, (int)rotY, 1, finalColor);
            }
        } else { // Pulsation
            // Draw pulsation as varying brightness around star
            float pulseRadius = 2.0f + 1.0f * sin(hotFeatures[i].pulsationPhase);
            float pulseBrightness = hotFeatures[i].brightness * (0.6f + 0.4f * sin(hotFeatures[i].pulsationPhase));
            
            uint16_t pulseColor = hotFeatures[i].featureColor;
            uint8_t r5 = ((pulseColor >> 11) & 0x1F) * pulseBrightness;
            uint8_t g6 = ((pulseColor >> 5) & 0x3F) * pulseBrightness;
            uint8_t b5 = (pulseColor & 0x1F) * pulseBrightness;
            uint16_t finalColor = (r5 << 11) | (g6 << 5) | b5;
            
            if (featureX >= 0 && featureX < SCREEN_WIDTH && featureY >= 0 && featureY < SCREEN_HEIGHT) {
                tft.drawCircle((int)featureX, (int)featureY, (int)(pulseRadius * scale), finalColor);
            }
        }
    }
    
    // Draw stars on top (foreground layer)
    for (int i = 0; i < MAX_PLEIADES_STARS; i++) {
        if (!pleiadesStars[i].active) continue;
        
        pleiadesStars[i].x = centerX + pleiadesStars[i].originalX * scale;
        pleiadesStars[i].y = centerY + pleiadesStars[i].originalY * scale;
        
        int drawX = (int)pleiadesStars[i].x;
        int drawY = (int)pleiadesStars[i].y;
        
        // Erase previous position
        if (pleiadesStars[i].prevX != drawX || pleiadesStars[i].prevY != drawY) {
            if (pleiadesStars[i].prevX >= 0 && pleiadesStars[i].prevX < SCREEN_WIDTH &&
                pleiadesStars[i].prevY >= 0 && pleiadesStars[i].prevY < SCREEN_HEIGHT) {
                int eraseSize = pleiadesStars[i].size + 2;
                for (int ex = -eraseSize; ex <= eraseSize; ex++) {
                    for (int ey = -eraseSize; ey <= eraseSize; ey++) {
                        int px = pleiadesStars[i].prevX + ex;
                        int py = pleiadesStars[i].prevY + ey;
                        if (px >= 0 && px < SCREEN_WIDTH && py >= 0 && py < SCREEN_HEIGHT) {
                            tft.drawPixel(px, py, BG_COLOR);
                        }
                    }
                }
            }
        }
        
        // Enhanced twinkling for different star types
        pleiadesStars[i].twinklePhase += pleiadesStars[i].twinkleSpeed * deltaTime;
        if (pleiadesStars[i].twinklePhase > 2 * PI) {
            pleiadesStars[i].twinklePhase -= 2 * PI;
        }
        
        // Calculate brightness with stellar physics
        float twinkleFactor;
        if (pleiadesStars[i].isSister) {
            // Sisters are bright and relatively stable
            twinkleFactor = 0.9f + 0.1f * sin(pleiadesStars[i].twinklePhase);
        } else if (pleiadesStars[i].isHotStar) {
            // Hot B-type stars twinkle more due to stellar winds
            twinkleFactor = 0.75f + 0.25f * sin(pleiadesStars[i].twinklePhase + stellarWindPhase);
        } else {
            // Cooler stars are more stable
            twinkleFactor = 0.8f + 0.2f * sin(pleiadesStars[i].twinklePhase);
        }
        
        float currentBrightness = pleiadesStars[i].brightness * twinkleFactor;
        
        // Sisters get luminosity boost
        if (pleiadesStars[i].isSister) {
            currentBrightness *= 1.4f;
        }
        
        // Hot stars are intrinsically brighter
        if (pleiadesStars[i].isHotStar && !pleiadesStars[i].isSister) {
            currentBrightness *= 1.2f;
        }
        
        currentBrightness = constrain(currentBrightness, 0.1f, 1.0f);
        
        // Apply brightness to color with temperature effects
        uint16_t drawColor;
        if (currentBrightness >= 1.0f) {
            drawColor = pleiadesStars[i].color;
        } else {
            uint8_t r5 = (pleiadesStars[i].color >> 11) & 0x1F;
            uint8_t g6 = (pleiadesStars[i].color >> 5) & 0x3F;
            uint8_t b5 = pleiadesStars[i].color & 0x1F;
            
            uint8_t r_scaled = (uint8_t)(r5 * currentBrightness);
            uint8_t g_scaled = (uint8_t)(g6 * currentBrightness);
            uint8_t b_scaled = (uint8_t)(b5 * currentBrightness);
            
            drawColor = (r_scaled << 11) | (g_scaled << 5) | b_scaled;
        }
        
        // Draw enhanced star
        if (drawX >= 0 && drawX < SCREEN_WIDTH && drawY >= 0 && drawY < SCREEN_HEIGHT) {
            if (pleiadesStars[i].size == 1) {
                tft.drawPixel(drawX, drawY, drawColor);
            } else {
                int radius = pleiadesStars[i].size / 2;
                tft.fillCircle(drawX, drawY, radius, drawColor);
                
                // Enhanced diffraction spikes for Sisters
                if (pleiadesStars[i].isSister && currentBrightness > 0.8f) {
                    int spikeLength = radius + 2 + (pleiadesStars[i].sisterIndex < 5 ? 2 : 0); // Brightest 5 get longer spikes
                    
                    // Main cross spikes
                    tft.drawLine(drawX - spikeLength, drawY, drawX + spikeLength, drawY, drawColor);
                    tft.drawLine(drawX, drawY - spikeLength, drawX, drawY + spikeLength, drawColor);
                    
                    // Diagonal spikes for the brightest sisters
                    if (pleiadesStars[i].sisterIndex < 3) { // Alcyone, Atlas, Electra
                        int diagSpike = spikeLength * 0.7f;
                        tft.drawLine(drawX - diagSpike, drawY - diagSpike, drawX + diagSpike, drawY + diagSpike, drawColor);
                        tft.drawLine(drawX - diagSpike, drawY + diagSpike, drawX + diagSpike, drawY - diagSpike, drawColor);
                    }
                }
                
                // Hot stars get blue halos
                if (pleiadesStars[i].isHotStar && currentBrightness > 0.7f) {
                    uint16_t haloColor = 0x1E7F; // Deep blue halo
                    uint8_t halo_r = ((haloColor >> 11) & 0x1F) * 0.3f;
                    uint8_t halo_g = ((haloColor >> 5) & 0x3F) * 0.4f;
                    uint8_t halo_b = ((haloColor & 0x1F)) * 0.6f; // Stronger blue
                    uint16_t dimHalo = (halo_r << 11) | (halo_g << 5) | halo_b;
                    
                    tft.drawCircle(drawX, drawY, radius + 1, dimHalo);
                }
            }
        }
        
        pleiadesStars[i].prevX = drawX;
        pleiadesStars[i].prevY = drawY;
    }
}

void erasePleiades() {
    if (!pleiadesInitialized) return;
    
    // Erase nebula
    for (int i = 0; i < MAX_NEBULA_WISPS; i++) {
        if (pleiadesNebula[i].prevX >= 0 && pleiadesNebula[i].prevX < SCREEN_WIDTH &&
            pleiadesNebula[i].prevY >= 0 && pleiadesNebula[i].prevY < SCREEN_HEIGHT) {
            
            int eraseSize = pleiadesNebula[i].size + 2;
            for (int ex = -eraseSize; ex <= eraseSize; ex++) {
                for (int ey = -eraseSize; ey <= eraseSize; ey++) {
                    int px = pleiadesNebula[i].prevX + ex;
                    int py = pleiadesNebula[i].prevY + ey;
                    if (px >= 0 && px < SCREEN_WIDTH && py >= 0 && py < SCREEN_HEIGHT) {
                        tft.drawPixel(px, py, BG_COLOR);
                    }
                }
            }
        }
        pleiadesNebula[i].prevX = -1;
        pleiadesNebula[i].prevY = -1;
    }
    
    // Erase stars
    for (int i = 0; i < MAX_PLEIADES_STARS; i++) {
        if (pleiadesStars[i].prevX >= 0 && pleiadesStars[i].prevX < SCREEN_WIDTH &&
            pleiadesStars[i].prevY >= 0 && pleiadesStars[i].prevY < SCREEN_HEIGHT) {
            
            int eraseSize = pleiadesStars[i].size + 3; // Extra size for cross pattern
            for (int ex = -eraseSize; ex <= eraseSize; ex++) {
                for (int ey = -eraseSize; ey <= eraseSize; ey++) {
                    int px = pleiadesStars[i].prevX + ex;
                    int py = pleiadesStars[i].prevY + ey;
                    if (px >= 0 && px < SCREEN_WIDTH && py >= 0 && py < SCREEN_HEIGHT) {
                        tft.drawPixel(px, py, BG_COLOR);
                    }
                }
            }
        }
        pleiadesStars[i].prevX = -1;
        pleiadesStars[i].prevY = -1;
    }
    
    pleiadesInitialized = false;
}

#endif // PLEIADES_H