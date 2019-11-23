/*
 * Displays a "Hello, I'm" badge with a rotating selection of text messages
 */

#include "HelloRotate.h"

HelloRotate::HelloRotate(uint32_t rotate_ms)
{
	_rotate_ms = rotate_ms;
}

/*
 * Draw the badge background and title line
 */
void HelloRotate::begin(Adafruit_ILI9341 *tft, const GFXfont *gfxFontHello)
{
  // Draw badge background
  tft->fillRect(0, 0, SCREEN_WD, 80, HLOR_COLOR_HEADER);
  tft->fillRect(0, 81, SCREEN_WD, 150, ILI9341_WHITE);
  tft->fillRect(0, 232, SCREEN_WD, 7, HLOR_COLOR_HEADER);

  tft->setFont(gfxFontHello);
  tft->setTextColor(ILI9341_WHITE);
  tft->setCursor(32, 50);
  tft->print("HELLO, I'm");
}

/*
 * Update the badge lights and display
 */
void HelloRotate::update(Adafruit_ILI9341 *tft, const GFXfont *gfxFontText)
{
  if (_last_rotate_ms > 0 && _last_rotate_ms + _rotate_ms > millis())
  {
    // not yet time to update / change the message
    return;
  }
  
  int16_t  x1, y1; 
  uint16_t wd, ht1, ht2, ht3;
  
  tft->fillRect(0, 81, SCREEN_WD, 150, ILI9341_WHITE);
  tft->setFont(gfxFontText);
  tft->getTextBounds(_message[_message_idx][0], 10, 50, &x1, &y1, &wd, &ht1);
  
  tft->setCursor(SCREEN_WD / 2 - (wd / 2), 96 + ht1);
  tft->setTextColor(HLOR_COLOR_TEXT);
  tft->print(_message[_message_idx][0]);

  tft->getTextBounds(_message[_message_idx][1], 10, 50, &x1, &y1, &wd, &ht2);
  tft->setCursor(SCREEN_WD / 2 - (wd / 2), 96 + HLOR_LINESPACE + ht1 + ht2);
  tft->print(_message[_message_idx][1]);

  tft->getTextBounds(_message[_message_idx][2], 10, 50, &x1, &y1, &wd, &ht3);
  tft->setCursor(SCREEN_WD / 2 - (wd / 2), 96 + (HLOR_LINESPACE * 2) + ht1 + ht2 + ht3);
  tft->print(_message[_message_idx][2]);

  _message_idx = (_message_idx + 1) % HLOR_MESSAGE_CNT;
  _last_rotate_ms = millis();
}
