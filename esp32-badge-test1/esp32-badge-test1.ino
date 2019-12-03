/***************************************************
ESP32 Badge test app
Requires:
 - ESP32 Devkit v1 
 - ILI9431 SPI TFT  
 - Passive buzzer/speaker
 - 1k trim pot
 - (8) momentary buttons
 - (4) 4k7 pullup resistors
 - (3) 220R LED resistors
 ****************************************************/

#include <WiFi.h>
#include "esp32_badge.h"
#include "codemash_logo.h"

bool btnA_pressed, btnB_pressed, btnX_pressed, btnY_pressed;
bool btnUp_pressed, btnDown_pressed, btnLeft_pressed, btnRight_pressed;
bool spkr_on, led1_on, led2_on, led3_on;

uint8_t spkr_channel = 1;

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK, TFT_RST, TFT_MISO);

/*
 * Initialization and peripheral setup.  Called once at program start. 
 */
void setup() 
{
  Serial.begin(115200);
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
  
  // Set up the TFT screen
  tft.begin();
  tft.setRotation(SCREEN_ROT);

  // read screen diagnostics (optional but can help debug problems)
  uint8_t x = tft.readcommand8(ILI9341_RDMODE);
  Serial.print("Display Power Mode: 0x"); Serial.println(x, HEX);
  x = tft.readcommand8(ILI9341_RDMADCTL);
  Serial.print("MADCTL Mode: 0x"); Serial.println(x, HEX);
  x = tft.readcommand8(ILI9341_RDPIXFMT);
  Serial.print("Pixel Format: 0x"); Serial.println(x, HEX);
  x = tft.readcommand8(ILI9341_RDIMGFMT);
  Serial.print("Image Format: 0x"); Serial.println(x, HEX);
  x = tft.readcommand8(ILI9341_RDSELFDIAG);
  Serial.print("Self Diagnostic: 0x"); Serial.println(x, HEX); 

  tft.fillScreen(ILI9341_BLACK);
  tft.setTextColor(ILI9341_WHITE);  
  tft.setTextSize(3);

  tft.setCursor(16, 0);
  tft.println("ESP32 Badge Test");
  
  tft.drawLine(0, 35, 319, 35, ILI9341_BLUE);
  tft.drawLine(0, 180, 319, 180, ILI9341_BLUE);

  delay(200);

  //WiFi.mode(WIFI_STA);
  //WiFi.disconnect();
  delay(200);
  int  wifi_count;
  wifi_count = 0; //WiFi.scanNetworks();

  tft.setTextSize(2);
  tft.setCursor(0, 190);
  tft.setTextColor(ILI9341_ORANGE);
  tft.print("WiFi Nets: ");
  tft.print(wifi_count);

  tft.drawRGBBitmap(250, 50, (uint16_t *)codemash_logo, 57, 67);
}

/*
 * Use to toggle between normal and inverted text display
 */
void setTextColor(uint16_t color, bool inverted)
{
  if (inverted)
    tft.setTextColor(ILI9341_BLACK, color);
  else
    tft.setTextColor(color, ILI9341_BLACK);
  
}

/*
 * Draws the little battery level icon
 */
void drawBatteryLevel(int battLevel, int x, int y)
{
  uint16_t fill_color = ILI9341_GREEN;
  int batt_percent = max(min((battLevel - BATT_FULL + 150) / 2, 100), 0);

  if (batt_percent < 25)
  { 
    digitalWrite(LED_1, HIGH);
    ledcWrite(spkr_channel, 128);
    ledcWriteTone(spkr_channel, 440);
  }
  else if (batt_percent < 50)
  {
    digitalWrite(LED_2, HIGH);
    ledcWrite(spkr_channel, 128);
    ledcWriteTone(spkr_channel, 440);
  }
 
  if (batt_percent <= 50)
      fill_color = ILI9341_YELLOW;
 
  if (batt_percent <= 25)
      fill_color = ILI9341_RED;
 
  // draw outline of battery
  tft.fillRect(x, y + 2, 3, 6, ILI9341_DARKGREY);
  tft.drawRect(x + 3, y, 24, 10, ILI9341_DARKGREY);
 
  for (int i = 0; i < 4; i++)
  {
    if (batt_percent < 12 + i * 25)
      fill_color = ILI9341_BLACK;
    tft.fillRect(x + 6 + (i * 5), y + 2, 4, 6, fill_color);
  }

  digitalWrite(LED_1, LOW);
  digitalWrite(LED_2, LOW);
}

