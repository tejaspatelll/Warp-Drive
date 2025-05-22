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
  if (soundEnabled) {
    tone(BUZZER_PIN, adjustVolume(NOTE_C4), 30);
  }
}

void playMenuSelectSound() {
  if (soundEnabled) {
    tone(BUZZER_PIN, adjustVolume(NOTE_G4), 50);
    delay(50);
    tone(BUZZER_PIN, adjustVolume(NOTE_C5), 80);
  }
}

void stopBuzzerSound() {
  noTone(BUZZER_PIN);
}

// --- Menu Background Music ---
// A collection of retro space game inspired themes that kids can vibe to
const int MENU_PATTERN_LENGTH = 64; // Increased length for more complexity
const int MENU_VARIATIONS = 5;
int menuMusicNotes[MENU_VARIATIONS][MENU_PATTERN_LENGTH] = {
  // Variation 1: Space Adventure Theme (Heroic & Sweeping)
  {
    NOTE_C4, NOTE_E4, NOTE_G4, NOTE_C5, NOTE_G4, NOTE_E4, NOTE_C4, 0, // C Major Arpeggio
    NOTE_G4, NOTE_B4, NOTE_D5, NOTE_G5, NOTE_D5, NOTE_B4, NOTE_G4, 0, // G Major Arpeggio
    NOTE_A4, NOTE_C5, NOTE_E5, NOTE_A5, NOTE_E5, NOTE_C5, NOTE_A4, 0, // A Minor Arpeggio
    NOTE_F4, NOTE_A4, NOTE_C5, NOTE_F5, NOTE_C5, NOTE_A4, NOTE_F4, 0, // F Major Arpeggio
    NOTE_C5, NOTE_G4, NOTE_E4, NOTE_C4, NOTE_E4, NOTE_G4, NOTE_C5, NOTE_C6, // Ascending/Descending with Octave Jump
    NOTE_G5, NOTE_D5, NOTE_B4, NOTE_G4, NOTE_B4, NOTE_D5, NOTE_G5, NOTE_G6,
    NOTE_A5, NOTE_E5, NOTE_C5, NOTE_A4, NOTE_C5, NOTE_E5, NOTE_A5, NOTE_A6,
    NOTE_F5, NOTE_C5, NOTE_A4, NOTE_F4, NOTE_A4, NOTE_C5, NOTE_F5, NOTE_F6
  },
  // Variation 2: Retro Space Game (8-bit Arcade Style)
  {
    NOTE_C5, 0, NOTE_G4, 0, NOTE_E4, 0, NOTE_C4, 0, // Staccato descending
    NOTE_C5, 0, NOTE_G4, 0, NOTE_E4, 0, NOTE_C4, 0,
    NOTE_G4, 0, NOTE_D4, 0, NOTE_B3, 0, NOTE_G3, 0, // Lower register
    NOTE_G4, 0, NOTE_D4, 0, NOTE_B3, 0, NOTE_G3, 0,
    NOTE_C5, NOTE_C5, NOTE_C5, NOTE_C5, NOTE_G4, NOTE_G4, NOTE_G4, NOTE_G4, // Repeated notes
    NOTE_A4, NOTE_A4, NOTE_A4, NOTE_A4, NOTE_G4, 0, 0, 0, // Catchy motif
    NOTE_F4, NOTE_F4, NOTE_E4, NOTE_E4, NOTE_D4, NOTE_D4, NOTE_C4, 0,
    NOTE_G4, NOTE_G4, NOTE_F4, NOTE_F4, NOTE_E4, NOTE_E4, NOTE_D4, 0
  },
  // Variation 3: Cosmic Dance (Upbeat & Syncopated)
  {
    NOTE_C4, NOTE_E4, NOTE_G4, NOTE_C5, 0, NOTE_G4, NOTE_E4, NOTE_C4, // Syncopated arpeggio
    NOTE_F4, NOTE_A4, NOTE_C5, NOTE_F5, 0, NOTE_C5, NOTE_A4, NOTE_F4,
    NOTE_G4, NOTE_B4, NOTE_D5, NOTE_G5, 0, NOTE_D5, NOTE_B4, NOTE_G4,
    NOTE_E4, NOTE_GS4, NOTE_B4, NOTE_E5, 0, NOTE_B4, NOTE_GS4, NOTE_E4, // E Major feels bright
    NOTE_C5, 0, NOTE_E5, 0, NOTE_G5, 0, NOTE_C6, 0, // Higher notes with rests
    NOTE_A4, 0, NOTE_C5, 0, NOTE_E5, 0, NOTE_A5, 0,
    NOTE_G4, 0, NOTE_B4, 0, NOTE_D5, 0, NOTE_G5, 0,
    NOTE_F4, 0, NOTE_A4, 0, NOTE_C5, 0, NOTE_F5, 0
  },
  // Variation 4: Star Chaser (Fast & Sequential)
  {
    NOTE_C5, NOTE_D5, NOTE_E5, NOTE_F5, NOTE_G5, NOTE_A5, NOTE_B5, NOTE_C6, // Fast scale run
    NOTE_C6, NOTE_B5, NOTE_A5, NOTE_G5, NOTE_F5, NOTE_E5, NOTE_D5, NOTE_C5,
    NOTE_G4, NOTE_A4, NOTE_B4, NOTE_C5, NOTE_D5, NOTE_E5, NOTE_FS5, NOTE_G5, // Sequential pattern
    NOTE_G5, NOTE_FS5, NOTE_E5, NOTE_D5, NOTE_C5, NOTE_B4, NOTE_A4, NOTE_G4,
    NOTE_C5, NOTE_E5, NOTE_G5, NOTE_C6, NOTE_E5, NOTE_G5, NOTE_C6, NOTE_E6, // Arpeggio sequence
    NOTE_G5, NOTE_B5, NOTE_D6, NOTE_G6, NOTE_B5, NOTE_D6, NOTE_G6, NOTE_B6,
    NOTE_A5, NOTE_C6, NOTE_E6, NOTE_A6, NOTE_C6, NOTE_E6, NOTE_A6, NOTE_C7,
    NOTE_F5, NOTE_A5, NOTE_C6, NOTE_F6, NOTE_A5, NOTE_C6, NOTE_F6, NOTE_A6
  },
  // Variation 5: Galaxy Groove (Funky Bass & Melody)
  {
    NOTE_C3, 0, NOTE_G3, 0, NOTE_AS3, 0, NOTE_G3, 0, // "Bassline" feel
    NOTE_F3, 0, NOTE_C4, 0, NOTE_AS3, 0, NOTE_C4, 0,
    NOTE_G3, 0, NOTE_D4, 0, NOTE_AS3, 0, NOTE_D4, 0,
    NOTE_E3, 0, NOTE_B3, 0, NOTE_G3, 0, NOTE_B3, 0,
    NOTE_C5, NOTE_E5, NOTE_G5, NOTE_C5, NOTE_E5, NOTE_G5, NOTE_C5, NOTE_E5, // Syncopated high melody
    NOTE_F5, NOTE_A5, NOTE_C6, NOTE_F5, NOTE_A5, NOTE_C6, NOTE_F5, NOTE_A5,
    NOTE_G5, NOTE_B5, NOTE_D6, NOTE_G5, NOTE_B5, NOTE_D6, NOTE_G5, NOTE_B5,
    NOTE_E5, NOTE_GS5, NOTE_B5, NOTE_E5, NOTE_GS5, NOTE_B5, NOTE_E5, NOTE_GS5
  }
};

