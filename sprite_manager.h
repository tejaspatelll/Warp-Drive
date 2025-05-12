// sprite_manager.h
#ifndef SPRITE_MANAGER_H
#define SPRITE_MANAGER_H

#include <TFT_eSPI.h>
#include <Arduino.h>

extern TFT_eSPI tft;
extern const int SCREEN_WIDTH;
extern const int SCREEN_HEIGHT;
extern uint16_t BG_COLOR;

class SpriteManager {
public:
  // Overload for square sprites (maintains previous simple calls if needed)
  static void createObjectSprite(TFT_eSprite& sprite, int requestedSize, const char* objectName, bool preferPsram) {
    createObjectSprite(sprite, requestedSize, requestedSize, objectName, preferPsram);
  }

  // Creates a sprite (rectangular). Caller is responsible for explicit deletion via safeDeleteSprite().
  static void createObjectSprite(TFT_eSprite& sprite, int requestedWidth, int requestedHeight, const char* objectName, bool preferPsram) {
    uint32_t initialFreeHeap = 0;
    uint32_t initialFreePsram = 0;
    uint32_t requiredMem = requestedWidth * requestedHeight; // 8-bit color depth = 1 byte per pixel

    #ifdef ESP32
    initialFreeHeap = ESP.getFreeHeap();
    initialFreePsram = ESP.getFreePsram();
    
    Serial.printf("[%s Sprite] Initial Check: ReqSize %dx%d (%u bytes). Prefer PSRAM: %s. Free Heap: %u, Free PSRAM: %u\n", 
                 objectName, requestedWidth, requestedHeight, requiredMem, preferPsram ? "Yes" : "No", initialFreeHeap, initialFreePsram);

    bool canTryPsram = preferPsram && (initialFreePsram > requiredMem + 1024);
    bool canTryHeap = !preferPsram && (initialFreeHeap > requiredMem + 1024);
    bool chosenPathIsPsram = false;

    if (preferPsram) {
        if (canTryPsram) {
            chosenPathIsPsram = true;
        } else if (initialFreeHeap > requiredMem + 1024) { 
            Serial.printf("[%s Sprite] WARNING: Preferred PSRAM but insufficient. Attempting Heap.\n", objectName);
            chosenPathIsPsram = false;
        } else {
            Serial.printf("[%s Sprite] ERROR: Insufficient memory for %dx%d in PSRAM or Heap. Aborting.\n", objectName, requestedWidth, requestedHeight);
            return;
        }
    } else { // prefer HEAP
        if (canTryHeap) {
            chosenPathIsPsram = false;
        } else if (initialFreePsram > requiredMem + 1024) {
             Serial.printf("[%s Sprite] WARNING: Preferred HEAP but insufficient. Attempting PSRAM.\n", objectName);
            chosenPathIsPsram = true;
        } else {
            Serial.printf("[%s Sprite] ERROR: Insufficient memory for %dx%d in Heap or PSRAM. Aborting.\n", objectName, requestedWidth, requestedHeight);
            return;
        }
    }

    int finalWidth = requestedWidth;
    int finalHeight = requestedHeight;

    // Size reduction logic (very basic for now, can be more sophisticated)
    // This part might need more thought for rectangular sprites if one dimension is much larger.
    // For now, it simplifies to a general memory check.
    if ((chosenPathIsPsram && initialFreePsram < requiredMem * 1.5) || (!chosenPathIsPsram && initialFreeHeap < requiredMem * 1.5)) {
        // If memory is tight, one option is to try to shrink, e.g., proportionally or to fixed smaller sizes.
        // For now, we'll just log and proceed if it passed the earlier check, 
        // or rely on the critical check below to abort if still too small.
        Serial.printf("[%s Sprite] INFO: Memory seems tight for %dx%d. Proceeding with caution.\n", objectName, finalWidth, finalHeight);
        // Example reduction (needs better logic for rectangles):
        // if (finalWidth > 64) finalWidth = 64;
        // if (finalHeight > 64) finalHeight = 64;
        // requiredMem = finalWidth * finalHeight;
    }
    
    // Ensure dimensions are at least 4x4 and even
    finalWidth = std::max(4, finalWidth);
    finalHeight = std::max(4, finalHeight);
    if (finalWidth % 2 != 0) finalWidth += 1;
    if (finalHeight % 2 != 0) finalHeight += 1;
        
    requiredMem = finalWidth * finalHeight; // Recalculate requiredMem for final dimensions

    if ((chosenPathIsPsram && initialFreePsram < requiredMem + 512) || (!chosenPathIsPsram && initialFreeHeap < requiredMem + 512)) {
        Serial.printf("[%s Sprite] CRITICAL MEMORY after final size adjustment for %dx%d. Aborting.\n", objectName, finalWidth, finalHeight);
        return;
    }

    if (sprite.getPointer() != nullptr) {
        Serial.printf("[%s Sprite] WARNING: createObjectSprite called on an already initialized sprite. Explicitly delete first for safety.\n", objectName);
    }
    #else // Not ESP32
    int finalWidth = requestedWidth;
    int finalHeight = requestedHeight;
    if (finalWidth % 2 != 0) finalWidth += 1;
    if (finalHeight % 2 != 0) finalHeight += 1;
    finalWidth = std::max(4, std::min(finalWidth, SCREEN_WIDTH)); // Cap at screen dimensions
    finalHeight = std::max(4, std::min(finalHeight, SCREEN_HEIGHT));
    bool chosenPathIsPsram = false;
    #endif

    sprite.setColorDepth(8);
    sprite.setAttribute(PSRAM_ENABLE, chosenPathIsPsram);
    
    #ifdef ESP32
    Serial.printf("[%s Sprite] Attempting %dx%d. Target: %s. Heap: %u, PSRAM: %u\n", 
                  objectName, finalWidth, finalHeight, chosenPathIsPsram ? "PSRAM" : "Heap", ESP.getFreeHeap(), ESP.getFreePsram());
    #else
    Serial.printf("[%s Sprite] Attempting %dx%d on non-ESP32 target (Heap assumed)\n", objectName, finalWidth, finalHeight);
    #endif
    
    bool success = false;
    if (finalWidth > 0 && finalHeight > 0) {
        success = sprite.createSprite(finalWidth, finalHeight);
    }
    
    if (success && sprite.width() > 0 && sprite.height() > 0) {
        sprite.fillSprite(BG_COLOR);
        #ifdef ESP32
        bool inPsramResult = SpriteManager::isInPSRAM(sprite);
        uint32_t currentFreeHeap = ESP.getFreeHeap();
        uint32_t currentFreePsram = ESP.getFreePsram();
        Serial.printf("[%s Sprite] Created %dx%d. Allocated in: %s. Heap: %u, PSRAM free: %u\n", 
                      objectName, sprite.width(), sprite.height(), // Use actual created dimensions
                      inPsramResult ? "PSRAM" : "HEAP", 
                      currentFreeHeap, currentFreePsram);
        
        if (chosenPathIsPsram && inPsramResult && currentFreePsram == 0 && initialFreePsram > requiredMem) {
            Serial.printf("[%s Sprite] CRITICAL WARNING: PSRAM allocation success reported, but Free PSRAM is now 0! PSRAM heap may be unstable.\n", objectName);
        }
        if (chosenPathIsPsram && !inPsramResult && initialFreePsram > requiredMem) {
             Serial.printf("[%s Sprite] WARNING: Sprite allocated in HEAP despite PSRAM preference and apparent availability.\n", objectName);
        }
        #else
        Serial.printf("[%s Sprite] Created %dx%d.\n", objectName, sprite.width(), sprite.height());
        #endif
    } else {
        #ifdef ESP32
        Serial.printf("[%s Sprite] FAILED to create %dx%d. Heap: %u, PSRAM free: %u\n", 
                      objectName, finalWidth, finalHeight, ESP.getFreeHeap(), ESP.getFreePsram());
        #else
        Serial.printf("[%s Sprite] FAILED to create %dx%d.\n", objectName, finalWidth, finalHeight);
        #endif
    }
  }
  
