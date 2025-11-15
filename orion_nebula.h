#ifndef ORION_NEBULA_H
#define ORION_NEBULA_H

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

// Orion Nebula parameters - Enhanced for realism
#define MAX_ORION_PARTICLES 150
#define MAX_TRAPEZIUM_STARS 4
#define MAX_BACKGROUND_STARS 35
#define MAX_PROTOPLANETARY_DISKS 8
#define MAX_HERBIG_HARO_OBJECTS 6

struct OrionParticle
{
    float x, y;
    float originalX, originalY;
    uint16_t color;
    float brightness;
    float swirlingPhase;
    float swirlingSpeed;
    float radius;      // Distance from center for swirling
    float angle;       // Current angle for swirling
    float density;     // Affects brightness and color intensity
    float temperature; // Affects color (hot=blue, cool=red)
    int size;
    int prevX, prevY;
    bool active;
    bool isGas;        // True for gas particles, false for dust
    bool isShockFront; // True for shock wave fronts from stellar winds
    int gasType;       // 0=H-alpha (red), 1=OIII (green-blue), 2=dust (brown), 3=hot gas (blue)
};

struct OrionStar
{
    float x, y;
    float originalX, originalY;
    uint16_t color;
    float brightness;
    float twinklePhase;
    float twinkleSpeed;
    float stellarWind; // Intensity of stellar wind
    float mass;        // Stellar mass affects wind strength
    int size;
    int prevX, prevY;
    bool active;
    bool isTrapezium; // True for the central 4 bright stars
    bool isProtostar; // True for newly forming stars
};

struct ProtoDisk
{
    float x, y;
    float originalX, originalY;
    float diskAngle;
    float diskRadius;
    float rotationSpeed;
    uint16_t diskColor;
    float brightness;
    bool active;
    int parentStar; // Index of associated star
};

struct HerbigHaro
{
    float x, y;
    float originalX, originalY;
    float jetAngle;
    float jetSpeed;
    float jetPhase;
    uint16_t jetColor;
    float brightness;
    bool active;
    int sourceIndex; // Index of source protostar
};

// Global variables for Orion animation
OrionParticle *orionParticles = nullptr; // Pointer for dynamic allocation in PSRAM
int numOrionParticles = 0;               // To store the actual number of allocated particles
OrionStar orionStars[MAX_TRAPEZIUM_STARS + MAX_BACKGROUND_STARS];
ProtoDisk protoDisks[MAX_PROTOPLANETARY_DISKS];
HerbigHaro hhObjects[MAX_HERBIG_HARO_OBJECTS];
bool orionInitialized = false;
unsigned long orionLastUpdateTime = 0;
float globalSwirl = 0.0f;
extern float stellarWindPhase;

// Enhanced color palette for Orion Nebula (more authentic astronomical colors)
const uint16_t H_ALPHA_COLORS[] = {
    0xF800, // Deep red (H-alpha 656nm)
    0xF820, // Red-orange
    0xFD00, // Orange-red
    0xF81F  // Magenta (H-alpha + some blue)
};
const int NUM_H_ALPHA_COLORS = sizeof(H_ALPHA_COLORS) / sizeof(H_ALPHA_COLORS[0]);

const uint16_t OIII_COLORS[] = {
    0x1E7F, // Deep blue
    0x3F7F, // Blue-white
    0x07FF, // Cyan-blue
    0xFFFF  // White-hot
};
const int NUM_OIII_COLORS = sizeof(OIII_COLORS) / sizeof(OIII_COLORS[0]);

const uint16_t DUST_COLORS[] = {
    0x4208, // Dark brown
    0x6309, // Medium brown
    0x8410, // Light brown
    0xA514  // Tan
};
const int NUM_DUST_COLORS = sizeof(DUST_COLORS) / sizeof(DUST_COLORS[0]);

const uint16_t HOT_GAS_COLORS[] = {
    0x07FF, // Hot blue (high temperature gas)
    0x1E7F, // Deep blue
    0x3F7F, // Blue-white
    0xFFFF  // White hot
};
const int NUM_HOT_GAS_COLORS = sizeof(HOT_GAS_COLORS) / sizeof(HOT_GAS_COLORS[0]);

