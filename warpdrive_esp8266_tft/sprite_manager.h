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
  // Enhanced sprite creation with stricter memory checks and safety features
  static void createObjectSprite(TFT_eSprite& sprite, int size, const char* objectName) {
    // First check if we even have enough memory for this sprite (approximate check)
    #ifdef ESP32
    uint32_t requiredMem = size * size; // 8-bit color depth = 1 byte per pixel

    // Enhanced memory check with lower thresholds
    uint32_t freeHeap = ESP.getFreeHeap();
    uint32_t freePsram = ESP.getFreePsram();
    
    // More conservative memory availability check
    bool memoryAvailable = ((freePsram > requiredMem * 2) || (freeHeap > requiredMem * 2)) && 
                           (freePsram > 10000) && (freeHeap > 10000);
    
    // Log the memory situation
    Serial.printf("[%s Sprite] Memory check: Needs %u bytes. Free PSRAM: %u, Heap: %u\n", 
                 objectName, requiredMem, freePsram, freeHeap);
    
    if (!memoryAvailable) {
      // If memory is tight, reduce size more aggressively
      int originalSize = size;
      if (size > 128) size = 128;
      else if (size > 64) size = 64;
      else size = 32; // Minimum reasonable size
      
      Serial.printf("[%s Sprite] MEMORY LOW - reduced size from %d to %d\n", 
                  objectName, originalSize, size);
      
      // If we're critically low on memory, abort sprite creation
      if (freeHeap < 8000 || freePsram < 8000) {
        Serial.printf("[%s Sprite] CRITICAL MEMORY - aborting sprite creation\n", objectName);
        // Mark sprite as invalid by setting dimensions to 0
        sprite.deleteSprite();
        return;
      }
    }
    
    // Ensure size is at least 4 pixels (min 2x2) but not too large
    size = std::max(4, size);
    size = std::min(size, 240); // Cap absolute max size regardless of calculation
    
    // Make sprite size even for better memory alignment
    if (size % 2 != 0) {
      size += 1;
    }
    #endif
    
    // Delete any existing sprite buffer to prevent memory leaks
    if (sprite.width() > 0 || sprite.height() > 0) {
      // Add a try-catch block to handle potential deletion issues
      #ifdef ESP32
      try {
      #endif
        sprite.deleteSprite();
        delay(10); // Small delay to allow memory management to complete
      #ifdef ESP32
      } catch (...) {
        Serial.printf("[%s Sprite] Error in sprite deletion\n", objectName);
      }
      #endif
    }
    
    // Set sprite properties
    sprite.setColorDepth(8); // Use 8-bit for memory efficiency
    sprite.setAttribute(PSRAM_ENABLE, true);
    
    #ifdef ESP32
    Serial.printf("[%s Sprite] Attempting %dx%d. Heap: %u, PSRAM: %u\n", 
                  objectName, size, size, ESP.getFreeHeap(), ESP.getFreePsram());
    #endif
    
    // Try to create the sprite - with size validation and error handling
    bool success = false;
    
    #ifdef ESP32
    try {
    #endif
      if (size > 0) {
        success = sprite.createSprite(size, size);
      }
    #ifdef ESP32
    } catch (...) {
      Serial.printf("[%s Sprite] Exception during sprite creation\n", objectName);
      success = false;
    }
    #endif
    
    // Check if creation was successful and validate dimensions
    if (success && sprite.width() > 0 && sprite.height() > 0) {
      // Fill with background color immediately to prevent garbage
      sprite.fillSprite(BG_COLOR);
      
      #ifdef ESP32
      bool inPsram = SpriteManager::isInPSRAM(sprite);
      Serial.printf("[%s Sprite] Created %dx%d. PSRAM used: %s. Heap: %u, PSRAM free: %u\n", 
                    objectName, size, size, 
                    inPsram ? "Yes" : "No", 
                    ESP.getFreeHeap(), ESP.getFreePsram());
      
      // If not in PSRAM when it should be, we might have memory issues
      if (!inPsram && ESP.getFreePsram() > requiredMem) {
        Serial.printf("[%s Sprite] WARNING: Sprite not in PSRAM despite available memory\n", objectName);
      }
      #else
      Serial.printf("[%s Sprite] Created %dx%d.\n", objectName, size, size);
      #endif
    } else {
      // Creation failed - try to handle gracefully
      if (sprite.width() > 0 || sprite.height() > 0) {
        // If dimensions are non-zero but creation "failed", try to delete
        sprite.deleteSprite();
      }
      
      #ifdef ESP32
      Serial.printf("[%s Sprite] FAILED to create %dx%d. Heap: %u, PSRAM free: %u\n", 
                    objectName, size, size, ESP.getFreeHeap(), ESP.getFreePsram());
      #else
      Serial.printf("[%s Sprite] FAILED to create %dx%d.\n", objectName, size, size);
      #endif
    }
  }
  
  // Safely delete a sprite with proper error handling
  static void safeDeleteSprite(TFT_eSprite& sprite, const char* objectName) {
    if (sprite.width() > 0 || sprite.height() > 0) {
      #ifdef ESP32
      Serial.printf("[%s Sprite] Deleting sprite %dx%d\n", 
                    objectName, sprite.width(), sprite.height());
      try {
      #endif
        sprite.deleteSprite();
      #ifdef ESP32
      } catch (...) {
        Serial.printf("[%s Sprite] Exception during sprite deletion\n", objectName);
      }
      Serial.printf("[%s Sprite] After delete - Heap: %u, PSRAM: %u\n", 
                    objectName, ESP.getFreeHeap(), ESP.getFreePsram());
      #endif
    }
  }
  
  // Check if a sprite buffer is in PSRAM
  static bool isInPSRAM(TFT_eSprite& sprite) {
    uint8_t* spriteBuffer = (uint8_t*)sprite.getPointer();
    if (spriteBuffer != nullptr) {
      #ifdef ESP32
      return esp_ptr_external_ram(spriteBuffer);
      #else
      return false;
      #endif
    }
    return false;
  }
};

#endif // SPRITE_MANAGER_H 