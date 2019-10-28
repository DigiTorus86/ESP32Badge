/*
 * 
 */

#include "BatteryLevel.h"

BatteryLevel::BatteryLevel(int pin, int check_interval_ms)
{
  _pin = pin;
  _check_interval_ms;
}


bool BatteryLevel::isTimeToCheck()
{
  return (millis() - _last_checked > _check_interval_ms);
}


int BatteryLevel::checkLevel()
{
  _raw_level = analogRead(_pin);
  _percent_remaining = max(min((_raw_level - 700 + 150) / 2, 100), 0);  // MAY NEED TO ADJUST THIS!!
  _last_checked = millis();
  return _percent_remaining;
}


int BatteryLevel::getRawLevel()
{
  return _raw_level;
}

        
void  BatteryLevel::drawLevel(int16_t x, int16_t y, Adafruit_ILI9341 *tft)
{
  uint16_t fill_color = ILI9341_GREEN;
 
  if (_percent_remaining <= 50)
      fill_color = ILI9341_YELLOW;
 
  if (_percent_remaining <= 25)
      fill_color = ILI9341_RED;
 
  // draw outline of battery
  tft->fillRect(x, y + 2, 3, 6, ILI9341_DARKGREY);
  tft->drawRect(x + 3, y, 24, 10, ILI9341_DARKGREY);
 
  for (int i = 0; i < 4; i++)
  {
    if (_percent_remaining < 12 + i * 25)
      fill_color = ILI9341_BLACK;
    tft->fillRect(x + 6 + (i * 5), y + 2, 4, 6, fill_color);
  }
}


bool BatteryLevel::checkAlarm(int warning_led, int danger_led, int spkr_channel)
{
  if (_percent_remaining < 25)
  { 
    digitalWrite(warning_led, HIGH);

    if (spkr_channel >= 0)
    {
      ledcWrite(spkr_channel, 128);
      ledcWriteTone(spkr_channel, 440);
    }
  }
  else if (_percent_remaining < 50)
  {
    digitalWrite(danger_led, HIGH);

    if (spkr_channel >= 0)
    {
      ledcWrite(spkr_channel, 128);
      ledcWriteTone(spkr_channel, 440);
    }
  }
}
