# Deep Sleep Power Optimization

## Overview
This document describes the comprehensive power optimization implemented for deep sleep mode to minimize battery drain when the device is powered off.

## Problem Statement
The original implementation had high power consumption during deep sleep (estimated 50-500µA), draining the battery quickly when powered off. Analysis revealed multiple issues preventing proper deep sleep current reduction.

## Root Causes Identified

### Critical Issues (5-25mA impact)
1. **TFT Display Not in Sleep Mode** - Display IC remained active, drawing 5-20mA
2. **SPI Pins Not Isolated** - Floating SPI pins caused leakage current (1-5mA)

### High Priority Issues (100-500µA impact)
3. **Potentiometer Voltage Divider** - Hardware draws ~330µA continuously
4. **WS2812B Residual Current** - LEDs not fully powered down (1-5mA potential)
5. **PSRAM Not Released** - Standby current ~100µA

### Medium/Low Priority Issues (10-50µA impact)
6. **Unused GPIO Not Isolated** - Multiple pins leaking 1-10µA each
7. **Secondary Button Pull-up** - Internal pull-up drawing ~33µA
8. **GPIO Hold Timing** - Current spikes during state transitions

## Implemented Solutions

### 1. TFT Display Sleep Mode ⚡ **CRITICAL**
**Power Savings: 5-20mA → <10µA**

```cpp
// Enter display sleep mode before deep sleep
tft.writecommand(0x10); // SLPIN command (ST7789/ST7735)
delay(120);             // Wait for sleep mode entry

// On wake-up:
tft.writecommand(0x11); // SLPOUT command
delay(120);             // Wait for display to wake
```

**Impact**: This is the single most important fix, reducing power consumption by 5-20mA.

### 2. SPI Pin Isolation ⚡ **HIGH PRIORITY**
**Power Savings: 1-5mA → <1µA**

```cpp
// CS pin - de-select and hold
pinMode(TFT_CS, OUTPUT);
digitalWrite(TFT_CS, HIGH);
gpio_hold_en((gpio_num_t)TFT_CS);

// Other SPI pins - set to INPUT to prevent leakage
pinMode(TFT_DC, INPUT);
pinMode(TFT_RST, INPUT);
pinMode(TFT_MOSI, INPUT);
pinMode(TFT_SCLK, INPUT);
```

**Impact**: Prevents current leakage through floating SPI pins.

### 3. PSRAM Power Domain Control
**Power Savings: ~100µA → <1µA**

```cpp
// Release warp buffer allocation
if (warpBuffer != nullptr) {
    free(warpBuffer);
    warpBuffer = nullptr;
}

// Power down PSRAM domain
esp_sleep_pd_config(ESP_PD_DOMAIN_VDDSDIO, ESP_PD_OPTION_OFF);

// On wake-up:
esp_sleep_pd_config(ESP_PD_DOMAIN_VDDSDIO, ESP_PD_OPTION_ON);
```

**Impact**: Reduces PSRAM standby current during deep sleep.

### 4. WS2812B LED Power-Down Improvement
**Power Savings: 1-5mA → <1µA**

```cpp
// Double clear for reliability
fill_solid(leds, NUM_LEDS, CRGB::Black);
FastLED.show();
FastLED.show(); // Send twice for reliability
delay(10);
FastLED.clear(true);
delay(10);

// Then set pin to OUTPUT LOW with hold
pinMode(LED_PIN, OUTPUT);
digitalWrite(LED_PIN, LOW);
delayMicroseconds(100); // Stabilization delay
gpio_hold_en((gpio_num_t)LED_PIN);
```

**Impact**: Ensures LEDs are fully off and data pin doesn't float.

### 5. GPIO Pin Stabilization
**Power Savings: Prevents current spikes**

```cpp
// Add stabilization delay before holding any output pin
pinMode(VIBRATION_PIN, OUTPUT);
digitalWrite(VIBRATION_PIN, LOW);
delayMicroseconds(100); // Allow pin to stabilize
gpio_hold_en((gpio_num_t)VIBRATION_PIN);
```

**Impact**: Prevents brief current spikes during GPIO state transitions.

### 6. Secondary Button Optimization
**Power Savings: ~33µA → <1µA**

```cpp
// Disable pull-ups and isolate pin
pinMode(SECOND_BUTTON_PIN, INPUT);
rtc_gpio_pullup_dis((gpio_num_t)SECOND_BUTTON_PIN);
rtc_gpio_pulldown_dis((gpio_num_t)SECOND_BUTTON_PIN);
rtc_gpio_isolate((gpio_num_t)SECOND_BUTTON_PIN);

// On wake-up:
rtc_gpio_deinit((gpio_num_t)SECOND_BUTTON_PIN);
pinMode(SECOND_BUTTON_PIN, INPUT_PULLUP);
```

**Impact**: Removes unnecessary pull-up current draw during sleep.

### 7. ADC Power Release
**Power Savings: Ensures ADC is fully powered down**

```cpp
adc_power_release(); // Release any ADC power locks
```

**Note**: ESP32-S3 automatically powers down ADC in deep sleep, but releasing power locks ensures no software is preventing this.

