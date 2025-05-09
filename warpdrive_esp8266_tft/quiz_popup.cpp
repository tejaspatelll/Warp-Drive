// quiz_popup.cpp - Retro quiz result pop-up for Quiz Mode
#include "quiz_popup.h"


QuizPopupState quizPopupState = {false, false, 0};

// Retro colors
static const uint16_t POPUP_BG = 0x2104; // dark blue
static const uint16_t POPUP_BORDER = 0xFFE0; // yellow
static const uint16_t BUTTON_BG = 0x39E7; // teal
static const uint16_t BUTTON_SELECTED = 0xF800; // red
static const uint16_t BUTTON_TEXT = 0xFFFF; // white
static const uint16_t TEXT_CORRECT = 0x07E0; // green
static const uint16_t TEXT_WRONG = 0xF800; // red

// Retro pop-up UI dirty flags
static bool popupBgDrawn = false;
static bool popupLastCorrect = false;
static int popupLastSel = -1;

static String popupFact = "";

void setQuizPopupFact(const String& fact) {
    popupFact = fact;
}

void showQuizPopup(bool correct) {
    quizPopupState.active = true;
    quizPopupState.correct = correct;
    quizPopupState.selectedButton = 0;
    quizPopupState.waitForButtonRelease = true;
    popupBgDrawn = false; // Force full redraw
    popupLastCorrect = !correct; // Force full redraw
    popupLastSel = -1;
    updateQuizPopup();
}

// Helper: Draw wrapped text centered in a box
void drawWrappedText(const String& text, int x, int y, int w, int lineHeight, uint16_t color, uint16_t bg) {
    int cursorY = y;
    int start = 0;
    int len = text.length();
    while (start < len) {
        int end = start;
        int lastSpace = -1;
        int lineWidth = 0;
        // Find the longest substring that fits
        while (end < len) {
            String sub = text.substring(start, end + 1);
            int subWidth = tft.textWidth(sub);
            if (subWidth > w) break;
            if (text[end] == ' ') lastSpace = end;
            end++;
        }
        // If we broke in the middle of a word, go back to last space
        if (end < len && lastSpace > start) {
            end = lastSpace;
        }
        String line = text.substring(start, end);
        line.trim();
        tft.setTextDatum(MC_DATUM);
        tft.setTextColor(color, bg);
        tft.drawString(line, x, cursorY);
        cursorY += lineHeight;
        // Skip spaces
        while (end < len && text[end] == ' ') end++;
        start = end;
    }
}

void drawPopupBackground(bool correct) {
    tft.fillScreen(POPUP_BG);

    // --- Retro Box Layout ---
    int boxW = SCREEN_WIDTH * 0.8;
    int boxH = SCREEN_HEIGHT * 0.5;
    int boxX = (SCREEN_WIDTH - boxW) / 2;
    int boxY = (SCREEN_HEIGHT - boxH) / 2;

    int radius = boxH / 6;

    // Main box with thick border
    tft.fillRoundRect(boxX, boxY, boxW, boxH, radius, tft.color565(30, 30, 60));
    tft.drawRoundRect(boxX, boxY, boxW, boxH, radius, POPUP_BORDER);
    tft.drawRoundRect(boxX+3, boxY+3, boxW-6, boxH-6, radius-3, POPUP_BORDER);

    // Scanlines
    for (int y = boxY + 6; y < boxY + boxH - 6; y += 4) {
        tft.drawFastHLine(boxX + 8, y, boxW - 16, tft.color565(24,24,48));
    }

    // Corner dots
    int dotR = 3;
    tft.fillCircle(boxX + radius/2, boxY + radius/2, dotR, correct ? TEXT_CORRECT : TEXT_WRONG);
    tft.fillCircle(boxX + boxW - radius/2, boxY + radius/2, dotR, correct ? TEXT_CORRECT : TEXT_WRONG);
    tft.fillCircle(boxX + radius/2, boxY + boxH - radius/2, dotR, correct ? TEXT_CORRECT : TEXT_WRONG);
    tft.fillCircle(boxX + boxW - radius/2, boxY + boxH - radius/2, dotR, correct ? TEXT_CORRECT : TEXT_WRONG);

    // --- Title ---
    tft.setTextDatum(MC_DATUM);
    tft.setTextSize(SCREEN_HEIGHT > 160 ? 2 : 1);
    tft.setTextColor(correct ? TEXT_CORRECT : TEXT_WRONG, POPUP_BG);
    tft.drawString(correct ? "CORRECT" : "WRONG!", SCREEN_WIDTH / 2, boxY + radius);

    // --- Message ---
    tft.setTextSize(1);
    tft.setTextColor(BUTTON_TEXT, POPUP_BG);
    tft.drawString(correct ? "You got it right" : "Try again!", SCREEN_WIDTH / 2, boxY + boxH / 2 - 22);

    // --- Fact/info (wrapped) ---
    tft.setTextSize(1);
    int factBoxW = boxW - 24;
    int factY = boxY + boxH / 2 - 6;
    int lineHeight = 12;
    drawWrappedText(popupFact, SCREEN_WIDTH / 2, factY, factBoxW, lineHeight, tft.color565(200, 220, 255), POPUP_BG);

    // --- Retro indicator ---
    tft.setTextColor(tft.color565(255,255,0), POPUP_BG);
    //tft.drawString("TURN KNOB & SELECT", SCREEN_WIDTH / 2, boxY + boxH - radius);
}

