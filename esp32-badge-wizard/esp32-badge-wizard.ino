/***************************************************
ESP32 Badge - Have You Seen This Wizard?  

Requires:
 - ESP32 Badge kit

Copyright (c) 2019 Paul Pagel
This is free software; see the license.txt file for more information.
There is no warranty; not even for merchantability or fitness for a particular purpose.
*****************************************************/

#include "esp32_badge.h"
#include "WiFi.h"
#include "Wizard.h" 
#include "NotSirius.h"

#define BACK_COLOR 0xD678 // gray-ish
#define PIC_X   113
#define PIC_Y   110
#define PIC_CNT   6
#define FRAME_DELAY_MS  100

int  pic_idx = 0;
int  pic_chg = 1; 

uint8_t spkr_channel = 1;


Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK, TFT_RST, TFT_MISO);

/*
 * Initialization and peripheral setup.  Called once at program start. 
 */
void setup() 
{
  Serial.begin(115200);
  Serial.println("ESP32 Badge Wizard"); 
  delay(100);

  // Set up the speaker and LED output pins
  pinMode(SPKR, OUTPUT);
  pinMode(LED_1, OUTPUT);
  pinMode(LED_2, OUTPUT);
  pinMode(LED_3, OUTPUT);

  delay(100);
 
  tft.begin();
  tft.setRotation(SCREEN_ROT);
  tft.fillScreen(BACK_COLOR);


  tft.drawRGBBitmap(61, 0, (uint16_t *)HaveYouSeen, 197, 86);
  tft.drawRGBBitmap(30, 80, (uint16_t *)Star, 34, 34);
  tft.drawRGBBitmap(258, 80, (uint16_t *)Star, 34, 34);
  
  tft.drawRGBBitmap(10, 132, (uint16_t *)Approach, 80, 77);
  tft.drawRGBBitmap(225, 125, (uint16_t *)DoNotAttempt, 91, 94);

  tft.fillRect(109, 104, 109, 129, ILI9341_BLACK); // border
}

/*
 * Main program loop.  Called continuously after setup.
 */
void loop(void) 
{

  switch (pic_idx)
  {
    case 0:
      tft.drawRGBBitmap(PIC_X, PIC_Y, (uint16_t *)NotSirius1, 99, 120); break;
    case 1:
      tft.drawRGBBitmap(PIC_X, PIC_Y, (uint16_t *)NotSirius2, 99, 120); break;
    case 2:
      tft.drawRGBBitmap(PIC_X, PIC_Y, (uint16_t *)NotSirius3, 99, 120); break;  
    case 3:
      tft.drawRGBBitmap(PIC_X, PIC_Y, (uint16_t *)NotSirius4, 99, 120); break;
    case 4:
      tft.drawRGBBitmap(PIC_X, PIC_Y, (uint16_t *)NotSirius5, 99, 120); break;
    case 5:
      tft.drawRGBBitmap(PIC_X, PIC_Y, (uint16_t *)NotSirius6, 99, 120); break;
  }

  pic_idx += pic_chg;

  if (pic_idx < 0)
  {
    pic_idx = 1;
    pic_chg = 1;
  }

  if (pic_idx >= PIC_CNT)
  {
    pic_idx = PIC_CNT - 2;
    pic_chg = -1;
  }

  delay(FRAME_DELAY_MS);
}
