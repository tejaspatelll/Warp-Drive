#ifndef LED_ANIMATIONS_H
#define LED_ANIMATIONS_H

#include <FastLED.h>

// Define LED properties
#define LED_PIN 2
#define NUM_LEDS 2
#define LED_TYPE WS2812B
#define COLOR_ORDER GRB
#define BRIGHTNESS 30

// Declare the LED array and controller
extern CRGB leds[NUM_LEDS];

// Function declarations
void setupLeds();
void ledsOff();
void setLedColor(uint8_t ledIndex, uint32_t color);
void setLedModeMenu(int currentSelection, int numItems);
void setLedModeQuiz(bool answerCorrect, bool waitingForAnswer);
void setLedModeOff();
void setLedModeStory(); // New: Set LED mode for Story Mode
void updateLedEffects(); // General update function to be called in main loop

#endif // LED_ANIMATIONS_H 