void drawPopupButtons(int sel, bool correct) {
    // Button layout: two buttons, spaced evenly at the bottom of the popup box
    int boxW = SCREEN_WIDTH * 0.8;
    int boxH = SCREEN_HEIGHT * 0.5;
    int boxX = (SCREEN_WIDTH - boxW) / 2;
    int boxY = (SCREEN_HEIGHT - boxH) / 2;
    int radius = boxH / 6;

    // --- Button sizing and spacing ---
    int btnW = boxW / 3.2; // Make buttons a bit narrower
    int btnH = SCREEN_HEIGHT / 14;
    int gap = boxW / 12;   // Minimum gap between buttons

    // Calculate total width of both buttons and the gap
    int totalBtnW = btnW * 2 + gap;
    int btnY = boxY + boxH - btnH - radius/2;
    // Center the buttons horizontally in the popup
    int btn1X = boxX + (boxW - totalBtnW) / 2;
    int btn2X = btn1X + btnW + gap;

    // Left button: Try Again / Next
    tft.fillRoundRect(btn1X, btnY, btnW, btnH, btnH/3, sel == 0 ? BUTTON_SELECTED : BUTTON_BG);
    tft.drawRoundRect(btn1X, btnY, btnW, btnH, btnH/3, POPUP_BORDER);
    tft.setTextColor(BUTTON_TEXT, sel == 0 ? BUTTON_SELECTED : BUTTON_BG);
    tft.setTextDatum(MC_DATUM);
    tft.setTextSize(1);
    tft.drawString(correct ? "NEXT" : "TRY AGAIN", btn1X + btnW/2, btnY + btnH/2 + 1);

    // Right button: Menu
    tft.fillRoundRect(btn2X, btnY, btnW, btnH, btnH/3, sel == 1 ? BUTTON_SELECTED : BUTTON_BG);
    tft.drawRoundRect(btn2X, btnY, btnW, btnH, btnH/3, POPUP_BORDER);
    tft.setTextColor(BUTTON_TEXT, sel == 1 ? BUTTON_SELECTED : BUTTON_BG);
    tft.drawString("MENU", btn2X + btnW/2, btnY + btnH/2 + 1);

    // Arrow indicator under selected button
    int arrowY = btnY + btnH + 2;
    int arrowX = (sel == 0) ? (btn1X + btnW/2 - 3) : (btn2X + btnW/2 - 3);
    tft.fillTriangle(arrowX, arrowY, arrowX+6, arrowY, arrowX+3, arrowY+6, tft.color565(255,255,0));
}

void updateQuizPopup() {
    // Redraw background only if needed
    if (!popupBgDrawn || popupLastCorrect != quizPopupState.correct) {
        drawPopupBackground(quizPopupState.correct);
        popupBgDrawn = true;
        popupLastCorrect = quizPopupState.correct;
        popupLastSel = -1; // force redraw buttons
    }
    // Only redraw buttons if selection changed or forced
    if (popupLastSel != quizPopupState.selectedButton) {
        drawPopupButtons(quizPopupState.selectedButton, quizPopupState.correct);
        popupLastSel = quizPopupState.selectedButton;
    }
}

int processQuizPopupInput(bool buttonPressed) {
    // Potentiometer: 0-4095, <2048 = left, >=2048 = right
    int sel = (potValue < 2048) ? 0 : 1;
    if (quizPopupState.selectedButton != sel) {
        quizPopupState.selectedButton = sel;
        extern bool hapticOverrideActive;
        extern float hapticOverrideValue;
        extern unsigned long hapticOverrideEndTime;
        hapticOverrideActive = true;
        hapticOverrideValue = 0.3f; // Popup nav intensity
        hapticOverrideEndTime = millis() + 100; // 100ms burst
        updateQuizPopup();
    }
    // Debounce: Wait for button to be released after popup appears
    if (quizPopupState.waitForButtonRelease) {
        if (!buttonPressed) {
            quizPopupState.waitForButtonRelease = false;
        }
        return 0;
    }
    if (buttonPressed) {
        extern bool hapticOverrideActive;
        extern float hapticOverrideValue;
        extern unsigned long hapticOverrideEndTime;
        hapticOverrideActive = true;
        hapticOverrideValue = 0.6f; // Popup select intensity
        hapticOverrideEndTime = millis() + 200; // 200ms burst
        int result = quizPopupState.selectedButton + 1;
        // Hide popup after selection
        popupBgDrawn = false; // Force full redraw next time
        hideQuizPopup();
        return result;
    }
    return 0;
}

void hideQuizPopup() {
    quizPopupState.active = false;
}
