#include "celestial_animations.h"
#include "star.h" // Need complete Star struct definition for updateStarParallax

// Forward declarations
extern const int SCREEN_WIDTH;
extern const int SCREEN_HEIGHT;

// Global animation state
AnimationState arrivalAnimation;
AnimationState exitAnimation;

// Double buffering for smooth animations
TFT_eSprite animationBuffer(&tft);
bool animationBufferInitialized = false;

// Animation configuration
const unsigned long DEFAULT_ARRIVAL_DURATION = 1500; // 1.5 seconds
const unsigned long DEFAULT_EXIT_DURATION = 700;     // Faster default exit (0.7s)
const float MIN_SCALE = 0.1f;
const float MAX_SCALE = 2.0f;

void startArrivalAnimation(AnimationType type, unsigned long duration)
{
    arrivalAnimation.type = type;
    arrivalAnimation.startTime = millis();
    arrivalAnimation.duration = duration > 0 ? duration : DEFAULT_ARRIVAL_DURATION;
    arrivalAnimation.isActive = true;
    arrivalAnimation.isComplete = false;

    // Prevent overlap with any in-progress exit animation
    exitAnimation.isActive = false;
    exitAnimation.isComplete = false;
    exitAnimation.type = AnimationType::NONE;

    // Set up animation parameters based on type (simplified for scale-only)
    switch (type)
    {
    case AnimationType::SCALE_IN:
        // Start from a small, non-zero scale to avoid a blank first frame
        arrivalAnimation.startScale = MIN_SCALE;
        arrivalAnimation.endScale = 1.0f; // Scale to full size
        break;

    default:
        arrivalAnimation.startScale = 1.0f;
        arrivalAnimation.endScale = 1.0f;
        break;
    }

    // Initialize current values
    arrivalAnimation.currentScale = arrivalAnimation.startScale;
}

void startExitAnimation(AnimationType type, unsigned long duration)
{
    exitAnimation.type = type;
    exitAnimation.startTime = millis();
    exitAnimation.duration = duration > 0 ? duration : DEFAULT_EXIT_DURATION;
    exitAnimation.isActive = true;
    exitAnimation.isComplete = false;

    // Prevent overlap with any in-progress arrival animation
    arrivalAnimation.isActive = false;
    arrivalAnimation.isComplete = false;
    arrivalAnimation.type = AnimationType::NONE;

    // Set up animation parameters based on type (simplified for scale-only)
    switch (type)
    {
    case AnimationType::SCALE_OUT:
        exitAnimation.startScale = 1.0f; // Start at full size
        exitAnimation.endScale = 3.0f;   // Scale up and disappear (like moving away)
        break;

    default:
        exitAnimation.startScale = 1.0f;
        exitAnimation.endScale = 1.0f;
        break;
    }

    // Initialize current values
    exitAnimation.currentScale = exitAnimation.startScale;
}

