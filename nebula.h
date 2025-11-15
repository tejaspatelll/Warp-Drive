/**
 * nebula.h
 *
 * This file contains all code related to drawing and managing a nebula
 * object in the Warp Drive visualization.
 */

#ifndef NEBULA_H
#define NEBULA_H

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

// Helper function declarations
extern int scale_i(int v);
extern int red(uint16_t color);
extern int green(uint16_t color);
extern int blue(uint16_t color);

// Define PI if not already defined
#ifndef PI
#define PI 3.14159265358979323846
#endif

// Forward declarations
void drawNebula();
void eraseNebula();

// Enhanced nebula constants
#define MAX_NEBULA_PARTICLES 600 // Increased to 600 for better detail
#define MAX_NEBULA_CORES 3       // Multiple cores for complex structure
#define MAX_DUST_LANES 3         // Dark dust lanes for realism
#define MAX_NEBULA_FILAMENTS 6   // Filamentary structures in nebula
#define MAX_HALO_RADIUS 4        // Maximum halo radius to limit computational load

// Use static allocation instead of dynamic to prevent memory issues
#define USE_STATIC_ALLOCATION true

// Nebula path structures
struct NebulaFilament
{
    float centerX, centerY; // Filament center position
    float length, width;    // Filament dimensions
    float angle;            // Filament orientation
    float curvature;        // Filament curvature
    float temperature;      // Filament temperature affects color
};

struct NebulaParticle
{
    float x, y;
    float vx, vy;
    float density;     // Particle density affects brightness
    float temperature; // Temperature affects color
    uint16_t color;
    int radius;     // Core radius (center pixel)
    float haloSize; // Size of the halo effect
    int prevX, prevY;
    bool isDustLane;        // Whether this particle is part of a dark dust lane
    int filamentIndex;      // Which filament it belongs to (-1 if none)
    float filamentPosition; // Position along the filament (0.0 to 1.0)
};

struct NebulaCore
{
    float x, y;        // Core position
    float temperature; // Core temperature affects surrounding particles
    float intensity;   // Core brightness
    float radius;      // Influence radius
    float pulseRate;   // Individual pulse rate for this core
    float pulsePhase;  // Phase offset for pulsing
};

// Global variables - statically allocated if configured
#if USE_STATIC_ALLOCATION
static NebulaParticle nebulaParticles[MAX_NEBULA_PARTICLES];
static NebulaCore nebulaCores[MAX_NEBULA_CORES];
static NebulaFilament nebulaFilaments[MAX_NEBULA_FILAMENTS];
#else
static NebulaParticle *nebulaParticles = nullptr;
static NebulaCore *nebulaCores = nullptr;
static NebulaFilament *nebulaFilaments = nullptr;
#endif

extern bool nebulaInitialized;

// For batch processing - reduced for performance
#define PARTICLES_PER_BATCH 30

// Lookup table for sqrt to improve performance
#define SQRT_TABLE_SIZE 100
static float sqrtTable[SQRT_TABLE_SIZE];
void initSqrtTable()
{
    for (int i = 0; i < SQRT_TABLE_SIZE; i++)
    {
        sqrtTable[i] = sqrt(i / (float)(SQRT_TABLE_SIZE - 1) * MAX_HALO_RADIUS * MAX_HALO_RADIUS);
    }
}

// Fast approximation of sqrt for small values
float fastSqrt(float value)
{
    if (value <= 0)
        return 0;
    if (value >= MAX_HALO_RADIUS * MAX_HALO_RADIUS)
        return MAX_HALO_RADIUS;

    int index = (int)(value / (MAX_HALO_RADIUS * MAX_HALO_RADIUS) * (SQRT_TABLE_SIZE - 1));
    index = constrain(index, 0, SQRT_TABLE_SIZE - 1);
    return sqrtTable[index];
}

