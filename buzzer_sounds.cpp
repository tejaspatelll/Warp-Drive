#include "buzzer_sounds.h"

// Helper function to apply volume factor to frequency
inline int adjustVolume(int frequency) {
  return round(frequency * VOLUME_FACTOR);
}

void initBuzzer() {
  pinMode(BUZZER_PIN, OUTPUT);
  stopBuzzerSound();
}

void playMenuNavSound() {
  tone(BUZZER_PIN, adjustVolume(NOTE_C4), 30);
}

void playMenuSelectSound() {
  tone(BUZZER_PIN, adjustVolume(NOTE_G4), 50);
  delay(50);
  tone(BUZZER_PIN, adjustVolume(NOTE_C5), 80);
}

void stopBuzzerSound() {
  noTone(BUZZER_PIN);
}

// --- Menu Background Music ---
// A collection of retro space game inspired themes that kids can vibe to
const int MENU_PATTERN_LENGTH = 48;
const int MENU_VARIATIONS = 5;
int menuMusicNotes[MENU_VARIATIONS][MENU_PATTERN_LENGTH] = {
  // Variation 1: Space Adventure Theme (Major Scale with Jumps)
  {
    NOTE_C4, NOTE_E4, NOTE_G4, NOTE_C5, NOTE_G4, NOTE_E4, NOTE_C4, 0,
    NOTE_F4, NOTE_A4, NOTE_C5, NOTE_F5, NOTE_C5, NOTE_A4, NOTE_F4, 0,
    NOTE_G4, NOTE_B4, NOTE_D5, NOTE_G5, NOTE_D5, NOTE_B4, NOTE_G4, 0,
    NOTE_E4, NOTE_G4, NOTE_B4, NOTE_E5, NOTE_B4, NOTE_G4, NOTE_E4, 0,
    NOTE_A4, NOTE_C5, NOTE_E5, NOTE_A5, NOTE_E5, NOTE_C5, NOTE_A4, 0,
    NOTE_G4, NOTE_B4, NOTE_D5, NOTE_G5, NOTE_D5, NOTE_B4, NOTE_G4, 0
  },
  // Variation 2: Retro Space Game (Classic Arcade Style)
  {
    NOTE_C5, NOTE_C5, NOTE_G4, NOTE_G4, NOTE_A4, NOTE_A4, NOTE_G4, 0,
    NOTE_F4, NOTE_F4, NOTE_E4, NOTE_E4, NOTE_D4, NOTE_D4, NOTE_C4, 0,
    NOTE_G4, NOTE_G4, NOTE_F4, NOTE_F4, NOTE_E4, NOTE_E4, NOTE_D4, 0,
    NOTE_C5, NOTE_C5, NOTE_B4, NOTE_B4, NOTE_A4, NOTE_A4, NOTE_G4, 0,
    NOTE_A4, NOTE_A4, NOTE_G4, NOTE_G4, NOTE_F4, NOTE_F4, NOTE_E4, 0,
    NOTE_D5, NOTE_D5, NOTE_C5, NOTE_C5, NOTE_B4, NOTE_B4, NOTE_C5, 0
  },
  // Variation 3: Cosmic Dance (Upbeat and Bouncy)
  {
    NOTE_C4, NOTE_E4, NOTE_C4, NOTE_G4, NOTE_C5, NOTE_G4, NOTE_E4, NOTE_C4,
    NOTE_F4, NOTE_A4, NOTE_F4, NOTE_C5, NOTE_F5, NOTE_C5, NOTE_A4, NOTE_F4,
    NOTE_G4, NOTE_B4, NOTE_G4, NOTE_D5, NOTE_G5, NOTE_D5, NOTE_B4, NOTE_G4,
    NOTE_C5, NOTE_E5, NOTE_C5, NOTE_G5, NOTE_C6, NOTE_G5, NOTE_E5, NOTE_C5,
    NOTE_A4, NOTE_C5, NOTE_A4, NOTE_E5, NOTE_A5, NOTE_E5, NOTE_C5, NOTE_A4,
    NOTE_G4, NOTE_B4, NOTE_G4, NOTE_D5, NOTE_G5, NOTE_D5, NOTE_B4, NOTE_G4
  },
  // Variation 4: Star Chaser (Fast-paced and Exciting)
  {
    NOTE_C5, NOTE_G4, NOTE_E4, NOTE_C4, NOTE_E4, NOTE_G4, NOTE_C5, NOTE_E5,
    NOTE_F5, NOTE_C5, NOTE_A4, NOTE_F4, NOTE_A4, NOTE_C5, NOTE_F5, NOTE_A5,
    NOTE_G5, NOTE_D5, NOTE_B4, NOTE_G4, NOTE_B4, NOTE_D5, NOTE_G5, NOTE_B5,
    NOTE_E5, NOTE_B4, NOTE_G4, NOTE_E4, NOTE_G4, NOTE_B4, NOTE_E5, NOTE_G5,
    NOTE_A5, NOTE_E5, NOTE_C5, NOTE_A4, NOTE_C5, NOTE_E5, NOTE_A5, NOTE_C6,
    NOTE_G5, NOTE_D5, NOTE_B4, NOTE_G4, NOTE_B4, NOTE_D5, NOTE_G5, NOTE_B5
  },
  // Variation 5: Galaxy Groove (Funky Space Rhythm)
  {
    NOTE_C4, NOTE_C5, NOTE_G4, NOTE_E4, NOTE_C4, NOTE_E4, NOTE_G4, NOTE_C5,
    NOTE_F4, NOTE_F5, NOTE_C5, NOTE_A4, NOTE_F4, NOTE_A4, NOTE_C5, NOTE_F5,
    NOTE_G4, NOTE_G5, NOTE_D5, NOTE_B4, NOTE_G4, NOTE_B4, NOTE_D5, NOTE_G5,
    NOTE_E4, NOTE_E5, NOTE_B4, NOTE_G4, NOTE_E4, NOTE_G4, NOTE_B4, NOTE_E5,
    NOTE_A4, NOTE_A5, NOTE_E5, NOTE_C5, NOTE_A4, NOTE_C5, NOTE_E5, NOTE_A5,
    NOTE_G4, NOTE_G5, NOTE_D5, NOTE_B4, NOTE_G4, NOTE_B4, NOTE_D5, NOTE_G5
  }
};

