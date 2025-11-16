#ifndef BUZZER_SOUNDS_H
#define BUZZER_SOUNDS_H

#include <Arduino.h> // Required for Arduino functions like pinMode and tone

// Define the pin connected to the buzzer
#define BUZZER_PIN 6 // User requested pin 6

// Volume control (50% as user adjusted)
#define VOLUME_FACTOR 0.5f

// Global sound toggle flag
extern bool soundEnabled;

// Function to toggle sound on/off
void toggleSound();

// Note frequencies for easier music composition
#define NOTE_B0 31
#define NOTE_C1 33
#define NOTE_CS1 35
#define NOTE_D1 37
#define NOTE_DS1 39
#define NOTE_E1 41
#define NOTE_F1 44
#define NOTE_FS1 46
#define NOTE_G1 49
#define NOTE_GS1 52
#define NOTE_A1 55
#define NOTE_AS1 58
#define NOTE_B1 62
#define NOTE_C2 65
#define NOTE_CS2 69
#define NOTE_D2 73
#define NOTE_DS2 78
#define NOTE_E2 82
#define NOTE_F2 87
#define NOTE_FS2 93
#define NOTE_G2 98
#define NOTE_GS2 104
#define NOTE_A2 110
#define NOTE_AS2 117
#define NOTE_B2 123
#define NOTE_C3 131
#define NOTE_CS3 139
#define NOTE_D3 147
#define NOTE_DS3 156
#define NOTE_E3 165
#define NOTE_F3 175
#define NOTE_FS3 185
#define NOTE_G3 196
#define NOTE_GS3 208
#define NOTE_A3 220
#define NOTE_AS3 233
#define NOTE_B3 247
#define NOTE_C4 262
#define NOTE_CS4 277
#define NOTE_D4 294
#define NOTE_DS4 311
#define NOTE_E4 330
#define NOTE_F4 349
#define NOTE_FS4 370
#define NOTE_G4 392
#define NOTE_GS4 415
#define NOTE_A4 440
#define NOTE_AS4 466
#define NOTE_B4 494
#define NOTE_C5 523
#define NOTE_CS5 554
#define NOTE_D5 587
#define NOTE_DS5 622
#define NOTE_E5 659
#define NOTE_F5 698
#define NOTE_FS5 740
#define NOTE_G5 784
#define NOTE_GS5 831
#define NOTE_A5 880
#define NOTE_AS5 932
#define NOTE_B5 988
#define NOTE_C6 1047 // Add definition for C6
#define NOTE_CS6 1109
#define NOTE_D6 1175 // Add definition for D6
#define NOTE_DS6 1245
#define NOTE_E6 1319 // Add definition for E6
#define NOTE_F6 1397 // Add definition for F6
#define NOTE_FS6 1480
#define NOTE_G6 1568 // Add definition for G6
#define NOTE_GS6 1661
#define NOTE_A6 1760 // Add definition for A6
#define NOTE_AS6 1865
#define NOTE_B6 1976 // Add definition for B6
#define NOTE_C7 2093 // Add definition for C7

// Warp sound effect
void updateWarpSound(float warpFactor);

// Initialize the buzzer pin
void initBuzzer();

// Helper function to apply volume factor to frequency
int adjustVolume(int frequency);

// Menu sounds
void playMenuNavSound();
void playMenuSelectSound();
void stopBuzzerSound();

// New UI Sounds
void playUISound_Beep();
void playUISound_Boop();

// State transition sounds
void playStateTransitionSound(const char *stateName);

// Discovery object ambient sounds
void playDiscoveryObjectSound(const char *objectType);

// Menu Background Music (80s sci-fi theme)
void startMenuBackgroundMusic();
void stopMenuBackgroundMusic();
void updateMenuBackgroundMusic();

// Quiz Mode Sounds
void startQuizQuestionMusic(); // Mysterious exploration theme
void stopQuizQuestionMusic();
void updateQuizQuestionMusic();

void startQuizHelplineMusic(); // Special theme for helpline state
void stopQuizHelplineMusic();
void updateQuizHelplineMusic();

void startQuizPopupMusic(); // Theme for popup states
void stopQuizPopupMusic();
void updateQuizPopupMusic();

void playQuizCorrectAnswerSound();   // Triumphant jingle
void playQuizWrongAnswerSound();     // Error jingle
void playQuizOptionHighlightSound(); // Option selection blip

// Sound state management
void updateAllSoundStates(); // Call this in main loop to handle all active sound states
void stopAllSounds();        // Emergency stop all sounds

// Declare boolean flags for music states so they can be accessed from other files
extern bool menuMusicPlaying;
extern bool quizQuestionMusicPlaying;
extern bool helplineMusicPlaying;
extern bool popupMusicPlaying;

#endif // BUZZER_SOUNDS_H