// Trapezium colors (O and B type stars - very hot and blue)
const uint16_t TRAPEZIUM_COLORS[] = {
    0x07FF, // Cyan-blue (O-type)
    0x1E7F, // Deep blue (B-type)
    0x3F7F, // Blue-white
    0xFFFF  // White-hot
};
const int NUM_TRAPEZIUM_COLORS = sizeof(TRAPEZIUM_COLORS) / sizeof(TRAPEZIUM_COLORS[0]);

void initializeOrion()
{
    if (orionInitialized)
        return;

    // Determine how many particles to allocate based on available PSRAM
    // Let's target a high number, but be prepared for allocation failure.
    const int TARGET_MAX_ORION_PARTICLES = 1000; // Target count
    size_t particleSize = sizeof(OrionParticle);
    size_t availablePSRAM = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);

    numOrionParticles = TARGET_MAX_ORION_PARTICLES;
    size_t requiredSize = numOrionParticles * particleSize;

    // Reduce target particle count if PSRAM is insufficient
    if (requiredSize > availablePSRAM)
    {
        numOrionParticles = availablePSRAM / particleSize;
        if (numOrionParticles < 50)
            numOrionParticles = 50; // Ensure a minimum
        requiredSize = numOrionParticles * particleSize;
        Serial.printf("WARN: Not enough PSRAM for %d Orion particles, reducing to %d. Available: %u bytes\n",
                      TARGET_MAX_ORION_PARTICLES, numOrionParticles, availablePSRAM);
    }

    // Allocate memory in PSRAM (SPIRAM)
    orionParticles = (OrionParticle *)heap_caps_malloc(requiredSize, MALLOC_CAP_SPIRAM);

    if (orionParticles == nullptr)
    {
        Serial.println("ERROR: Failed to allocate PSRAM for Orion particles!");
        numOrionParticles = 0;   // Allocation failed
        orionInitialized = true; // Mark as initialized but with no particles
        orionLastUpdateTime = millis();
        return; // Exit initialize if allocation failed
    }
    else
    {
        Serial.printf("Allocated %u bytes for %d Orion particles in PSRAM\n", requiredSize, numOrionParticles);
    }

    // Initialize enhanced gas and dust particles with realistic distribution
    for (int i = 0; i < numOrionParticles; i++)
    {
        orionParticles[i].active = true;
        orionParticles[i].prevX = -1;
        orionParticles[i].prevY = -1;

        // Create authentic Orion Nebula shape - asymmetric with bright central region
        float angle = random(360) * PI / 180.0f;
        float maxRadius = 28.0f;

        // Create realistic wing-like structure based on actual nebula shape
        float wingFactor = 1.0f;
        if (angle > PI / 4 && angle < 3 * PI / 4)
        { // Upper wing
            wingFactor = 1.3f + 0.4f * sin((angle - PI / 4) * 2);
        }
        else if (angle > 5 * PI / 4 && angle < 7 * PI / 4)
        { // Lower wing
            wingFactor = 1.2f + 0.3f * sin((angle - 5 * PI / 4) * 2);
        }

        // Central bar structure
        if (abs(sin(angle)) < 0.3f)
        {
            wingFactor *= 1.5f; // Brighten central bar
        }

        float radius = sqrt(random(1000) / 1000.0f) * maxRadius * wingFactor;

        orionParticles[i].originalX = cos(angle) * radius;
        orionParticles[i].originalY = sin(angle) * radius * 0.85f; // Slightly flatten

        orionParticles[i].radius = radius;
        orionParticles[i].angle = angle;
        orionParticles[i].swirlingSpeed = 0.003f + random(8) / 2000.0f;
        orionParticles[i].swirlingPhase = random(360) * PI / 180.0f;
        orionParticles[i].density = 0.2f + random(80) / 100.0f;
        orionParticles[i].temperature = 500.0f + random(2000); // 500-2500K range

        // Determine particle type based on position and randomness
        float distanceFromCenter = sqrt(orionParticles[i].originalX * orionParticles[i].originalX +
                                        orionParticles[i].originalY * orionParticles[i].originalY);

        if (distanceFromCenter < 8.0f)
        { // Central region - lots of ionized gas
            int gasRand = random(100);
            if (gasRand < 60)
            {                                  // Increased chance for H-alpha in central region
                orionParticles[i].gasType = 0; // H-alpha (red)
                orionParticles[i].color = H_ALPHA_COLORS[random(NUM_H_ALPHA_COLORS)];
            }
            else if (gasRand < 80)
            {                                  // Increased chance for Hot gas
                orionParticles[i].gasType = 3; // Hot gas (blue)
                orionParticles[i].color = HOT_GAS_COLORS[random(NUM_HOT_GAS_COLORS)];
            }
            else
            {
                orionParticles[i].gasType = 2; // Dust (brown)
                orionParticles[i].color = DUST_COLORS[random(NUM_DUST_COLORS)];
                orionParticles[i].brightness = 0.3f; // Dust is dimmer
            }
            orionParticles[i].isGas = gasRand < 80;
        }
        else
        { // Outer regions - more H-alpha and dust
            int gasRand = random(100);
            if (gasRand < 70)
            {                                  // Increased chance for H-alpha in outer regions
                orionParticles[i].gasType = 0; // H-alpha dominates outer regions
                orionParticles[i].color = H_ALPHA_COLORS[random(NUM_H_ALPHA_COLORS)];
                orionParticles[i].isGas = true;
            }
            else
            {
                orionParticles[i].gasType = 2; // Dust lanes
                orionParticles[i].color = DUST_COLORS[random(NUM_DUST_COLORS)];
                orionParticles[i].isGas = false;
                orionParticles[i].brightness = 0.2f;
            }
        }

        // Set brightness based on density and type
        if (orionParticles[i].isGas)
        {
            orionParticles[i].brightness = 0.3f + orionParticles[i].density * 0.6f;
        }

        orionParticles[i].size = 1 + random(3);
        orionParticles[i].isShockFront = random(100) < 5; // 5% chance of shock front
    }

    // Ensure all particles are size 1 for point drawing
    for (int i = 0; i < numOrionParticles; i++)
    {
        orionParticles[i].size = 1;
    }

    // Initialize Trapezium stars (the central powerhouse)
    float trapeziumPositions[4][2] = {
        {-2.5f, -1.8f}, // Theta1 Ori A (hottest, most massive)
        {1.8f, -2.2f},  // Theta1 Ori B
        {-1.2f, 2.3f},  // Theta1 Ori C (very hot O-type)
        {3.0f, 1.2f}    // Theta1 Ori D
    };

    float trapeziumMasses[4] = {15.0f, 8.5f, 35.0f, 6.5f}; // Solar masses

    for (int i = 0; i < MAX_TRAPEZIUM_STARS; i++)
    {
        orionStars[i].active = true;
        orionStars[i].isTrapezium = true;
        orionStars[i].isProtostar = false;
        orionStars[i].originalX = trapeziumPositions[i][0];
        orionStars[i].originalY = trapeziumPositions[i][1];
        orionStars[i].mass = trapeziumMasses[i];
        orionStars[i].stellarWind = orionStars[i].mass / 10.0f; // Wind strength
        orionStars[i].size = 3 + (orionStars[i].mass > 20.0f ? 2 : random(2));
        orionStars[i].brightness = 0.95f + random(5) / 100.0f;
        orionStars[i].twinklePhase = random(360) * PI / 180.0f;
        orionStars[i].twinkleSpeed = 0.025f + random(5) / 1000.0f;
        orionStars[i].color = TRAPEZIUM_COLORS[random(NUM_TRAPEZIUM_COLORS)];
        orionStars[i].prevX = -1;
        orionStars[i].prevY = -1;
    }

    // Initialize background and embedded stars
    for (int i = MAX_TRAPEZIUM_STARS; i < MAX_TRAPEZIUM_STARS + MAX_BACKGROUND_STARS; i++)
    {
        orionStars[i].active = true;
        orionStars[i].isTrapezium = false;

        // Mix of background stars and embedded protostars
        if (random(100) < 25)
        { // 25% are protostars/young stellar objects
            orionStars[i].isProtostar = true;
            // Protostars tend to be in dense regions
            float angle = random(360) * PI / 180.0f;
            float radius = 5.0f + random(150) / 10.0f; // Closer to center
            orionStars[i].originalX = cos(angle) * radius;
            orionStars[i].originalY = sin(angle) * radius;
            orionStars[i].color = 0xFD20;                   // Orange-red (cooler protostars)
            orionStars[i].mass = 0.5f + random(30) / 10.0f; // 0.5-3.5 solar masses
        }
        else
        { // Background field stars
            orionStars[i].isProtostar = false;
            float angle = random(360) * PI / 180.0f;
            float radius = 20.0f + random(300) / 10.0f; // Scattered background
            orionStars[i].originalX = cos(angle) * radius;
            orionStars[i].originalY = sin(angle) * radius;
            orionStars[i].color = 0xFFFF; // White background stars
            orionStars[i].mass = 1.0f;    // Solar mass
        }

        orionStars[i].stellarWind = orionStars[i].mass / 15.0f;
        orionStars[i].size = 1 + random(2);
        orionStars[i].brightness = 0.4f + random(40) / 100.0f;
        orionStars[i].twinklePhase = random(360) * PI / 180.0f;
        orionStars[i].twinkleSpeed = 0.01f + random(10) / 1000.0f;
        orionStars[i].prevX = -1;
        orionStars[i].prevY = -1;
    }

    // Initialize protoplanetary disks around some young stars
    for (int i = 0; i < MAX_PROTOPLANETARY_DISKS; i++)
    {
        protoDisks[i].active = random(100) < 60; // 60% chance of active disk
        if (!protoDisks[i].active)
            continue;

        // Attach to a protostar or young star
        int parentIndex = MAX_TRAPEZIUM_STARS + random(MAX_BACKGROUND_STARS);
        protoDisks[i].parentStar = parentIndex;
        protoDisks[i].originalX = orionStars[parentIndex].originalX;
        protoDisks[i].originalY = orionStars[parentIndex].originalY;
        protoDisks[i].diskRadius = 1.5f + random(25) / 10.0f; // 1.5-4.0 units
        protoDisks[i].diskAngle = random(360) * PI / 180.0f;
        protoDisks[i].rotationSpeed = 0.01f + random(15) / 1000.0f;
        protoDisks[i].diskColor = 0x8410; // Brown (dust disk)
        protoDisks[i].brightness = 0.3f + random(30) / 100.0f;
    }

    // Initialize Herbig-Haro objects (jets from protostars)
    for (int i = 0; i < MAX_HERBIG_HARO_OBJECTS; i++)
    {
        hhObjects[i].active = random(100) < 40; // 40% chance
        if (!hhObjects[i].active)
            continue;

        // Source from a protostar
        int sourceIndex = MAX_TRAPEZIUM_STARS + random(MAX_BACKGROUND_STARS / 2);
        hhObjects[i].sourceIndex = sourceIndex;
        hhObjects[i].originalX = orionStars[sourceIndex].originalX;
        hhObjects[i].originalY = orionStars[sourceIndex].originalY;
        hhObjects[i].jetAngle = random(360) * PI / 180.0f;
        hhObjects[i].jetSpeed = 0.02f + random(15) / 1000.0f;
        hhObjects[i].jetPhase = random(360) * PI / 180.0f;
        hhObjects[i].jetColor = 0x07E0; // Green (OIII emission in jets)
        hhObjects[i].brightness = 0.5f + random(30) / 100.0f;
    }

    orionInitialized = true;
    orionLastUpdateTime = millis();
}