int menuMusicDurations[MENU_PATTERN_LENGTH] = {
  150, 150, 150, 200, 150, 150, 150, 100,  // More dynamic rhythm
  150, 150, 150, 200, 150, 150, 150, 100,
  150, 150, 150, 200, 150, 150, 150, 100,
  150, 150, 150, 200, 150, 150, 150, 100,
  150, 150, 150, 200, 150, 150, 150, 100,
  150, 150, 150, 200, 150, 150, 150, 100
};

int currentMenuNote = 0;
int currentMenuVariation = 0;
unsigned long lastMenuNoteTime = 0;
bool menuMusicPlaying = false;
unsigned long menuPatternStartTime = 0;
const unsigned long VARIATION_CHANGE_TIME = 10000; // Change pattern every 10 seconds

void startMenuBackgroundMusic() {
  currentMenuNote = 0;
  currentMenuVariation = 0;
  lastMenuNoteTime = millis();
  menuPatternStartTime = millis();
  menuMusicPlaying = true;
  tone(BUZZER_PIN, adjustVolume(menuMusicNotes[currentMenuVariation][currentMenuNote]));
}

void stopMenuBackgroundMusic() {
  menuMusicPlaying = false;
  stopBuzzerSound();
}

void updateMenuBackgroundMusic() {
  if (!menuMusicPlaying) return;

  unsigned long currentTime = millis();
  
  // Check if it's time to change variation
  if (currentTime - menuPatternStartTime >= VARIATION_CHANGE_TIME) {
    currentMenuVariation = (currentMenuVariation + 1) % MENU_VARIATIONS;
    menuPatternStartTime = currentTime;
  }

  // Update current note
  if (currentTime - lastMenuNoteTime >= menuMusicDurations[currentMenuNote]) {
    noTone(BUZZER_PIN);
    currentMenuNote = (currentMenuNote + 1) % MENU_PATTERN_LENGTH;
    
    // Add small random variations to timing and pitch for less mechanical feel
    int durationVariation = random(-20, 21);
    int pitchVariation = random(-5, 6);
    
    tone(BUZZER_PIN, 
         adjustVolume(menuMusicNotes[currentMenuVariation][currentMenuNote] + pitchVariation));
    lastMenuNoteTime = currentTime;
  }
}

// --- Quiz Mode Sounds ---

