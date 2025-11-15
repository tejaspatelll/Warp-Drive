#ifndef RING_NEBULA_H
#define RING_NEBULA_H

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

// Ring Nebula parameters - Enhanced for realism
#define MAX_RING_PARTICLES 150
#define MAX_BACKGROUND_STARS 18
#define MAX_IONIZATION_ZONES 3 // Inner, middle, outer zones

struct RingParticle
{
    float x, y;
    float originalX, originalY;
    float angle;          // Angle around ring
    float radius;         // Distance from center
    float originalRadius; // Starting radius
    uint16_t color;
    float brightness;
    float expansionPhase;
    float expansionSpeed;
    float pulsePhase;
    float pulseSpeed;
    float ionizationLevel; // How ionized the gas is
    float density;         // Gas density affects brightness
    float temperature;     // Gas temperature affects color
    int size;
    int prevX, prevY;
    bool active;
    int ionizationZone; // 0=inner (hot), 1=middle (moderate), 2=outer (cool)
    bool isKnot;        // Dense gas knots in the ring
};

struct RingStar
{
    float x, y;
    float originalX, originalY;
    uint16_t color;
    float brightness;
    float twinklePhase;
    float twinkleSpeed;
    float stellarWind; // White dwarf has strong winds
    float temperature; // For central white dwarf
    int size;
    int prevX, prevY;
    bool active;
    bool isCentralStar; // True for the white dwarf at center
};

// Global variables for Ring Nebula animation
RingParticle *ringParticles = nullptr;        // Pointer for dynamic allocation in PSRAM
int numRingParticles = 0;                     // To store the actual number of allocated particles
RingStar ringStars[MAX_BACKGROUND_STARS + 1]; // +1 for central white dwarf
bool ringInitialized = false;
unsigned long ringLastUpdateTime = 0;
float globalExpansion = 0.0f;
float ionizationPhase = 0.0f;
float centralStarActivity = 0.0f;

// Enhanced color palette based on actual Ring Nebula emission lines
const uint16_t INNER_ZONE_COLORS[] = {
    0x07E0, // Green (OIII 501nm - highly ionized oxygen)
    0x07FF, // Cyan (OIII 496nm)
    0x05FF, // Blue-green (high ionization)
    0x04FF  // Aqua (very hot gas)
};
const int NUM_INNER_ZONE_COLORS = sizeof(INNER_ZONE_COLORS) / sizeof(INNER_ZONE_COLORS[0]);

const uint16_t MIDDLE_ZONE_COLORS[] = {
    0x07E0, // Green (OIII still dominant)
    0xFFE0, // Yellow (NII and other ions)
    0x07FF, // Cyan (mixed ionization)
    0xF7E0  // Light yellow-green
};
const int NUM_MIDDLE_ZONE_COLORS = sizeof(MIDDLE_ZONE_COLORS) / sizeof(MIDDLE_ZONE_COLORS[0]);

const uint16_t OUTER_ZONE_COLORS[] = {
    0xF800, // Red (H-alpha 656nm)
    0xFD20, // Orange-red (lower ionization)
    0xF820, // Red-orange
    0xF81F  // Magenta (H-alpha + some blue)
};
const int NUM_OUTER_ZONE_COLORS = sizeof(OUTER_ZONE_COLORS) / sizeof(OUTER_ZONE_COLORS[0]);

// Central white dwarf properties
const float WHITE_DWARF_TEMP = 50000.0f;     // Very hot surface temperature
const float WHITE_DWARF_LUMINOSITY = 200.0f; // Solar luminosities (hot but small)

