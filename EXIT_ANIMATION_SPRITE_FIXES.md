# Exit Animation Sprite Fixes

## Problem Summary

Two issues were identified with exit animations:

1. **SpriteManager Warnings**: Multiple "WARNING: Pool entry with ID 0 has invalid size (0x0)" messages
2. **Sprite Cropping**: Space station (and other objects) sprites were cropped during exit animation because they weren't sized for the maximum scale

## Root Causes

### 1. SpriteManager Warnings

The `dumpReport()` function was checking all pool entries, including dead/invalid ones (ID 0). These entries are created when sprites are destroyed and marked as dead, but the report function was still warning about them.

### 2. Sprite Cropping During Exit

Celestial objects create sprites based on their current scale (usually 1.0), but exit animations can scale objects up to 3.0x. When the object scales beyond the sprite size, it gets cropped.

## Fixes Applied

### 1. SpriteManager Warning Fix

**File**: `sprite_manager.cpp`
**Change**: Modified `dumpReport()` to skip dead/invalid entries

```cpp
void SpriteManager::dumpReport()
{
  // ... existing code ...
  for (const auto &e : _pool)
  {
    // Skip dead/invalid entries to avoid spam warnings
    if (!e.alive || e.id == 0)
      continue;
    // ... rest of function ...
  }
}
```

**Result**: Eliminates spam warnings about invalid sprite entries

### 2. Space Station Sprite Sizing Fix

**File**: `spacestation.h`
**Changes**:

#### Sprite Creation

```cpp
// EXIT ANIMATION FIX: Account for maximum possible scale during exit animation
// Exit animation can scale up to 3.0x, so we need a sprite large enough for that
float maxPossibleScale = 3.0f; // Maximum scale during exit animation
int spriteSize = ceil(diagonal * maxPossibleScale * 1.2); // Add safety margin for max scale
```

#### Direct Draw Cleanup

```cpp
// EXIT ANIMATION FIX: Account for maximum possible scale during exit animation
float maxPossibleScale = 3.0f; // Maximum scale during exit animation
int clearSize = ceil(diagonal * maxPossibleScale * 1.2); // Match sprite size calculation
```

**Result**: Space station sprite is now large enough for 3x scaling during exit animation

### 3. Binary Star Sprite Sizing Fix

**File**: `binarystar.h`
**Changes**:

#### Sprite Size Calculation

```cpp
// EXIT ANIMATION FIX: Use maximum possible scale for sprite sizing
// Exit animation can scale up to 3.0x, so we need a sprite large enough for that
float maxPossibleScale = 3.0f; // Maximum scale during exit animation
float spriteScale = std::max(scale, maxPossibleScale); // Use larger of current or max scale
```

#### Direct Draw Cleanup

```cpp
// EXIT ANIMATION FIX: Account for maximum possible scale during exit animation
float maxPossibleScale = 3.0f; // Maximum scale during exit animation
int maxSize = calculateOptimalSpriteSize(maxPossibleScale * scaleFactor); // Calculate size for max scale
```

**Result**: Binary star sprite is now large enough for 3x scaling during exit animation

## Technical Details

### Maximum Scale Factor

Exit animations use `easeInCubic` scaling from 1.0 to 3.0:

```cpp
exitAnimation.startScale = 1.0f; // Start at full size
exitAnimation.endScale = 3.0f;   // Scale up and disappear (like moving away)
```

### Sprite Size Calculation

Objects now calculate sprite size using the maximum possible scale:

- **Space Station**: `diagonal * 3.0 * 1.2` (diagonal extent × max scale × safety margin)
- **Binary Star**: Uses `calculateOptimalSpriteSize()` with max scale factor

### Memory Impact

- **Space Station**: Sprite size increased from ~60x60 to ~180x180 pixels
- **Binary Star**: Sprite size increased proportionally to max scale
- **Memory Usage**: ~3x larger sprites, but only during object display
- **PSRAM**: Sprites prefer PSRAM allocation, so impact on heap is minimal

## Testing Recommendations

### Visual Tests

1. **Space Station Exit**: View space station, trigger exit animation

   - **Expected**: Station scales out smoothly without cropping
   - **Verify**: No parts of station disappear during scaling

2. **Binary Star Exit**: View binary star, trigger exit animation

   - **Expected**: Both stars scale out smoothly without cropping
   - **Verify**: Orbit animation continues smoothly during scaling

3. **SpriteManager Logs**: Check Serial output
   - **Expected**: No "WARNING: Pool entry with ID 0" messages
   - **Verify**: Clean sprite manager reports

### Performance Tests

1. **Memory Usage**: Monitor heap/PSRAM during exit animations
2. **Frame Rate**: Verify smooth 60 FPS during exit animations
3. **Sprite Creation**: Check sprite creation success messages

## Related Files

- `sprite_manager.cpp` - SpriteManager warning fix
- `spacestation.h` - Space station sprite sizing fix
- `binarystar.h` - Binary star sprite sizing fix
- `celestial_animations.cpp` - Exit animation scale definitions

## Future Considerations

- Apply same fix to other sprite-based objects (nebula, galaxy, etc.)
- Consider dynamic sprite resizing during exit animation
- Monitor memory usage with larger sprites
- Add sprite size validation to prevent future cropping issues