// Quiz Question Music - Mysterious, evolving pattern
const int QUIZ_PATTERN_LENGTH = 71;
int quizQuestionNotes[QUIZ_PATTERN_LENGTH] = {
  NOTE_C3, 0, NOTE_DS3, 0, NOTE_G3, 0, NOTE_AS3, 0, // Minor, sparse
  NOTE_C3, 0, NOTE_DS3, 0, NOTE_G3, 0, NOTE_AS3, 0,
  NOTE_F3, 0, NOTE_AS3, 0, NOTE_DS4, 0, NOTE_G4, 0, // Related chord
  NOTE_F3, 0, NOTE_AS3, 0, NOTE_DS4, 0, NOTE_G4, 0,
  NOTE_D3, 0, NOTE_F3, 0, NOTE_A3, 0, NOTE_C4, 0,   // Another related chord
  NOTE_D3, 0, NOTE_F3, 0, NOTE_A3, 0, NOTE_C4, 0,
  NOTE_G3, 0, NOTE_AS3, 0, NOTE_D4, 0, NOTE_F4, 0,   // Build tension
  NOTE_G3, 0, NOTE_AS3, 0, NOTE_D4, 0, NOTE_F4, 0,
  NOTE_C4, 0, NOTE_GS3, 0, NOTE_E3, 0, NOTE_C3
};

int quizQuestionDurations[QUIZ_PATTERN_LENGTH] = {
  300, 100, 300, 100, 300, 100, 300, 300,
  300, 100, 300, 100, 300, 100, 300, 300,
  250, 100, 250, 100, 250, 100, 250, 400,
  250, 100, 250, 100, 250, 100, 250, 400,
  280, 100, 280, 100, 280, 100, 280, 450,
  280, 100, 280, 100, 280, 100, 280, 450,
  200, 100, 200, 100, 200, 100, 200, 350,
  200, 100, 200, 100, 200, 100, 200, 350,
  400, 150, 400, 150, 400, 150, 600
};

int currentQuizNote = 0;
unsigned long lastQuizNoteTime = 0;
bool quizQuestionMusicPlaying = false;

void startQuizQuestionMusic() {
  currentQuizNote = 0;
  lastQuizNoteTime = millis();
  quizQuestionMusicPlaying = true;
  tone(BUZZER_PIN, adjustVolume(quizQuestionNotes[currentQuizNote]));
}

void stopQuizQuestionMusic() {
  quizQuestionMusicPlaying = false;
  stopBuzzerSound();
}

void updateQuizQuestionMusic() {
  if (!quizQuestionMusicPlaying) return;

  unsigned long currentTime = millis();
  if (currentTime - lastQuizNoteTime >= quizQuestionDurations[currentQuizNote]) {
    noTone(BUZZER_PIN);
    currentQuizNote = (currentQuizNote + 1) % QUIZ_PATTERN_LENGTH;
    
    // Add slight variations
    int pitchVariation = random(-3, 4);
    tone(BUZZER_PIN, adjustVolume(quizQuestionNotes[currentQuizNote] + pitchVariation));
    lastQuizNoteTime = currentTime;
  }
}

// Helpline Music - More electronic, scanning feel
const int HELPLINE_PATTERN_LENGTH = 24;
int helplineNotes[HELPLINE_PATTERN_LENGTH] = {
  NOTE_G4, NOTE_C5, NOTE_G4, NOTE_C5, NOTE_G4, NOTE_C5, NOTE_G4, NOTE_C5, // Fast high pulse
  NOTE_F4, NOTE_AS4, NOTE_F4, NOTE_AS4, NOTE_F4, NOTE_AS4, NOTE_F4, NOTE_AS4, // Slightly lower pulse
  NOTE_E4, NOTE_A4, NOTE_E4, NOTE_A4, NOTE_E4, NOTE_A4, NOTE_E4, NOTE_A4 // Even lower pulse
};

int helplineDurations[HELPLINE_PATTERN_LENGTH] = {
  100, 100, 100, 100, 100, 100, 100, 200,
  100, 100, 100, 100, 100, 100, 100, 200,
  100, 100, 100, 100, 100, 100, 100, 300
};

int currentHelplineNote = 0;
unsigned long lastHelplineNoteTime = 0;
bool helplineMusicPlaying = false;

void startQuizHelplineMusic() {
  currentHelplineNote = 0;
  lastHelplineNoteTime = millis();
  helplineMusicPlaying = true;
  tone(BUZZER_PIN, adjustVolume(helplineNotes[currentHelplineNote]));
}