void initializeRing()
{
    if (ringInitialized)
        return;

    // Determine how many particles to allocate based on available PSRAM
    // Let's target a high number, but be prepared for allocation failure.
    const int TARGET_MAX_RING_PARTICLES = 800; // Target count, slightly less than Orion
    size_t particleSize = sizeof(RingParticle);
    size_t availablePSRAM = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);

    numRingParticles = TARGET_MAX_RING_PARTICLES;
    size_t requiredSize = numRingParticles * particleSize;

    // Reduce target particle count if PSRAM is insufficient
    if (requiredSize > availablePSRAM)
    {
        numRingParticles = availablePSRAM / particleSize;
        if (numRingParticles < 50)
            numRingParticles = 50; // Ensure a minimum
        requiredSize = numRingParticles * particleSize;
        Serial.printf("WARN: Not enough PSRAM for %d Ring particles, reducing to %d. Available: %u bytes\n",
                      TARGET_MAX_RING_PARTICLES, numRingParticles, availablePSRAM);
    }

    // Allocate memory in PSRAM (SPIRAM)
    ringParticles = (RingParticle *)heap_caps_malloc(requiredSize, MALLOC_CAP_SPIRAM);

    if (ringParticles == nullptr)
    {
        Serial.println("ERROR: Failed to allocate PSRAM for Ring particles!");
        numRingParticles = 0;   // Allocation failed
        ringInitialized = true; // Mark as initialized but with no particles
        ringLastUpdateTime = millis();
        return; // Exit initialize if allocation failed
    }
    else
    {
        Serial.printf("Allocated %u bytes for %d Ring particles in PSRAM\n", requiredSize, numRingParticles);
    }

    // Enhanced ring particle initialization with realistic structure
    float innerRadius = 6.0f;   // Inner edge (hottest)
    float middleRadius = 12.0f; // Peak brightness zone
    float outerRadius = 20.0f;  // Outer edge (coolest)

    for (int i = 0; i < numRingParticles; i++)
    {
        ringParticles[i].active = true;
        ringParticles[i].prevX = -1;
        ringParticles[i].prevY = -1;

        // Distribute particles across three ionization zones
        float zoneRand = random(1000) / 1000.0f;
        if (zoneRand < 0.25f)
        { // 25% in inner zone (hottest, greenest)
            ringParticles[i].ionizationZone = 0;
            ringParticles[i].originalRadius = innerRadius + random(60) / 10.0f;
            ringParticles[i].ionizationLevel = 0.8f + random(20) / 100.0f;
            ringParticles[i].temperature = 15000.0f + random(5000);
            ringParticles[i].color = INNER_ZONE_COLORS[random(NUM_INNER_ZONE_COLORS)];
        }
        else if (zoneRand < 0.65f)
        { // 40% in middle zone (peak brightness)
            ringParticles[i].ionizationZone = 1;
            ringParticles[i].originalRadius = middleRadius + (random(1000) / 1000.0f - 0.5f) * 4.0f;
            ringParticles[i].ionizationLevel = 0.5f + random(40) / 100.0f;
            ringParticles[i].temperature = 10000.0f + random(3000);
            ringParticles[i].color = MIDDLE_ZONE_COLORS[random(NUM_MIDDLE_ZONE_COLORS)];
        }
        else
        { // 35% in outer zone (cooler, redder)
            ringParticles[i].ionizationZone = 2;
            ringParticles[i].originalRadius = outerRadius + (random(1000) / 1000.0f - 0.5f) * 6.0f;
            ringParticles[i].ionizationLevel = 0.2f + random(40) / 100.0f;
            ringParticles[i].temperature = 6000.0f + random(2000);
            ringParticles[i].color = OUTER_ZONE_COLORS[random(NUM_OUTER_ZONE_COLORS)];
        }

        // Position around the ring with some randomness
        ringParticles[i].angle = (float)i / numRingParticles * 2 * PI + random(100) / 1000.0f;
        ringParticles[i].radius = ringParticles[i].originalRadius;

        // Calculate position
        ringParticles[i].originalX = cos(ringParticles[i].angle) * ringParticles[i].radius;
        ringParticles[i].originalY = sin(ringParticles[i].angle) * ringParticles[i].radius;

        // Gas density varies around the ring (creates knots)
        ringParticles[i].density = 0.4f + random(60) / 100.0f;

        // Some particles are dense knots
        ringParticles[i].isKnot = random(100) < 15; // 15% are knots
        if (ringParticles[i].isKnot)
        {
            ringParticles[i].density *= 1.8f;
            ringParticles[i].size = 2 + random(2);
        }
        else
        {
            ringParticles[i].size = 1 + random(2);
        }

        // Brightness depends on density and ionization
        ringParticles[i].brightness = 0.3f + ringParticles[i].density * 0.5f +
                                      ringParticles[i].ionizationLevel * 0.3f;

        // Expansion and pulsation parameters
        ringParticles[i].expansionPhase = random(360) * PI / 180.0f;
        ringParticles[i].expansionSpeed = 0.001f + random(3) / 2000.0f; // Very slow expansion
        ringParticles[i].pulsePhase = random(360) * PI / 180.0f;
        ringParticles[i].pulseSpeed = 0.008f + random(8) / 1000.0f;
    }

    // Initialize central white dwarf (the star that created the nebula)
    ringStars[0].active = true;
    ringStars[0].isCentralStar = true;
    ringStars[0].originalX = 0.0f;
    ringStars[0].originalY = 0.0f;
    ringStars[0].size = 2; // Small but very hot
    ringStars[0].brightness = 0.9f;
    ringStars[0].temperature = WHITE_DWARF_TEMP;
    ringStars[0].stellarWind = 5.0f; // Strong stellar wind
    ringStars[0].twinklePhase = 0.0f;
    ringStars[0].twinkleSpeed = 0.03f; // Rapid variations due to strong activity
    ringStars[0].color = 0xFFFF;       // White-hot
    ringStars[0].prevX = -1;
    ringStars[0].prevY = -1;

    // Initialize background field stars
    for (int i = 1; i <= MAX_BACKGROUND_STARS; i++)
    {
        ringStars[i].active = true;
        ringStars[i].isCentralStar = false;

        // Scatter around the nebula area, avoiding the ring itself
        float angle = random(360) * PI / 180.0f;
        float radius;
        if (random(100) < 30)
        {
            radius = random(50) / 10.0f; // Some very close (foreground stars)
        }
        else
        {
            radius = 25.0f + random(200) / 10.0f; // Most are distant background
        }

        ringStars[i].originalX = cos(angle) * radius;
        ringStars[i].originalY = sin(angle) * radius;

        ringStars[i].size = 1 + random(2);
        ringStars[i].brightness = 0.3f + random(40) / 100.0f;
        ringStars[i].temperature = 3000.0f + random(7000); // Various types
        ringStars[i].stellarWind = 0.1f + random(5) / 10.0f;
        ringStars[i].twinklePhase = random(360) * PI / 180.0f;
        ringStars[i].twinkleSpeed = 0.01f + random(8) / 1000.0f;

        // Color based on temperature
        if (ringStars[i].temperature > 7000)
        {
            ringStars[i].color = 0xFFFF; // White/blue-white
        }
        else if (ringStars[i].temperature > 5000)
        {
            ringStars[i].color = 0xFFE0; // Yellow-white
        }
        else
        {
            ringStars[i].color = 0xFD20; // Orange
        }

        ringStars[i].prevX = -1;
        ringStars[i].prevY = -1;
    }

    ringInitialized = true;
    ringLastUpdateTime = millis();
}

