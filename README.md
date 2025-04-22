
# Cosmic Knobulator - Universe Explorer Toy 🚀🪐

**Blast off into a hands-on, educational space adventure!**  
Turn an ESP32 microcontroller and a vibrant TFT display into your personal Star Trek-inspired Cosmic Knobulator and explore a dazzling universe—no screens required!

---

## See It in Action!

| ![Toy](images/product_main.jpg) <br> 🌟 The finished toy — ready for lift-off! | ![3D Printed Shell](images/3d_printed_module.jpg) <br> 🛠️ The sturdy, 3D-printed shell keeps everything kid-safe |
|:---:|:---:|
| ![Setup](images/setup_photo.jpg) <br> 🔌 Simple wiring: ESP8266 + TFT + potentiometer knob | [![Demo: Engage warp speed & discover a galaxy!](images/demo.gif)](images/demo.gif) <br> 🎬 Demo: Engage warp speed & discover a galaxy! (click for video) |


*All images are pre-cropped for best display. Click the demo image to watch the video!*

---

## What is Cosmic Knobulator?

**Cosmic Knobulator** is a playful, STEAM-powered gadget that lets kids (and adults!) travel the cosmos from their hands.  
Turn the sturdy knob to speed up—stars zip by, and every “warp jump” reveals a new cosmic discovery!  
Built to inspire curiosity and imagination, it’s a real electronics project that’s safe, robust, and magical.

---

## Features

- ✨ **Twinkling Starfield:** Watch a mesmerizing field of stars shimmer and shine.
- 🌠 **Shooting Stars:** Catch glimpses of shooting stars streaking across space.
- 🚀 **Warp Speed Effects:** Turn the knob to engage warp—stars stretch into light speed lines!
- 🪐 **Explore the Cosmos:** Drop out of warp to discover planets, nebulae, black holes, and more.
- 🧠 **Quiz Mode:** Test your cosmic knowledge with interactive, multiple-choice science questions (potentiometer to select, button to answer!).
- 📖 **Story Mode:** Experience an interactive, step-by-step cosmic adventure—advance the story with the knob and immerse yourself in the universe.
- 🗂️ **Easy Menu Navigation:** Pick your mode (Discovery, Quiz, Story) with a twist and a click.
- 🔋 **Power Management:** Long-press to enter deep sleep; wake up with a button—perfect for classrooms or battery projects.
- ⚡ **Optimized Performance:** Smooth animation, clever redraws, and efficient memory use for buttery visuals on microcontrollers.

---

## Modes

- **Discovery Mode:** Pilot your Cosmic Knobulator, discover new celestial objects, and watch unique animations for each one.
- **Quiz Mode:** Answer fun science questions and get instant feedback—learn as you play!
- **Story Mode:** Follow a cosmic narrative, step by step, with the stars twinkling in the background.

---

## Why This Toy is Special

- **STEAM Learning:** Makes coding, physics, and astronomy fun and approachable.
- **Engaged Senses:** Bright colors, smooth motion, and a tactile knob keep kids captivated.
- **Discovery Mode:** No two adventures are alike—each journey reveals different cosmic objects!
- **Screen-Free:** Real-world, hands-on fun—no phone or tablet required.
- **Kid-Proof:** Solid case and simple controls, perfect for little hands in classrooms, camps, or family rooms.

---

## Celestial Objects to Discover

Who knows what you’ll find each time you explore? Each object is lovingly animated with its own personality!

- 🌟 Stars (twinkling, flaring, bright and dim)
- 🪐 Planets (with rings, clouds, and vibrant colors)
- ☁️ Nebulae (swirling clouds of cosmic gas)
- 🌌 Galaxies (majestic spirals and ellipticals)
- ☀️ Solar Systems (stars with orbiting planets)
- ☄️ Asteroid Fields (dense rocks drifting in space)
- ⚫ Black Holes (gravity-bending wells, animated accretion disks)
- 💫 Pulsars (spinning neutron stars with sweeping beams)
- 💥 Supernovae (exploding with color and expanding shockwaves)
- 🌠 Comets (icy travelers with glowing, fading tails)
- ✨ Binary Stars, Space Stations, and more—plus easy expansion for new discoveries!

---

## How Does It Work?

- **ESP32 Microcontroller:** The brain of the operation—fast, affordable, and beginner-friendly.
- **ST7735 TFT Display:** 128x128 pixel color screen brings the cosmos to life.
- **Potentiometer Knob:** Twist to change your speed, select menu items, and answer quiz questions.
- **Button:** Confirm selections, return to menu, or power down with a long press.
- **Custom ESP32 Code:** Modular, well-commented, and easy to extend—now with state machine logic for modes!
- **TFT_eSPI Library:** For lightning-fast graphics on microcontrollers.

---

## Build Your Own

