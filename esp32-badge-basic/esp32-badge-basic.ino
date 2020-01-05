/***************************************************
 * ESP32 Basic Bro Badge
 * - Displays several different basic badges
 * 1. Marquee
 * 2. Hello, My Name Is....
 * 3. Hello, I'm... with periodically rotating messages
 * 4. Gangster (or whatever static picture you choose)
 *   
 * Requires:
 * - ESP32 Conference Badge
 * 
 * Controls:
 * - X/Y: Change the display mode
 *
 * * Copyright (c) 2019 Paul Pagel
 * This is free software; see the license.txt file for more information.
 * There is no warranty; not even for merchantability or fitness for a particular purpose.
 ****************************************************/

#include "esp32_badge.h"
#include "BatteryLevel.h"
#include "Marquee.h"
#include "Hello.h"
#include "HelloRotate.h"
#include "gangster.h"
#include <Fonts/FreeSansBold24pt7b.h>  
#include <Fonts/FreeSansBold9pt7b.h>  
#include <Fonts/FreeSansOblique18pt7b.h>

const char* my_name  = "Paul Pagel";
const char* company  = "First Solar";

#define DISPLAY_MODE_CNT  4
enum display_mode_type 
{
  MODE_MARQUEE,
  MODE_HELLO,
  MODE_HELLO_ROTATE,
  MODE_GANGSTER
};
enum display_mode_type display_mode, prev_display_mode;

bool btnA_pressed, btnB_pressed, btnX_pressed, btnY_pressed;
bool btnUp_pressed, btnDown_pressed, btnLeft_pressed, btnRight_pressed;
bool btnUp_released, btnDown_released, btnLeft_released, btnRight_released;

Marquee marquee = Marquee();
Hello hello = Hello();
HelloRotate hello_rot = HelloRotate(10000); // rotate messages every 10 seconds

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK, TFT_RST, TFT_MISO);
BatteryLevel battery = BatteryLevel(BATT_LVL, 5000);

/*
 * Initial Setup - called once
 */
void setup() 
{
  Serial.begin(115200);
  Serial.println("ESP32 Basic Bro Badge"); 
  delay(100);

  display_mode = MODE_HELLO;

  // Set up user input pins
  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);   
  pinMode(BTN_LEFT, INPUT_PULLUP);
  pinMode(BTN_RIGHT, INPUT_PULLUP);
  pinMode(BTN_A, INPUT_PULLUP);
  pinMode(BTN_B, INPUT_PULLUP);
  pinMode(BTN_X, INPUT_PULLUP);
  pinMode(BTN_Y, INPUT_PULLUP);
  
  // Set up the TFT screen
  tft.begin();
  tft.setRotation(SCREEN_ROT);
  
  battery.checkLevel();
  battery.drawLevel(286, 214, &tft);
}

void updateScreen()
{
  if (prev_display_mode != display_mode)
  {
    // do the initial drawing of the screen for the new display mode
    switch (display_mode)
    {
      case MODE_MARQUEE: 
        tft.setFont(&FreeSansBold24pt7b);
        marquee.begin(my_name, company, &tft);
        break;
        
      case MODE_HELLO: 
        marquee.resetLEDs();
        hello.begin("Inigo Montoya", "You killed my father.", "Prepare to die.", &tft, &FreeSansBold24pt7b, &FreeSansBold9pt7b, &FreeSansOblique18pt7b);
        break;

      case MODE_HELLO_ROTATE: 
        // see HelloRotate.h message array to change the displayed text
        hello_rot.begin(&tft, &FreeSansBold24pt7b);
        break;  

      case MODE_GANGSTER:
        // put whatever 320x240 image you want displayed in gangster.h
        tft.drawRGBBitmap(0, 0, (uint16_t *)gangster, 320, 240);
        break;
    }
  }

  prev_display_mode = display_mode;
  
  // do the update for the current display mode
  switch (display_mode)
  {
    case MODE_MARQUEE: 
        marquee.update(&tft);
        break;
        
      case MODE_HELLO: 
        hello.update(&tft);
        break;

      case MODE_HELLO_ROTATE: 
        hello_rot.update(&tft, &FreeSansOblique18pt7b);
        break;  

      case MODE_GANGSTER: 
        // static image displayed - nothing to do
        break;  
  }
}

/*
 * Main program loop - called repeatedly
 */
void loop(void) 
{
  btnUp_released = false; 
  btnDown_released = false; 
  btnLeft_released = false;
  btnRight_released = false;
  
  if(digitalRead(BTN_UP) == LOW)
  {
    btnUp_pressed = true;
  }
  else if (btnUp_pressed)
  {
    btnUp_released = true;
    btnUp_pressed = false;
  }
  else
  {
    btnUp_pressed = false;
  }

  if(digitalRead(BTN_DOWN) == LOW)
  {
    btnDown_pressed = true;
  }
  else if (btnDown_pressed)
  {
    btnDown_released = true;
    btnDown_pressed = false;
  }
  else
  {
    btnDown_pressed = false;
  }

  if(digitalRead(BTN_LEFT) == LOW)
  {
    btnLeft_pressed = true;
  }
  else if (btnLeft_pressed)
  {
    
    btnLeft_released = true;
    btnLeft_pressed = false;
  }
  else
  {
    btnLeft_pressed = false;
  }

  if(digitalRead(BTN_RIGHT) == LOW)
  {
    btnRight_pressed = true;
  }
  else if (btnRight_pressed)
  {
    btnRight_released = true;
    btnRight_pressed = false;
  }
  else
  {
    btnRight_pressed = false;
  }
  
  
  if(digitalRead(BTN_X) == LOW)
  {
    btnX_pressed = true;
  }
  else if (btnX_pressed)
  {
    display_mode = (display_mode_type)(display_mode > 0 ? display_mode - 1 : DISPLAY_MODE_CNT - 1);
    btnX_pressed = false;
  }
  else
  {
    btnX_pressed = false;
  }

  if(digitalRead(BTN_Y) == LOW)
  {
    btnY_pressed = true;
  }
  else if (btnY_pressed) // was just pressed
  {
    display_mode = (display_mode_type)((display_mode + 1) % DISPLAY_MODE_CNT);
    btnY_pressed = false;
  }
  else
  {
    btnY_pressed = false;
  }

  if(digitalRead(BTN_A) == LOW)
  {
    btnA_pressed = true;
  }
  else if (btnA_pressed)
  {
    
    btnA_pressed = false;
  }
  else
  {
    btnA_pressed = false;
  }
  
  if(digitalRead(BTN_B) == LOW)
  {
    btnB_pressed = true;
    
  }
  else if (btnB_pressed)
  {
    //dumpScreen(0, 0, 320, 240, tft); // TESTING!!
    btnB_pressed = false;
  }
  else
  {
    btnB_pressed = false;
  }
  
  updateScreen();
  
  if (battery.isTimeToCheck())
  {
    battery.checkLevel();
    battery.drawLevel(286, 214, &tft);
  }
  delay(50);
}
