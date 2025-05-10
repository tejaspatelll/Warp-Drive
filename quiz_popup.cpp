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

// Helper: Calculate text height for wrapped text
int calculateWrappedTextHeight(const String& text, int maxWidth) {
    int lineHeight = 12;
    int lines = 1;
    int start = 0;
    int len = text.length();
    
    while (start < len) {
        int end = start;
        int lastSpace = -1;
        while (end < len) {
            String sub = text.substring(start, end + 1);
            int subWidth = tft.textWidth(sub);
            if (subWidth > maxWidth) break;
            if (text[end] == ' ') lastSpace = end;
            end++;
        }
        if (end < len && lastSpace > start) {
            end = lastSpace;
        }
        lines++;
        while (end < len && text[end] == ' ') end++;
        start = end;
    }
    return lines * lineHeight;
}

void drawPopupBackground(bool correct) {
    tft.fillScreen(BG_COLOR);

    // Draw retro grid background
    const int gridSize = 16;
    for (int x = 0; x < SCREEN_WIDTH; x += gridSize) {
        for (int y = 0; y < SCREEN_HEIGHT; y += gridSize) {
            tft.drawRect(x, y, gridSize, gridSize, tft.color565(0, 20, 40));
        }
    }

    // --- Retro Box Layout ---
    int boxW = SCREEN_WIDTH * 0.85;
    int boxH = SCREEN_HEIGHT * 0.6;
    int boxX = (SCREEN_WIDTH - boxW) / 2;
    int boxY = (SCREEN_HEIGHT - boxH) / 2;

    // Main box with retro styling
    tft.fillRect(boxX + 4, boxY + 4, boxW - 8, boxH - 8, tft.color565(0, 40, 80));
    tft.drawRect(boxX + 2, boxY + 2, boxW - 4, boxH - 4, tft.color565(0, 160, 255));
    tft.drawRect(boxX, boxY, boxW, boxH, POPUP_BORDER);

    // Add scanlines inside the box
    for (int y = boxY + 6; y < boxY + boxH - 6; y += 3) {
        tft.drawFastHLine(boxX + 6, y, boxW - 12, tft.color565(0, 30, 60));
    }

    // Pixel corners
    int cornerSize = 6;
    for(int i = 0; i < cornerSize; i++) {
        for(int j = 0; j < cornerSize-i; j++) {
            uint16_t color = correct ? TEXT_CORRECT : TEXT_WRONG;
            // Top left
            tft.drawPixel(boxX + i, boxY + j, color);
            // Top right
            tft.drawPixel(boxX + boxW - 1 - i, boxY + j, color);
            // Bottom left
            tft.drawPixel(boxX + i, boxY + boxH - 1 - j, color);
            // Bottom right
            tft.drawPixel(boxX + boxW - 1 - i, boxY + boxH - 1 - j, color);
        }
    }

    // --- Title with retro shadow effect ---
    tft.setTextDatum(MC_DATUM);
    tft.setTextSize(2);
    
    // Draw shadow layers
    for(int i = 3; i > 0; i--) {
        tft.setTextColor(correct ? 
            tft.color565(0, 100 - i*20, 0) : 
            tft.color565(100 - i*20, 0, 0));
        tft.drawString(correct ? "CORRECT!" : "TRY AGAIN", 
            SCREEN_WIDTH/2 + i, boxY + 25 + i);
    }
    
    // Main title text
    tft.setTextColor(correct ? TEXT_CORRECT : TEXT_WRONG);
    tft.drawString(correct ? "CORRECT!" : "TRY AGAIN", SCREEN_WIDTH/2, boxY + 25);

    // Add pixel-style underline
    for(int i = 0; i < 2; i++) {
        tft.drawFastHLine(SCREEN_WIDTH/2 - 60 + i*2, boxY + 40 + i, 120 - i*4,
            correct ? TEXT_CORRECT : TEXT_WRONG);
    }

    // --- Message with glow effect ---
    tft.setTextSize(1);
    tft.setTextColor(tft.color565(100, 200, 255));
    tft.drawString(correct ? "You got it right!" : "Keep exploring!", 
        SCREEN_WIDTH/2 + 1, boxY + 60);
    tft.setTextColor(BUTTON_TEXT);
    tft.drawString(correct ? "You got it right!" : "Keep exploring!", 
        SCREEN_WIDTH/2, boxY + 59);

    // --- Fact/info with retro style ---
    int factBoxW = boxW - 24;
    int factY = boxY + boxH/2 - 20;
    int lineHeight = 12;
    
    // Calculate required height for text
    int textWidth = factBoxW - 20; // Account for padding
    int textHeight = calculateWrappedTextHeight(popupFact, textWidth);
    
    // Add padding to the box height
    int factBoxH = textHeight + 20; // 10px padding top and bottom
    int factBoxX = SCREEN_WIDTH/2 - factBoxW/2;
    
    // Draw fact box background
    tft.fillRect(factBoxX, factY - 10, factBoxW, factBoxH, tft.color565(0, 30, 60));
    tft.drawRect(factBoxX, factY - 10, factBoxW, factBoxH, tft.color565(0, 100, 200));
    
    drawWrappedText(popupFact, SCREEN_WIDTH/2, factY, 
        factBoxW - 10, lineHeight, tft.color565(200, 220, 255), tft.color565(0, 30, 60));
}

