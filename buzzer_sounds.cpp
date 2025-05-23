#include "buzzer_sounds.h"

// Global sound toggle flag - initially enabled
bool soundEnabled = true;

// Function to toggle sound on/off
void toggleSound() {
  soundEnabled = !soundEnabled;
  if (!soundEnabled) {
    // Immediately stop any active sounds when toggling off
    stopBuzzerSound();
  }
}

// Helper function to apply volume factor to frequency
inline int adjustVolume(int frequency) {
  return round(frequency * VOLUME_FACTOR);
}

void initBuzzer() {
  pinMode(BUZZER_PIN, OUTPUT);
  stopBuzzerSound();
}

// --- RETRO-FUTURISTIC UI SOUND PALETTE ---
// Using a consistent set of frequencies based on pentatonic and chromatic scales
// to create a cohesive retro sci-fi aesthetic

void playUISound_Beep() {
  if (!soundEnabled) return;
  
  // Retro-futuristic button press: ascending harmonic
  tone(BUZZER_PIN, adjustVolume(NOTE_C5), 40);
  delay(40);
  tone(BUZZER_PIN, adjustVolume(NOTE_G5), 60);
  delay(60);
  noTone(BUZZER_PIN);
}

void playUISound_Boop() {
  if (!soundEnabled) return;
  
  // Retro-futuristic navigation: quick descending tone
  tone(BUZZER_PIN, adjustVolume(NOTE_A4), 35);
  delay(35);
  tone(BUZZER_PIN, adjustVolume(NOTE_F4), 25);
  delay(25);
  noTone(BUZZER_PIN);
}

// Enhanced state transition sounds
void playStateTransitionSound(const char* stateName) {
  if (!soundEnabled) return;
  
  if (strcmp(stateName, "MENU") == 0) {
    // Menu entry: welcoming harmonic sequence
    int notes[] = {NOTE_C4, NOTE_E4, NOTE_G4, NOTE_C5};
    int durations[] = {80, 80, 80, 120};
    for (int i = 0; i < 4; i++) {
      tone(BUZZER_PIN, adjustVolume(notes[i]), durations[i]);
      delay(durations[i]);
    }
  } else if (strcmp(stateName, "DISCOVERY") == 0) {
    // Discovery entry: mysterious rising sweep
    for (int freq = NOTE_C3; freq <= NOTE_C4; freq += 20) {
      tone(BUZZER_PIN, adjustVolume(freq), 15);
      delay(15);
    }
  } else if (strcmp(stateName, "QUIZ") == 0) {
    // Quiz entry: intellectual tone sequence
    int notes[] = {NOTE_F4, NOTE_A4, NOTE_C5, NOTE_F5};
    int durations[] = {60, 60, 60, 100};
    for (int i = 0; i < 4; i++) {
      tone(BUZZER_PIN, adjustVolume(notes[i]), durations[i]);
      delay(durations[i]);
    }
  } else if (strcmp(stateName, "STORY") == 0) {
    // Story entry: narrative beginning chord
    int notes[] = {NOTE_D4, NOTE_FS4, NOTE_A4, NOTE_D5};
    int durations[] = {70, 70, 70, 110};
    for (int i = 0; i < 4; i++) {
      tone(BUZZER_PIN, adjustVolume(notes[i]), durations[i]);
      delay(durations[i]);
    }
  }
  noTone(BUZZER_PIN);
}

// Discovery object ambient sounds
void playDiscoveryObjectSound(const char* objectType) {
  if (!soundEnabled) return;
  
  if (strcmp(objectType, "STAR") == 0) {
    // Star: bright, quick twinkling sound
    tone(BUZZER_PIN, adjustVolume(NOTE_G5), 100);
  } else if (strcmp(objectType, "PLANET") == 0) {
    // Planet: deep, resonant tone
    tone(BUZZER_PIN, adjustVolume(NOTE_C3), 150);
  } else if (strcmp(objectType, "BLACK_HOLE") == 0) {
    // Black hole: ominous low tone
    tone(BUZZER_PIN, adjustVolume(NOTE_C2), 200);
  } else if (strcmp(objectType, "NEBULA") == 0) {
    // Nebula: ethereal mid tone
    tone(BUZZER_PIN, adjustVolume(NOTE_E4), 180);
  } else if (strcmp(objectType, "PULSAR") == 0) {
    // Pulsar: quick pulsing tone
    tone(BUZZER_PIN, adjustVolume(NOTE_A4), 80);
  } else if (strcmp(objectType, "SUPERNOVA") == 0) {
    // Supernova: explosive high tone
    tone(BUZZER_PIN, adjustVolume(NOTE_C6), 120);
  } else if (strcmp(objectType, "GALAXY") == 0) {
    // Galaxy: vast, sweeping tone
    tone(BUZZER_PIN, adjustVolume(NOTE_G4), 160);
  } else if (strcmp(objectType, "COMET") == 0) {
    // Comet: whooshing sound
    tone(BUZZER_PIN, adjustVolume(NOTE_F4), 120);
  } else if (strcmp(objectType, "BINARY_STAR") == 0) {
    // Binary star: dual tone effect
    tone(BUZZER_PIN, adjustVolume(NOTE_D4), 100);
  } else if (strcmp(objectType, "SPACE_STATION") == 0) {
    // Space station: mechanical beep
    tone(BUZZER_PIN, adjustVolume(NOTE_B4), 90);
  } else if (strcmp(objectType, "SOLAR_SYSTEM") == 0) {
    // Solar system: harmonic tone
    tone(BUZZER_PIN, adjustVolume(NOTE_A3), 140);
  } else if (strcmp(objectType, "ASTEROID_FIELD") == 0) {
    // Asteroid field: scattered tone
    tone(BUZZER_PIN, adjustVolume(NOTE_E3), 110);
  }
  // Note: No delay() calls - the tone will automatically stop after the specified duration
}

