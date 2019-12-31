/***************************************************
ESP32 Badge starter framework

Requires:
 - ESP32 Badge
 ****************************************************/

#include "esp32_badge.h"

bool btnA_pressed, btnB_pressed, btnX_pressed, btnY_pressed;
bool btnA_released, btnB_released, btnX_released, btnY_released;
bool btnUp_pressed, btnDown_pressed, btnLeft_pressed, btnRight_pressed;
bool btnUp_released, btnDown_released, btnLeft_released, btnRight_released;
bool spkr_on, led1_on, led2_on, led3_on;

uint8_t spkr_channel = 1;

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK, TFT_RST, TFT_MISO);

/*
 * Initialization and peripheral setup.  Called once at program start. 
 */
void setup() 
{
  Serial.begin(115200);
  Serial.println("ESP32 Badge"); 
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
  
  // Set up the TFT screen
  tft.begin();
  tft.setRotation(SCREEN_ROT);

  tft.fillScreen(ILI9341_BLACK);
  tft.setTextColor(ILI9341_WHITE);  
  tft.setTextSize(2);

  tft.setCursor(110, 60);
  tft.println("ESP32 Badge");

  delay(200);
}


/*
 * Updates the screen based on which buttons are being pressed
 */
void updateScreen()
{
 
  
}

/*
 * Main program loop
 */
void loop(void)
{
  unsigned long start_time = millis();

  btnA_released = false; 
  btnB_released = false;
  btnX_released = false;
  btnY_released = false;

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
    btnX_released = true;
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
  else if (btnY_pressed) 
  {
    btnY_released = true;
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
    btnA_released = true;
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
    btnB_released = true;
    btnB_pressed = false;
  }
  else
  {
    btnB_pressed = false;
  }

  // TODO:  put specific app mode/logic here
  
  delay(50);
}