### Hardware Required

- ESP32 development board
- 128×128 TFT display (ST7735)
- 10k potentiometer (knob)
- Momentary pushbutton
- Breadboard, jumper wires
- (Optional) 3D-printed or DIY kid-safe enclosure

### Wiring

| TFT Pin | ESP32 Pin     | Function        |
|---------|---------------|-----------------|
| CS      | GPIO5 (D1)    | Chip Select     |
| RST     | GPIO4 (D2)    | Reset           |
| DC      | GPIO0 (D3)    | Data/Command    |
| MOSI    | GPIO13 (D7)   | Data Out        |
| SCLK    | GPIO14 (D5)   | Clock           |
| LED/BLK | GPIO16/19     | Backlight (optional) |
| VCC     | 3.3V          | Power           |
| GND     | GND           | Ground          |
| POT     | GPIO35 (A0)   | Speed/Menu/Quiz Control   |
| BUTTON  | GPIO15        | Menu/Power/Quiz Confirm   |

### Software Setup

1. **Install Arduino IDE** (or PlatformIO).
2. **Add ESP32 Board Support** ([Official instructions](https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html)).
3. **Install TFT_eSPI Library** from Arduino Library Manager.
4. **Configure TFT_eSPI:**
    - Copy this project’s `User_Setup.h` to the TFT_eSPI library folder,  
      *OR* edit your existing `User_Setup.h` to match the wiring above.
5. **Open and upload the code.**
6. **Power up and explore!**

---

## For Educators, Makers, and Families

- **Perfect for classrooms, STEM camps, or maker clubs.**
- **Starter project for learning coding, electronics, and astronomy.**
- **Encourage creativity—kids can invent their own cosmic stories and discoveries!**

---

## Dream Big: Future Ideas

- Even more celestial objects and wild cosmic effects!
- Sound and music support (maybe a warp whoosh?).
- Bigger, higher-res displays.
- Multiplayer or Wi-Fi-connected discoveries.
- “Build your own planet” modes.
- More quiz questions and interactive stories.

---

## License

MIT License — Free for classrooms, makerspaces, and curious explorers everywhere.

---

## Credits & Thanks

Inspired by classic science fiction, the open-source community, and every kid who’s ever looked up and wondered.  
Special thanks to ESP32, Arduino, and TFT_eSPI contributors.

---

**Ready to explore the universe? Power up, twist the knob, and go where no toy has gone before!**


# Cosmic Knobulator Toy 🚀🪐

**Blast off into a hands-on, educational space adventure!**  
Turn an ESP32 microcontroller and a vibrant TFT display into your personal Star Trek-inspired Cosmic Knobulator and explore a dazzling universe—no screens required!

---

## See It in Action!

| ![Toy](images/product_main.jpg) <br> 🌟 The finished toy — ready for lift-off! | ![3D Printed Shell](images/3d_printed_module.jpg) <br> 🛠️ The sturdy, 3D-printed shell keeps everything kid-safe |
|:---:|:---:|
| ![Setup](images/setup_photo.jpg) <br> 🔌 Simple wiring: ESP8266 + TFT + potentiometer knob | [![Demo: Engage warp speed & discover a galaxy!](images/demo_thumb.jpg)](images/demo.gif) <br> 🎬 Demo: Engage warp speed & discover a galaxy! (click for video) |


*All images are pre-cropped for best display. Click the demo image to watch the video!*

---

## What is Cosmic Knobulator?

**Cosmic Knobulator** is a playful, STEAM-powered gadget that lets kids (and adults!) travel the cosmos from their hands.  
Turn the sturdy knob to speed up—stars zip by, and every “warp jump” reveals a new cosmic discovery!  
Built to inspire curiosity and imagination, it’s a real electronics project that’s safe, robust, and magical.

---

## Features

- ✨ **Twinkling Starfield:** Watch a mesmerizing field of stars shimmer and shine.
- 🌠 **Shooting Stars:** Catch glimpses of shooting stars streaking across space.
- 🚀 **Warp Speed Effects:** Turn the knob to engage warp—stars stretch into light speed lines!
- 🪐 **Explore the Cosmos:** Drop out of warp to discover planets, nebulae, black holes, and more.
- 🧠 **Quiz Mode:** Test your cosmic knowledge with interactive, multiple-choice science questions (potentiometer to select, button to answer!).
- 📖 **Story Mode:** Experience an interactive, step-by-step cosmic adventure—advance the story with the knob and immerse yourself in the universe.
- 🗂️ **Easy Menu Navigation:** Pick your mode (Discovery, Quiz, Story) with a twist and a click.
- 🔋 **Power Management:** Long-press to enter deep sleep; wake up with a button—perfect for classrooms or battery projects.
- ⚡ **Optimized Performance:** Smooth animation, clever redraws, and efficient memory use for buttery visuals on microcontrollers.

