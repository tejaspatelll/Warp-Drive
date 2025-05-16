#include "sprite_manager.h"
#include <esp_heap_caps.h> // for PSRAM queries on ESP32

// static member definitions
std::vector<SpriteManager::Entry>  SpriteManager::_pool;
uint16_t                           SpriteManager::_nextId = 1;

// Generate next unique ID, skip reserved values
uint16_t SpriteManager::_generateNextId() {
    uint16_t id = _nextId++;
    // Skip invalid/reserved IDs: 0 and max (0xFFFF)
    if (_nextId == 0 || _nextId == 0xFFFF) {
        _nextId = 1;
    }
    return id;
}

void SpriteManager::begin() {
  _pool.clear();
  _pool.reserve(50); // Reserve space for 50 entries to reduce reallocations
  _nextId = 1;
  Serial.println("[SpriteMgr] Initialized. Pool capacity reserved for 50 entries.");
}

int16_t SpriteManager::_alignSize(int16_t v) {
  v = std::max<int16_t>(4, v);
  if (v & 1) v++;
  return v;
}

SpriteManager::Entry * SpriteManager::_findEntry(uint16_t id) {
  // Ensure ID is valid before searching
  if (id == 0 || id == 65535) return nullptr;
  for (auto &e : _pool) {
    // Check for matching ID and ensure the entry is marked as alive
    if (e.id == id && e.alive) {
      if (e.w <= 0 || e.h <= 0) {
        Serial.printf("[SpriteMgr] WARNING: Entry ID %u has invalid size (%d x %d), marking as dead\n", e.id, e.w, e.h);
        // Invalidate this entry to prevent future lookup storms
        e.alive = false;
        e.id = 0;
        e.w = 0;
        e.h = 0;
        e.inPsram = false;
        continue;  // skip invalid entry
      }
      if (e.id == 65535) {
        Serial.println("[SpriteMgr] WARNING: Found entry with invalid ID 65535, marking as dead!");
        e.alive = false;
        e.id = 0;
        continue;
      }
      return &e;
    }
  }
  return nullptr;
}

// Implementation for the added helper function
TFT_eSprite* SpriteManager::getSpriteRef(const SpriteHandle& h) {
    if (auto *e = _findEntry(h.id)) {
        return &(e->sprite);
    }
    return nullptr;
}

// Implementation for getting width
int16_t SpriteManager::getWidth(const SpriteHandle& h) {
    if (auto *e = _findEntry(h.id)) {
        return e->w;
    }
    return 0; // Return 0 if handle is invalid or entry not found
}

// Implementation for getting height
int16_t SpriteManager::getHeight(const SpriteHandle& h) {
    if (auto *e = _findEntry(h.id)) {
        return e->h;
    }
    return 0; // Return 0 if handle is invalid or entry not found
}

bool SpriteManager::_tryAllocate(Entry &e, int16_t w, int16_t h, bool usePsram) {
  // Ensure any previous buffer associated with this Entry's sprite object is freed
  e.sprite.deleteSprite(); 
  e.sprite.setColorDepth(8);
  e.sprite.setAttribute(PSRAM_ENABLE, usePsram ? 1 : 0);

  Serial.printf("[SpriteMgr] _tryAllocate: About to call e.sprite.createSprite(%d, %d) for ID %u\n", w, h, e.id);
  // Attempt to create the sprite buffer
  bool ok = e.sprite.createSprite(w, h);
  Serial.printf("[SpriteMgr] _tryAllocate: createSprite returned %s for ID %u. Sprite w: %d, h: %d\n", 
                ok ? "true" : "false", e.id, e.sprite.width(), e.sprite.height());

  if (!ok) { 
      Serial.printf("[SpriteMgr] Low-level createSprite(%d, %d, %s) failed! ID %u\n", w, h, usePsram ? "PSRAM" : "HEAP", e.id);
      return false;
  }
  
  // Check if the sprite buffer was actually created (width/height > 0)
  // Sometimes createSprite might return true but fail to allocate if memory is *very* fragmented
  if (e.sprite.width() <= 0 || e.sprite.height() <= 0) {
    Serial.printf("[SpriteMgr] createSprite reported success but resulted in 0 dimension sprite (%d x %d). ID %u\n", 
                  e.sprite.width(), e.sprite.height(), e.id);
    e.sprite.deleteSprite(); // Clean up potentially problematic state
    return false;
  }
  
  // Success - fill and update entry state
  e.sprite.fillSprite(BG_COLOR);
  e.w = e.sprite.width(); // Use actual width/height returned
  e.h = e.sprite.height();
  e.inPsram = usePsram;
  e.alive   = true;
  Serial.printf("[SpriteMgr] Successfully created sprite ID %u: %dx%d in %s\n", e.id, e.w, e.h, usePsram ? "PSRAM" : "HEAP");
  return true;
}

