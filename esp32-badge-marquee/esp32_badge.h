#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>

#define TFT_DC     4
#define TFT_CS    15
#define TFT_RST    2
#define TFT_MISO  19         
#define TFT_MOSI  23           
#define TFT_CLK   18 

#define BTN_UP    36
#define BTN_DOWN  39
#define BTN_LEFT  34
#define BTN_RIGHT 35
#define BTN_A     33
#define BTN_B      5
#define BTN_X     26 
#define BTN_Y     27 

#define SPKR      25  // DAC1
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

#define SCREEN_ROT 1 // horizontal, w/TFT pins on the left
#define SCREEN_WD  ILI9341_TFTWIDTH  // 320
#define SCREEN_HT  ILI9341_TFTHEIGHT // 240



static bool btnA_pressed, btnB_pressed, btnX_pressed, btnY_pressed;
static bool btnUp_pressed, btnDown_pressed, btnLeft_pressed, btnRight_pressed;
static bool spkr_on, led1_on, led2_on, led3_on;

uint8_t spkr_channel = 1;