// Adjusted durations for more varied and syncopated rhythms
int menuMusicDurations[MENU_PATTERN_LENGTH] = {
  150, 50, 150, 200, 150, 50, 150, 100, // Pattern 1 timing
  150, 50, 150, 200, 150, 50, 150, 100,
  150, 50, 150, 200, 150, 50, 150, 100,
  150, 50, 150, 200, 150, 50, 150, 100,
  100, 100, 100, 100, 100, 100, 100, 200, // Faster section timing
  100, 100, 100, 100, 100, 100, 100, 200,
  100, 100, 100, 100, 100, 100, 100, 200,
  100, 100, 100, 100, 100, 100, 100, 200
};

int currentMenuNote = 0;
int currentMenuVariation = 0;
unsigned long lastMenuNoteTime = 0;
bool menuMusicPlaying = false;
unsigned long menuPatternStartTime = 0;
const unsigned long VARIATION_CHANGE_TIME = random(5000, 15000); // Change pattern every 15 seconds (slightly longer)

void startMenuBackgroundMusic() {
  if (!soundEnabled) return;
  
  currentMenuNote = 0;
  currentMenuVariation = 0;
  lastMenuNoteTime = millis();
  menuPatternStartTime = millis();
  menuMusicPlaying = true;
  
  // Play the first note immediately
  if (menuMusicNotes[currentMenuVariation][currentMenuNote] > 0) {
    tone(BUZZER_PIN, adjustVolume(menuMusicNotes[currentMenuVariation][currentMenuNote]));
  }
}

void stopMenuBackgroundMusic() {
  menuMusicPlaying = false;
  stopBuzzerSound();
}

void updateMenuBackgroundMusic() {
  if (!menuMusicPlaying || !soundEnabled) return;

  unsigned long currentTime = millis();
  
  // Add a small random variation to the *duration* check as well, for subtle rhythmic humanization
  int effectiveDuration = menuMusicDurations[currentMenuNote] + random(-5, 6); // Small random variation
  if (effectiveDuration < 10) effectiveDuration = 10; // Minimum duration

  if (currentTime - lastMenuNoteTime >= effectiveDuration) {
    noTone(BUZZER_PIN); // Stop the previous note
    
    // --- Dynamic Pattern / Variation Switching with Musical Jumps ---
    // Decide whether to jump to a random part of a random variation
    // Increased probability to jump to make it feel more dynamic and less like fixed patterns cycling.
    if (random(100) < 45) { // 45% chance to jump to a random musical phrase start
        // Jump to a random variation
        currentMenuVariation = random(MENU_VARIATIONS);
        
        // Jump to the start of a random 8-note segment (musical phrase)
        int randomSegment = random(MENU_PATTERN_LENGTH / 8); // Choose a segment (0 to 7 for 64 notes)
        currentMenuNote = randomSegment * 8; // Calculate the starting index of the segment
        
    } else {
        // Otherwise, just move to the next note in the current variation
        currentMenuNote = (currentMenuNote + 1) % MENU_PATTERN_LENGTH; // Wrap around if at the end
    }

    lastMenuNoteTime = currentTime; // Update timer for the *next* note

    // Play the selected note if it's not a rest (0)
    if (menuMusicNotes[currentMenuVariation][currentMenuNote] > 0) {
      // Add small random variations to pitch for a less mechanical feel
      int pitchVariation = random(-2, 3); // Smaller range for subtle variation
      
      // Apply pitch variation and play the note
      int finalPitch = menuMusicNotes[currentMenuVariation][currentMenuNote] + pitchVariation;
      // Ensure pitch doesn't go below a reasonable minimum (e.g., NOTE_C1) to avoid issues
      if (finalPitch < NOTE_C1) finalPitch = NOTE_C1;
      
      tone(BUZZER_PIN, adjustVolume(finalPitch));
      
    } else {
        // If it's a rest (note 0), make sure no tone is playing
        noTone(BUZZER_PIN);
        // The timer still advances by the duration of the rest.
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