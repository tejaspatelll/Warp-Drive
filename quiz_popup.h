// quiz_popup.h - Retro quiz result pop-up for Quiz Mode
#ifndef QUIZ_POPUP_H
#define QUIZ_POPUP_H

#include <TFT_eSPI.h>
#include <Arduino.h>

// Quiz result popup state
struct QuizPopupState {
    bool active;         // Is the popup currently active?
    bool correct;        // Was the answer correct?
    int selectedButton;  // 0 = Try Again/Next, 1 = Menu
    bool waitForButtonRelease; // Ignore button press until released after popup shown
};

extern TFT_eSPI tft;
extern uint16_t BG_COLOR;
extern int potValue;
extern const int SCREEN_WIDTH;
extern const int SCREEN_HEIGHT;

// Call to show the popup
void showQuizPopup(bool correct);
// Call to update and redraw the popup UI
void updateQuizPopup();
// Call to handle input for the popup
// Returns: 0 = nothing, 1 = left button, 2 = right button
int processQuizPopupInput(bool buttonPressed);
// Call to hide the popup
void hideQuizPopup();
// Expose the popup state
extern QuizPopupState quizPopupState;
void setQuizPopupFact(const String& fact);

#endif // QUIZ_POPUP_H
