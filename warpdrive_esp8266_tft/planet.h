/**
 * planet.h
 * 
 * This file contains all code related to drawing and managing a planet
 * object in the Warp Drive visualization.
 */

#ifndef PLANET_H
#define PLANET_H

#include <TFT_eSPI.h>
#include <Arduino.h>
#include <cmath>

// Forward declarations from main sketch
extern TFT_eSPI tft;
extern const int SCREEN_WIDTH;
extern const int SCREEN_HEIGHT;
extern uint16_t BG_COLOR;
extern int objectX;
extern int objectY;
extern float objectScale;
extern float scaleFactor;

// Helper function declarations from main sketch
extern int red(uint16_t color);
extern int green(uint16_t color);
extern int blue(uint16_t color);

// Define PI if not already defined
#ifndef PI
#define PI 3.14159265358979323846
#endif
#ifndef DEG_TO_RAD
#define DEG_TO_RAD 0.017453292519943295
#endif

/**
 * Draws a planet with atmosphere and surface details
 * Modified to cycle through planet types
 */
// Static variables to hold the current planet's generated configuration
static bool planetConfigured = false;
static uint32_t planetSeed = 0; // Seed for procedural generation
static uint16_t planetBaseColor1 = 0; // Will be set to TFT_BLACK at runtime
static uint16_t planetBaseColor2 = 0; // Will be set to TFT_BLACK at runtime
static uint16_t planetFeatureColor = 0; // For clouds/details
static uint16_t planetAtmosColor = 0;
static float lightAngle = 0.0f; // Angle from which light is coming (radians)
static int planetType = 0;      // 0=Rocky, 1=Gas, 2=Earth-like, 3=Ice
static int baseRadius = 0;      // Store base radius used for configuration

// Forward declarations
void drawPlanet();
void erasePlanet();

// Simple pseudo-random noise function (replace with Perlin/Simplex if available/performant)
// Uses integer coordinates and a seed for deterministic noise
float simpleNoise(int x, int y, uint32_t seed) {
    // Simple hash-like function - basic but gives variation
    uint32_t hash = seed;
    hash = (hash ^ 61) ^ (x * 31);
    hash = hash + (y * 53);
    hash = hash ^ (hash >> 16);
    hash = hash * 0xDEADBEEF; // Mix it up
    hash = hash ^ (hash >> 13);
    return (hash & 0xFFFF) / 65535.0f; // Normalize to 0.0 - 1.0
}

// Helper to blend two colors
uint16_t blendColor(uint16_t color1, uint16_t color2, float ratio) {
    ratio = constrain(ratio, 0.0f, 1.0f);
    uint8_t r1 = ((color1 >> 11) & 0x1F);
    uint8_t g1 = ((color1 >> 5) & 0x3F);
    uint8_t b1 = (color1 & 0x1F);

    uint8_t r2 = ((color2 >> 11) & 0x1F);
    uint8_t g2 = ((color2 >> 5) & 0x3F);
    uint8_t b2 = (color2 & 0x1F);

    uint8_t r = r1 * (1.0 - ratio) + r2 * ratio;
    uint8_t g = g1 * (1.0 - ratio) + g2 * ratio;
    uint8_t b = b1 * (1.0 - ratio) + b2 * ratio;

    return tft.color565(r << 3, g << 2, b << 3); // Re-encode to 565 directly might lose precision, better use full RGB if possible
    // Or re-encode using the TFT function if it handles 8-bit RGB input:
    // return tft.color565( (uint8_t)((r1 * (1.0 - ratio) + r2 * ratio) * 8.1),  // Approximate scaling back to 8-bit range
    //                      (uint8_t)((g1 * (1.0 - ratio) + g2 * ratio) * 4.05),
    //                      (uint8_t)((b1 * (1.0 - ratio) + b2 * ratio) * 8.1) );
}

