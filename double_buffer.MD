# Double Buffering with SpriteManager: Implementation Takeaways

This document summarizes key takeaways from implementing complex animated objects (Space Station and Binary Star) using sprites for double buffering on ESP32 with TFT displays.

## Core Concepts

1. **Double Buffering Principle**
   - Draw complete frames to an off-screen buffer first
   - Push the entire buffer to screen at once for flicker-free animation
   - Prevents partially drawn frames from being visible to the user

2. **Memory Management**
   - ESP32 has limited memory - careful allocation is critical
   - Use PSRAM when available (set with `setAttribute(PSRAM_ENABLE, true)`)
   - Calculate minimum necessary sprite size to conserve memory
   - Release sprite resources when not actively needed

## Space Station Implementation Success Factors

1. **Precise Dimension Calculation**
   - Calculate sprite size based on expected object dimensions
   - Add reasonable margin (~20%) for rotation/glow effects
   - Example: `spriteSize = ceil(diagonal * 1.2)`

2. **Graceful Fallback**
   - Implement direct drawing as fallback when sprites fail
   - Check memory after operations to detect potential issues
   - Delete sprites if memory becomes critically low

3. **Error Handling**
   - Validate sprite creation with `if (sprite.width() > 0 && sprite.height() > 0)`
   - Clear screen area even if sprite creation fails

4. **Background Continuity**
   - Draw stars from main array to maintain visual continuity
   - Only render stars that would be visible within sprite bounds

## Binary Star Implementation Challenges & Solutions

1. **Precise Sprite Sizing**
   - Calculate exact dimensions based on orbit radii and star sizes
   - Formula: `maxDistance = orbitRadius1 + orbitRadius2 + star1Radius + star2Radius`
   - Add minimal margin (5-10%) for anti-aliasing and glow effects
   - Keep dimensions even for better memory alignment

2. **Memory-Adaptive Rendering**
   - Scale sprite dimensions based on available memory
   - Adjust visual complexity when memory is limited
   - Use memory ratio calculation: `memoryRatio = availableMemory / requiredMemory`
   - Reduce object scale (80-90%) when memory is tight

3. **Flickering Prevention**
   - Ensure sprite dimensions are even numbers
   - Pre-clear screen area before pushing sprite
   - Bound-check all pixel operations to stay within sprite
   - Use simplified rendering (fewer gradient steps, thicker lines) for stability
   - Implement multiple cleanup passes for thorough artifact removal

4. **Transition Management**
   - Explicit cleanup during state transitions (discovery → warp → discovery)
   - Double-verify sprite deletion and screen clearing
   - Add small delays after memory operations to let system stabilize

## SpriteManager Optimization Techniques

1. **Resource Management**
   - Delete existing sprites before creating new ones
   - Implement safe deletion with error handling
   - Track sprite dimensions for proper cleanup

2. **Memory Safety**
   - Check available memory before allocation
   - Use PSRAM for large sprites whenever possible
   - Fall back to direct drawing when memory is low
   - Cap maximum dimensions regardless of calculations

3. **Performance Optimizations**
   - Reduce visual complexity when memory constrained
   - Draw fewer background elements in sprites
   - Selectively render only brightest stars
   - Simplify gradient effects and animations

4. **Error Resilience**
   - Implement try/catch blocks for sprite operations
   - Validate sprite dimensions after creation
   - Add bounds checking for all rendering operations
   - Track object positions for reliable cleanup

## Key Differences Between Implementations

1. **Space Station**
   - More complex rotation/rendering but stable memory footprint
   - Fixed dimensions with one-time calculation
   - Efficient with multiple small components

2. **Binary Star**
   - Simpler rendering but more dynamic memory requirements
   - Highly variable dimensions based on scale and orbits
   - More sensitive to memory constraints
   - Required more aggressive memory management

## ESP32-Specific Considerations

1. **Memory Tracking**
   - Use `ESP.getFreeHeap()` and `ESP.getFreePsram()` for memory monitoring
   - Log memory status before/after major operations
   - Set thresholds for fallback to direct rendering

2. **Safe Deletion**
   - Always check sprite dimensions before deletion
   - Use try/catch blocks for error handling
   - Add small delays after memory operations

3. **Optimal Performance**
   - Prefer 8-bit color depth for memory efficiency
   - Keep sprite dimensions as small as possible
   - Pre-calculate values to reduce in-frame computation
   - Consider direct drawing for very small objects

## General Best Practices

1. **Sprite Sizing and Memory**
   - Calculate exact dimensions needed based on object scale
   - Keep sprite dimensions even for memory alignment
   - Implement dynamic scaling based on available memory
   - Consider object position and screen bounds when drawing

