#ifndef QUIZ_MODE_H
#define QUIZ_MODE_H

#include <TFT_eSPI.h>
#include <Arduino.h>
#include <vector>
#include "quiz_popup.h"
#include "story_mode.h" // Added include for story_mode.h to access STORY_STOPS_DATA and StoryStop struct

extern QuizPopupState quizPopupState;
// Helpline state for quiz mode (external): toggles highlighting of two wrong options
extern bool quizHelplineActive;
extern int quizHelplineIndices[2];

// --- Data Structures ---

// We'll reuse StoryStop for quiz options
struct QuizState {
    int correctIndex;         // Index of the correct option (0-3)
    int selectedIndex;        // Current selected option (0-3)
    int quizObjectIndex;      // Index in storyStops of the correct object
    int optionIndices[4];     // Indices in storyStops for the four options
    bool answeredCorrectly;   // Whether user has selected the correct answer
    bool showHint;            // Whether to show hint/fact
};

// --- Globals ---
extern TFT_eSPI tft;
extern uint16_t BG_COLOR;
extern int potValue; // Current potentiometer value
extern bool quizActive;

static QuizState quizState;
static int lastSelectedIndex = -1;
static bool lastAnsweredCorrectly = false;
static bool lastShowHint = false;
static String lastOptionLabel = "";
static String lastFeedback = "";
static String lastFact = "";
static bool uiInitialized = false;

// Add tracking for shown objects
static bool objectsShownInQuiz[STORY_STOPS_DATA_COUNT] = {false};
static int objectsRemainingInQuiz = STORY_STOPS_DATA_COUNT;

inline void resetQuizObjectTracking() {
    memset(objectsShownInQuiz, false, sizeof(objectsShownInQuiz));
    objectsRemainingInQuiz = STORY_STOPS_DATA_COUNT;
}

// --- Function Prototypes ---
void startQuiz();
void updateQuiz();
void processQuizInput(int potValue, bool buttonPressed);

// --- Helper Functions ---

// Shuffle an array of 4 ints
void shuffle4(int arr[4]) {
    for (int i = 3; i > 0; i--) {
        int j = random(i + 1);
        int temp = arr[i];
        arr[i] = arr[j];
        arr[j] = temp;
    }
}

// Pick a random quiz object and 3 distractors
void setupQuizOptions() {
    quizState.correctIndex = -1;
    quizState.answeredCorrectly = false;
    quizState.showHint = false;
    // Reset helpline on new question
    quizHelplineActive = false;

    // STORY_STOPS_DATA and STORY_STOPS_DATA_COUNT are available via #include "story_mode.h"
    // extern const StoryStop STORY_STOPS_DATA[]; // Removed redundant extern
    // extern const int STORY_STOPS_DATA_COUNT; // Removed redundant extern

    int total = STORY_STOPS_DATA_COUNT;
    if (objectsRemainingInQuiz == 0) {
        resetQuizObjectTracking();
    }

    int correctIdx = -1;
    if (objectsRemainingInQuiz > 0) {
        do {
            correctIdx = random(total);
        } while (objectsShownInQuiz[correctIdx]);
        objectsShownInQuiz[correctIdx] = true;
        objectsRemainingInQuiz--;
        quizState.quizObjectIndex = correctIdx;
    } else {
        // Handle case where no objects are remaining, though reset should prevent this.
        // For safety, pick a random one if this state is ever reached.
        correctIdx = random(total);
        quizState.quizObjectIndex = correctIdx;
    }

    // Declare the 'used' array before its first use.
    int used[STORY_STOPS_DATA_COUNT] = {0};

    // Fill options array with correct answer and 3 unique distractors
    int opt[4];
    opt[0] = correctIdx;
    // Ensure correctIdx is a valid index for 'used' before this assignment
    if (correctIdx >= 0 && correctIdx < STORY_STOPS_DATA_COUNT) {
        used[correctIdx] = 1;
    }

    for (int i = 1; i < 4; i++) {
        int didx;
        do {
            didx = random(total);
        } while (used[didx]);
        opt[i] = didx;
        used[didx] = 1;
    }

    // Shuffle options
    shuffle4(opt);

    // Copy to quizState and find correct index
    for (int i = 0; i < 4; i++) {
        quizState.optionIndices[i] = opt[i];
        if (opt[i] == correctIdx) quizState.correctIndex = i;
    }
    quizState.selectedIndex = 0;
    quizState.answeredCorrectly = false;
    quizState.showHint = false;
}


// --- Quiz Mode Functions ---

void startQuiz() {
    quizActive = true;
    // Reset object tracking when starting a new quiz session
    resetQuizObjectTracking();
    setupQuizOptions();
    quizPopupState.active = false;

    tft.fillScreen(BG_COLOR);
    uiInitialized = false;
    updateQuiz();
}

