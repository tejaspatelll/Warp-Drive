#ifndef PULSAR_H
#define PULSAR_H

#include <TFT_eSPI.h>

// Forward declarations of external variables and constants
extern TFT_eSPI tft;
extern uint16_t BG_COLOR;
extern const int SCREEN_WIDTH;
extern const int SCREEN_HEIGHT;
extern int objectX, objectY;
extern float objectScale;

// Pulsar parameters
const float ROTATION_PERIOD = 2000.0f; // ms for one full rotation

// Variables visible only within this module
namespace {
    bool pulsarInitialized = false;
    int pulsarRadius = 0;
    int prevPulsarX = 0, prevPulsarY = 0;
    float prevAngle = 0;
    uint8_t intensityMap[128]; // Pre-calculated intensity map for beams
}

// Function prototypes for internal functions
void drawPulsarBeam(int centerX, int centerY, float angle, int baseRadius, float scale, float intensity, int maxLength);
void drawPulsarRipple(int centerX, int centerY, int distance, float angle, float scale, float intensity, float distFactor);
void erasePulsarBeam(int centerX, int centerY, float angle, int baseRadius, float scale, int maxLength);
void erasePulsarRipple(int centerX, int centerY, int distance, float angle, float scale, float distFactor);
void precalculateIntensityMap();

void precalculateIntensityMap() {
    for (int i = 0; i < 128; i++) {
        float distFactor = (float)i / 128.0f;
        intensityMap[i] = (uint8_t)(255 * (1.0f - distFactor * distFactor));
    }
}

/**
 * Draws a pulsar - a rapidly rotating neutron star that emits beams of radiation
 */
void drawPulsar() {
    if (!pulsarInitialized) {
        precalculateIntensityMap();
        pulsarInitialized = true;
    }

    int centerX = objectX;
    int centerY = objectY;
    float scale = objectScale;
    pulsarRadius = 6 * scale;
    unsigned long currentTime = millis();

    // Calculate current angle for continuous rotation
    float currentAngle = (currentTime % (unsigned long)ROTATION_PERIOD) / ROTATION_PERIOD * 2 * PI;

    // Calculate parameters for beam and core
    float time = currentTime / 1000.0f; // seconds
    float intensity = 0.5f + 0.5f * sin(time); // 0-1 intensity for beams
    int maxBeamLength = max(
        max(centerX, SCREEN_WIDTH - centerX),
        max(centerY, SCREEN_HEIGHT - centerY)
    ) + 10;

    // Always erase previous beams before drawing new ones
    erasePulsarBeam(centerX, centerY, prevAngle, pulsarRadius, scale, maxBeamLength);
    erasePulsarBeam(centerX, centerY, prevAngle + PI, pulsarRadius, scale, maxBeamLength);
    
    // Update previous position and angle
    prevPulsarX = centerX;
    prevPulsarY = centerY;
    prevAngle = currentAngle;

    // Draw the pulsar's core
    uint16_t coreColor = tft.color565(200, 200, 255); // Bright blue-white
    tft.fillCircle(centerX, centerY, pulsarRadius, coreColor);

    // Draw corona around the core with 3D-like effect
    for (int i = 0; i < 3; i++) {
        uint8_t brightness = map(i, 0, 2, 180, 100);
        uint16_t coronaColor = tft.color565(brightness, brightness, 255);
        tft.drawCircle(centerX, centerY, pulsarRadius + i, coronaColor);
    }

    // Draw the two radiation beams with intensity variation
    drawPulsarBeam(centerX, centerY, currentAngle, pulsarRadius, scale, intensity, maxBeamLength);
    drawPulsarBeam(centerX, centerY, currentAngle + PI, pulsarRadius, scale, intensity, maxBeamLength);

    // Pulse the core for a realistic effect
    float pulseFactor = 0.8f + 0.2f * sin(time * 3.0f);
    uint16_t corePulseColor = tft.color565(
        200 * pulseFactor,
        200 * pulseFactor,
        255 * pulseFactor
    );
    tft.fillCircle(centerX, centerY, pulsarRadius - 2, corePulseColor);

    // Update previous angle for the next frame
    prevAngle = currentAngle;
}

/**
 * Draws a single pulsar radiation beam
 */