SpriteAllocResult SpriteManager::create(int16_t reqW,
                                        int16_t reqH,
                                        bool    preferPsram,
                                        SpriteHandle & outHandle)
{
  // param check
  if (reqW <= 0 || reqH <= 0) {
    Serial.printf("[SpriteMgr] Invalid sprite dimensions requested: %dx%d\n", reqW, reqH);
    outHandle.id = 0; // Ensure handle is invalid
    return SpriteAllocResult::InvalidParameters;
  }

  // align & cap requested size
  int16_t w = _alignSize(reqW);
  int16_t h = _alignSize(reqH);
  w = std::min<int16_t>(w, SCREEN_WIDTH);
  h = std::min<int16_t>(h, SCREEN_HEIGHT);

  // compute bytes needed (assuming 8-bit depth)
  size_t need = size_t(w) * size_t(h);

  // query available memory pools (add buffer for overhead)
  size_t freeHeap   = ESP.getFreeHeap();
  size_t freePsram  = ESP.getFreePsram();  
  const size_t buffer = 1024; // Allocation overhead buffer

  bool  canHeap  = freeHeap  >= (need + buffer);
  bool  canPsram = freePsram >= (need + buffer);
  
  Serial.printf("[SpriteMgr] Request %dx%d (%u bytes). Heap Free: %u, PSRAM Free: %u. Prefer PSRAM: %s\n",
                 w, h, (unsigned)need, (unsigned)freeHeap, (unsigned)freePsram, preferPsram ? "Yes" : "No");

  bool  usePsram = false;
  bool  fallback = false; // Track if we fell back
  SpriteAllocResult finalResult = SpriteAllocResult::Success; // Assume success initially

  // Determine allocation strategy
  if (preferPsram) {
      if (canPsram) {
          usePsram = true;
      } else if (canHeap) {
          usePsram = false;
          fallback = true;
          finalResult = SpriteAllocResult::FellBackToHeap;
          Serial.println("[SpriteMgr] WARN: Preferred PSRAM but insufficient. Falling back to Heap.");
      } else {
          // Neither preferred PSRAM nor fallback Heap available at current size
          Serial.println("[SpriteMgr] WARN: Insufficient PSRAM (preferred) or Heap for requested size.");
      }
  } else { // Prefer Heap
      if (canHeap) {
          usePsram = false;
      } else if (canPsram) {
          usePsram = true;
          fallback = true;
          // Note: This case doesn't have a specific result code in the enum, treat as Success.
          Serial.println("[SpriteMgr] WARN: Preferred Heap but insufficient. Falling back to PSRAM.");
      } else {
          // Neither preferred Heap nor fallback PSRAM available at current size
          Serial.println("[SpriteMgr] WARN: Insufficient Heap (preferred) or PSRAM for requested size.");
      }
  }

  // If neither pool worked at the requested size (or after fallback), try shrinking
  if (!canPsram && !canHeap && !fallback) { // Only shrink if primary and fallback failed initially
    int16_t sw = w/2, sh = h/2;
    sw = _alignSize(sw);
    sh = _alignSize(sh);
    size_t  sneed = size_t(sw)*size_t(sh);

    // Recheck availability with shrunk size
    bool canShrunkHeap = freeHeap >= (sneed + buffer);
    bool canShrunkPsram = freePsram >= (sneed + buffer);

    if ((preferPsram && canShrunkPsram) || 
        (!preferPsram && canShrunkHeap) ||
        canShrunkPsram || canShrunkHeap) // Check if *either* pool can fit the shrunk size
    { 
      Serial.printf("[SpriteMgr] INFO: Attempting to shrink from %dx%d to %dx%d due to low memory.\n", w,h, sw,sh);
      w = sw;
      h = sh;
      need = sneed;
      finalResult = SpriteAllocResult::SuccessShrunk;

      // Re-evaluate target pool based on shrunk size availability
      if (preferPsram) {
          usePsram = canShrunkPsram; // Use PSRAM if available (preferred)
          if (!usePsram && canShrunkHeap) {
             Serial.println("[SpriteMgr] INFO: Using Heap for shrunk sprite as PSRAM still insufficient.");
          } else if (!usePsram && !canShrunkHeap) {
             Serial.printf("[SpriteMgr] ERROR: Cannot allocate even shrunk sprite %dx%d. Heap: %u PSRAM: %u\n",
                    w, h, (unsigned)freeHeap, (unsigned)freePsram);
             outHandle.id = 0;
             return SpriteAllocResult::OutOfMemory; // Failed even after shrinking
          }
      } else { // Prefer Heap
          usePsram = !canShrunkHeap; // Use Heap if available (preferred)
          if (usePsram && !canShrunkPsram) { // If Heap failed, try PSRAM
               Serial.println("[SpriteMgr] INFO: Using PSRAM for shrunk sprite as Heap still insufficient.");
          } else if (!usePsram && !canShrunkHeap) { // If Heap preferred, try PSRAM as last resort for shrunk
             Serial.printf("[SpriteMgr] ERROR: Cannot allocate even shrunk sprite %dx%d. Heap: %u PSRAM: %u\n",
                    w, h, (unsigned)freeHeap, (unsigned)freePsram);
             outHandle.id = 0;
             return SpriteAllocResult::OutOfMemory; // Failed even after shrinking
          }
      }
    } else {
      // Cannot fit even the shrunk size
      Serial.printf("[SpriteMgr] ERROR: Cannot allocate %dx%d sprite (%u bytes), even after shrink attempt. Heap: %u PSRAM: %u\n",
                    reqW, reqH, (unsigned)need, (unsigned)freeHeap, (unsigned)freePsram);
      outHandle.id = 0;
      return SpriteAllocResult::OutOfMemory;
    }
  }
  
  // --- Attempt Allocation --- 
  Entry* entryToUse = nullptr;
  Serial.println("[SpriteMgr] Searching for reusable entry or creating new..."); // Log entry point
  dumpReport(); // Log state *before* potentially modifying pool

  // Try to reuse a non-alive entry with a valid ID
  for (auto& existingEntry : _pool) {
    if (!existingEntry.alive && existingEntry.id != 0) { // Only reuse if id != 0
      uint16_t newId = _generateNextId();
      Serial.printf("[SpriteMgr] Reusing pool entry (old ID: %u) for new sprite ID: %u\n", existingEntry.id, newId);
      // Free any existing buffer
      existingEntry.sprite.deleteSprite();
      existingEntry.id = newId;
      existingEntry.w = 0;
      existingEntry.h = 0;
      existingEntry.inPsram = false;
      existingEntry.alive = false;
      entryToUse = &existingEntry;
      break;
    }
  }

  // If no reusable entry found, create a new pool entry
  if (!entryToUse) {
    uint16_t newId = _generateNextId();
    Serial.printf("[SpriteMgr] Creating new pool entry for sprite ID: %u\n", newId);
    _pool.emplace_back(newId);
    entryToUse = &_pool.back();
  }

  // Check if entryToUse is valid before proceeding
  if (!entryToUse) {
      Serial.println("[SpriteMgr] CRITICAL ERROR: Failed to get a valid entry to use!");
      outHandle.id = 0;
      return SpriteAllocResult::OutOfMemory; // Or a new error code
  }
  
  // Now try the actual allocation using the determined parameters
  Serial.printf("[SpriteMgr] Attempting _tryAllocate for ID %u with %dx%d in %s\n", 
                entryToUse->id, w, h, usePsram ? "PSRAM" : "HEAP");
  bool ok = _tryAllocate(*entryToUse, w, h, usePsram);
  if (!ok) {
    Serial.printf("[SpriteMgr] ERROR: Final allocation attempt failed for %dx%d in %s\n",
                  w,h, usePsram?"PSRAM":"HEAP");
    // Reset entry to safe state
    entryToUse->alive = false;
    entryToUse->w = 0;
    entryToUse->h = 0;
    entryToUse->id = 0;
    entryToUse->inPsram = false;
    outHandle.id = 0;
    return SpriteAllocResult::OutOfMemory;
  }

  // Success! Assign handle and return status.
  outHandle.id = entryToUse->id;
  
  // Adjust result code if fallback occurred AFTER shrinking decision
  if (finalResult == SpriteAllocResult::SuccessShrunk) {
      if ((preferPsram && !usePsram && canHeap >= (need + buffer)) || (!preferPsram && usePsram && canPsram >= (need + buffer))) {
          Serial.println("[SpriteMgr] Note: Fallback occurred for the shrunk sprite.");
          // You might want a specific enum like SuccessShrunkFellBack if needed
      }
  }
  
  // Report final decision
  Serial.printf("[SpriteMgr] Final decision: Alloc ID %u (%dx%d) in %s. Result: %d\n", 
                 outHandle.id, w, h, usePsram ? "PSRAM" : "HEAP", (int)finalResult);
                 
  return finalResult;
}

