#ifndef CELESTIAL_ANIMATIONS_H
#define CELESTIAL_ANIMATIONS_H

#include <Arduino.h>
#include <TFT_eSPI.h>

// Forward declarations
extern TFT_eSPI tft;
extern uint16_t BG_COLOR;
extern const int SCREEN_WIDTH;
extern const int SCREEN_HEIGHT;

// Double buffering for smooth animations
extern TFT_eSprite animationBuffer;
extern bool animationBufferInitialized;

// Animation types
enum class AnimationType
{
    NONE,
    SCALE_IN, // Zoom in from 0 to target scale (arriving at object)
    SCALE_OUT // Zoom out from target scale to larger scale (leaving object behind)
};

// Animation state
struct AnimationState
{
    AnimationType type = AnimationType::NONE;
    unsigned long startTime = 0;
    unsigned long duration = 1000; // Default 1 second
    bool isActive = false;
    bool isComplete = false;

    // Animation parameters (simplified for scale-only animations)
    float startScale = 1.0f;
    float endScale = 1.0f;

    // Current animation values
    float currentScale = 1.0f;

    // Speed control (from potentiometer)
    float speedMultiplier = 1.0f; // 0.5 to 2.0 range
};

// Global animation state
extern AnimationState arrivalAnimation;
extern AnimationState exitAnimation;

// Animation functions
void startArrivalAnimation(AnimationType type, unsigned long duration = 1000);
void startExitAnimation(AnimationType type, unsigned long duration = 1000);
void updateAnimations();
bool isArrivalComplete();
bool isExitComplete();
void resetAnimations();

// Easing functions
float easeInOutCubic(float t);
float easeOutCubic(float t);
float easeInCubic(float t);
float easeOutBounce(float t);
float easeInBounce(float t);

// Animation effect functions (simplified for scale-only)
void applyArrivalEffect(float &scale);
void applyExitEffect(float &scale);

// Double buffering functions
void initializeAnimationBuffer();
void clearAnimationBuffer();
void drawToAnimationBuffer();
void pushAnimationBuffer();

// Utility functions (simplified)
AnimationType getArrivalAnimation();
AnimationType getExitAnimation();

// Forward declaration for Star struct
struct Star;

// Speed control functions
void setAnimationSpeed(float potValue); // potValue 0-4095 from analogRead
float getAnimationSpeed();

// Seamless star parallax update for exit animations
// Updates stars during exit animation using potentiometer-matched warp speed
// This creates a seamless transition from celestial object to warp mode with no speed jumps
void updateStarParallax(Star stars[], int starCount, int centerX, int centerY, float animationProgress, float warpFactor);

#endif // CELESTIAL_ANIMATIONS_H
