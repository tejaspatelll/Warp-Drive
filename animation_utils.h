#ifndef ANIMATION_UTILS_H
#define ANIMATION_UTILS_H

#include <Arduino.h>

// Helper function for celestial objects to get animation time
// Returns frozen time during exit animation, otherwise returns current millis()
// Note: exitSnapshotCaptured and frozenAnimationTime are declared in warpdrive_esp32_tft.ino
inline unsigned long getAnimationTime()
{
    extern bool exitSnapshotCaptured;
    extern unsigned long frozenAnimationTime;
    return exitSnapshotCaptured ? frozenAnimationTime : millis();
}

#endif // ANIMATION_UTILS_H