// Color temperature mapping (Blackbody radiation approximation)
uint16_t getColorFromTemperature(float temp, float density)
{
    int r, g, b;

    // Temperature ranges from 0 (cool) to 1 (hot)
    if (temp < 0.3)
    {
        // Cool reds and purples
        r = 255 * temp * 3;
        g = 0;
        b = 100 * temp;
    }
    else if (temp < 0.6)
    {
        // Transition to blues
        r = 150 - (temp - 0.3) * 200;
        g = (temp - 0.3) * 200;
        b = 150 + (temp - 0.3) * 200;
    }
    else
    {
        // Hot blues and whites
        r = (temp - 0.6) * 400;
        g = 120 + (temp - 0.6) * 300;
        b = 255;
    }

    // Apply density modulation
    float brightness = constrain(density * 1.2f, 0.2f, 1.0f);
    r = constrain((int)(r * brightness), 0, 255);
    g = constrain((int)(g * brightness), 0, 255);
    b = constrain((int)(b * brightness), 0, 255);

    return tft.color565(r, g, b);
}

// Get color with adjusted alpha for halo effect
uint16_t getHaloColor(uint16_t baseColor, float alpha)
{
    // Extract RGB components
    int r = ((baseColor >> 11) & 0x1F) * 8;
    int g = ((baseColor >> 5) & 0x3F) * 4;
    int b = (baseColor & 0x1F) * 8;

    // Apply alpha blending with background
    int bgR = red(BG_COLOR);
    int bgG = green(BG_COLOR);
    int bgB = blue(BG_COLOR);

    r = bgR + (r - bgR) * alpha;
    g = bgG + (g - bgG) * alpha;
    b = bgB + (b - bgB) * alpha;

    return tft.color565(r, g, b);
}

// Optimize halo by using a more efficient circle drawing technique
// Pre-computed halo offsets for up to MAX_HALO_RADIUS
#define MAX_HALO_POINTS (MAX_HALO_RADIUS * 8)
struct HaloPoint
{
    int8_t dx, dy;
    float dist;
};
static HaloPoint haloOffsets[MAX_HALO_POINTS];
static int haloPointCount = 0;

void initHaloOffsets()
{
    haloPointCount = 0;

    // Generate points for all possible halos up to MAX_HALO_RADIUS
    for (int8_t dy = -MAX_HALO_RADIUS; dy <= MAX_HALO_RADIUS; dy++)
    {
        for (int8_t dx = -MAX_HALO_RADIUS; dx <= MAX_HALO_RADIUS; dx++)
        {
            float dist = sqrt(dx * dx + dy * dy);
            if (dist <= MAX_HALO_RADIUS)
            {
                haloOffsets[haloPointCount].dx = dx;
                haloOffsets[haloPointCount].dy = dy;
                haloOffsets[haloPointCount].dist = dist;
                haloPointCount++;

                if (haloPointCount >= MAX_HALO_POINTS)
                {
                    return; // Safety check
                }
            }
        }
    }
}

// Calculate position along a filament
void calculateFilamentPosition(NebulaFilament &filament, float position, float offset, float &x, float &y)
{
    // Position goes from 0.0 to 1.0 along the filament
    // Offset is perpendicular to the filament (-1.0 to 1.0)

    // Calculate the base position along the filament with curvature
    float angle = filament.angle;
    float curveFactor = (position - 0.5) * filament.curvature;
    angle += curveFactor;

    float distanceFromCenter = (position - 0.5) * filament.length;
    float baseX = filament.centerX + cos(angle) * distanceFromCenter;
    float baseY = filament.centerY + sin(angle) * distanceFromCenter;

    // Add perpendicular offset
    float perpAngle = angle + PI / 2;
    float perpDistance = offset * filament.width * (0.5 - abs(position - 0.5)); // Narrower at the ends

    x = baseX + cos(perpAngle) * perpDistance;
    y = baseY + sin(perpAngle) * perpDistance;
}

