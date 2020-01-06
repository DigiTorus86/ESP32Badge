#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <Fonts/FreeSansBold24pt7b.h>  
#include <Fonts/FreeSansBold9pt7b.h>  
#include <Fonts/FreeSansOblique18pt7b.h>
#include "mypicture.h"

Adafruit_ILI9341 tft = Adafruit_ILI9341(15, 4, 23, 18, 2, 19);

const char* my_name  = "";  // TODO: put your name here

void setup() {
  tft.begin();
  tft.setRotation(3); 
  
  // Draw badge background
  tft.fillRect(0, 0, 320, 80, ILI9341_RED);
  tft.fillRect(0, 81, 320, 150, ILI9341_WHITE);
  tft.fillRect(0, 232, 320, 7, ILI9341_RED);

  tft.setFont(&FreeSansBold24pt7b);
  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(50, 46);
  tft.print("H E L L O");

  tft.setFont(&FreeSansBold9pt7b);
  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(106, 68);
  tft.print("my name is");

  int16_t  x1, y1; 
  uint16_t wd, ht;
  tft.setFont(&FreeSansOblique18pt7b);
  // Get the size of the displayed name using this font so we can center it
  tft.getTextBounds(my_name, 10, 50, &x1, &y1, &wd, &ht);
  
  tft.setCursor(220 - (wd / 2), 120 + ht);
  tft.setTextColor(ILI9341_BLACK);
  tft.print(my_name);

  tft.drawRGBBitmap(0, 100, (uint16_t *)mypicture, 120, 120);

  // Set up the LED GPIO pins
  pinMode(14, OUTPUT);  // red
  pinMode(12, OUTPUT);  // yellow
  pinMode(13, OUTPUT);  // green
}

void loop() {

  digitalWrite(14, HIGH);
  digitalWrite(12, HIGH);
  digitalWrite(13, HIGH);

  delay(250);

  digitalWrite(14, LOW);
  digitalWrite(12, LOW);
  digitalWrite(13, LOW);

  delay(250);
}