void drawOrion()
{
    if (!orionInitialized)
    {
        initializeOrion();
    }

    unsigned long currentTime = millis();
    float deltaTime = (currentTime - orionLastUpdateTime) / 1000.0f;
    if (deltaTime > 0.1f)
        deltaTime = 0.1f;
    orionLastUpdateTime = currentTime;

    int centerX = objectX;
    int centerY = objectY;
    float scale = objectScale;

    // Update global phases
    globalSwirl += 0.002f * deltaTime; // Very slow swirling motion
    if (globalSwirl > 2 * PI)
        globalSwirl -= 2 * PI;

    stellarWindPhase += 0.015f * deltaTime; // Stellar wind variations
    if (stellarWindPhase > 2 * PI)
        stellarWindPhase -= 2 * PI;

    // Draw nebula particles first (background layer)
    for (int i = 0; i < numOrionParticles; i++)
    {
        if (!orionParticles[i].active)
            continue;

        // Enhanced particle motion with stellar wind effects
        orionParticles[i].swirlingPhase += orionParticles[i].swirlingSpeed * deltaTime;

        float windEffect = 0.0f;
        // Calculate stellar wind effects from Trapezium stars
        for (int j = 0; j < MAX_TRAPEZIUM_STARS; j++)
        {
            if (!orionStars[j].active)
                continue;
            float dx = orionParticles[i].originalX - orionStars[j].originalX;
            float dy = orionParticles[i].originalY - orionStars[j].originalY;
            float distance = sqrt(dx * dx + dy * dy);
            if (distance < 15.0f && distance > 0.1f)
            {
                float windStrength = orionStars[j].stellarWind / (distance * distance);
                windEffect += windStrength * sin(stellarWindPhase + j * PI / 2);
            }
        }

        // Enhanced swirling with wind and shock front effects
        float swirlingOffset = sin(orionParticles[i].swirlingPhase + globalSwirl) * 0.8f;
        float windOffset = windEffect * 2.0f;

        // Shock front particles have different motion
        if (orionParticles[i].isShockFront)
        {
            swirlingOffset *= 2.0f; // More dramatic motion
            windOffset *= 1.5f;
        }

        float currentX = orionParticles[i].originalX + swirlingOffset + windOffset;
        float currentY = orionParticles[i].originalY + cos(orionParticles[i].swirlingPhase + globalSwirl) * 0.6f;

        orionParticles[i].x = centerX + currentX * scale;
        orionParticles[i].y = centerY + currentY * scale;

        int drawX = (int)orionParticles[i].x;
        int drawY = (int)orionParticles[i].y;

        // Erase previous position
        if (orionParticles[i].prevX != drawX || orionParticles[i].prevY != drawY)
        {
            if (orionParticles[i].prevX >= 0 && orionParticles[i].prevX < SCREEN_WIDTH &&
                orionParticles[i].prevY >= 0 && orionParticles[i].prevY < SCREEN_HEIGHT)
            {
                int eraseSize = orionParticles[i].size + 1;
                for (int ex = -eraseSize; ex <= eraseSize; ex++)
                {
                    for (int ey = -eraseSize; ey <= eraseSize; ey++)
                    {
                        int px = orionParticles[i].prevX + ex;
                        int py = orionParticles[i].prevY + ey;
                        if (px >= 0 && px < SCREEN_WIDTH && py >= 0 && py < SCREEN_HEIGHT)
                        {
                            tft.drawPixel(px, py, BG_COLOR);
                        }
                    }
                }
            }
        }

        // Enhanced brightness calculation based on gas type and stellar illumination
        float variation = 0.7f + 0.3f * sin(orionParticles[i].swirlingPhase);
        float currentBrightness = orionParticles[i].brightness * variation;

        // Illumination effects from nearby Trapezium stars
        float illumination = 0.0f;
        for (int j = 0; j < MAX_TRAPEZIUM_STARS; j++)
        {
            if (!orionStars[j].active)
                continue;
            float dx = orionParticles[i].originalX - orionStars[j].originalX;
            float dy = orionParticles[i].originalY - orionStars[j].originalY;
            float distance = sqrt(dx * dx + dy * dy);
            if (distance < 20.0f && distance > 0.1f)
            {
                illumination += orionStars[j].brightness / (1.0f + distance * 0.3f);
            }
        }
        currentBrightness *= (1.0f + illumination * 0.4f);

        // Gas type specific effects
        if (orionParticles[i].gasType == 0)
        { // H-alpha - responds to UV radiation
            currentBrightness *= (1.0f + illumination * 0.3f);
        }
        else if (orionParticles[i].gasType == 1)
        { // OIII - high ionization
          // No OIII in this version, remove specific effect
        }
        else if (orionParticles[i].gasType == 3)
        { // Hot gas
            currentBrightness *= (1.0f + illumination * 0.7f);
        }

        // Shock front particles are brighter and flicker more
        if (orionParticles[i].isShockFront)
        {
            currentBrightness *= (1.2f + 0.4f * sin(stellarWindPhase * 3));
        }

        currentBrightness = constrain(currentBrightness, 0.05f, 1.0f);

        // Apply brightness to color with enhanced color mixing
        uint16_t drawColor;
        if (currentBrightness >= 1.0f)
        {
            drawColor = orionParticles[i].color;
        }
        else
        {
            uint8_t r5 = (orionParticles[i].color >> 11) & 0x1F;
            uint8_t g6 = (orionParticles[i].color >> 5) & 0x3F;
            uint8_t b5 = orionParticles[i].color & 0x1F;

            uint8_t r_scaled = (uint8_t)(r5 * currentBrightness);
            uint8_t g_scaled = (uint8_t)(g6 * currentBrightness);
            uint8_t b_scaled = (uint8_t)(b5 * currentBrightness);

            drawColor = (r_scaled << 11) | (g_scaled << 5) | b_scaled;
        }

        // Draw enhanced particle
        if (drawX >= 0 && drawX < SCREEN_WIDTH && drawY >= 0 && drawY < SCREEN_HEIGHT)
        {
            tft.drawPixel(drawX, drawY, drawColor);
        }

        orionParticles[i].prevX = drawX;
        orionParticles[i].prevY = drawY;
    }

    // Draw protoplanetary disks (middle layer)
    for (int i = 0; i < MAX_PROTOPLANETARY_DISKS; i++)
    {
        if (!protoDisks[i].active)
            continue;

        // Update disk rotation
        protoDisks[i].diskAngle += protoDisks[i].rotationSpeed * deltaTime;
        if (protoDisks[i].diskAngle > 2 * PI)
            protoDisks[i].diskAngle -= 2 * PI;

        // Draw disk as ellipse
        float diskX = centerX + protoDisks[i].originalX * scale;
        float diskY = centerY + protoDisks[i].originalY * scale;

        // Draw simple disk representation (line segments)
        for (int j = 0; j < 8; j++)
        {
            float angle = j * PI / 4;
            float x1 = diskX + cos(angle + protoDisks[i].diskAngle) * protoDisks[i].diskRadius * scale * 0.7f;
            float y1 = diskY + sin(angle + protoDisks[i].diskAngle) * protoDisks[i].diskRadius * scale * 0.3f;

            if (x1 >= 0 && x1 < SCREEN_WIDTH && y1 >= 0 && y1 < SCREEN_HEIGHT)
            {
                uint16_t diskColor = protoDisks[i].diskColor;
                // Apply brightness
                uint8_t r5 = ((diskColor >> 11) & 0x1F) * protoDisks[i].brightness;
                uint8_t g6 = ((diskColor >> 5) & 0x3F) * protoDisks[i].brightness;
                uint8_t b5 = (diskColor & 0x1F) * protoDisks[i].brightness;
                uint16_t finalColor = (r5 << 11) | (g6 << 5) | b5;

                tft.drawPixel((int)x1, (int)y1, finalColor);
            }
        }
    }

    // Herbig-Haro jets removed for more realistic appearance

    // Draw stars on top (foreground layer)
    for (int i = 0; i < MAX_TRAPEZIUM_STARS + MAX_BACKGROUND_STARS; i++)
    {
        if (!orionStars[i].active)
            continue;

        orionStars[i].x = centerX + orionStars[i].originalX * scale;
        orionStars[i].y = centerY + orionStars[i].originalY * scale;

        int drawX = (int)orionStars[i].x;
        int drawY = (int)orionStars[i].y;

        // Erase previous position
        if (orionStars[i].prevX != drawX || orionStars[i].prevY != drawY)
        {
            if (orionStars[i].prevX >= 0 && orionStars[i].prevX < SCREEN_WIDTH &&
                orionStars[i].prevY >= 0 && orionStars[i].prevY < SCREEN_HEIGHT)
            {
                int eraseSize = orionStars[i].size + 2;
                for (int ex = -eraseSize; ex <= eraseSize; ex++)
                {
                    for (int ey = -eraseSize; ey <= eraseSize; ey++)
                    {
                        int px = orionStars[i].prevX + ex;
                        int py = orionStars[i].prevY + ey;
                        if (px >= 0 && px < SCREEN_WIDTH && py >= 0 && py < SCREEN_HEIGHT)
                        {
                            tft.drawPixel(px, py, BG_COLOR);
                        }
                    }
                }
            }
        }

        // Enhanced twinkling for different star types
        orionStars[i].twinklePhase += orionStars[i].twinkleSpeed * deltaTime;
        if (orionStars[i].twinklePhase > 2 * PI)
        {
            orionStars[i].twinklePhase -= 2 * PI;
        }

        // Calculate brightness with star type effects
        float twinkleFactor;
        if (orionStars[i].isTrapezium)
        {
            // Trapezium stars are more stable but still twinkle
            twinkleFactor = 0.85f + 0.15f * sin(orionStars[i].twinklePhase);
        }
        else if (orionStars[i].isProtostar)
        {
            // Protostars are variable and flicker more
            twinkleFactor = 0.6f + 0.4f * sin(orionStars[i].twinklePhase + stellarWindPhase);
        }
        else
        {
            // Normal background stars
            twinkleFactor = 0.7f + 0.3f * sin(orionStars[i].twinklePhase);
        }

        float currentBrightness = orionStars[i].brightness * twinkleFactor;

        // Trapezium stars are intrinsically brighter
        if (orionStars[i].isTrapezium)
        {
            currentBrightness *= 1.5f;
        }

        currentBrightness = constrain(currentBrightness, 0.1f, 1.0f);

        // Apply brightness to color
        uint16_t drawColor;
        if (currentBrightness >= 1.0f)
        {
            drawColor = orionStars[i].color;
        }
        else
        {
            uint8_t r5 = (orionStars[i].color >> 11) & 0x1F;
            uint8_t g6 = (orionStars[i].color >> 5) & 0x3F;
            uint8_t b5 = orionStars[i].color & 0x1F;

            uint8_t r_scaled = (uint8_t)(r5 * currentBrightness);
            uint8_t g_scaled = (uint8_t)(g6 * currentBrightness);
            uint8_t b_scaled = (uint8_t)(b5 * currentBrightness);

            drawColor = (r_scaled << 11) | (g_scaled << 5) | b_scaled;
        }

        // Draw enhanced star
        if (drawX >= 0 && drawX < SCREEN_WIDTH && drawY >= 0 && drawY < SCREEN_HEIGHT)
        {
            if (orionStars[i].size == 1)
            {
                tft.drawPixel(drawX, drawY, drawColor);
            }
            else
            {
                int radius = orionStars[i].size / 2;
                tft.fillCircle(drawX, drawY, radius, drawColor);

                // Enhanced diffraction spikes for bright stars
                if (orionStars[i].isTrapezium && currentBrightness > 0.8f)
                {
                    int spikeLength = radius + 3 + (orionStars[i].mass > 20.0f ? 2 : 0);

                    // Draw main spikes
                    tft.drawLine(drawX - spikeLength, drawY, drawX + spikeLength, drawY, drawColor);
                    tft.drawLine(drawX, drawY - spikeLength, drawX, drawY + spikeLength, drawColor);

                    // Draw diagonal spikes for very massive stars
                    if (orionStars[i].mass > 25.0f)
                    {
                        int diagSpike = spikeLength * 0.7f;
                        tft.drawLine(drawX - diagSpike, drawY - diagSpike, drawX + diagSpike, drawY + diagSpike, drawColor);
                        tft.drawLine(drawX - diagSpike, drawY + diagSpike, drawX + diagSpike, drawY - diagSpike, drawColor);
                    }
                }

                // Protostars get a subtle orange halo
                if (orionStars[i].isProtostar && currentBrightness > 0.6f)
                {
                    uint16_t haloColor = 0xFD20; // Orange
                    uint8_t halo_r5 = ((haloColor >> 11) & 0x1F) * 0.4f;
                    uint8_t halo_g6 = ((haloColor >> 5) & 0x3F) * 0.4f;
                    uint8_t halo_b5 = (haloColor & 0x1F) * 0.4f;
                    uint16_t dimHalo = (halo_r5 << 11) | (halo_g6 << 5) | halo_b5;

                    tft.drawCircle(drawX, drawY, radius + 1, dimHalo);
                }
            }
        }

        orionStars[i].prevX = drawX;
        orionStars[i].prevY = drawY;
    }
}

void eraseOrion()
{
    if (!orionInitialized)
        return;

    // Full screen clear for faster erase
    tft.fillScreen(BG_COLOR);

    // Free allocated memory if it exists (already present, ensure it's here)
    if (orionParticles != nullptr)
    {
        heap_caps_free(orionParticles);
        orionParticles = nullptr;
        numOrionParticles = 0; // Reset count
        Serial.println("Freed PSRAM for Orion particles.");
    }

    orionInitialized = false;
}

#endif // ORION_NEBULA_H