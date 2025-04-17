# ESP32 Warp Drive: A Space Visualization Toy!



Blast off into a fun, interactive space adventure! This project turns an ESP32 microcontroller and a small TFT display into a captivating *Star Trek*-inspired warp drive visualization.

It's a fantastic educational tool to explore coding, electronics, and the wonders of space!

## Features

✨ **Twinkling Starfield:** Watch a mesmerizing field of stars shimmer and shine.
🌠 **Shooting Stars:** Catch glimpses of shooting stars streaking across the void.
🚀 **Warp Speed!** Turn the control knob (potentiometer) to engage the warp drive and see the stars stretch into lines as you accelerate through space.
🪐 **Explore the Cosmos:** When you drop out of warp, you might discover exciting celestial objects!
🔋 **Power Control:** Simple power management features (check code for details, potentially long-press).

### Discovered Celestial Objects

Who knows what you'll find? Possibilities include:

1.  🌟 **Stars:** Bright points varying in light and flare.
2.  🪐 **Planets:** Colorful worlds, some with atmospheres.
3.  ☁️ **Nebulae:** Swirling clouds of cosmic gas and dust.
4.  🌌 **Galaxies:** Majestic spiral structures.
5.  ☀️ **Solar Systems:** Stars with orbiting planets.
6.  ☄️ **Asteroid Fields:** Navigate through space rocks.
7.  ⚫ **Black Holes:** Mysterious gravitational wells.
8.  💫 **Pulsars:** Rapidly spinning neutron stars.
9.  💥 **Supernovae:** Exploding stars!
10. 🌠 **Comets:** Icy bodies with bright tails.
11. ✨ **Binary Star Systems:** Two stars orbiting each other.
12. 🛰️ **Space Stations:** Artificial structures in orbit.

## Improvements
*   **More Celestial Objects:** Expand the list of discovered objects.



## How it Works (The Fun Tech Stuff!)

This project uses:

*   **An ESP32 microcontroller:** The 'brain' of the operation.
*   **A ST7735 TFT Display (128x128 pixels):** A small color screen to show the visuals.
*   **A Potentiometer:** A knob to control the warp speed effect.
*   **The TFT_eSPI Library:** An optimized library for fast graphics on the ESP32.

## Getting Started: Build Your Own!

### Hardware Needed

*   ESP32 development board
*   ST7735 TFT Display (128x128 pixels)
*   10k Potentiometer
*   Breadboard and jumper wires

### Wiring

Connect the components as follows (refer to `User_Setup.h` for precise pins used in *this* project's configuration):

*   **TFT Display** -> **ESP32**
    *   CS (Chip Select) -> GPIO5 (D1)
    *   RST (Reset) -> GPIO4 (D2)
    *   DC (Data/Command) -> GPIO0 (D3)
    *   MOSI (Data In) -> GPIO13 (D7)
    *   SCLK (Clock) -> GPIO14 (D5)
    *   LED/BLK (Backlight) -> GPIO16 (Optional, for brightness control)
    *   VCC -> 3.3V
    *   GND -> GND
*   **Potentiometer** -> **ESP32**
    *   Signal Pin (Middle) -> A0 (Analog Pin)
    *   One Outer Pin -> 3.3V
    *   Other Outer Pin -> GND

### Software Setup

1.  **Install Arduino IDE:** Download and install from the [Arduino website](https://www.arduino.cc/en/software).
2.  **Add ESP32 Board Support:** Follow [these instructions](https://randomnerdtutorials.com/installing-the-esp32-board-in-arduino-ide-v2/) to add ESP32 boards to your Arduino IDE.
3.  **Install TFT_eSPI Library:** Open the Arduino IDE, go to `Sketch > Include Library > Manage Libraries...`. Search for `TFT_eSPI` and install it.
4.  **Configure TFT_eSPI:**
    *   **IMPORTANT:** This project uses specific pin settings. You need to tell the TFT_eSPI library about them.
    *   Locate the TFT_eSPI library folder in your Arduino libraries directory (e.g., `Documents/Arduino/libraries/TFT_eSPI`).
    *   **Either:** Replace the `User_Setup.h` file inside the library folder with the `User_Setup.h` file from this project.
    *   **Or:** Edit the library's `User_Setup.h` (or `User_Setup_Select.h` to point to a custom setup) to match the pin definitions (`TFT_CS`, `TFT_RST`, `TFT_DC`, etc.) and the driver (`ST7735_DRIVER`) specified in this project's `User_Setup.h`.
5.  **Open Project:** Open the `.ino` file (`warpdrive_esp8266_tft.ino`) in the Arduino IDE.
6.  **Select Board & Port:** Choose your ESP32 board model and the correct COM port from the `Tools` menu.
7.  **Upload!** Click the Upload button.

## Usage

*   **Power On:** Connect the ESP32 to power.
*   **Control Warp:** Turn the potentiometer knob to increase or decrease warp speed.
*   **Enjoy the view!** See what celestial objects you discover when exiting warp.

## Contributing

Contributions, issues, and feature requests are welcome! Feel free to check the [issues page](https://github.com/tejaspatell/warpdrive_esp8266_tft/issues) (replace with your actual GitHub repo link if applicable).

## License

This project is available under the [![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)