// Legacy menu sounds (now redirect to new UI sounds)
void playMenuNavSound() {
  playUISound_Boop();
}

void playMenuSelectSound() {
  playUISound_Beep();
}

void stopBuzzerSound() {
  noTone(BUZZER_PIN);
}

// --- ENHANCED WARP SOUND EFFECT ---
void updateWarpSound(float warpFactor) {
  static unsigned long lastPulseTime = 0;
  static unsigned long lastSweepTime = 0;
  static bool toneOn = false;
  static int sweepDirection = 1;
  static int currentSweepFreq = NOTE_C4;
  static int pulsePattern = 0; // For LED synchronization
  unsigned long now = millis();

  if (!soundEnabled) {
    noTone(BUZZER_PIN);
    toneOn = false;
    return;
  }

  // If warp is not engaged, stop sound
  if (warpFactor < 0.05f) {
    noTone(BUZZER_PIN);
    toneOn = false;
    pulsePattern = 0;
    return;
  }

  // Enhanced warp sound with better LED synchronization
  int baseFreq = NOTE_C4 + (NOTE_C6 - NOTE_C4) * warpFactor;
  
  // Rhythmic pulse rate that syncs with LED patterns
  int pulseRate;
  if (warpFactor < 0.3f) {
    pulseRate = 200 - 100 * warpFactor; // Slow pulsing (200ms to 170ms)
  } else if (warpFactor < 0.7f) {
    pulseRate = 120 - 60 * (warpFactor - 0.3f); // Medium pulsing (120ms to 96ms)
  } else {
    pulseRate = 60 - 30 * (warpFactor - 0.7f); // Fast pulsing (60ms to 30ms)
  }
  
  // Add frequency sweep for dynamic effect
  if (now - lastSweepTime > (150 - 100 * warpFactor)) {
    lastSweepTime = now;
    int sweepAmount = 5 + 20 * warpFactor;
    currentSweepFreq += sweepDirection * sweepAmount;
    
    if (currentSweepFreq > baseFreq + 80) sweepDirection = -1;
    if (currentSweepFreq < baseFreq - 80) sweepDirection = 1;
  }
  
  // Pulse the enhanced tone with LED-sync pattern
  if (now - lastPulseTime > pulseRate) {
    lastPulseTime = now;
    pulsePattern = (pulsePattern + 1) % 4; // 4-beat pattern for LED sync
    
    if (!toneOn) {
      // Add harmonic variations based on pulse pattern
      int harmonicOffset = 0;
      switch (pulsePattern) {
        case 0: harmonicOffset = 0; break;        // Base tone
        case 1: harmonicOffset = 12; break;      // Octave higher
        case 2: harmonicOffset = 7; break;       // Fifth higher
        case 3: harmonicOffset = 4; break;       // Major third higher
      }
      
      int finalFreq = currentSweepFreq + harmonicOffset;
      
      // Add intensity-based modulation
      int modulation = random(-5, 6) * warpFactor;
      finalFreq += modulation;
      
      tone(BUZZER_PIN, adjustVolume(finalFreq));
      toneOn = true;
    } else {
      noTone(BUZZER_PIN);
      toneOn = false;
    }
  }
}

// --- Menu Background Music --- - Enhanced for retro-futuristic feel
const int MENU_THEME_LENGTH = 20; // Extended for richer melody

int menuThemeNotes[MENU_THEME_LENGTH] = {
  NOTE_C5, 0, NOTE_G4, NOTE_E5,     // Sci-fi opening phrase
  NOTE_F5, NOTE_E5, NOTE_C5, 0,     // Descending resolution
  NOTE_A4, 0, NOTE_F4, NOTE_D5,     // Contrasting phrase
  NOTE_E5, NOTE_D5, NOTE_A4, 0,     // Mirror resolution
  NOTE_G4, NOTE_C5, NOTE_E5, NOTE_G5 // Ascending finale
};