2. **Robust Error Handling**
   - Always validate sprite creation
   - Implement fallback direct drawing
   - Clear screen areas thoroughly during transitions
   - Track positions and sizes for reliable cleanup

3. **Performance Optimization**
   - Draw minimal background elements in sprites
   - Simplify effects when memory constrained
   - Prefer simple shapes over pixel-by-pixel operations
   - Use direct drawing for small objects

4. **Clean Transitions**
   - Explicitly cleanup sprites during state changes
   - Perform multiple clear operations for reliability
   - Add small delays for memory operations to complete
   - Track previous positions for thorough cleanup 

## Implementation Guide for New Celestial Objects

Follow these step-by-step instructions to implement double buffering for new celestial objects using SpriteManager:

### 1. Sprite Declaration and Setup

```cpp
// In your object's header file
extern TFT_eSprite myObjectSprite;
extern bool myObjectSpriteCreated;

// In the main sketch file
TFT_eSprite myObjectSprite = TFT_eSprite(&tft);
bool myObjectSpriteCreated = false;
```

### 2. Position and Dimension Tracking

```cpp
// Add tracking variables for proper cleanup
int prevObjectX = 0;
int prevObjectY = 0;
int prevObjectSize = 0;
```

### 3. Optimal Size Calculation Function

```cpp
int calculateOptimalSpriteSize(float scale) {
  // 1. Calculate base dimensions based on object features
  int baseSize = scale_i(30) * scale; // Adjust formula based on object
  
  // 2. Add margin for effects (5-20% depending on complexity)
  int calculatedSize = ceil(baseSize * 1.1f);
  
  // 3. Adjust for memory constraints
  #ifdef ESP32
  uint32_t freeHeap = ESP.getFreeHeap();
  uint32_t freePsram = ESP.getFreePsram();
  uint32_t requiredMem = calculatedSize * calculatedSize;
  
  float memoryRatio = (freePsram > 10000) ? 
                     std::min((float)freePsram / (float)requiredMem, 10.0f) :
                     std::min((float)freeHeap / (float)requiredMem, 5.0f);
                     
  if (memoryRatio < 3.0f) {
    // If tight on memory, reduce size proportionally
    float adjustFactor = std::max(0.6f, memoryRatio / 3.0f);
    calculatedSize = ceil(calculatedSize * adjustFactor);
  }
  #endif
  
  // 4. Ensure even dimensions for memory alignment
  if (calculatedSize % 2 != 0) {
    calculatedSize++;
  }
  
  // 5. Cap maximum size and ensure minimum size
  calculatedSize = std::min(calculatedSize, 160); // Set appropriate max
  calculatedSize = std::max(calculatedSize, 40);  // Set appropriate min
  
  return calculatedSize;
}
```

### 4. Main Drawing Function Structure

```cpp
void drawMyObject() {
  // 1. Get object parameters
  int centerX = objectX;
  int centerY = objectY;
  float scale = objectScale * scaleFactor;
  
  // 2. Store position for proper cleanup
  prevObjectX = centerX;
  prevObjectY = centerY;
  
  // 3. Calculate optimal sprite size
  int spriteSize = calculateOptimalSpriteSize(scale);
  prevObjectSize = spriteSize;
  
  // 4. Check if we need to create or recreate the sprite
  if (!myObjectSpriteCreated) {
    // Critical memory check
    #ifdef ESP32
    if (ESP.getFreeHeap() < 8000 || ESP.getFreePsram() < 8000) {
      // Fall back to direct drawing if memory is critically low
      drawMyObjectDirect();
      return;
    }
    #endif
    
    // Create the sprite using SpriteManager
    SpriteManager::createObjectSprite(myObjectSprite, spriteSize, "MyObject");
    
    // Validate creation success
    if (myObjectSprite.width() > 0 && myObjectSprite.height() > 0) {
      myObjectSpriteCreated = true;
    } else {
      myObjectSpriteCreated = false;
      drawMyObjectDirect();
      return;
    }
  }
  
  // 5. Calculate sprite positioning
  int spriteWidth = myObjectSprite.width();
  int spriteHeight = myObjectSprite.height();
  int spriteOffsetX = centerX - spriteWidth / 2;
  int spriteOffsetY = centerY - spriteHeight / 2;
  int spriteCenterX = spriteWidth / 2;
  int spriteCenterY = spriteHeight / 2;
  
  // 6. Clear sprite with background color
  myObjectSprite.fillSprite(BG_COLOR);
  
  // 7. Draw starfield background (minimal/selective)
  for (int i = 0; i < STAR_COUNT; i += 4) { // Draw only some stars
    if (stars[i].brightness > 200) { // Only brightest stars
      int spriteStarX = stars[i].x - spriteOffsetX;
      int spriteStarY = stars[i].y - spriteOffsetY;
      if (spriteStarX >= 0 && spriteStarX < spriteWidth && 
          spriteStarY >= 0 && spriteStarY < spriteHeight) {
        uint16_t color = myObjectSprite.color565(stars[i].brightness, 
                                               stars[i].brightness, 
                                               stars[i].brightness);
        myObjectSprite.drawPixel(spriteStarX, spriteStarY, color);
      }
    }
  }
  
  // 8. Draw your object elements here
  // (Add your specific drawing code)
  
  // 9. Push sprite to screen with bounds checking
  if (myObjectSpriteCreated && 
      spriteOffsetX >= -spriteWidth/2 && spriteOffsetX <= SCREEN_WIDTH - spriteWidth/2 &&
      spriteOffsetY >= -spriteHeight/2 && spriteOffsetY <= SCREEN_HEIGHT - spriteHeight/2) {
    
    // Optional: Pre-clear the area for cleaner rendering
    tft.fillRect(spriteOffsetX, spriteOffsetY, spriteWidth, spriteHeight, BG_COLOR);
    
    // Push sprite to screen
    myObjectSprite.pushSprite(spriteOffsetX, spriteOffsetY);
    
    // Post-operation memory check
    #ifdef ESP32
    if (ESP.getFreeHeap() < 4000) {
      myObjectSprite.deleteSprite();
      myObjectSpriteCreated = false;
    }
    #endif
  } else {
    // Fall back to direct drawing if sprite is offscreen
    drawMyObjectDirect();
  }
}
```

