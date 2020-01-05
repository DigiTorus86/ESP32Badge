#ifndef _ESP32_BADGE_
#define _ESP32_BADGE_

#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>

// GPIO pin assignments for the ILI9341 TFT display
#define TFT_DC     4  // Data/Command pin
#define TFT_CS    15  // Chip Select pin
#define TFT_RST    2  // Reset pin, also builit-in blue LED
#define TFT_MISO  19  // Master In Slave Out       
#define TFT_MOSI  23  // Master Out Slave In         
#define TFT_CLK   18  // SPI Clock
#define SD_CS      0  // SD card reader Chip Select pin

// GPIO pin assignments for the badge buttons
#define BTN_UP    36
#define BTN_DOWN  39
#define BTN_LEFT  34
#define BTN_RIGHT 35
#define BTN_A     33
#define BTN_B      5
#define BTN_X     26 
#define BTN_Y     27 

#define SPKR      25  // Speaker pin DAC1
#define BATT_LVL  32  // voltage divider

#define LED_1     14  // red
#define LED_2     12  // yellow
#define LED_3     13  // green

// ESP32 DevKit I2C Pins
// SDA = 21
// SCL = 22

// Serial 2 pins
// RX2 = 16
// TX2 = 17

#define BATT_USB  780  //  enter the reading from the battery level pin when the ESP32 is running off of USB power
#define BATT_FULL 700  //  enter the reading from the battery level pin when running from a fully-charged battery
#define BATT_WARN 630
#define BATT_DIRE 610

#define SCREEN_ROT 3   // 1 = horizontal, w/SD pins on the left, 3 = horizontal, w/SD pins on the right
#define SCREEN_WD  320
#define SCREEN_HT  240


#endif // _ESP32_BADGE_