void stopQuizHelplineMusic() {
  helplineMusicPlaying = false;
  stopBuzzerSound();
}

void updateQuizHelplineMusic() {
  if (!helplineMusicPlaying) return;

  unsigned long currentTime = millis();
  if (currentTime - lastHelplineNoteTime >= helplineDurations[currentHelplineNote]) {
    noTone(BUZZER_PIN);
    currentHelplineNote = (currentHelplineNote + 1) % HELPLINE_PATTERN_LENGTH;
    tone(BUZZER_PIN, adjustVolume(helplineNotes[currentHelplineNote]));
    lastHelplineNoteTime = currentTime;
  }
}

// Popup Music - Gentle, anticipatory theme
const int POPUP_PATTERN_LENGTH = 32;
int popupNotes[POPUP_PATTERN_LENGTH] = {
  NOTE_E4, 0, NOTE_G4, 0, NOTE_B4, 0, NOTE_C5, 0, // Building arpeggio
  NOTE_D4, 0, NOTE_F4, 0, NOTE_A4, 0, NOTE_B4, 0, // Another arpeggio
  NOTE_C4, 0, NOTE_E4, 0, NOTE_G4, 0, NOTE_A4, 0, // Third arpeggio
  NOTE_B3, 0, NOTE_D4, 0, NOTE_FS4, 0, NOTE_G4, 0  // Resolution hint
};

int popupDurations[POPUP_PATTERN_LENGTH] = {
  250, 50, 250, 50, 250, 50, 500,
  250, 50, 250, 50, 250, 50, 500,
  250, 50, 250, 50, 250, 50, 500,
  250, 50, 250, 50, 250, 50, 700
};

int currentPopupNote = 0;
unsigned long lastPopupNoteTime = 0;
bool popupMusicPlaying = false;

void startQuizPopupMusic() {
  currentPopupNote = 0;
  lastPopupNoteTime = millis();
  popupMusicPlaying = true;
  tone(BUZZER_PIN, adjustVolume(popupNotes[currentPopupNote]));
}

void stopQuizPopupMusic() {
  popupMusicPlaying = false;
  stopBuzzerSound();
}

void updateQuizPopupMusic() {
  if (!popupMusicPlaying) return;

  unsigned long currentTime = millis();
  if (currentTime - lastPopupNoteTime >= popupDurations[currentPopupNote]) {
    noTone(BUZZER_PIN);
    currentPopupNote = (currentPopupNote + 1) % POPUP_PATTERN_LENGTH;
    tone(BUZZER_PIN, adjustVolume(popupNotes[currentPopupNote]));
    lastPopupNoteTime = currentTime;
  }
}

void playQuizCorrectAnswerSound() {
  // Triumphant ascending arpeggio with harmony
  tone(BUZZER_PIN, adjustVolume(NOTE_C4), 100);
  delay(100);
  tone(BUZZER_PIN, adjustVolume(NOTE_E4), 100);
  delay(100);
  tone(BUZZER_PIN, adjustVolume(NOTE_G4), 100);
  delay(100);
  tone(BUZZER_PIN, adjustVolume(NOTE_C5), 200);
  delay(200);
  // Final chord
  tone(BUZZER_PIN, adjustVolume(NOTE_E5), 300);
}

void playQuizWrongAnswerSound() {
  // Descending minor second with echo
  tone(BUZZER_PIN, adjustVolume(NOTE_C4), 200);
  delay(200);
  tone(BUZZER_PIN, adjustVolume(NOTE_B3), 300);
  delay(300);
  tone(BUZZER_PIN, adjustVolume(NOTE_AS3), 200);
  delay(200);
  noTone(BUZZER_PIN);
}

void playQuizOptionHighlightSound() {
  // Quick electronic blip
  tone(BUZZER_PIN, adjustVolume(NOTE_G4), 30);
}

void updateAllSoundStates() {
  if (menuMusicPlaying) updateMenuBackgroundMusic();
  if (quizQuestionMusicPlaying) updateQuizQuestionMusic();
  if (helplineMusicPlaying) updateQuizHelplineMusic();
  if (popupMusicPlaying) updateQuizPopupMusic();
}

void stopAllSounds() {
  menuMusicPlaying = false;
  quizQuestionMusicPlaying = false;
  helplineMusicPlaying = false;
  popupMusicPlaying = false;
  stopBuzzerSound();
} 