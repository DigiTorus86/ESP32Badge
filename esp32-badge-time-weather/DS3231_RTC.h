/*
 * Class for managing the DS3231 Real Time Clock via I2C
 */

#ifndef _DS3231_RTC__
#define _DS3231_RTC__

#include "Arduino.h"
#include <Wire.h>
#include "time.h"

#define DS3231_I2C_ADDRESS 0x68

struct ds3231_time_t 
{ 
  uint8_t second; 
  uint8_t minute; 
  uint8_t hour; 
  uint8_t dayOfWeek; 
  uint8_t dayOfMonth; 
  uint8_t month; 
  uint8_t year;
};


class DS3231_RTC
{
  public:
    DS3231_RTC();

    uint8_t decToBcd(uint8_t val);
    uint8_t bcdToDec(uint8_t val);
    bool setDS3231time(ds3231_time_t set_time);
    bool readDS3231time(struct ds3231_time_t *read_time);
    
    const char* getDayAbbr(uint8_t dayOfWeek);
    const char* getMonthAbbr(uint8_t month);

  private:
    const char* _dayOfWeekAbbr[8]  = { "   ", "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
    const char* _monthAbbr[13]  = { "   ", "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
};

#endif // _DS3231_RTC__