int menuThemeDurations[MENU_THEME_LENGTH] = {
  140, 80, 120, 180,   // Rhythmic and spacious
  100, 100, 140, 120,  // Flowing
  140, 80, 120, 180,   // Echo pattern
  100, 100, 140, 120,  // Consistent
  120, 120, 140, 200   // Grand finish
};

int currentMenuNote = 0;
unsigned long lastMenuNoteTime = 0;
bool menuMusicPlaying = false;

void startMenuBackgroundMusic() {
  if (!soundEnabled) return;
  
  currentMenuNote = 0;
  lastMenuNoteTime = millis();
  menuMusicPlaying = true;
  
  // Play the first note immediately
  if (menuThemeNotes[currentMenuNote] > 0) {
    tone(BUZZER_PIN, adjustVolume(menuThemeNotes[currentMenuNote]));
  }
}

void stopMenuBackgroundMusic() {
  menuMusicPlaying = false;
  stopBuzzerSound();
}

void updateMenuBackgroundMusic() {
  if (!menuMusicPlaying || !soundEnabled) return;

  unsigned long currentTime = millis();
  
  if (currentTime - lastMenuNoteTime >= menuThemeDurations[currentMenuNote]) {
    noTone(BUZZER_PIN);
    
    currentMenuNote++;
    if (currentMenuNote >= MENU_THEME_LENGTH) {
      noTone(BUZZER_PIN);
      menuMusicPlaying = false;
      currentMenuNote = 0;
      return;
    }

    lastMenuNoteTime = currentTime;
    
    // Add subtle pitch variation for organic feel
    int pitchVariation = random(-3, 4);
    int finalPitch = menuThemeNotes[currentMenuNote] + pitchVariation;
    if (finalPitch < NOTE_C1) finalPitch = NOTE_C1;
    
    if (menuThemeNotes[currentMenuNote] > 0) {
      tone(BUZZER_PIN, adjustVolume(finalPitch));
    } else {
      noTone(BUZZER_PIN);
    }
  }
}

// --- Quiz Mode Sounds - Enhanced for retro-futuristic feel ---
const int QUIZ_PATTERN_LENGTH = 32; // Shorter, more focused pattern
int quizQuestionNotes[QUIZ_PATTERN_LENGTH] = {
  NOTE_F3, 0, NOTE_AS3, 0, NOTE_DS4, 0, NOTE_F4, 0,   // Mystery theme
  NOTE_E3, 0, NOTE_A3, 0, NOTE_D4, 0, NOTE_E4, 0,     // Variation
  NOTE_G3, 0, NOTE_C4, 0, NOTE_F4, 0, NOTE_G4, 0,     // Building
  NOTE_F3, 0, NOTE_AS3, 0, NOTE_DS4, 0, NOTE_F4, 0    // Return
};

int quizQuestionDurations[QUIZ_PATTERN_LENGTH] = {
  250, 100, 250, 100, 250, 100, 350, 200,
  250, 100, 250, 100, 250, 100, 350, 200,
  200, 100, 200, 100, 200, 100, 300, 200,
  250, 100, 250, 100, 250, 100, 400, 300
};

int currentQuizNote = 0;
unsigned long lastQuizNoteTime = 0;
bool quizQuestionMusicPlaying = false;

void startQuizQuestionMusic() {
  if (!soundEnabled) return;
  
  currentQuizNote = 0;
  lastQuizNoteTime = millis();
  quizQuestionMusicPlaying = true;
  if (quizQuestionNotes[currentQuizNote] > 0) {
    tone(BUZZER_PIN, adjustVolume(quizQuestionNotes[currentQuizNote]));
  }
}

void stopQuizQuestionMusic() {
  quizQuestionMusicPlaying = false;
  stopBuzzerSound();
}

void updateQuizQuestionMusic() {
  if (!quizQuestionMusicPlaying || !soundEnabled) return;

  unsigned long currentTime = millis();
  if (currentTime - lastQuizNoteTime >= quizQuestionDurations[currentQuizNote]) {
    noTone(BUZZER_PIN);
    currentQuizNote = (currentQuizNote + 1) % QUIZ_PATTERN_LENGTH;
    
    // Subtle variations for organic feel
    int pitchVariation = random(-2, 3);
    int finalPitch = quizQuestionNotes[currentQuizNote] + pitchVariation;
    
    if (quizQuestionNotes[currentQuizNote] > 0) {
      tone(BUZZER_PIN, adjustVolume(finalPitch));
    }
    lastQuizNoteTime = currentTime;
  }
}

