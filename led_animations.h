#ifndef LED_ANIMATIONS_H
#define LED_ANIMATIONS_H

#include <FastLED.h>

// LED strip configuration
#define LED_PIN     2
#define NUM_LEDS    2
#define BRIGHTNESS  64
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB

// LED array - Declaration only
extern CRGB leds[NUM_LEDS];

// External reference to ledEnabled
extern bool ledEnabled;

// Function declarations (Prototypes)
void setupLeds();
void setLedModeMenu(int currentSelection, int numItems);
void setLedModeQuiz(bool answerCorrect, bool waitingForAnswer);
void setLedModeStory();
void setLedModeWarp();
void setLedModeDiscovery(const char* objectName);
void setLedModeOff();
void updateLedEffects();

#endif // LED_ANIMATIONS_H 