void drawNebula()
{
    int centerX = objectX;
    int centerY = objectY;
    float scale = objectScale * scaleFactor;

    // Initialize tables if needed
    static bool tablesInitialized = false;
    if (!tablesInitialized)
    {
        initSqrtTable();
        initHaloOffsets();
        tablesInitialized = true;
    }

    // Initialize nebula if not already done OR if scale changed significantly
    static float prevScale = -1.0f;
    bool scaleChanged = abs(scale - prevScale) > 0.1f;

    if (!nebulaInitialized || scaleChanged)
    {
#if !USE_STATIC_ALLOCATION
        // Try to use PSRAM if available
        if (ESP.getFreePsram() > sizeof(NebulaParticle) * MAX_NEBULA_PARTICLES +
                                     sizeof(NebulaCore) * MAX_NEBULA_CORES +
                                     sizeof(NebulaFilament) * MAX_NEBULA_FILAMENTS)
        {
            nebulaParticles = (NebulaParticle *)ps_malloc(sizeof(NebulaParticle) * MAX_NEBULA_PARTICLES);
            nebulaCores = (NebulaCore *)ps_malloc(sizeof(NebulaCore) * MAX_NEBULA_CORES);
            nebulaFilaments = (NebulaFilament *)ps_malloc(sizeof(NebulaFilament) * MAX_NEBULA_FILAMENTS);
        }
        else
        {
            // Fall back to heap memory
            nebulaParticles = (NebulaParticle *)malloc(sizeof(NebulaParticle) * MAX_NEBULA_PARTICLES);
            nebulaCores = (NebulaCore *)malloc(sizeof(NebulaCore) * MAX_NEBULA_CORES);
            nebulaFilaments = (NebulaFilament *)malloc(sizeof(NebulaFilament) * MAX_NEBULA_FILAMENTS);
        }

        // Check if memory allocation was successful
        if (!nebulaParticles || !nebulaCores || !nebulaFilaments)
        {
            // Memory allocation failed - display error and abort
            if (nebulaParticles)
                free(nebulaParticles);
            if (nebulaCores)
                free(nebulaCores);
            if (nebulaFilaments)
                free(nebulaFilaments);

            nebulaParticles = nullptr;
            nebulaCores = nullptr;
            nebulaFilaments = nullptr;
            return;
        }
#endif

        // Initialize cores with varying properties
        for (int i = 0; i < MAX_NEBULA_CORES; i++)
        {
            nebulaCores[i].x = centerX + scale_i(random(-30, 30)) * scale;
            nebulaCores[i].y = centerY + scale_i(random(-30, 30)) * scale;
            nebulaCores[i].temperature = random(60, 100) / 100.0f;
            nebulaCores[i].intensity = random(70, 100) / 100.0f;
            nebulaCores[i].radius = scale_i(random(15, 25)) * scale;
            nebulaCores[i].pulseRate = 1.0 + random(200) / 100.0f; // 1.0-3.0
            nebulaCores[i].pulsePhase = random(628) / 100.0f;      // 0-2π
        }

        // Initialize filamentary structures
        for (int i = 0; i < MAX_NEBULA_FILAMENTS; i++)
        {
            // Choose a core to anchor this filament
            int coreIndex = random(MAX_NEBULA_CORES);
            nebulaFilaments[i].centerX = nebulaCores[coreIndex].x;
            nebulaFilaments[i].centerY = nebulaCores[coreIndex].y;
            nebulaFilaments[i].angle = random(628) / 100.0f; // 0-2π
            nebulaFilaments[i].length = nebulaCores[coreIndex].radius * (1.5 + random(100) / 100.0f);
            nebulaFilaments[i].width = nebulaCores[coreIndex].radius * (0.3 + random(50) / 100.0f);
            nebulaFilaments[i].curvature = (random(200) - 100) / 100.0f * PI / 2; // -π/2 to π/2
            nebulaFilaments[i].temperature = nebulaCores[coreIndex].temperature * (0.7 + random(60) / 100.0f);
        }

        // Initialize particles - distribute between cores and filaments
        int filamentParticles = MAX_NEBULA_PARTICLES * 0.4; // 40% on filaments
        int coreParticles = MAX_NEBULA_PARTICLES - filamentParticles;

        // Core-based particles (spherical distribution)
        for (int i = 0; i < coreParticles; i++)
        {
            // Randomly assign particle to a core's influence
            int coreIndex = random(MAX_NEBULA_CORES);
            float angle = random(360) * PI / 180.0;

            // Use gaussian-like distribution (denser toward center)
            float r1 = random(100) / 100.0;
            float r2 = random(100) / 100.0;
            float dist = nebulaCores[coreIndex].radius * sqrt(-2 * log(r1)) * cos(2 * PI * r2) * 0.4;
            dist = constrain(dist, 0, nebulaCores[coreIndex].radius * 1.2);

            nebulaParticles[i].x = nebulaCores[coreIndex].x + cos(angle) * dist;
            nebulaParticles[i].y = nebulaCores[coreIndex].y + sin(angle) * dist;

            // Initialize velocity (slow, swirling motion)
            float speed = random(1, 5) / 1000.0;
            nebulaParticles[i].vx = cos(angle + PI / 2) * speed; // Tangential velocity
            nebulaParticles[i].vy = sin(angle + PI / 2) * speed;

            // Set particle properties
            nebulaParticles[i].density = random(60, 100) / 100.0f;
            nebulaParticles[i].temperature = nebulaCores[coreIndex].temperature * (0.7 + random(30) / 100.0f);
            nebulaParticles[i].radius = 1; // Core pixel size

            // Limit halo size for performance
            float haloRand = random(100);
            nebulaParticles[i].haloSize = haloRand < 60 ? 1.0 + random(min(20, MAX_HALO_RADIUS * 10 - 10)) / 10.0 : 0; // 60% with halo

            // Limit maximum halo size
            nebulaParticles[i].haloSize = min(nebulaParticles[i].haloSize, float(MAX_HALO_RADIUS));

            nebulaParticles[i].isDustLane = random(100) < 15; // 15% dust lanes
            nebulaParticles[i].filamentIndex = -1;            // Not on a filament

            // Initialize previous position
            nebulaParticles[i].prevX = -1;
            nebulaParticles[i].prevY = -1;
        }

        // Filament-based particles
        for (int i = coreParticles; i < MAX_NEBULA_PARTICLES; i++)
        {
            int filamentIndex = random(MAX_NEBULA_FILAMENTS);
            float position = random(100) / 100.0;       // Position along filament (0.0-1.0)
            float offset = (random(200) - 100) / 100.0; // Perpendicular offset (-1.0 to 1.0)

            // Calculate position along filament
            float x, y;
            calculateFilamentPosition(nebulaFilaments[filamentIndex], position, offset, x, y);

            nebulaParticles[i].x = x;
            nebulaParticles[i].y = y;
            nebulaParticles[i].filamentIndex = filamentIndex;
            nebulaParticles[i].filamentPosition = position;

            // Small random motion
            float angle = random(360) * PI / 180.0;
            float speed = random(1, 3) / 1000.0;
            nebulaParticles[i].vx = cos(angle) * speed;
            nebulaParticles[i].vy = sin(angle) * speed;

            // Set particle properties
            nebulaParticles[i].density = random(70, 100) / 100.0f;
            nebulaParticles[i].temperature = nebulaFilaments[filamentIndex].temperature * (0.9 + random(20) / 100.0f);
            nebulaParticles[i].radius = 1; // Core pixel size

            // Limit halo size for performance
            nebulaParticles[i].haloSize = 1.0 + random(min(30, MAX_HALO_RADIUS * 10)) / 10.0;
            nebulaParticles[i].haloSize = min(nebulaParticles[i].haloSize, float(MAX_HALO_RADIUS));

            nebulaParticles[i].isDustLane = random(100) < 5; // 5% dust lanes

            // Initialize previous position
            nebulaParticles[i].prevX = -1;
            nebulaParticles[i].prevY = -1;
        }

        nebulaInitialized = true;
        prevScale = scale; // Store current scale for next comparison
    }

    // Animation timing
    static unsigned long lastUpdate = 0;
    unsigned long currentTime = millis();
    float deltaTime = (currentTime - lastUpdate) / 1000.0f;
    if (deltaTime > 0.1f)
        deltaTime = 0.1f; // Cap delta time to prevent jumps
    lastUpdate = currentTime;

    // Global nebula pulsing
    float globalPulse = (sin(currentTime / 3000.0f) + 1.0f) / 2.0f;

    // Process particles in batches for smooth animation
    static int startIndex = 0;
    int particlesToUpdate = min(PARTICLES_PER_BATCH, MAX_NEBULA_PARTICLES);

    // Update core pulses - only update every few frames for performance
    static int pulseFrameCounter = 0;
    if (pulseFrameCounter++ % 3 == 0)
    {
        for (int c = 0; c < MAX_NEBULA_CORES; c++)
        {
            nebulaCores[c].intensity = 0.7 + 0.3 * sin(currentTime / 1000.0f * nebulaCores[c].pulseRate + nebulaCores[c].pulsePhase);
        }
    }

    // Erase old positions - only for particles in the current batch
    for (int i = 0; i < particlesToUpdate; i++)
    {
        int index = (startIndex + i) % MAX_NEBULA_PARTICLES;
        if (nebulaParticles[index].prevX >= 0)
        {
            // Simple erase for standard particles
            if (nebulaParticles[index].haloSize <= 0)
            {
                tft.drawPixel(nebulaParticles[index].prevX,
                              nebulaParticles[index].prevY,
                              BG_COLOR);
            }
            // For halo particles, optimize erasing by using pre-computed points
            else
            {
                int x = nebulaParticles[index].prevX;
                int y = nebulaParticles[index].prevY;
                float haloSize = nebulaParticles[index].haloSize;

                // Use only the points we need based on halo size
                for (int p = 0; p < haloPointCount; p++)
                {
                    if (haloOffsets[p].dist <= haloSize)
                    {
                        int px = x + haloOffsets[p].dx;
                        int py = y + haloOffsets[p].dy;
                        // Only erase if within screen bounds
                        if (px >= 0 && px < SCREEN_WIDTH && py >= 0 && py < SCREEN_HEIGHT)
                        {
                            tft.drawPixel(px, py, BG_COLOR);
                        }
                    }
                }
            }
        }
    }

    // Update and draw new positions
    for (int i = 0; i < particlesToUpdate; i++)
    {
        int index = (startIndex + i) % MAX_NEBULA_PARTICLES;
        NebulaParticle &particle = nebulaParticles[index];

        // Different update logic based on whether particle is on a filament
        if (particle.filamentIndex >= 0)
        {
            // Filament-based particle - moves along filament
            particle.filamentPosition += (random(10) - 5) / 1000.0 * deltaTime * 60;
            particle.filamentPosition = constrain(particle.filamentPosition, 0.0, 1.0);

            // Recalculate position based on filament
            float offset = (sin(currentTime / (2000.0 + index % 500) + index) * 0.3) +
                           ((particle.filamentPosition - 0.5) * 0.5);

            calculateFilamentPosition(
                nebulaFilaments[particle.filamentIndex],
                particle.filamentPosition,
                offset,
                particle.x,
                particle.y);

            // Add small random motion
            particle.x += particle.vx * deltaTime * 30;
            particle.y += particle.vy * deltaTime * 30;
        }
        else
        {
            // Core-based particle - swirls around core
            // Update position with smooth motion
            particle.x += particle.vx * deltaTime * 60;
            particle.y += particle.vy * deltaTime * 60;

            // Apply influence from nearest core
            float minDist = 1000;
            int nearestCore = 0;
            for (int c = 0; c < MAX_NEBULA_CORES; c++)
            {
                float dx = nebulaCores[c].x - particle.x;
                float dy = nebulaCores[c].y - particle.y;
                float dist = sqrt(dx * dx + dy * dy);
                if (dist < minDist)
                {
                    minDist = dist;
                    nearestCore = c;
                }
            }

            // Adjust velocity based on core influence - creates swirling motion
            if (minDist < nebulaCores[nearestCore].radius)
            {
                float angle = atan2(particle.y - nebulaCores[nearestCore].y,
                                    particle.x - nebulaCores[nearestCore].x);

                // Adjust speed based on distance from core (faster closer to core)
                float speedFactor = 1.0 - (minDist / nebulaCores[nearestCore].radius);
                float baseSpeed = 0.0001 * nebulaCores[nearestCore].intensity;

                particle.vx += cos(angle + PI / 2) * baseSpeed * speedFactor;
                particle.vy += sin(angle + PI / 2) * baseSpeed * speedFactor;

                // Dampen velocity to prevent excessive speeds
                float velocityMag = sqrt(particle.vx * particle.vx + particle.vy * particle.vy);
                if (velocityMag > 0.01)
                {
                    particle.vx = particle.vx * 0.01 / velocityMag;
                    particle.vy = particle.vy * 0.01 / velocityMag;
                }
            }
        }

        // Calculate final color
        float effectiveTemp = particle.temperature;
        float effectiveDensity = particle.density *
                                 (0.7 + 0.3 * globalPulse) *
                                 (particle.isDustLane ? 0.2 : 1.0);

        // Apply core influence to brightness
        if (particle.filamentIndex < 0)
        { // Only for core-based particles
            for (int c = 0; c < MAX_NEBULA_CORES; c++)
            {
                float dx = nebulaCores[c].x - particle.x;
                float dy = nebulaCores[c].y - particle.y;
                float dist = sqrt(dx * dx + dy * dy);

                if (dist < nebulaCores[c].radius * 1.2)
                {
                    float influence = 1.0 - (dist / (nebulaCores[c].radius * 1.2));
                    effectiveDensity += influence * 0.3 * nebulaCores[c].intensity;
                    effectiveTemp = max(effectiveTemp,
                                        float(particle.temperature + influence * 0.2 * nebulaCores[c].temperature));
                }
            }
        }

        // Ensure we're in valid ranges
        effectiveDensity = constrain(effectiveDensity, 0.0, 1.0);
        effectiveTemp = constrain(effectiveTemp, 0.0, 1.0);

        // Draw particle with halo effect
        int x = round(particle.x);
        int y = round(particle.y);

        if (x >= 0 && x < SCREEN_WIDTH && y >= 0 && y < SCREEN_HEIGHT)
        {
            // Calculate base color
            uint16_t baseColor = getColorFromTemperature(effectiveTemp, effectiveDensity);

            // If we have a halo effect, draw it
            if (particle.haloSize > 0)
            {
                // Use the pre-computed halo points for faster drawing
                for (int p = 0; p < haloPointCount; p++)
                {
                    if (haloOffsets[p].dist <= particle.haloSize)
                    {
                        int px = x + haloOffsets[p].dx;
                        int py = y + haloOffsets[p].dy;

                        // Skip if outside screen bounds
                        if (px < 0 || px >= SCREEN_WIDTH || py < 0 || py >= SCREEN_HEIGHT)
                        {
                            continue;
                        }

                        // Calculate alpha based on distance from center
                        float alpha = haloOffsets[p].dist == 0 ? 1.0 : (1.0 - haloOffsets[p].dist / particle.haloSize);
                        alpha *= effectiveDensity; // Adjust by density

                        // Apply color with alpha
                        uint16_t color = getHaloColor(baseColor, alpha);
                        tft.drawPixel(px, py, color);
                    }
                }
            }
            else
            {
                // No halo, just draw center pixel
                tft.drawPixel(x, y, baseColor);
            }

            // Remember position for next erasure
            particle.prevX = x;
            particle.prevY = y;
        }
    }

    startIndex = (startIndex + particlesToUpdate) % MAX_NEBULA_PARTICLES;
}

