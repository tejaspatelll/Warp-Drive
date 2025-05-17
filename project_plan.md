# ESP32-S3 Warp Drive Visualization Project Plan

## Project Overview

The Warp Drive Visualization is an Arduino-based project that creates a Star Trek-inspired space exploration simulator on a small TFT display driven by an ESP32 microcontroller. The project uses the limited hardware resources of the ESP32 to create a rich, interactive universe exploration experience with realistic celestial objects and engaging visual effects.

## Hardware Components

- **ESP32-S3 development board** with PSRAM
- **2" 240×320 TFT SPI display** (240x320 pixels)
- **WS2812B LED strip** (2 LEDs) on `LED_PIN` (GPIO 2)
- **10kΩ potentiometer** on `POT_PIN` (GPIO 7)
- **Momentary pushbutton** on `BUTTON_PIN` (GPIO 3)
- **Vibration motor** on `VIBRATION_PIN` (GPIO 26)
- **Backlight control** on `TFT_LED` (GPIO 19)

### Pin Connections
| Component     | Pin             | Function                             |
|---------------|-----------------|--------------------------------------|
| BUTTON_PIN    | GPIO 3          | Pushbutton (INPUT_PULLUP)            |
| POT_PIN       | GPIO 7 (ADC1)   | Potentiometer control and menu knob  |
| LED_PIN       | GPIO 2          | WS2812B LED strip data               |
| VIBRATION_PIN | GPIO 26         | Vibration motor PWM                  |
| TFT_LED       | GPIO 19         | TFT backlight control                |

## Code Architecture

### State Management

The application utilizes a state machine with the following primary states managed in `warpdrive_esp32_tft.ino`:

1.  **`State::MENU`**: Initial state. Displays a menu to select Discovery, Quiz, or Story mode. Uses `drawMenu()` and `processMenuInput()`.
2.  **`State::DISCOVERY`**: Entered from Menu (indirectly via Warp) or after exiting Warp. Shows celestial objects. Transitions back to Menu on button press or to Warp if potentiometer is turned up.
3.  **`State::WARP`**: Entered from Menu (for Discovery), Discovery, or Story mode. Shows warp star effect. Transitions to Discovery when warp speed drops.
4.  **`State::QUIZ`**: Entered from Menu. Manages quiz logic via `updateQuizMode()`, `startQuiz()`, `processQuizInput()` (from `quiz_mode.h`). Uses `quiz_popup.h` for results.
5.  **`State::STORY`**: Entered from Menu. Manages narrative via `storyModeManager` (`StoryMode` class from `story_mode.h`).
6.  **`State::NORMAL`**: (Less used directly now) Base state for starfield, sometimes a transitional state.

### Display Update Architecture

Performance optimization is critical on the resource-constrained ESP32. The code implements several techniques to maintain smooth animation:

1. **Selective Updates**: Only the parts of the screen that change are redrawn, rather than refreshing the entire display
2. **Previous Position Tracking**: Previous coordinates are stored to allow targeted erasing of objects
3. **Batch Processing**: Updates are staggered across multiple frames to distribute processing load
4. **Dynamic Frame Timing**: Frame rates are adjusted based on the current state and complexity of the scene
5. **SPI Optimization**: SPI communication is maximized (40MHz) for faster drawing operations
6. **Task Distribution**: Different animations are updated on different frames using counters

### Memory Management

- **PSRAM Utilization**: The ESP32-S3's PSRAM is crucial. `SpriteManager` (`sprite_manager.cpp`) attempts to allocate sprite buffers in PSRAM first (`e.sprite.setAttribute(PSRAM_ENABLE, true);` if `usePsram` is true during `_tryAllocate`). If PSRAM allocation fails or is not preferred, it falls back to heap. PSRAM initialization (`psramInit()`) is checked in `setup()` of the main `.ino` file.
- **SpriteManager**: A static class (`SpriteManager`) manages a pool of `TFT_eSprite` objects for off-screen rendering, primarily for complex animations like celestial objects in Discovery mode and UI elements in Story mode. It handles creation, destruction, and reuse of sprites.
- **Static Memory Allocation**: For core data like star arrays and some UI elements, static allocation is used to avoid heap fragmentation.
- **Struct-based Data**: Organizes data for celestial objects, menu items, etc.
- **Constexpr Constants**: Reduces runtime overhead.

## Current Features