/*
 * Checks the battery level and displays it to the screen
 */
void checkBattery()
{
  // 1020 on USB
  // 960 on full batt
  // 935 = 3.9v

  static uint8_t cntr = 0;

  if (cntr == 0)
  {
    int val = analogRead(BATT_LVL);
  
    tft.fillRect(140, 210, 50, 20, ILI9341_BLUE);
  
    tft.setTextSize(2);
    tft.setCursor(0, 210);
    tft.setTextColor(ILI9341_ORANGE);
    tft.print("Battery:");
    tft.setCursor(140, 210);
    tft.print(val);

    drawBatteryLevel(val, 200, 214);
  }

  cntr = (cntr + 1) % 10;
}

/*
 * Updates the screen based on which buttons are being pressed
 */
void updateScreen()
{
  // Direction buttons
  tft.setCursor(45, 50);
  setTextColor(ILI9341_CYAN, btnUp_pressed);
  tft.print("UP"); 

  tft.setCursor(0, 70);
  setTextColor(ILI9341_CYAN, btnLeft_pressed);
  tft.print("<-LT"); 

  tft.setCursor(60, 70);
  setTextColor(ILI9341_CYAN, btnRight_pressed);
  tft.print("RT->");
 
  tft.setCursor(45, 90);
  setTextColor(ILI9341_CYAN, btnDown_pressed);
  tft.print("DN");

  int x = 140;

  // Action buttons
  tft.setCursor(x+30, 50);
  setTextColor(ILI9341_CYAN, btnY_pressed);
  tft.print("Y"); 

  tft.setCursor(x, 70);
  setTextColor(ILI9341_CYAN, btnX_pressed);
  tft.print("X"); 
 
  tft.setCursor(x+60, 70);
  setTextColor(ILI9341_CYAN, btnB_pressed);
  tft.print("B");

  tft.setCursor(x+30, 90);
  setTextColor(ILI9341_CYAN, btnA_pressed);
  tft.print("A");

  tft.setCursor(x+30, 90);
  setTextColor(ILI9341_CYAN, btnA_pressed);
  tft.print("A");


  tft.setCursor(0, 140);
  setTextColor(ILI9341_LIGHTGREY, spkr_on);
  tft.print("(( SPKR ))");

  tft.setCursor(140, 140);
  setTextColor(ILI9341_RED, led1_on);
  tft.print("LED1");

  tft.setCursor(200, 140);
  setTextColor(ILI9341_YELLOW, led2_on);
  tft.print("LED2");

  tft.setCursor(260, 140);
  setTextColor(ILI9341_GREEN, led3_on);
  tft.print("LED3");
  
}

/*
 * Main program loop.  Called continuously after setup.
 */
void loop(void) 
{
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
    digitalWrite(LED_1, HIGH);
    led1_on = true;
  }
  else
  {
    btnX_pressed = false;
    digitalWrite(LED_1, LOW);
    led1_on = false;
  }

  if(digitalRead(BTN_Y) == LOW)
  {
    Serial.println("Button Y pressed");
    btnY_pressed = true;
    digitalWrite(LED_2, HIGH);
    led2_on = true;
  }
  else
  {
    btnY_pressed = false;
    digitalWrite(LED_2, LOW);
    led2_on = false;
  }

  if(digitalRead(BTN_A) == LOW)
  {
    Serial.println("Button A pressed");
    btnA_pressed = true;
    digitalWrite(LED_3, HIGH);
    led3_on = true;
  }
  else
  {
    btnA_pressed = false;
    digitalWrite(LED_3, LOW);
    led3_on = false;
  }
  
  if(digitalRead(BTN_B) == LOW)
  {
    Serial.println("Button B pressed");
    btnB_pressed = true;
    ledcWrite(spkr_channel, 128);
    ledcWriteTone(spkr_channel, 1000);
    spkr_on = true;
    
  }
  else
  {
    btnB_pressed = false;
    ledcWrite(spkr_channel, 0);
    spkr_on = false;
  }
  
  updateScreen();
  checkBattery();
  delay(100);
}
