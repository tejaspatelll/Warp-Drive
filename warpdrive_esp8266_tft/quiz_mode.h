#ifndef QUIZ_MODE_H
#define QUIZ_MODE_H

#include <TFT_eSPI.h>
#include <Arduino.h>
#include "quiz_popup.h"

extern QuizPopupState quizPopupState;
#include "story_mode.h" // For StoryStop and draw functions

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
    int total = STORY_STOPS_COUNT;
    // Defensive: ensure at least 4 options available
    if (total < 4) return;
    // Pick correct answer first
    int correctIdx = random(total);
    quizState.quizObjectIndex = correctIdx;
    int used[STORY_STOPS_COUNT] = {0};
    used[correctIdx] = 1;
    // Fill options array with correct answer and 3 unique distractors
    int opt[4];
    opt[0] = correctIdx;
    int count = 1;
    while (count < 4) {
        int idx = random(total);
        if (!used[idx]) {
            opt[count++] = idx;
            used[idx] = 1;
        }
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
    setupQuizOptions();
    quizPopupState.active = false;
    tft.fillScreen(BG_COLOR);
    uiInitialized = false;
    updateQuiz();
}

void updateQuiz() {
    // Always update the animation (object)
    objectX = SCREEN_WIDTH / 2;
    int objectOffsetY = SCREEN_HEIGHT / 8;
    objectY = SCREEN_HEIGHT / 2 - objectOffsetY;
    objectScale = 1.5;
    const StoryStop& obj = storyStops[quizState.quizObjectIndex];
    obj.drawFunction();

    // Draw static UI only once or if screen size changes
    if (!uiInitialized) {
        tft.fillScreen(BG_COLOR);

        // Title
        tft.setTextDatum(MC_DATUM);
        tft.setTextColor(tft.color565(0, 200, 255));
        tft.setTextSize(SCREEN_HEIGHT > 160 ? 2 : 1);
        tft.drawString("Identify the Object!", SCREEN_WIDTH / 2, SCREEN_HEIGHT / 16);

        // Selector box at the bottom, edge-to-edge
        int boxMargin = SCREEN_WIDTH / 16;
        int boxW = SCREEN_WIDTH - 2 * boxMargin;
        int boxH = SCREEN_HEIGHT / 10;
        int boxX = boxMargin;
        int boxY = SCREEN_HEIGHT - boxH - SCREEN_HEIGHT / 24;

        // Shadow
        tft.fillRoundRect(boxX, boxY + 3, boxW, boxH, boxH / 3, tft.color565(30, 30, 60));
        // Main box
        tft.fillRoundRect(boxX, boxY, boxW, boxH, boxH / 3, tft.color565(60, 0, 120));
        // Highlight border
        tft.drawRoundRect(boxX, boxY, boxW, boxH, boxH / 3, tft.color565(255, 255, 0));

        // Up/down arrows (right side of box)
        int arrowX = boxX + boxW - boxH / 2;
        int upY = boxY + boxH / 4, downY = boxY + boxH * 3 / 4;
        int arrowSize = boxH / 5;
        tft.fillTriangle(
            arrowX, upY,
            arrowX + arrowSize, upY,
            arrowX + arrowSize / 2, upY - arrowSize,
            tft.color565(255,255,0)
        );
        tft.fillTriangle(
            arrowX, downY,
            arrowX + arrowSize, downY,
            arrowX + arrowSize / 2, downY + arrowSize,
            tft.color565(255,255,0)
        );

        // Instructions at very bottom
        tft.setTextColor(tft.color565(0, 255, 0));
        tft.setTextSize(1);
        tft.setTextDatum(MC_DATUM);
        tft.drawString("KNOB: Select  |  BTN: Confirm", SCREEN_WIDTH / 2, SCREEN_HEIGHT - 4);

        uiInitialized = true;
        // Force redraw of dynamic elements
        lastSelectedIndex = -1;
        lastAnsweredCorrectly = false;
        lastShowHint = false;
        lastOptionLabel = "";
        lastFeedback = "";
        lastFact = "";
    }

    // --- Dynamic UI: Selector label ---
    int boxMargin = SCREEN_WIDTH / 16;
    int boxW = SCREEN_WIDTH - 2 * boxMargin;
    int boxH = SCREEN_HEIGHT / 10;
    int boxX = boxMargin;
    int boxY = SCREEN_HEIGHT - boxH - SCREEN_HEIGHT / 24;

    String optionLabel = storyStops[quizState.optionIndices[quizState.selectedIndex]].name;
    if (quizState.selectedIndex != lastSelectedIndex || optionLabel != lastOptionLabel) {
        // Erase previous label area
        tft.fillRoundRect(boxX+2, boxY+2, boxW-4, boxH-4, boxH / 3 - 2, tft.color565(60, 0, 120));
        // Draw new label
        tft.setTextDatum(MC_DATUM);
        tft.setTextColor(tft.color565(255, 255, 0), tft.color565(60, 0, 120));
        tft.setTextSize(SCREEN_HEIGHT > 160 ? 2 : 1);
        tft.drawString(optionLabel, SCREEN_WIDTH / 2, boxY + boxH / 2 + 1);
        lastSelectedIndex = quizState.selectedIndex;
        lastOptionLabel = optionLabel;
    }

    // --- Dynamic UI: Feedback ---
    int feedbackY = boxY - boxH - SCREEN_HEIGHT / 40;
    String feedback, fact;
    bool showFeedback = false;
    if (quizState.answeredCorrectly) {
        feedback = "Correct! Ready for warp.";
        fact = storyStops[quizState.quizObjectIndex].fact;
        showFeedback = true;
    } else if (quizState.showHint) {
        feedback = "Try again!";
        fact = storyStops[quizState.quizObjectIndex].fact;
        showFeedback = true;
    }
    if (showFeedback && (feedback != lastFeedback || fact != lastFact ||
                         quizState.answeredCorrectly != lastAnsweredCorrectly ||
                         quizState.showHint != lastShowHint)) {
        // Erase previous feedback area
        int feedbackH = (SCREEN_HEIGHT > 160 ? 24 : 12) * 2;
        tft.fillRect(0, feedbackY, SCREEN_WIDTH, feedbackH, BG_COLOR);

        // Draw feedback
        tft.setTextDatum(MC_DATUM);
        tft.setTextColor(quizState.answeredCorrectly ? tft.color565(0, 255, 0) : tft.color565(255, 0, 0), BG_COLOR);
        tft.setTextSize(SCREEN_HEIGHT > 160 ? 2 : 1);
        tft.drawString(feedback, SCREEN_WIDTH / 2, feedbackY);
        lastFeedback = feedback;
        lastFact = fact;
        lastAnsweredCorrectly = quizState.answeredCorrectly;
        lastShowHint = quizState.showHint;
    } else if (!showFeedback && (lastFeedback != "" || lastFact != "")) {
        // Erase feedback if not needed
        int feedbackH = (SCREEN_HEIGHT > 160 ? 24 : 12) * 2;
        tft.fillRect(0, feedbackY, SCREEN_WIDTH, feedbackH, BG_COLOR);
        lastFeedback = "";
        lastFact = "";
        lastAnsweredCorrectly = false;
        lastShowHint = false;
    }

    if (quizState.answeredCorrectly) {
        setQuizPopupFact(storyStops[quizState.quizObjectIndex].fact);
        showQuizPopup(true); // Correct
    } else if (quizState.showHint) {
        setQuizPopupFact(storyStops[quizState.quizObjectIndex].fact);
        showQuizPopup(false); // Wrong
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
