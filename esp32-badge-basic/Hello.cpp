/*
 * Displays a "Hello, my name is" badge with two lines of text (i.e. attendee name and company)
 */

#include "Hello.h"

Hello::Hello()
{
}
 
void Hello::begin(const char* textLine1, const char* textLine2, const char* textLine3, Adafruit_ILI9341 *tft, const GFXfont *gfxFontHello, const GFXfont *gfxFontMyNameIs, const GFXfont *gfxFontTextLines)
{
  // Draw badge background
  tft->fillRect(0, 0, SCREEN_WD, 80, HLO_COLOR_HEADER);
  tft->fillRect(0, 81, SCREEN_WD, 150, ILI9341_WHITE);
  tft->fillRect(0, 232, SCREEN_WD, 7, HLO_COLOR_HEADER);

  tft->setFont(gfxFontHello);
  tft->setTextColor(ILI9341_WHITE);
  tft->setCursor(50, 46);
  tft->print("H E L L O");

  tft->setFont(gfxFontMyNameIs);
  tft->setTextColor(ILI9341_WHITE);
  tft->setCursor(106, 68);
  tft->print("my name is");
  
  // Display the text lines
  
  int16_t  x1, y1; 
  uint16_t wd, ht;
  tft->setFont(gfxFontTextLines);
  tft->getTextBounds(textLine1, 10, 50, &x1, &y1, &wd, &ht);
   
  tft->setCursor(SCREEN_WD / 2 - (wd / 2), 96 + ht);
  tft->setTextColor(HLO_COLOR_TEXTLINE1);
  tft->print(textLine1);

  tft->getTextBounds(textLine2, 10, 50, &x1, &y1, &wd, &ht);
  tft->setCursor(SCREEN_WD / 2 - (wd / 2), 180);
  tft->setTextColor(HLO_COLOR_TEXTLINE2);
  tft->print(textLine2);

  tft->getTextBounds(textLine3, 10, 50, &x1, &y1, &wd, &ht);
  tft->setCursor(SCREEN_WD / 2 - (wd / 2), 214);
  tft->setTextColor(HLO_COLOR_TEXTLINE2);
  tft->print(textLine3);
}

/*
 * Update the badge lights and display
 */
void Hello::update(Adafruit_ILI9341 *tft)
{

}