void updateAnimations()
{
    unsigned long currentTime = millis();

    // Update arrival animation
    if (arrivalAnimation.isActive && !arrivalAnimation.isComplete)
    {
        unsigned long elapsed = currentTime - arrivalAnimation.startTime;
        // Adjust duration based on speed multiplier (faster speed = shorter duration)
        float adjustedDuration = arrivalAnimation.duration / arrivalAnimation.speedMultiplier;
        float progress = (float)elapsed / adjustedDuration;

        if (progress >= 1.0f)
        {
            progress = 1.0f;
            arrivalAnimation.isComplete = true;
            arrivalAnimation.isActive = false;
        }

        // Apply easing for smooth scale animation
        float easedProgress = easeOutCubic(progress);

        // Update current scale
        arrivalAnimation.currentScale = arrivalAnimation.startScale +
                                        (arrivalAnimation.endScale - arrivalAnimation.startScale) * easedProgress;
        // Clamp to safe range to avoid under/overshoot artifacts
        if (arrivalAnimation.currentScale < MIN_SCALE)
        {
            arrivalAnimation.currentScale = MIN_SCALE;
        }
        else if (arrivalAnimation.currentScale > MAX_SCALE)
        {
            arrivalAnimation.currentScale = MAX_SCALE;
        }
    }

    // Update exit animation
    if (exitAnimation.isActive && !exitAnimation.isComplete)
    {
        unsigned long elapsed = currentTime - exitAnimation.startTime;
        // Adjust duration based on speed multiplier (faster speed = shorter duration)
        float adjustedDuration = exitAnimation.duration / exitAnimation.speedMultiplier;
        float progress = (float)elapsed / adjustedDuration;

        if (progress >= 1.0f)
        {
            progress = 1.0f;
            exitAnimation.isComplete = true;
            exitAnimation.isActive = false;
        }

        // Apply easing for smooth scale animation
        float easedProgress = easeInCubic(progress);

        // Update current scale
        exitAnimation.currentScale = exitAnimation.startScale +
                                     (exitAnimation.endScale - exitAnimation.startScale) * easedProgress;
        // Clamp exit scale to avoid runaway values due to timing jitter
        if (exitAnimation.currentScale < 1.0f)
        {
            exitAnimation.currentScale = 1.0f;
        }
        else if (exitAnimation.currentScale > MAX_SCALE)
        {
            exitAnimation.currentScale = MAX_SCALE;
        }
    }
}

bool isArrivalComplete()
{
    return arrivalAnimation.isComplete;
}

bool isExitComplete()
{
    return exitAnimation.isComplete;
}

void resetAnimations()
{
    arrivalAnimation.isActive = false;
    arrivalAnimation.isComplete = false;
    arrivalAnimation.type = AnimationType::NONE;

    exitAnimation.isActive = false;
    exitAnimation.isComplete = false;
    exitAnimation.type = AnimationType::NONE;
}

// Easing functions
float easeInOutCubic(float t)
{
    return t < 0.5f ? 4.0f * t * t * t : 1.0f - powf(-2.0f * t + 2.0f, 3) / 2.0f;
}

float easeOutCubic(float t)
{
    return 1.0f - powf(1.0f - t, 3);
}

float easeInCubic(float t)
{
    return t * t * t;
}

float easeOutBounce(float t)
{
    const float n1 = 7.5625f;
    const float d1 = 2.75f;

    if (t < 1.0f / d1)
    {
        return n1 * t * t;
    }
    else if (t < 2.0f / d1)
    {
        return n1 * (t -= 1.5f / d1) * t + 0.75f;
    }
    else if (t < 2.5f / d1)
    {
        return n1 * (t -= 2.25f / d1) * t + 0.9375f;
    }
    else
    {
        return n1 * (t -= 2.625f / d1) * t + 0.984375f;
    }
}

float easeInBounce(float t)
{
    return 1.0f - easeOutBounce(1.0f - t);
}

void applyArrivalEffect(float &scale)
{
    if (arrivalAnimation.isActive && !arrivalAnimation.isComplete)
    {
        updateAnimations();

        // Apply scale multiplier for zoom-in effect
        scale *= arrivalAnimation.currentScale;
    }
}

void applyExitEffect(float &scale)
{
    if (exitAnimation.isActive && !exitAnimation.isComplete)
    {
        updateAnimations();

        // Apply scale multiplier for zoom-out effect
        scale *= exitAnimation.currentScale;
    }
}

AnimationType getArrivalAnimation()
{
    return AnimationType::SCALE_IN;
}

AnimationType getExitAnimation()
{
    return AnimationType::SCALE_OUT;
}

// Double buffering functions
void initializeAnimationBuffer()
{
    if (!animationBufferInitialized)
    {
        animationBuffer.createSprite(SCREEN_WIDTH, SCREEN_HEIGHT);
        animationBuffer.setColorDepth(16);
        animationBuffer.fillSprite(BG_COLOR);
        animationBufferInitialized = true;
        Serial.println("[Animation] Double buffer initialized");
    }
}

