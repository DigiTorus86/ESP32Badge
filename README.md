# ESP32Badge
Source code and resources for the ESP32 Conference Badge

![alt text](https://raw.githubusercontent.com/DigiTorus86/ESP32Badge/master/images/BadgeRender_v1b.png)

Requires the ESP32 conference badge or similar hardware:
- PCB:  https://www.pcbway.com/project/shareproject/ESP32_Conference_Badge_v1_2.html
- ESP32 DEVKIT v1 w/30 pins or DevKitC w/38 pins (other ESP32 boards can be used with minimal PIN changes)
- ILI9341 320x240 TFT
- [8] Tactile momentary buttons
- [4] 4k7 resistors (button pullups)
- [3] 5mm LEDs (one each of red, yellow, green)
- [3] 100R resistors (LED current limiters)
- small speaker/active buzzer  (8 Ohm / 2 Watt)
- CR18650 battery and battery holder are optional
- Some sketches have additional hardware requirements (components attached to I2C or Serial pins, or SD card)

Required Libraries for Arduino platform sketches:
- Adafruit GFX
- Adafruit ILI9341
- ArduinoJson
- XTronical XT_DAC  https://www.xtronical.com/the-dacaudio-library-download-and-installation/

Example Sketches:
- Basic:  3 Simple badges combined into one (Marquee, Hello, Hello w/changing message)
- Gravitack:  Low-gravity lunar lander type game
- Marquee:  Name badge with moving light border and fading LEDs
- QR Code:  Name badge with QR code display of the ESP32 IP address and simple web page for controlling the LEDs
- Slideshow: Timed slideshow display.  Requires v1.2 of the badge with 38-pin DevKitC and an SD card.
- Test1:  Test app to verify functionality of the badge.  Also available for the ESP-IDF platform. 
- Time & Weather:  Displays time and weather data from DS3231 RTC and BME280 temperature/humidity/pressure sensor via I2C as well as network weather info
- Tombstone:  Classic Morg-shooting schooner game
- Wizard:  Animated badge for wizards and witches that have escaped from notorious magical prisons

Accessories:
- Kickstand:  Minimalist 3D-printed kickstand for the badge to get it to stand upright on a flat surface.  Attach to the middle holes on badge (just below the buttons) with 2 small flat-headed woodscrews.

Additional Info:
- The TFT display orientation is rotated 180 degrees between the v1.1 /DevKitv1 board and v1.2 DEVKITC versions.  Adjust the SCREEN_ROT value in esp32_badge.h to 1 for v1.1 or 3 for v1.2 boards.  
- SD card slot on the TFT is not connected in v1.1