### Core Mechanics
- ✨ **Twinkling Starfield:** In `State::NORMAL`, `State::MENU`, and `State::DISCOVERY` (when no object is shown), `updateStars()` creates a gentle twinkle.
- 🌠 **Shooting Stars:** `updateShootingStars()` randomly generates shooting stars, visible in `State::DISCOVERY`.
- 🚀 **Warp Speed Effects:** In `State::WARP`, `updateWarpStars()` creates stretching star streaks based on `warpFactor` from potentiometer. Haptic feedback intensity also scales with `warpFactor`.
- 🪐 **Discovery Mode (`State::DISCOVERY`):** After exiting `State::WARP`, a celestial object is chosen randomly from `CelestialObject` enum. Each object has a dedicated draw (`drawCelestialObject() -> draw[Object]()`) and erase function (`eraseCelestialObject() -> erase[Object]()`). Many complex objects use `SpriteManager` for rendering.
- 🧠 **Quiz Mode (`State::QUIZ`):** Managed by functions in `quiz_mode.h`. Uses `STORY_STOPS_DATA` from `story_mode.h` for questions. `quiz_popup.h` handles displaying results. Potentiometer selects answers, button confirms.
- 📖 **Story Mode (`State::STORY`):** The `StoryMode` class (`story_mode.h`) manages a narrative sequence with scrolling text and associated visuals. It uses its own sprites for UI elements.
- 💡 **LED Animations:** `led_animations.cpp` defines effects for each state (e.g., `setLedModeMenu`, `setLedModeQuiz`, `setLedModeDiscovery`). `updateLedEffects()` is called in the main loop.
- 🔔 **Haptic Feedback:** `hapticFeedback()` function provides vibration patterns based on `warpFactor` or specific events (haptic overrides).
- ⚙️ **Menu System (`State::MENU`):** `drawMenu()` and `processMenuInput()` handle navigation. Potentiometer selects, button confirms.
- 🔋 **Power Management:** Long press on `BUTTON_PIN` triggers deep sleep (`esp_deep_sleep_start()`). Wake-up via `ESP_SLEEP_WAKEUP_EXT0` on the same button.

### Celestial Objects
Each object type is defined in its own header file (e.g., `planet.h`, `nebula.h`, `blackhole.h`) and typically includes `draw...()` and `erase...()` functions. Complex objects like `SpaceStation` and `BinaryStar` use `SpriteManager` for their animations.

1. **Stars** - Simple bright points
2. **Planets** - Colorful spheres with atmosphere effects
3. **Nebulae** - Dynamic gas clouds with particle systems
4. **Galaxies** - Spiral arms with core and rotation
5. **Solar Systems** - Star with orbiting planets
6. **Asteroid Fields** - Multiple moving space rocks
7. **Black Holes** - Gravitational wells with accretion disks and relativistic effects
8. **Pulsars** - Rapidly rotating neutron stars emitting beams of radiation
9. **Supernovae** - Explosive stellar events with shockwaves and particle effects
10. **Comets** - Fast-moving objects with particle tails
11. **Binary Star Systems** - Paired stars orbiting a shared center of mass
12. **Space Stations** - Artificial outposts with blinking lights

### Visual Effects
- Particle systems for nebulae, asteroid fields, and supernovae
- Smooth animations for rotation and movement
- Light bloom and glow effects around bright objects
- Adaptive brightness and color transitions
- Easing functions for smooth state transitions

### Performance Optimizations
- Frame-based task distribution
- Dynamic frame rate adjustment
- Partial screen updates
- Batch processing of particles
- Optimized clearing/redrawing techniques

## Future Improvement Plans

### New Celestial Objects
- **Neutron Star** - Ultra-dense stellar remnant with unique visual effects
- **Quasars** - Extremely bright active galactic nuclei
- **Planetary Rings** - Saturn-like ring systems around planets
- **Asteroid Fields** - Multiple moving space rocks
- **Black Holes** - Gravitational wells with accretion disks and relativistic effects
- **Pulsars** - Rapidly rotating neutron stars emitting beams of radiation
- **Supernovae** - Explosive stellar events with shockwaves and particle effects
- **Comets** - Fast-moving objects with particle tails

### Enhanced Visual Effects
- **Parallax Background** - Multiple depth layers for more immersive star movement
- **Color Filters** - Special visual modes like infrared or radio telescope views
- **Improved Light Physics** - More realistic lighting interactions, shadows and occlusions
- **Higher-Detail Particle Systems** - More particles and complex behaviors

### Interaction Improvements
- **Multiple Input Controls** - Additional buttons for camera control or object selection
- **Menu System** - Allow selection of specific celestial objects for viewing (currently menu is for modes only)
- **Scanning Mode** - Detailed information popup about discovered objects (currently facts are in `STORY_STOPS_DATA` but not fully used in Discovery)
- **Position Memory** - Ability to bookmark and return to interesting discoveries
- **Tutorial Mode** - Guided tour of celestial phenomena 

### Performance Enhancements
- **DMA Transfers** - Direct Memory Access for faster screen updates
- **Custom Bitmap Storage** - Pre-rendered elements to reduce computation time
- **Frame Buffer Management** - Double buffering to reduce visual artifacts
- **Adaptive Detail Levels** - Dynamic adjustment of visual complexity based on performance metrics
- **Assembly Optimizations** - Critical functions rewritten in assembly for speed

### Hardware Expansions
- **Sound Effects** - Adding a speaker for ambient space sounds and alerts
- **Larger Display Option** - Support for higher resolution displays
- **Wi-Fi Connectivity** - Share discoveries or download new objects
- **Battery Management** - Low power modes for portable operation

## Conclusion

The ESP32 Warp Drive Visualization project demonstrates how even resource-constrained microcontrollers can deliver engaging interactive experiences through clever optimization and thoughtful design. The modular architecture allows for ongoing expansion while maintaining backward compatibility with existing features.

The project balances visual fidelity with performance demands, creating a unique space exploration experience that responds dynamically to user input. Future enhancements will focus on adding more diversity to discoveries, improving visual quality, and expanding interaction options while maintaining the core experience of exploration and discovery. 