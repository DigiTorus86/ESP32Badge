/*
 * Class for managing the DS3231 Real Time Clock via I2C
 */


#include "DS3231_RTC.h"

DS3231_RTC::DS3231_RTC()
{}

/* 
 *  Convert normal decimal numbers to binary coded decimal
 */
uint8_t DS3231_RTC::decToBcd(uint8_t val)
{
  return( (val / 10 * 16) + (val % 10) );
}

/*
 * Convert binary coded decimal to normal decimal numbers
 */
uint8_t DS3231_RTC::bcdToDec(uint8_t val)
{
  return( (val / 16 * 10) + (val % 16) );
}

/*
 * Set the DS3231 RTC time 
 */
bool DS3231_RTC::setDS3231time(struct ds3231_time_t set_time)
{
  // sets time and date data to DS3231
  Wire.beginTransmission(DS3231_I2C_ADDRESS);
  Wire.write(0); // set next input to start at the seconds register
  Wire.write(decToBcd(set_time.second)); // set seconds
  Wire.write(decToBcd(set_time.minute)); // set minutes
  Wire.write(decToBcd(set_time.hour)); // set hours
  Wire.write(decToBcd(set_time.dayOfWeek)); // set day of week (1=Sunday, 7=Saturday)
  Wire.write(decToBcd(set_time.dayOfMonth)); // set date (1 to 31)
  Wire.write(decToBcd(set_time.month)); // set month
  Wire.write(decToBcd(set_time.year)); // set year (0 to 99)
  byte success = Wire.endTransmission();
  return (success == 0);
}

/*
 * Read the DS3231 RTC time
 */
bool DS3231_RTC::readDS3231time(struct ds3231_time_t *read_time)
{
  Wire.beginTransmission(DS3231_I2C_ADDRESS);
  Wire.write(0); // set DS3231 register pointer to 00h
  Wire.endTransmission();
  
  // request seven bytes of data from DS3231 starting from register 00h
  byte num_bytes = Wire.requestFrom(DS3231_I2C_ADDRESS, 7);

  if (num_bytes == 0)
  {
    return false;  // device failure
  }
  
  read_time->second = bcdToDec(Wire.read() & 0x7f);
  read_time->minute = bcdToDec(Wire.read());
  read_time->hour = bcdToDec(Wire.read() & 0x3f);
  read_time->dayOfWeek = bcdToDec(Wire.read());
  read_time->dayOfMonth = bcdToDec(Wire.read());
  read_time->month = bcdToDec(Wire.read());
  read_time->year = bcdToDec(Wire.read());
  return true;
}

/*
 * Returns the 3-letter abbreviation for the day of the week
 */
const char* DS3231_RTC::getDayAbbr(uint8_t dayOfWeek)
{
  if (dayOfWeek >= 1 && dayOfWeek <= 7)
    return _dayOfWeekAbbr[dayOfWeek];
  else
    return _dayOfWeekAbbr[0];
}

/*
 * Returns the 3-letter abbreviation for the month
 */
const char* DS3231_RTC::getMonthAbbr(uint8_t month)
{
  if (month >= 1 && month <= 12)
    return _monthAbbr[month];
  else
    return _monthAbbr[0];
}
