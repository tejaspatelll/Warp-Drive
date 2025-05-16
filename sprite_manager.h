#ifndef SPRITE_MANAGER_H
#define SPRITE_MANAGER_H

#include <TFT_eSPI.h>
#include <Arduino.h>
#include <vector>
#include <cstdint>
#include <optional>
#include <algorithm>

// extern declarations from your main sketch
extern TFT_eSPI    tft;
extern const int   SCREEN_WIDTH;
extern const int   SCREEN_HEIGHT;
extern uint16_t    BG_COLOR;

enum class SpriteAllocResult {
  Success,            // created exactly as requested
  SuccessShrunk,      // created, but dimensions were reduced
  FellBackToHeap,     // preferred PSRAM but used heap
  OutOfMemory,        // couldn't allocate in either pool
  InvalidParameters   // e.g. zero or negative size
};

struct SpriteHandle {
  uint16_t  id = 0; // Default initialize to 0 (invalid)
  // you can add more metadata here if you like
};

class SpriteManager {
public:
  // initialize manager (call in setup)
  static void begin();

  // allocate a new sprite; caller must check result before using handle
  static SpriteAllocResult create(int16_t w,
                                  int16_t h,
                                  bool     preferPsram,
                                  SpriteHandle & outHandle);

  // Get a reference to the underlying sprite for drawing into it
  // Returns nullptr if handle is invalid or sprite is not alive
  static TFT_eSprite* getSpriteRef(const SpriteHandle& h);

  // draw your sprite at (x,y)
  static bool      draw(const SpriteHandle& h, int16_t x, int16_t y);

  // free one sprite
  static bool      destroy(const SpriteHandle& h);

  // destroy all sprites at once
  static void      destroyAll();

  // diagnostics
  static void      dumpReport();

  // Public methods to get dimensions without exposing Entry
  static int16_t   getWidth(const SpriteHandle& h);
  static int16_t   getHeight(const SpriteHandle& h);

private:
  struct Entry {
    uint16_t      id;
    TFT_eSprite   sprite;
    int16_t       w, h;
    bool          inPsram;
    bool          alive;
    // Constructor initializes the sprite with the global tft object
    Entry(uint16_t _id) : id(_id), sprite(&tft), w(0), h(0), inPsram(false), alive(false) {}
  };

  static std::vector<Entry>  _pool;
  static uint16_t            _nextId;

  // Helpers
  static Entry *     _findEntry(uint16_t id);
  static int16_t     _alignSize(int16_t v);
  static bool        _tryAllocate(Entry &e, int16_t w, int16_t h, bool usePsram);
  // Generate next unique sprite ID, skipping reserved values
  static uint16_t    _generateNextId();
};

#endif // SPRITE_MANAGER_H 