void drawPopupButtons(int sel, bool correct) {
    // Button layout with retro styling
    int boxW = SCREEN_WIDTH * 0.85;
    int boxH = SCREEN_HEIGHT * 0.6;
    int boxX = (SCREEN_WIDTH - boxW) / 2;
    int boxY = (SCREEN_HEIGHT - boxH) / 2;

    int btnW = boxW / 3;
    int btnH = SCREEN_HEIGHT / 12;
    int gap = boxW / 8;
    int totalBtnW = btnW * 2 + gap;
    int btnY = boxY + boxH - btnH - 25;
    int btn1X = boxX + (boxW - totalBtnW) / 2;
    int btn2X = btn1X + btnW + gap;

    // Clear previous button areas including arrow spaces
    for(int i = 0; i < 2; i++) {
        int x = (i == 0) ? btn1X : btn2X;
        // Clear button area and surrounding space for arrows
        tft.fillRect(x - 20, btnY - 2, btnW + 40, btnH + 4, tft.color565(0, 40, 80));
    }

    // Draw buttons with retro styling
    for(int i = 0; i < 2; i++) {
        int x = (i == 0) ? btn1X : btn2X;
        bool isSelected = (sel == i);
        
        // Button background
        tft.fillRect(x + 2, btnY + 2, btnW - 4, btnH - 4, 
            isSelected ? tft.color565(0, 80, 160) : tft.color565(0, 40, 80));
        
        // Button border
        tft.drawRect(x, btnY, btnW, btnH, 
            isSelected ? POPUP_BORDER : tft.color565(0, 100, 200));
        
        // Button text with glow effect
        tft.setTextDatum(MC_DATUM);
        tft.setTextSize(1);
        
        if(isSelected) {
            tft.setTextColor(tft.color565(100, 200, 255));
            tft.drawString(i == 0 ? (correct ? "NEXT" : "TRY AGAIN") : "MENU",
                x + btnW/2 + 1, btnY + btnH/2 + 1);
            
            // Add selection arrows if selected
            int arrowSize = 6;
            for(int j = 0; j < 3; j++) {
                // Left arrow
                tft.fillTriangle(
                    x - 8 - j*2, btnY + btnH/2,
                    x - 2 - j*2, btnY + btnH/2 - arrowSize,
                    x - 2 - j*2, btnY + btnH/2 + arrowSize,
                    tft.color565(255, 255, j * 85)
                );
                // Right arrow
                tft.fillTriangle(
                    x + btnW + 8 + j*2, btnY + btnH/2,
                    x + btnW + 2 + j*2, btnY + btnH/2 - arrowSize,
                    x + btnW + 2 + j*2, btnY + btnH/2 + arrowSize,
                    tft.color565(255, 255, j * 85)
                );
            }
        }
        
        tft.setTextColor(isSelected ? POPUP_BORDER : BUTTON_TEXT);
        tft.drawString(i == 0 ? (correct ? "NEXT" : "TRY AGAIN") : "MENU",
            x + btnW/2, btnY + btnH/2);
    }
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