### 8. Wake-Up Monitoring (Debugging)

```cpp
Serial.println("=== WAKE FROM DEEP SLEEP ===");
Serial.printf("Wake-up reason: Button press (EXT0)\n");
Serial.printf("Time in deep sleep: %llu ms\n", esp_timer_get_time() / 1000);
```

**Impact**: Helps debug sleep/wake cycles and verify power savings.

## Hardware Limitations

### Potentiometer Voltage Divider ⚠️
**Current Draw: ~330µA (cannot be eliminated in software)**

The 10kΩ potentiometer creates a voltage divider from VCC to GND that continuously draws current:
- Current = 3.3V / 10kΩ = 330µA

**Software cannot fix this** - it's a hardware design issue.

**Hardware Fix Options**:
1. Add a P-channel MOSFET to switch potentiometer VCC
2. Add a dedicated power control IC
3. Use a digital potentiometer with shutdown mode

## Expected Power Consumption

### Before Optimization
- **Estimated**: 50-500µA (highly variable)
- **Main culprits**: TFT display (5-20mA), SPI pins (1-5mA)

### After Optimization
- **Expected**: 10-50µA for ESP32-S3 core
- **Hardware limitation**: +330µA from potentiometer
- **Total**: ~340-380µA

### With Hardware Fix (Future)
- **Target**: <50µA total (ESP32-S3 spec: 7-150µA in deep sleep)

## Battery Life Estimates

Assuming 2000mAh battery:

| Configuration | Current Draw | Battery Life (powered off) |
|--------------|-------------|---------------------------|
| Before fixes | ~50mA | ~40 hours (~1.7 days) |
| After software fixes | ~0.35mA | ~238 days (~8 months) |
| With hardware fix | ~0.05mA | ~4.5 years |

## Testing Checklist

1. **Verify deep sleep entry**
   - Monitor Serial output for "Entering deep sleep mode..."
   - Confirm all shutdown messages appear

2. **Measure actual current draw**
   - Use multimeter in series with battery
   - Typical deep sleep: 10-50µA (ESP32 only)
   - With pot: +330µA

3. **Test wake-up functionality**
   - Press button to wake from deep sleep
   - Verify Serial message shows wake reason
   - Confirm display wakes properly

4. **Monitor battery life**
   - Note battery voltage before/after extended sleep
   - Calculate actual current draw from voltage drop

## Pin Configuration Summary

### Pins Held LOW During Sleep
- `TFT_LED` (GPIO 13) - Backlight off
- `TFT_CS` (GPIO 8) - SPI chip select (held HIGH, not LOW)
- `LED_PIN` (GPIO 2) - WS2812B data line
- `VIBRATION_PIN` (GPIO 5) - Motor off
- `BUZZER_PIN` (GPIO 6) - Sound off

### Pins Set to INPUT
- `TFT_DC` (GPIO 9) - SPI data/command
- `TFT_RST` (GPIO 10) - Display reset
- `TFT_MOSI` (GPIO 11) - SPI data
- `TFT_SCLK` (GPIO 12) - SPI clock
- `POT_PIN` (GPIO 7) - Potentiometer (ADC)
- `SECOND_BUTTON_PIN` (GPIO 3) - Isolated

### Pins with Special Configuration
- `BUTTON_PIN` (GPIO 1) - **NOT HELD** (must be free for wake-up)
  - RTC pull-up enabled
  - Configured for EXT0 wake-up

## Troubleshooting

### Device Won't Wake Up
- Check that `BUTTON_PIN` is not held (no `rtc_gpio_hold_en` on this pin)
- Verify button is connected to GPIO 1
- Ensure button is normally HIGH, pulls LOW when pressed

### High Current Draw in Sleep
1. Verify TFT sleep command is executed (check Serial output)
2. Check for floating SPI pins with multimeter
3. Measure potentiometer circuit (should be ~330µA)
4. Look for external components still powered

### Display Issues on Wake
1. Ensure SLPOUT (0x11) command is sent on wake
2. Add longer delays if display is slow to wake
3. Check if `tft.init()` conflicts with SLPOUT command

## Implementation Details

All changes are in `warpdrive_esp32_tft.ino`:

- **Power-off sequence**: `powerOff()` function (lines ~3115-3250)
- **Wake-up sequence**: `setup()` function (lines ~1012-1075)
- **Pin definitions**: From `User_Setup.h` (TFT_CS=8, TFT_DC=9, etc.)

## Future Improvements

1. **Add potentiometer power control** (hardware modification)
2. **Implement unused GPIO isolation** (software)
3. **Add current measurement reporting** (software with INA219 sensor)
4. **Test with external battery monitor** for long-term validation

## References

- ESP32-S3 Technical Reference Manual (Deep Sleep Current)
- ST7789/ST7735 Display Driver Datasheet (Sleep Commands)
- ESP-IDF Sleep Mode Documentation
- WS2812B LED Datasheet (Power-down Behavior)

---

**Last Updated**: November 15, 2025  
**Author**: AI Assistant (implementing user's thorough analysis)  
**Status**: Implemented and ready for testing