void drawPlanet() {
    int centerX = objectX;
    int centerY = objectY;
    float scale = std::max(0.1f, objectScale * scaleFactor);
    int currentRadius = round(15 * scale);

    // If planet not configured OR the base radius changed significantly (needs regeneration)
    if (!planetConfigured || abs(currentRadius - baseRadius) > 2) {
        planetSeed = random(0xFFFFFFFF); // New seed for this planet
        planetType = 2; // Force Earth-like for demo, or keep random(0, 4) for variety
        lightAngle = random(0, 360) * DEG_TO_RAD; // Random light direction

        // Define color palettes based on type
        switch (planetType) {
            case 0: // Rocky (Mars/Desert like)
                planetBaseColor1 = tft.color565(110, 70, 50);   // Dark Brown/Red
                planetBaseColor2 = tft.color565(210, 140, 90);  // Lighter Tan/Orange
                planetFeatureColor = tft.color565(180, 170, 160); // Wispy clouds/dust
                planetAtmosColor = tft.color565(230, 180, 150); // Thin, dusty atmosphere
                break;
            case 1: // Gas Giant (Jupiter/Saturn like)
                planetBaseColor1 = tft.color565(160, 140, 110); // Beige/Brown band
                planetBaseColor2 = tft.color565(220, 200, 170); // Lighter Cream band
                planetFeatureColor = tft.color565(240, 230, 220); // Bright Storms/swirls
                planetAtmosColor = tft.color565(210, 200, 180); // Hazy atmosphere
                break;
            case 2: // Earth-like
                planetBaseColor1 = tft.color565(20, 80, 160);   // Deep Ocean Blue
                planetBaseColor2 = tft.color565(50, 140, 70);   // Land Green
                planetFeatureColor = tft.color565(250, 250, 250); // White Clouds
                planetAtmosColor = tft.color565(120, 180, 255); // Stronger blue for atmosphere
                break;
            case 3: // Ice World
                planetBaseColor1 = tft.color565(150, 180, 210); // Shadowed Ice Blue
                planetBaseColor2 = tft.color565(220, 235, 255); // Bright Ice/Snow White
                planetFeatureColor = tft.color565(190, 210, 230); // Cracks / Light Blue features
                planetAtmosColor = tft.color565(210, 225, 245); // Very thin, bright atmosphere
                break;
        }
        planetConfigured = true;
        baseRadius = currentRadius; // Store the radius used for this configuration
    }

    // --- Drawing ---
    // Calculate square of radius for faster distance check
    float radiusSq = (float)currentRadius * currentRadius;
    // Calculate light vector components once
    float lightVecX = cos(lightAngle);
    float lightVecY = sin(lightAngle);

    tft.startWrite(); // Optimize drawing speed

    for (int y = -currentRadius; y <= currentRadius; y++) {
        for (int x = -currentRadius; x <= currentRadius; x++) {
            float distSq = (float)x * x + (float)y * y;

            // Is the pixel inside the planet's circle?
            if (distSq <= radiusSq) {
                float dist = sqrt(distSq);
                float currentAbsX = centerX + x;
                float currentAbsY = centerY + y;

                // Check screen bounds (optional but good practice)
                if (currentAbsX < 0 || currentAbsX >= SCREEN_WIDTH || currentAbsY < 0 || currentAbsY >= SCREEN_HEIGHT) {
                    continue;
                }

                // --- Calculate Base Color using Noise ---
                // Use multiple noise layers for more detail (adjust frequencies/amplitudes)
                float noiseVal = simpleNoise(x / 2, y / 2, planetSeed) * 0.6f; // Base layer
                noiseVal += simpleNoise(x * 2, y * 2, planetSeed + 1) * 0.3f; // Detail layer
                noiseVal += simpleNoise(y / 4, x/ 4, planetSeed + 2) * 0.1f; // Subtle large features (gas giant bands?)
                noiseVal = constrain(noiseVal, 0.0f, 1.0f);

                // Map noise to color gradient
                uint16_t baseSurfaceColor;
                if (planetType == 2) { // Special case for Earth: bias towards water
                   baseSurfaceColor = blendColor(planetBaseColor1, planetBaseColor2, constrain(noiseVal * 1.5f - 0.3f, 0.0f, 1.0f)); // More water
                } else {
                   baseSurfaceColor = blendColor(planetBaseColor1, planetBaseColor2, noiseVal);
                }

                 // --- Cloud/Feature Layer (optional based on type) ---
                float featureNoise = simpleNoise(x * 3 + 50, y * 3, planetSeed + 3); // Different noise for features
                float featureThreshold;
                switch(planetType) {
                    case 1: featureThreshold = 0.65f; break; // More prominent storms/bands
                    case 2: featureThreshold = 0.7f; break; // Clouds
                    default: featureThreshold = 0.9f; break; // Less frequent features
                }
                if (featureNoise > featureThreshold) {
                    float featureIntensity = (featureNoise - featureThreshold) / (1.0f - featureThreshold); // How "strong" is the feature
                    baseSurfaceColor = blendColor(baseSurfaceColor, planetFeatureColor, constrain(featureIntensity * 0.8f, 0.0f, 0.8f)); // Blend feature color in
                }


                // --- Calculate Lighting ---
                // Normalize pixel vector relative to center
                float pixelVecX = x / (float)currentRadius;
                float pixelVecY = y / (float)currentRadius;
                // Dot product between light vector and pixel normal vector (approximated by pixelVec)
                float dotProd = pixelVecX * lightVecX + pixelVecY * lightVecY;
                // Intensity based on angle (cosine relationship), clamp negative values (shadow)
                float lightIntensity = std::max(0.0f, dotProd);
                // Add ambient light so shadow side isn't pitch black
                lightIntensity = 0.15f + lightIntensity * 0.85f; // Adjust ambient(0.15) vs directional(0.85) ratio

                // Apply lighting to the surface color
                uint8_t r = red(baseSurfaceColor);
                uint8_t g = green(baseSurfaceColor);
                uint8_t b = blue(baseSurfaceColor);
                r = constrain((int)(r * lightIntensity), 0, 255);
                g = constrain((int)(g * lightIntensity), 0, 255);
                b = constrain((int)(b * lightIntensity), 0, 255);
                uint16_t litColor = tft.color565(r, g, b);

                // --- Atmosphere Haze near edge ---
                float edgeFactor = dist / (float)currentRadius; // 0 at center, 1 at edge
                float hazeAmount = pow(edgeFactor, 4.0f); // Make haze stronger near edge (power > 1)
                hazeAmount = constrain(hazeAmount * 0.4f, 0.0f, 0.4f); // Control max haze effect
                uint16_t finalColor = blendColor(litColor, planetAtmosColor, hazeAmount);


                // --- Draw the pixel ---
                tft.drawPixel(currentAbsX, currentAbsY, finalColor);
            }
        }
    }

    // --- Draw Outer Atmosphere Glow (Softer version) ---
    int glowRadiusStart = currentRadius + 1;
    int glowRadiusEnd = currentRadius + std::max(2, (int)(6 * scale)); // Glow thickness scales
    for (int r = glowRadiusEnd; r >= glowRadiusStart; r--) {
        float progress = (float)(r - glowRadiusStart) / (float)(glowRadiusEnd - glowRadiusStart + 1); // 1.0 near planet, 0.0 far out
        float alpha = (1.0 - progress) * 0.5f; // Fade out, max alpha less than 1.0

        // Blend atmosphere color with background based on alpha
        uint16_t glowColor = blendColor(BG_COLOR, planetAtmosColor, alpha);

        // Draw circle - might be slow, consider drawing arcs or points if needed
        tft.drawCircle(centerX, centerY, r, glowColor);
    }

    tft.endWrite(); // End optimized drawing
}

// --- Updated Erase Function ---

/**
 * Erases the planet and resets the configuration flag.
 */
void erasePlanet() {
    int centerX = objectX;
    int centerY = objectY;
    float scale = std::max(0.1f, objectScale);
    int currentRadius = round(15 * scale); // Match radius calculation in drawPlanet
    int glowThickness = std::max(2, (int)(6 * scale)); // Match glow thickness

    // Erase a circle slightly larger than the planet + atmosphere glow
    int eraseRadius = currentRadius + glowThickness + 2; // Add buffer
    tft.fillCircle(centerX, centerY, eraseRadius, BG_COLOR);

    // Signal that the planet needs to be re-configured on the next draw call
    planetConfigured = false;
}

#endif // PLANET_H