void clearAnimationBuffer()
{
    if (animationBufferInitialized)
    {
        animationBuffer.fillSprite(BG_COLOR);
    }
}

void drawToAnimationBuffer()
{
    // This function will be called by the main drawing function
    // to redirect drawing to the animation buffer instead of the main screen
    // The actual redirection happens by temporarily replacing the global tft object
}

void pushAnimationBuffer()
{
    if (animationBufferInitialized)
    {
        animationBuffer.pushSprite(0, 0);
    }
}

// Speed control functions
static float currentAnimationSpeed = 1.0f;

void setAnimationSpeed(float potValue)
{
    // Map potentiometer value (0-4095) to speed multiplier (0.3 to 3.0)
    // Lower pot value = slower speed, higher pot value = faster speed
    currentAnimationSpeed = 0.3f + (potValue / 4095.0f) * 2.7f;

    // Update both animation states
    arrivalAnimation.speedMultiplier = currentAnimationSpeed;
    exitAnimation.speedMultiplier = currentAnimationSpeed;
}

float getAnimationSpeed()
{
    return currentAnimationSpeed;
}

void updateStarParallax(Star stars[], int starCount, int centerX, int centerY, float animationProgress, float warpFactor)
{
    // SEAMLESS EXIT ANIMATION STAR MOVEMENT
    // Uses the exact same warp mode star animation system for exit animations
    // This creates a seamless transition with stars moving at potentiometer-matched speeds

    // Only apply warp effect when exit animation is active
    if (!exitAnimation.isActive)
    {
        return;
    }

    // Use the same warp center as the main warp mode (center of screen)
    const float warpCenterX = SCREEN_WIDTH / 2.0f;
    const float warpCenterY = SCREEN_HEIGHT / 2.0f;

    // warpFactor is passed from exit animation loop and matches the current potentiometer value
    // This ensures stars move at the SAME speed during exit as they would in normal warp mode
    // No artificial speed changes - just pure potentiometer-driven motion

    // Base parameters (must match warp mode exactly for seamless transition)
    const float baseSpeed = 3.0f;
    const float minSpeed = 2.5f; // MIN_WARP_SPEED * 5.0f = 0.5 * 5.0

    for (int i = 0; i < starCount; i++)
    {
        // Calculate distance from warp center (not object center)
        float dx = stars[i].realX - warpCenterX;
        float dy = stars[i].realY - warpCenterY;
        float distance = sqrtf(dx * dx + dy * dy);
        if (distance < 1.0f)
            distance = 1.0f;

        float dirX = dx / distance;
        float dirY = dy / distance;

        // Update star position with warp speed (same as warp mode)
        float speed = (distance / 10.0f + 1.0f) * warpFactor * baseSpeed;
        speed = std::max(speed, minSpeed * warpFactor);

        stars[i].realX += dirX * speed;
        stars[i].realY += dirY * speed;

        // Convert to integer positions
        int newX = roundf(stars[i].realX);
        int newY = roundf(stars[i].realY);

        // Reset stars that move off screen (same as warp mode)
        if (newX < 0 || newX >= SCREEN_WIDTH ||
            newY < 0 || newY >= SCREEN_HEIGHT)
        {
            // Respawn star near center with random position
            float angle = random(360) * PI / 180.0f;
            float spawnDistance = random(10, 50);
            stars[i].realX = warpCenterX + cos(angle) * spawnDistance;
            stars[i].realY = warpCenterY + sin(angle) * spawnDistance;
            stars[i].x = roundf(stars[i].realX);
            stars[i].y = roundf(stars[i].realY);
            stars[i].brightness = random(150, 256);
            stars[i].increasing = random(0, 2);
        }
        else
        {
            stars[i].x = newX;
            stars[i].y = newY;
        }
    }
}