void updateQuiz() {
    // if (!quizSpriteCreated) return; // Temporarily allow drawing even if sprite failed for debugging

    // Get the current quiz object details
    const StoryStop& obj = STORY_STOPS_DATA[quizState.quizObjectIndex];

    // Set object position first
    objectX = SCREEN_WIDTH / 2;
    int objectOffsetY = SCREEN_HEIGHT / 8;
    objectY = SCREEN_HEIGHT / 2 - objectOffsetY;
    objectScale = 1.5;

    // Clear screen only during initialization
    if (!uiInitialized) {
        tft.fillScreen(BG_COLOR);
        uiInitialized = true;
        lastSelectedIndex = -1;
        lastAnsweredCorrectly = false;
        lastShowHint = false;
        lastOptionLabel = "";
        lastFeedback = "";
        lastFact = "";
    }

    // Draw title
    tft.setTextDatum(MC_DATUM);
    tft.setTextSize(2);
    
    // Draw shadow layers
    for(int i = 3; i > 0; i--) {
        tft.setTextColor(tft.color565(0, 60 - i*15, 120 - i*20));
        tft.drawString("IDENTIFY", SCREEN_WIDTH/2 + i, SCREEN_HEIGHT/16 + i);
        tft.drawString("THE OBJECT", SCREEN_WIDTH/2 + i, SCREEN_HEIGHT/16 + 20 + i);
    }
    
    // Main title text
    tft.setTextColor(tft.color565(0, 200, 255));
    tft.drawString("IDENTIFY", SCREEN_WIDTH/2, SCREEN_HEIGHT/16);
    tft.drawString("THE OBJECT", SCREEN_WIDTH/2, SCREEN_HEIGHT/16 + 20);

    // Add pixel-style underline
    for(int i = 0; i < 2; i++) {
        tft.drawFastHLine(SCREEN_WIDTH/2 - 80 + i*2, SCREEN_HEIGHT/16 + 40 + i, 160 - i*4, 
                         tft.color565(0, 200 - i*50, 255 - i*50));
    }

    // Draw the celestial object
    if (obj.drawFunction) {
        obj.drawFunction(); // Call the function pointer to draw the object
    } else {
        Serial.printf("[Quiz] Error: Draw function for object index %d is null!\n", quizState.quizObjectIndex);
        // Optionally draw a placeholder or error message on the TFT
        tft.setTextColor(TFT_RED);
        tft.drawCentreString("DRAW ERR", objectX, objectY, 2);
    }

    // Calculate selection box position
    int boxMargin = SCREEN_WIDTH / 16;
    int boxW = SCREEN_WIDTH - 2 * boxMargin;
    int boxH = SCREEN_HEIGHT / 8;
    int boxX = boxMargin;
    int boxY = SCREEN_HEIGHT - boxH - SCREEN_HEIGHT / 6;

    // Draw selection box and text
    // Draw retro-style box
    tft.fillRect(boxX + 4, boxY + 4, boxW - 8, boxH - 8, tft.color565(0, 40, 80));
    // Highlight eliminated options in red if helpline is active
    if (quizHelplineActive && (quizState.selectedIndex == quizHelplineIndices[0] || quizState.selectedIndex == quizHelplineIndices[1])) {
        uint16_t redCol = tft.color565(255, 0, 0);
        tft.drawRect(boxX + 2, boxY + 2, boxW - 4, boxH - 4, redCol);
        tft.drawRect(boxX, boxY, boxW, boxH, redCol);
    } else {
        tft.drawRect(boxX + 2, boxY + 2, boxW - 4, boxH - 4, tft.color565(0, 160, 255));
        tft.drawRect(boxX, boxY, boxW, boxH, tft.color565(255, 255, 0));
    }

    // Retro arrow indicators
    int arrowSize = 10;
    for (int j = 0; j < 3; j++) {
        // Left arrow
        tft.fillTriangle(
            boxX + 10 - j*2, boxY + boxH/2,
            boxX + 20 - j*2, boxY + boxH/2 - arrowSize,
            boxX + 20 - j*2, boxY + boxH/2 + arrowSize,
            tft.color565(255, 255, j * 85)
        );
        // Right arrow
        tft.fillTriangle(
            boxX + boxW - 10 + j*2, boxY + boxH/2,
            boxX + boxW - 20 + j*2, boxY + boxH/2 - arrowSize,
            boxX + boxW - 20 + j*2, boxY + boxH/2 + arrowSize,
            tft.color565(255, 255, j * 85)
        );
    }

    // Draw option label
    String optionLabel = STORY_STOPS_DATA[quizState.optionIndices[quizState.selectedIndex]].name;
    // Glow effect
    // Option text, red if helpline marks it as eliminated
    if (quizHelplineActive && (quizState.selectedIndex == quizHelplineIndices[0] || quizState.selectedIndex == quizHelplineIndices[1])) {
        uint16_t redCol = tft.color565(255, 0, 0);
        tft.setTextDatum(MC_DATUM);
        tft.setTextSize(2);
        // Red glow (same as main for simplicity)
        tft.setTextColor(redCol);
        tft.drawString(optionLabel, SCREEN_WIDTH/2 + 1, boxY + boxH/2 + 1);
        tft.drawString(optionLabel, SCREEN_WIDTH/2, boxY + boxH/2);
    } else {
        // Regular glow and text
        tft.setTextDatum(MC_DATUM);
        tft.setTextSize(2);
        tft.setTextColor(tft.color565(100, 200, 255));
        tft.drawString(optionLabel, SCREEN_WIDTH/2 + 1, boxY + boxH/2 + 1);
        tft.setTextColor(tft.color565(255, 255, 0));
        tft.drawString(optionLabel, SCREEN_WIDTH/2, boxY + boxH/2);
    }

    lastSelectedIndex = quizState.selectedIndex;
    lastOptionLabel = optionLabel;

    // Draw instructions
    int footerY = SCREEN_HEIGHT - SCREEN_HEIGHT / 12;
    tft.setTextColor(tft.color565(0, 255, 0));
    tft.setTextSize(1);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("KNOB: Select  |  BTN: Confirm", SCREEN_WIDTH/2, footerY);

    // Handle feedback text
    if (quizState.answeredCorrectly || quizState.showHint) {
        int feedbackY = boxY - boxH - SCREEN_HEIGHT / 40;
        String feedback = quizState.answeredCorrectly ? "Correct! Ready for warp." : "Try again!";
        
        if (feedback != lastFeedback || 
            quizState.answeredCorrectly != lastAnsweredCorrectly ||
            quizState.showHint != lastShowHint) {
            
            // Erase previous feedback area
            int feedbackH = (SCREEN_HEIGHT > 160 ? 24 : 12) * 2;
            tft.fillRect(0, feedbackY, SCREEN_WIDTH, feedbackH, BG_COLOR);

            // Draw new feedback
            tft.setTextDatum(MC_DATUM);
            tft.setTextColor(quizState.answeredCorrectly ? tft.color565(0, 255, 0) : tft.color565(255, 0, 0));
            tft.setTextSize(SCREEN_HEIGHT > 160 ? 2 : 1);
            tft.drawString(feedback, SCREEN_WIDTH / 2, feedbackY);
            
            lastFeedback = feedback;
            lastAnsweredCorrectly = quizState.answeredCorrectly;
            lastShowHint = quizState.showHint;
        }
    }

    // Handle popup if needed
    if (quizState.answeredCorrectly) {
        setQuizPopupFact(STORY_STOPS_DATA[quizState.quizObjectIndex].fact);
        showQuizPopup(true);
    } else if (quizState.showHint) {
        setQuizPopupFact(STORY_STOPS_DATA[quizState.quizObjectIndex].fact);
        showQuizPopup(false);
    }
}