void eraseNebula()
{
    if (nebulaInitialized)
    {
        // Only erase visible particles to save time
        for (int i = 0; i < MAX_NEBULA_PARTICLES; i++)
        {
            if (nebulaParticles[i].prevX >= 0 &&
                nebulaParticles[i].prevX < SCREEN_WIDTH &&
                nebulaParticles[i].prevY >= 0 &&
                nebulaParticles[i].prevY < SCREEN_HEIGHT)
            {

                if (nebulaParticles[i].haloSize <= 0)
                {
                    // Simple single pixel
                    tft.drawPixel(nebulaParticles[i].prevX, nebulaParticles[i].prevY, BG_COLOR);
                }
                else
                {
                    // Use the optimized halo erasing with pre-computed points
                    int x = nebulaParticles[i].prevX;
                    int y = nebulaParticles[i].prevY;
                    float haloSize = nebulaParticles[i].haloSize;

                    // Use only the points we need based on halo size
                    for (int p = 0; p < haloPointCount; p++)
                    {
                        if (haloOffsets[p].dist <= haloSize)
                        {
                            int px = x + haloOffsets[p].dx;
                            int py = y + haloOffsets[p].dy;
                            if (px >= 0 && px < SCREEN_WIDTH && py >= 0 && py < SCREEN_HEIGHT)
                            {
                                tft.drawPixel(px, py, BG_COLOR);
                            }
                        }
                    }
                }
            }
        }

#if !USE_STATIC_ALLOCATION
        // Free memory
        if (nebulaParticles)
            free(nebulaParticles);
        if (nebulaCores)
            free(nebulaCores);
        if (nebulaFilaments)
            free(nebulaFilaments);

        nebulaParticles = nullptr;
        nebulaCores = nullptr;
        nebulaFilaments = nullptr;
#endif
    }

    nebulaInitialized = false;
}

#endif // NEBULA_H