void drawRing()
{
    if (!ringInitialized)
    {
        initializeRing();
    }

    unsigned long currentTime = millis();
    float deltaTime = (currentTime - ringLastUpdateTime) / 1000.0f;
    if (deltaTime > 0.1f)
        deltaTime = 0.1f;
    ringLastUpdateTime = currentTime;

    int centerX = objectX;
    int centerY = objectY;
    float scale = objectScale;

    // Update global expansion (very slow)
    globalExpansion += 0.001f * deltaTime;
    if (globalExpansion > 2 * PI)
        globalExpansion -= 2 * PI;

    // Draw ring particles first
    for (int i = 0; i < numRingParticles; i++)
    {
        if (!ringParticles[i].active)
            continue;

        // Update expansion and pulsing
        ringParticles[i].expansionPhase += ringParticles[i].expansionSpeed * deltaTime;
        ringParticles[i].pulsePhase += ringParticles[i].pulseSpeed * deltaTime;

        // Calculate current radius with expansion
        float expansionFactor = 1.0f + 0.05f * sin(ringParticles[i].expansionPhase + globalExpansion);
        ringParticles[i].radius = ringParticles[i].originalRadius * expansionFactor;

        // Calculate position
        ringParticles[i].x = centerX + cos(ringParticles[i].angle) * ringParticles[i].radius * scale;
        ringParticles[i].y = centerY + sin(ringParticles[i].angle) * ringParticles[i].radius * scale;

        int drawX = (int)ringParticles[i].x;
        int drawY = (int)ringParticles[i].y;

        // Erase previous position
        if (ringParticles[i].prevX != drawX || ringParticles[i].prevY != drawY)
        {
            if (ringParticles[i].prevX >= 0 && ringParticles[i].prevX < SCREEN_WIDTH &&
                ringParticles[i].prevY >= 0 && ringParticles[i].prevY < SCREEN_HEIGHT)
            {
                int eraseSize = ringParticles[i].size + 1;
                for (int ex = -eraseSize; ex <= eraseSize; ex++)
                {
                    for (int ey = -eraseSize; ey <= eraseSize; ey++)
                    {
                        int px = ringParticles[i].prevX + ex;
                        int py = ringParticles[i].prevY + ey;
                        if (px >= 0 && px < SCREEN_WIDTH && py >= 0 && py < SCREEN_HEIGHT)
                        {
                            tft.drawPixel(px, py, BG_COLOR);
                        }
                    }
                }
            }
        }

        // Calculate brightness with pulsing
        float pulseFactor = 0.7f + 0.3f * sin(ringParticles[i].pulsePhase);
        float currentBrightness = ringParticles[i].brightness * pulseFactor;
        currentBrightness = constrain(currentBrightness, 0.1f, 1.0f);

        // Apply brightness to color
        uint16_t drawColor;
        if (currentBrightness >= 1.0f)
        {
            drawColor = ringParticles[i].color;
        }
        else
        {
            uint8_t r5 = (ringParticles[i].color >> 11) & 0x1F;
            uint8_t g6 = (ringParticles[i].color >> 5) & 0x3F;
            uint8_t b5 = ringParticles[i].color & 0x1F;

            uint8_t r_scaled = (uint8_t)(r5 * currentBrightness);
            uint8_t g_scaled = (uint8_t)(g6 * currentBrightness);
            uint8_t b_scaled = (uint8_t)(b5 * currentBrightness);

            drawColor = (r_scaled << 11) | (g_scaled << 5) | b_scaled;
        }

        // Draw the particle
        if (drawX >= 0 && drawX < SCREEN_WIDTH && drawY >= 0 && drawY < SCREEN_HEIGHT)
        {
            if (ringParticles[i].size == 1)
            {
                tft.drawPixel(drawX, drawY, drawColor);
            }
            else
            {
                int radius = ringParticles[i].size / 2;
                tft.fillCircle(drawX, drawY, radius, drawColor);
            }
        }

        ringParticles[i].prevX = drawX;
        ringParticles[i].prevY = drawY;
    }

    // Draw stars on top
    for (int i = 0; i <= MAX_BACKGROUND_STARS; i++)
    {
        if (!ringStars[i].active)
            continue;

        ringStars[i].x = centerX + ringStars[i].originalX * scale;
        ringStars[i].y = centerY + ringStars[i].originalY * scale;

        int drawX = (int)ringStars[i].x;
        int drawY = (int)ringStars[i].y;

        // Erase previous position
        if (ringStars[i].prevX != drawX || ringStars[i].prevY != drawY)
        {
            if (ringStars[i].prevX >= 0 && ringStars[i].prevX < SCREEN_WIDTH &&
                ringStars[i].prevY >= 0 && ringStars[i].prevY < SCREEN_HEIGHT)
            {
                int eraseSize = ringStars[i].size + 1;
                for (int ex = -eraseSize; ex <= eraseSize; ex++)
                {
                    for (int ey = -eraseSize; ey <= eraseSize; ey++)
                    {
                        int px = ringStars[i].prevX + ex;
                        int py = ringStars[i].prevY + ey;
                        if (px >= 0 && px < SCREEN_WIDTH && py >= 0 && py < SCREEN_HEIGHT)
                        {
                            tft.drawPixel(px, py, BG_COLOR);
                        }
                    }
                }
            }
        }

        // Update twinkling
        ringStars[i].twinklePhase += ringStars[i].twinkleSpeed * deltaTime;
        if (ringStars[i].twinklePhase > 2 * PI)
        {
            ringStars[i].twinklePhase -= 2 * PI;
        }

        // Calculate brightness
        float twinkleFactor = 0.7f + 0.3f * sin(ringStars[i].twinklePhase);
        float currentBrightness = ringStars[i].brightness * twinkleFactor;

        // Central star is brighter and more stable
        if (ringStars[i].isCentralStar)
        {
            currentBrightness = ringStars[i].brightness * (0.9f + 0.1f * sin(ringStars[i].twinklePhase));
        }

        currentBrightness = constrain(currentBrightness, 0.1f, 1.0f);

        // Apply brightness to color
        uint16_t drawColor;
        if (currentBrightness >= 1.0f)
        {
            drawColor = ringStars[i].color;
        }
        else
        {
            uint8_t r5 = (ringStars[i].color >> 11) & 0x1F;
            uint8_t g6 = (ringStars[i].color >> 5) & 0x3F;
            uint8_t b5 = ringStars[i].color & 0x1F;

            uint8_t r_scaled = (uint8_t)(r5 * currentBrightness);
            uint8_t g_scaled = (uint8_t)(g6 * currentBrightness);
            uint8_t b_scaled = (uint8_t)(b5 * currentBrightness);

            drawColor = (r_scaled << 11) | (g_scaled << 5) | b_scaled;
        }

        // Draw the star
        if (drawX >= 0 && drawX < SCREEN_WIDTH && drawY >= 0 && drawY < SCREEN_HEIGHT)
        {
            if (ringStars[i].size == 1)
            {
                tft.drawPixel(drawX, drawY, drawColor);
            }
            else
            {
                int radius = ringStars[i].size / 2;
                tft.fillCircle(drawX, drawY, radius, drawColor);

                // Add cross pattern for central white dwarf
                if (ringStars[i].isCentralStar && currentBrightness > 0.7f)
                {
                    int crossLength = radius + 1;
                    tft.drawLine(drawX - crossLength, drawY, drawX + crossLength, drawY, drawColor);
                    tft.drawLine(drawX, drawY - crossLength, drawX, drawY + crossLength, drawColor);
                }
            }
        }

        ringStars[i].prevX = drawX;
        ringStars[i].prevY = drawY;
    }
}

void eraseRing()
{
    if (!ringInitialized)
        return;

    // Full screen clear for faster erase
    tft.fillScreen(BG_COLOR);

    // Free allocated memory if it exists (already present, ensure it's here)
    if (ringParticles != nullptr)
    {
        heap_caps_free(ringParticles);
        ringParticles = nullptr;
        numRingParticles = 0; // Reset count
        Serial.println("Freed PSRAM for Ring particles.");
    }

    ringInitialized = false;
}

#endif // RING_NEBULA_H