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

void playMenuNavSound() {
  // This function is now deprecated, using playUISound_Boop instead
  // if (soundEnabled) {
  //   tone(BUZZER_PIN, adjustVolume(NOTE_C4), 30);
  // }
}

void playMenuSelectSound() {
  // This function is now deprecated, using playUISound_Beep instead
  // if (soundEnabled) {
  //   tone(BUZZER_PIN, adjustVolume(NOTE_G4), 50);
  //   delay(50);
  //   tone(BUZZER_PIN, adjustVolume(NOTE_C5), 80);
  // }
}

void stopBuzzerSound() {
  noTone(BUZZER_PIN);
}

// --- Menu Background Music --- - New Iconic & Catchy Theme
const int MENU_THEME_LENGTH = 16; // A short, catchy loop

int menuThemeNotes[MENU_THEME_LENGTH] = {
  NOTE_C5, 0,       NOTE_E5, NOTE_G5, // Upbeat arpeggio start
  NOTE_A5, NOTE_G5, NOTE_E5, 0,       // Quick melodic phrase
  NOTE_B4, 0,       NOTE_D5, NOTE_F5, // Contrasting phrase
  NOTE_G5, NOTE_F5, NOTE_D5, 0        // Resolution and loop point
};

int menuThemeDurations[MENU_THEME_LENGTH] = {
  150, 100, 150, 200, // Staccato and held notes
  120, 120, 150, 150, // Rhythmic variation
  150, 100, 150, 200, // Similar to first line
  120, 120, 150, 250  // Slower end to loop nicely
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
  if (!menuMusicPlaying || !soundEnabled) return; // Stop if not playing or sound is off

  unsigned long currentTime = millis();
  
  // Check if it's time for the next note
  if (currentTime - lastMenuNoteTime >= menuThemeDurations[currentMenuNote]) {
    noTone(BUZZER_PIN); // Stop the previous note
    
    currentMenuNote++; // Move to the next note

    if (currentMenuNote >= MENU_THEME_LENGTH) {
      // End of the theme reached, stop music and reset note for next start
      noTone(BUZZER_PIN); // Stop the last note explicitly
      menuMusicPlaying = false; // Stop the music playback logic
      currentMenuNote = 0; // Reset note index for the next time music starts
      return; // Exit the function as the loop is finished
    }

    lastMenuNoteTime = currentTime; // Update timer for the next note

    // Play the next note in the theme if it's not a rest
    // Add small random variations to pitch for a less mechanical feel (optional)
    int pitchVariation = random(-2, 3); // Smaller range for subtle variation
    
    // Apply pitch variation and play the note
    int finalPitch = menuThemeNotes[currentMenuNote] + pitchVariation;
    // Ensure pitch doesn't go below a reasonable minimum (e.g., NOTE_C1) to avoid issues
    if (finalPitch < NOTE_C1) finalPitch = NOTE_C1;
    
    if (menuThemeNotes[currentMenuNote] > 0) {
      tone(BUZZER_PIN, adjustVolume(finalPitch));
    } else {
        noTone(BUZZER_PIN); // Ensure no tone if it's a rest
    }
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
  if (!soundEnabled) return;
  
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
  if (!quizQuestionMusicPlaying || !soundEnabled) return;

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
  if (!soundEnabled) return;
  
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
  if (!popupMusicPlaying || !soundEnabled) return;

  unsigned long currentTime = millis();
  if (currentTime - lastPopupNoteTime >= popupDurations[currentPopupNote]) {
    noTone(BUZZER_PIN);
    currentPopupNote = (currentPopupNote + 1) % POPUP_PATTERN_LENGTH;
    tone(BUZZER_PIN, adjustVolume(popupNotes[currentPopupNote]));
    lastPopupNoteTime = currentTime;
  }
}

void playQuizCorrectAnswerSound() {
  if (!soundEnabled) return;
  
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
  if (!soundEnabled) return;
  
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
  if (!soundEnabled) return;
  
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

// --- Warp Sound Effect ---
void updateWarpSound(float warpFactor) {
  static unsigned long lastPulseTime = 0;
  static bool toneOn = false;
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
    return;
  }

  // Map warpFactor to frequency and pulse rate
  int minFreq = NOTE_C4; // Minimum frequency
  int maxFreq = NOTE_C6; // Maximum frequency
  int freq = minFreq + (maxFreq - minFreq) * warpFactor;

  // Add slight randomness to frequency for a more dynamic sound
  int pitchVariation = random(-5, 6); // Random variation for a richer sound
  freq += pitchVariation;

  // Ensure frequency stays within bounds
  if (freq < minFreq) freq = minFreq;
  if (freq > maxFreq) freq = maxFreq;

  int minPulse = 120; // ms
  int maxPulse = 30;  // ms
  int pulse = minPulse - (minPulse - maxPulse) * warpFactor;
  if (pulse < maxPulse) pulse = maxPulse;

  // Pulse the tone for a choppy engine effect
  if (now - lastPulseTime > pulse) {
    lastPulseTime = now;
    if (!toneOn) {
      tone(BUZZER_PIN, freq);
      toneOn = true;
    } else {
      noTone(BUZZER_PIN);
      toneOn = false;
    }
  }
}

// New UI Sounds Implementation
void playUISound_Beep() {
  if (soundEnabled) {
    // A simple, slightly higher pitch tone for button presses
    tone(BUZZER_PIN, adjustVolume(NOTE_G5), 50); // Using G5
    delay(50); // Keep buzzer active for the tone duration
    noTone(BUZZER_PIN); // Ensure tone stops after playing
  }
}

void playUISound_Boop() {
  if (soundEnabled) {
    // A slightly lower pitch tone for navigation/highlighting
    tone(BUZZER_PIN, adjustVolume(NOTE_E4), 40); // Using E4
    delay(40); // Keep buzzer active for the tone duration
    noTone(BUZZER_PIN); // Ensure tone stops after playing
  }
} 