  // Safely delete a sprite.
  static void safeDeleteSprite(TFT_eSprite& sprite, const char* objectName) {
    if (sprite.getPointer() != nullptr) { 
      #ifdef ESP32
      uint32_t heapBefore = ESP.getFreeHeap();
      uint32_t psramBefore = ESP.getFreePsram();
      Serial.printf("[%s Sprite] Deleting sprite %dx%d. Heap before: %u, PSRAM before: %u\n", 
                    objectName, sprite.width(), sprite.height(), heapBefore, psramBefore);
      #else
      Serial.printf("[%s Sprite] Deleting sprite %dx%d\n", objectName, sprite.width(), sprite.height());
      #endif
      
      sprite.deleteSprite(); 
      
      #ifdef ESP32
      Serial.printf("[%s Sprite] After delete. Heap: %u, PSRAM: %u\n", 
                    objectName, ESP.getFreeHeap(), ESP.getFreePsram());
      #else
      Serial.printf("[%s Sprite] After delete.\n", objectName);
      #endif
    } else {
      Serial.printf("[%s Sprite] safeDeleteSprite: No active buffer to delete.\n", objectName);
    }
  }
  
  static bool isInPSRAM(TFT_eSprite& sprite) {
    #ifdef ESP32
    uint8_t* spriteBuffer = (uint8_t*)sprite.getPointer();
    if (spriteBuffer != nullptr) {
      return esp_ptr_external_ram(spriteBuffer);
    }
    #endif
    return false; 
  }
};

#endif // SPRITE_MANAGER_H 