// Helpline Music - More electronic scanner effect
const int HELPLINE_PATTERN_LENGTH = 16;
int helplineNotes[HELPLINE_PATTERN_LENGTH] = {
  NOTE_G4, NOTE_C5, NOTE_G4, NOTE_C5, NOTE_G4, NOTE_C5, NOTE_G4, NOTE_C5,
  NOTE_F4, NOTE_AS4, NOTE_F4, NOTE_AS4, NOTE_F4, NOTE_AS4, NOTE_F4, NOTE_AS4
};

int helplineDurations[HELPLINE_PATTERN_LENGTH] = {
  80, 80, 80, 80, 80, 80, 80, 150,
  80, 80, 80, 80, 80, 80, 80, 150
};

int currentHelplineNote = 0;
unsigned long lastHelplineNoteTime = 0;
bool helplineMusicPlaying = false;

void startQuizHelplineMusic() {
  if (!soundEnabled) return;
  
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
  if (!helplineMusicPlaying || !soundEnabled) return;

  unsigned long currentTime = millis();
  if (currentTime - lastHelplineNoteTime >= helplineDurations[currentHelplineNote]) {
    noTone(BUZZER_PIN);
    currentHelplineNote = (currentHelplineNote + 1) % HELPLINE_PATTERN_LENGTH;
    tone(BUZZER_PIN, adjustVolume(helplineNotes[currentHelplineNote]));
    lastHelplineNoteTime = currentTime;
  }
}

// Popup Music - Anticipatory retro theme
const int POPUP_PATTERN_LENGTH = 16;
int popupNotes[POPUP_PATTERN_LENGTH] = {
  NOTE_E4, 0, NOTE_G4, 0, NOTE_B4, 0, NOTE_E5, 0,
  NOTE_D4, 0, NOTE_F4, 0, NOTE_A4, 0, NOTE_D5, 0
};

int popupDurations[POPUP_PATTERN_LENGTH] = {
  200, 50, 200, 50, 200, 50, 400, 100,
  200, 50, 200, 50, 200, 50, 400, 100
};

int currentPopupNote = 0;
unsigned long lastPopupNoteTime = 0;
bool popupMusicPlaying = false;

void startQuizPopupMusic() {
  if (!soundEnabled) return;
  
  currentPopupNote = 0;
  lastPopupNoteTime = millis();
  popupMusicPlaying = true;
  if (popupNotes[currentPopupNote] > 0) {
    tone(BUZZER_PIN, adjustVolume(popupNotes[currentPopupNote]));
  }
}

void stopQuizPopupMusic() {
  popupMusicPlaying = false;
  stopBuzzerSound();
}

void updateQuizPopupMusic() {
  if (!popupMusicPlaying || !soundEnabled) return;

  unsigned long currentTime = millis();
  if (currentTime - lastPopupNoteTime >= popupDurations[currentPopupNote]) {
    noTone(BUZZER_PIN);
    currentPopupNote = (currentPopupNote + 1) % POPUP_PATTERN_LENGTH;
    if (popupNotes[currentPopupNote] > 0) {
      tone(BUZZER_PIN, adjustVolume(popupNotes[currentPopupNote]));
    }
    lastPopupNoteTime = currentTime;
  }
}

// Enhanced quiz feedback sounds
void playQuizCorrectAnswerSound() {
  if (!soundEnabled) return;
  
  // Triumphant retro success sequence
  int notes[] = {NOTE_C4, NOTE_E4, NOTE_G4, NOTE_C5, NOTE_E5, NOTE_G5};
  int durations[] = {80, 80, 80, 100, 100, 200};
  
  for (int i = 0; i < 6; i++) {
    tone(BUZZER_PIN, adjustVolume(notes[i]), durations[i]);
    delay(durations[i]);
  }
  noTone(BUZZER_PIN);
}

void playQuizWrongAnswerSound() {
  if (!soundEnabled) return;
  
  // Retro error sound: descending chromatic
  int notes[] = {NOTE_F4, NOTE_E4, NOTE_DS4, NOTE_D4, NOTE_CS4};
  int durations[] = {100, 100, 100, 100, 200};
  
  for (int i = 0; i < 5; i++) {
    tone(BUZZER_PIN, adjustVolume(notes[i]), durations[i]);
    delay(durations[i]);
  }
  noTone(BUZZER_PIN);
}

void playQuizOptionHighlightSound() {
  if (!soundEnabled) return;
  
  // Quick retro blip with harmonic
  tone(BUZZER_PIN, adjustVolume(NOTE_A4), 25);
  delay(25);
  tone(BUZZER_PIN, adjustVolume(NOTE_E5), 15);
  delay(15);
  noTone(BUZZER_PIN);
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