bool SpriteManager::draw(const SpriteHandle &h, int16_t x, int16_t y) {
  if (h.id == 0 || h.id == 65535) {
    Serial.printf("[SpriteMgr] WARN: Attempted to draw sprite with invalid handle ID %u\n", h.id);
    return false;
  }
  if (auto *e = _findEntry(h.id)) {
    // Check if sprite dimensions are valid before pushing
    if (e->w > 0 && e->h > 0) {
        e->sprite.pushSprite(x, y);
        return true;
    } else {
        Serial.printf("[SpriteMgr] WARN: Attempted to draw sprite ID %u with invalid dimensions (%dx%d)\n", e->id, e->w, e->h);
    }
  }
  return false;
}

bool SpriteManager::destroy(const SpriteHandle &h) {
  if (h.id == 0 || h.id == 65535) {
    Serial.printf("[SpriteMgr] WARN: Attempted to destroy invalid handle ID %u\n", h.id);
    return false;
  }
  if (auto *e = _findEntry(h.id)) {
    Serial.printf("[SpriteMgr] Destroying sprite ID %u (%dx%d) in %s\n",
                  e->id, e->w,e->h, e->inPsram?"PSRAM":"HEAP");
    if (e->sprite.width() <= 0 || e->sprite.height() <= 0) {
      Serial.printf("[SpriteMgr] WARNING: Attempted to deleteSprite on invalid sprite (ID: %u, w: %d, h: %d). Skipping deleteSprite.\n", e->id, e->sprite.width(), e->sprite.height());
    } else {
      Serial.printf("[SpriteMgr] About to call deleteSprite() for ID %u\n", e->id);
      e->sprite.deleteSprite();
      Serial.printf("[SpriteMgr] Finished deleteSprite() for ID %u\n", e->id);
    }
    // Reset all fields to safe values
    e->id = 0;
    e->alive = false;
    e->w = 0;
    e->h = 0;
    e->inPsram = false;
    return true;
  }
  Serial.printf("[SpriteMgr] WARN: Attempted to destroy invalid or already dead sprite handle ID %u\n", h.id);
  return false;
}