### 5. Direct Drawing Fallback

```cpp
void drawMyObjectDirect() {
  int centerX = objectX;
  int centerY = objectY;
  float scale = objectScale * scaleFactor;
  
  // Store position for proper cleanup
  prevObjectX = centerX;
  prevObjectY = centerY;
  
  // Calculate size for cleanup
  int objectSize = calculateOptimalSpriteSize(scale);
  prevObjectSize = objectSize;
  
  // Clear area first
  tft.fillRect(centerX - objectSize/2, centerY - objectSize/2, 
               objectSize, objectSize, BG_COLOR);
  
  // Direct drawing implementation
  // (Add your simplified direct drawing code here)
}
```

### 6. Cleanup Function

```cpp
void eraseMyObject() {
  // 1. Delete sprite if it exists
  if (myObjectSpriteCreated) {
    SpriteManager::safeDeleteSprite(myObjectSprite, "MyObject");
    myObjectSpriteCreated = false;
  }
  
  // 2. Clear screen area
  int eraseSize = prevObjectSize;
  if (eraseSize <= 0) {
    eraseSize = 100; // Fallback if no size was stored
  }
  
  // Add extra margin for thorough cleanup
  eraseSize = eraseSize * 1.2;
  
  // Bounds check to prevent drawing outside screen
  int left = prevObjectX - eraseSize/2;
  int top = prevObjectY - eraseSize/2;
  if (left < 0) left = 0;
  if (top < 0) top = 0;
  
  int right = left + eraseSize;
  int bottom = top + eraseSize;
  if (right > SCREEN_WIDTH) right = SCREEN_WIDTH;
  if (bottom > SCREEN_HEIGHT) bottom = SCREEN_HEIGHT;
  
  int width = right - left;
  int height = bottom - top;
  
  // Multiple clearing passes for reliability
  if (width > 0 && height > 0) {
    tft.fillRect(left, top, width, height, BG_COLOR);
    delay(5); // Small delay to allow display to update
    tft.fillRect(left, top, width, height, BG_COLOR);
  }
  
  // Reset tracking variables
  prevObjectSize = 0;
}
```

### 7. Integration with Main Program

Add state transition handling in the main sketch:

```cpp
// In state transitions (e.g., when entering warp mode)
if (currentObject == CelestialObject::MY_OBJECT && showingCelestialObject) {
  // Clean up object during transition
  eraseMyObject();
  showingCelestialObject = false;
  
  // Allow memory management to complete
  delay(10);
}

// In the drawCelestialObject() function
case CelestialObject::MY_OBJECT:
  drawMyObject();
  displayObjectName("MY OBJECT");
  break;

// In the eraseCelestialObject() function
case CelestialObject::MY_OBJECT:
  eraseMyObject();
  break;
```

### 8. Key Implementation Checklist

- [ ] Declare sprite and created flag in header and main file
- [ ] Implement position tracking variables
- [ ] Create optimal size calculation function
- [ ] Implement main sprite-based drawing function
- [ ] Create direct drawing fallback function 
- [ ] Add proper cleanup/erase function
- [ ] Integrate with main program state transitions
- [ ] Add memory checks and validation
- [ ] Test with various memory conditions and scales
- [ ] Fine-tune parameters for optimal performance 