void drawPulsarBeam(int centerX, int centerY, float angle, int baseRadius, float scale, float intensity, int maxLength) {
    float cosAngle = cos(angle);
    float sinAngle = sin(angle);

    for (int r = baseRadius; r < maxLength; r += 1) {
        float distFactor = min(1.0f, 2.0f * (1.0f - (float)(r - baseRadius) / maxLength));
        uint8_t beamIntensity = (uint8_t)(intensityMap[min(r, 127)] * intensity);

        int x = centerX + cosAngle * r;
        int y = centerY + sinAngle * r;

        if (x >= 0 && x < SCREEN_WIDTH && y >= 0 && y < SCREEN_HEIGHT) {
            uint16_t beamColor = tft.color565(beamIntensity, beamIntensity, 255);
            tft.drawPixel(x, y, beamColor);

            // Add a simple 3D-like effect by drawing a faint trail
            if (r > baseRadius + 5) {
                uint16_t trailColor = tft.color565(beamIntensity / 2, beamIntensity / 2, 255);
                tft.drawPixel(x - cosAngle, y - sinAngle, trailColor);
            }
        }

        // Add ripple effect at intervals
        if (r % 15 == 0 && r > baseRadius + 15) {
            drawPulsarRipple(centerX, centerY, r, angle, scale, intensity * 0.7f, distFactor);
        }
    }
}

/**
 * Draws a ripple effect perpendicular to the beam
 */
void drawPulsarRipple(int centerX, int centerY, int distance, float angle, float scale, float intensity, float distFactor) {
    float rippleWidth = 6.0f * scale * distFactor;
    float perpAngle = angle + PI / 2.0f;

    for (float w = 0; w <= rippleWidth; w += 0.5f) {
        float rippleFactor = (1.0f - (w / rippleWidth)) * intensity;
        if (rippleFactor < 0.05f) continue;

        for (int s = -1; s <= 1; s += 2) {
            int offsetX = cos(perpAngle) * w * s;
            int offsetY = sin(perpAngle) * w * s;
            int x = centerX + cos(angle) * distance + offsetX;
            int y = centerY + sin(angle) * distance + offsetY;

            if (x >= 0 && x < SCREEN_WIDTH && y >= 0 && y < SCREEN_HEIGHT) {
                uint16_t rippleColor = tft.color565(
                    80 * rippleFactor,
                    80 * rippleFactor,
                    255 * rippleFactor
                );
                tft.drawPixel(x, y, rippleColor);
            }
        }
    }
}

// Erase functions remain largely the same
void erasePulsarBeam(int centerX, int centerY, float angle, int baseRadius, float scale, int maxLength) {
    float cosAngle = cos(angle);
    float sinAngle = sin(angle);

    for (int r = baseRadius; r < maxLength; r += 1) {
        int x = centerX + cosAngle * r;
        int y = centerY + sinAngle * r;

        // Erase pixels within a slightly larger area to cover the beam's width
        for (int dx = -1; dx <= 1; dx++) {
            for (int dy = -1; dy <= 1; dy++) {
                int eraseX = x + dx;
                int eraseY = y + dy;

                if (eraseX >= 0 && eraseX < SCREEN_WIDTH &&
                    eraseY >= 0 && eraseY < SCREEN_HEIGHT) {
                    tft.drawPixel(eraseX, eraseY, BG_COLOR);
                }
            }
        }

        if (r % 15 == 0 && r > baseRadius + 15) {
            erasePulsarRipple(centerX, centerY, r, angle, scale, 1.0f);
        }
    }
}


void erasePulsarRipple(int centerX, int centerY, int distance, float angle, float scale, float distFactor) {
    float rippleWidth = 6.0f * scale * distFactor;
    float perpAngle = angle + PI / 2.0f;

    for (float w = 0; w <= rippleWidth; w += 0.5f) {
        for (int s = -1; s <= 1; s += 2) {
            int offsetX = cos(perpAngle) * w * s;
            int offsetY = sin(perpAngle) * w * s;
            int x = centerX + cos(angle) * distance + offsetX;
            int y = centerY + sin(angle) * distance + offsetY;

            if (x >= 0 && x < SCREEN_WIDTH && y >= 0 && y < SCREEN_HEIGHT) {
                tft.drawPixel(x, y, BG_COLOR);
            }
        }
    }
}

void erasePulsar() {
    if (pulsarInitialized) {
        tft.fillCircle(prevPulsarX, prevPulsarY, pulsarRadius + 3, BG_COLOR);
        int maxRadius = max(
            max(prevPulsarX, SCREEN_WIDTH - prevPulsarX),
            max(prevPulsarY, SCREEN_HEIGHT - prevPulsarY)
        ) + 10;
        erasePulsarBeam(prevPulsarX, prevPulsarY, prevAngle, pulsarRadius, objectScale, maxRadius);
        erasePulsarBeam(prevPulsarX, prevPulsarY, prevAngle + PI, pulsarRadius, objectScale, maxRadius);

        pulsarInitialized = false;
    }
}

#endif // PULSAR_H