---

## Modes

- **Discovery Mode:** Pilot your Cosmic Knobulator, discover new celestial objects, and watch unique animations for each one.
- **Quiz Mode:** Answer fun science questions and get instant feedback—learn as you play!
- **Story Mode:** Follow a cosmic narrative, step by step, with the stars twinkling in the background.

---

## Why This Toy is Special

- **STEAM Learning:** Makes coding, physics, and astronomy fun and approachable.
- **Engaged Senses:** Bright colors, smooth motion, and a tactile knob keep kids captivated.
- **Discovery Mode:** No two adventures are alike—each journey reveals different cosmic objects!
- **Screen-Free:** Real-world, hands-on fun—no phone or tablet required.
- **Kid-Proof:** Solid case and simple controls, perfect for little hands in classrooms, camps, or family rooms.

---

## Celestial Objects to Discover

Who knows what you’ll find each time you explore? Each object is lovingly animated with its own personality!

- 🌟 Stars (twinkling, flaring, bright and dim)
- 🪐 Planets (with rings, clouds, and vibrant colors)
- ☁️ Nebulae (swirling clouds of cosmic gas)
- 🌌 Galaxies (majestic spirals and ellipticals)
- ☀️ Solar Systems (stars with orbiting planets)
- ☄️ Asteroid Fields (dense rocks drifting in space)
- ⚫ Black Holes (gravity-bending wells, animated accretion disks)
- 💫 Pulsars (spinning neutron stars with sweeping beams)
- 💥 Supernovae (exploding with color and expanding shockwaves)
- 🌠 Comets (icy travelers with glowing, fading tails)
- ✨ Binary Stars, Space Stations, and more—plus easy expansion for new discoveries!

---

## How Does It Work?

- **ESP32 Microcontroller:** The brain of the operation—fast, affordable, and beginner-friendly.
- **ST7735 TFT Display:** 128x128 pixel color screen brings the cosmos to life.
- **Potentiometer Knob:** Twist to change your speed, select menu items, and answer quiz questions.
- **Button:** Confirm selections, return to menu, or power down with a long press.
- **Custom ESP32 Code:** Modular, well-commented, and easy to extend—now with state machine logic for modes!
- **TFT_eSPI Library:** For lightning-fast graphics on microcontrollers.

---

## Build Your Own

### Hardware Required

- ESP32 development board
- 128×128 TFT display (ST7735)
- 10k potentiometer (knob)
- Momentary pushbutton
- Breadboard, jumper wires
- (Optional) 3D-printed or DIY kid-safe enclosure

### Wiring

| TFT Pin | ESP32 Pin     | Function        |
|---------|---------------|-----------------|
| CS      | GPIO5 (D1)    | Chip Select     |
| RST     | GPIO4 (D2)    | Reset           |
| DC      | GPIO0 (D3)    | Data/Command    |
| MOSI    | GPIO13 (D7)   | Data Out        |
| SCLK    | GPIO14 (D5)   | Clock           |
| LED/BLK | GPIO16/19     | Backlight (optional) |
| VCC     | 3.3V          | Power           |
| GND     | GND           | Ground          |
| POT     | GPIO35 (A0)   | Speed/Menu/Quiz Control   |
| BUTTON  | GPIO15        | Menu/Power/Quiz Confirm   |

### Software Setup

1. **Install Arduino IDE** (or PlatformIO).
2. **Add ESP32 Board Support** ([Official instructions](https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html)).
3. **Install TFT_eSPI Library** from Arduino Library Manager.
4. **Configure TFT_eSPI:**
    - Copy this project’s `User_Setup.h` to the TFT_eSPI library folder,  
      *OR* edit your existing `User_Setup.h` to match the wiring above.
5. **Open and upload the code.**
6. **Power up and explore!**

---

## For Educators, Makers, and Families

- **Perfect for classrooms, STEM camps, or maker clubs.**
- **Starter project for learning coding, electronics, and astronomy.**
- **Encourage creativity—kids can invent their own cosmic stories and discoveries!**

---

## Dream Big: Future Ideas

- Even more celestial objects and wild cosmic effects!
- Sound and music support (maybe a warp whoosh?).
- Bigger, higher-res displays.
- Multiplayer or Wi-Fi-connected discoveries.
- “Build your own planet” modes.
- More quiz questions and interactive stories.

---

## License

MIT License — Free for classrooms, makerspaces, and curious explorers everywhere.

---

## Credits & Thanks

Inspired by classic science fiction, the open-source community, and every kid who’s ever looked up and wondered.  
Special thanks to ESP32, Arduino, and TFT_eSPI contributors.

---

**Ready to explore the universe? Power up, twist the knob, and go where no toy has gone before!**