void SpriteManager::destroyAll() {
  Serial.printf("[SpriteMgr] Destroying all %d sprites in pool...\n", _pool.size());
  for (auto &e : _pool) {
    if (e.alive) {
      e.sprite.deleteSprite();
      e.alive = false;
      e.w = 0;
      e.h = 0;
    }
  }
  // Optionally clear the pool if you don't want to reuse entries
  // _pool.clear(); 
  // _nextId = 1;
  Serial.println("[SpriteMgr] All live sprites marked as destroyed.");
}

void SpriteManager::dumpReport() {
  size_t liveCount = 0;
  size_t totalMemHeap = 0;
  size_t totalMemPsram = 0;
  for (const auto &e : _pool) {
    if (e.id == 65535) {
      Serial.println("[SpriteMgr] CRITICAL: Pool contains entry with ID 65535!");
    }
    if (e.w <= 0 || e.h <= 0) {
      Serial.printf("[SpriteMgr] WARNING: Pool entry with ID %u has invalid size (%dx%d)\n", e.id, e.w, e.h);
    }
    if (e.alive) {
      liveCount++;
      size_t mem = size_t(e.w) * size_t(e.h);
      if (e.inPsram) totalMemPsram += mem;
      else totalMemHeap += mem;
    }
  }

  Serial.println("--- Sprite Manager Report ---");
  Serial.printf(" Pool Size: %d | Live Sprites: %d\n", _pool.size(), liveCount);
  Serial.printf(" Est. Heap Used: %u bytes | Est. PSRAM Used: %u bytes\n", (unsigned)totalMemHeap, (unsigned)totalMemPsram);
  Serial.printf(" ESP Free Heap: %u bytes | ESP Free PSRAM: %u bytes\n", ESP.getFreeHeap(), ESP.getFreePsram());
  
  if (liveCount > 0) {
      Serial.println(" Live Sprite Details:");
      for (const auto &e : _pool) {
        if (e.alive) {
          Serial.printf("  - ID: %u | Size: %dx%d | Location: %s\n",
                        e.id, e.w, e.h,
                        e.inPsram ? "PSRAM" : "HEAP");
        }
      }
  } else {
      Serial.println(" No live sprites.");
  }
  Serial.println("-----------------------------");
} 