void processQuizInput(int potValue, bool buttonPressed) {
    // Map potValue (0-4095) to 0-3
    int idx = map(potValue, 0, 4095, 0, 3);
    if (idx != quizState.selectedIndex) {
        quizState.selectedIndex = idx;
        quizState.showHint = false;
        // Haptic feedback on quiz option change
        extern bool hapticOverrideActive;
        extern float hapticOverrideValue;
        extern unsigned long hapticOverrideEndTime;
        hapticOverrideActive = true;
        hapticOverrideValue = 0.3f; // Option nav intensity
        hapticOverrideEndTime = millis() + 100; // 100ms burst
        updateQuiz();
    }
    if (buttonPressed) {
        // Haptic feedback on quiz option selection
        extern bool hapticOverrideActive;
        extern float hapticOverrideValue;
        extern unsigned long hapticOverrideEndTime;
        hapticOverrideActive = true;
        hapticOverrideValue = 0.6f; // Selection intensity
        hapticOverrideEndTime = millis() + 200; // 200ms burst
        if (quizState.selectedIndex == quizState.correctIndex) {
            quizState.answeredCorrectly = true;
            quizState.showHint = false;
            updateQuiz();
            // Now allow user to re-enter warp (handled in main loop)
        } else {
            quizState.showHint = true;
            quizState.answeredCorrectly = false;
            updateQuiz();
        }
    }
}

#endif // QUIZ_MODE_H
