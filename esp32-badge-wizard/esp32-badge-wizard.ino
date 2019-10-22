/***************************************************
ESP32 Badge - Have You Seen This Wizard?  
Requires:
 - ESP32 Badge kit
 ****************************************************/

#include "esp32_badge.h"
#include "WiFi.h"
#include "Wizard.h" 
#include "NotSirius.h"

#define BACK_COLOR 0xD678 // gray-ish
#define PIC_X   113
#define PIC_Y   110
#define PIC_CNT   6

static int  pic_idx = 0;
static int  pic_chg = 1; 


Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK, TFT_RST, TFT_MISO);

void setup() 
{
  Serial.begin(9600);
  Serial.println("ESP32 Badge Test1"); 
  delay(100);

  ledcSetup(spkr_channel, 12000, 8);  // 12 kHz max, 8 bit resolution
  ledcAttachPin(SPKR, spkr_channel);

  // Set up user input pins
  // Pins 24, 35, 36, & 39 on ESP32 do NOT have internal pullups, so need 4k7 physical resistors for these
  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);   
  pinMode(BTN_LEFT, INPUT_PULLUP);
  pinMode(BTN_RIGHT, INPUT_PULLUP);
  pinMode(BTN_A, INPUT_PULLUP);
  pinMode(BTN_B, INPUT_PULLUP);
  pinMode(BTN_X, INPUT_PULLUP);
  pinMode(BTN_Y, INPUT_PULLUP);

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


void checkBattery()
{
  // 1020 on USB
  // 960 on full batt
  // 935 = 3.9v

  static uint8_t cntr = 0;

  if (cntr == 0)
  {
    int val = analogRead(BATT_LVL);
  
    tft.fillRect(180, 210, 50, 20, ILI9341_BLUE);
  
    tft.setTextSize(2);
    tft.setCursor(0, 210);
    tft.setTextColor(ILI9341_ORANGE);
    tft.print("Battery:");
    tft.setCursor(180, 210);
    tft.print(val);
  }

  cntr = (cntr + 1) % 10;
}


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
  

  
  if(digitalRead(BTN_UP) == LOW)
  {
    Serial.println("Button Up pressed");
    btnUp_pressed = true;
  }
  else
  {
    btnUp_pressed = false;
  }

  if(digitalRead(BTN_DOWN) == LOW)
  {
    Serial.println("Button Down pressed");
    btnDown_pressed = true;
  }
  else
  {
    btnDown_pressed = false;
  }

  if(digitalRead(BTN_LEFT) == LOW)
  {
    Serial.println("Button Left pressed");
    btnLeft_pressed = true;
  }
  else
  {
    btnLeft_pressed = false;
  }

  if(digitalRead(BTN_RIGHT) == LOW)
  {
    Serial.println("Button Right pressed");
    btnRight_pressed = true;
  }
  else
  {
    btnRight_pressed = false;
  }
  
  
  if(digitalRead(BTN_X) == LOW)
  {
    Serial.println("Button X pressed");
    btnX_pressed = true;
  }
  else
  {
    btnX_pressed = false;
  }

  if(digitalRead(BTN_Y) == LOW)
  {
    Serial.println("Button Y pressed");
    btnY_pressed = true;
  }
  else
  {
    btnY_pressed = false;
  }

  if(digitalRead(BTN_A) == LOW)
  {
    Serial.println("Button A pressed");
    btnA_pressed = true;
  }
  else
  {
    btnA_pressed = false;
  }
  
  if(digitalRead(BTN_B) == LOW)
  {
    Serial.println("Button B pressed");
    btnB_pressed = true; 
  }
  else
  {
    btnB_pressed = false;
  }
  

  //checkBattery();
  delay(100);
}
