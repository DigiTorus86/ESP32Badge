/***************************************************
ESP32 Badge Time and Weather Display
  By Paul Pagel
  http://twobittinker.com/

Requires:
 - DS3231 Real Time Clock breakout
 - BME280 Temperature, pressure, & humidity breakout
 - WiFi network and credentials

 Controls:
 - X/Y:         Cycle through display modes
 - A:           Enter / commit set time
 - Left/Right:  Move selection field
 - Up/Down:     Increase/decrease selection value
 ****************************************************/

#include <Wire.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <Fonts/FreeMonoBold12pt7b.h> 
#include <Fonts/FreeSansBold18pt7b.h> 
#include <WiFi.h>
#include <HTTPClient.h>
#include "time.h"
#include "esp32_badge.h"
#include "DS3231_RTC.h"
#include "SparkFunBME280.h"
#include <ArduinoJson.h>  // ArduinoJson by Benoit Blanchon

#define FORECAST_REFRESH_MS       3600000  // 1 hr
#define BME280_I2C_ADDR           0X76     // default address for BME280 is 0x77, mine was set to 0x76

// Display element color settings
#define COLOR_SCREEN_BGD          ILI9341_BLACK
#define COLOR_DATE                ILI9341_ORANGE
#define COLOR_TIME                ILI9341_BLACK
#define COLOR_TIME_BGD            ILI9341_ORANGE
#define COLOR_TIME_EDIT           ILI9341_BLUE
#define COLOR_SENSOR_LINES        ILI9341_DARKGREY
#define COLOR_SENSOR_TEMP         ILI9341_LIGHTGREY
#define COLOR_SENSOR_HUMID        ILI9341_CYAN
#define COLOR_SENSOR_PRESSURE     ILI9341_BLUE
#define COLOR_FORECAST_TEMP       ILI9341_GREEN
#define COLOR_FORECAST_TITLE      ILI9341_BLACK
#define COLOR_FORECAST_TITLE_BGD  ILI9341_GREEN
#define COLOR_FORECAST_BODY       ILI9341_DARKGREY

// Calculates the screen X position for the given time digit 
// digit index values:  0 = h1, 1=h2, 2=m1, 3=m2, 4=s1, 5=s2
#define TIME_DIGIT_X(digit)       (20 + digit * 38 + (digit > 1 ? 20 : 0) + (digit > 3 ? 20 : 0))

const char*    ssid     = ""; // add your network ID here 
const char*    password = ""; // add your network password here 
const uint16_t net_connect_timeout_sec = 10;
const char*    forecast_api_url = "https://api.weather.gov/gridpoints/CLE/19,62/forecast?units=us";  // x=19,y=62 for Perrysburg, OH

const char* ntp_server = "pool.ntp.org";
const long  gmt_offset_sec = -18000;      // EST GMT-5
const int   daylight_offset_sec = -14400; // EDT GMT-4 

bool network_available, ds3231_available, bme280_available;  

bool btnUp_released, btnDown_released, btnLeft_released, btnRight_released;

#define DISPLAY_MODE_CNT  4
enum display_mode_type 
{
  MODE_TIME_AND_SENSOR,
  MODE_DAY_FORECAST,
  MODE_TOMORROW_FORECAST,
  MODE_SET_TIME
};
enum display_mode_type display_mode, prev_display_mode;

struct ds3231_time_t curr_time;
DS3231_RTC rtc = DS3231_RTC();
BME280 sensor;

// use https://arduinojson.org/v5/assistant/ to find appropriate JSON memory size
StaticJsonDocument<15000> json_doc; 
unsigned long forecast_age_ms;
bool forecast_loaded = false;

uint8_t curr_time_digits[6];
uint8_t prev_time_digits[6];
uint8_t prev_day;

int16_t curr_temp_f, curr_temp_c, curr_humid, curr_pressure, forecast_temp_f;
int16_t prev_temp_f, prev_temp_c, prev_humid, prev_pressure, prev_forecast_temp_f;

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK, TFT_RST, TFT_MISO);

void setup() 
{
  Serial.begin(115200);
  Serial.println("ESP32 Badge RTC Test"); 
  delay(100);

  ledcSetup(spkr_channel, 12000, 8);  // 12 kHz max, 8 bit resolution
  ledcAttachPin(SPKR, spkr_channel);

  // Set up user input pins
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

  Wire.begin();  // fire up I2C

  // Initialize the BME280 sensor
  sensor.setI2CAddress(BME280_I2C_ADDR);    
  bme280_available = sensor.beginI2C();
  if (bme280_available == false)
  {
    Serial.println("The BME280 sensor did not respond. Check I2C wiring and address jumpers.");
    digitalWrite(LED_1, HIGH);  // redlight!
  }

  // Check the DS3231 RTC
  ds3231_available = rtc.readDS3231time(&curr_time);
  if (ds3231_available == false)
  {
    Serial.println("The DS3231 RTC did not respond. Check I2C wiring and address jumpers.");
    digitalWrite(LED_1, HIGH);  // redlight!
  }

  // Try to get the current local time from NTP server
  network_available = getNtpTime();
  if (network_available)
  {
    getForecast();
  }
  else
  {
    Serial.println("Unable to get Network Time. Check network SSID and password.");
    digitalWrite(LED_1, HIGH);  // redlight!
  }

  display_mode = MODE_TIME_AND_SENSOR;
  beginTimeAndSensor();
}

/*
 * Store the current time value in hhmmss digit array
 */
void currTimeToDigits()
{
  curr_time_digits[0] = curr_time.hour / 10;
  curr_time_digits[1] = curr_time.hour % 10;
  curr_time_digits[2] = curr_time.minute / 10;
  curr_time_digits[3] = curr_time.minute % 10;
  curr_time_digits[4] = curr_time.second / 10;
  curr_time_digits[5] = curr_time.second % 10;
}

/*
 * 
 */
void digitsToCurrTime()
{
  curr_time.hour = min(curr_time_digits[0] * 10 + curr_time_digits[1], 24); 
  curr_time.minute = min(curr_time_digits[2] * 10 + curr_time_digits[3], 59);
  curr_time.second = min(curr_time_digits[4] * 10 + curr_time_digits[5], 59); 
}

/*
 * Read temperature, humidity, and pressure sensor values from the BME280
 */
void readBME280()
{
  static uint16_t cntr = 0;

  if (cntr == 0)
  {
    curr_temp_f = (int16_t)sensor.readTempF();  
    curr_temp_c = (int16_t)sensor.readTempC();  
    curr_humid = (int16_t)sensor.readFloatHumidity();    
    float pressure = sensor.readFloatPressure() * 0.0002952998751f;  // kPa to inHg
    curr_pressure = (int16_t)(pressure); 
  }
  cntr = (cntr + 1) % 100;
}

/*
 * Connects the configured WiFi network
 */
bool connectToWiFi(uint16_t timeout_sec)
{
  uint16_t elapsed_sec = 0;
  
  Serial.printf("Connecting to %s ", ssid);
  digitalWrite(LED_1, LOW);  // reset error indicator
  WiFi.begin(ssid, password);
  
  while ((WiFi.status() != WL_CONNECTED) && (elapsed_sec < timeout_sec)) 
  {
    digitalWrite(LED_2, HIGH);
    delay(500);
    Serial.print(".");
    digitalWrite(LED_2, LOW);
    delay(500);
    elapsed_sec += 1;
  }
  Serial.println();
  
  //if (WiFi.status() != WL_CONNECTED)
  if (elapsed_sec >= timeout_sec)
  {
    digitalWrite(LED_1, HIGH); // red=problem 
    Serial.println("Network connection timeout.");
    network_available = false;
    return false;  // failure 
  }

  network_available = true;
  return true; // success
}

/*
 * Get the forecast data from the National Weather Service API
 */
bool getForecast()
{
  HTTPClient client;
  bool success = false;
  
  if (forecast_loaded == true && forecast_age_ms < FORECAST_REFRESH_MS)  
  {
    return true;  // forecast is still fresh - no need to load
  }
  
  if (!connectToWiFi(net_connect_timeout_sec))
  {
    return false;  // failed to connect to WiFi
  }

  client.begin(forecast_api_url);
  int16_t http_code = client.GET();

  if (http_code > 0) // success!
  { 
    String payload = client.getString();
    Serial.println(http_code);
    Serial.println(payload);

    DeserializationError error = deserializeJson(json_doc, payload);
    if (!error) 
    {
      digitalWrite(LED_3, HIGH);
      Serial.println("JSON parsed successfully.");
      forecast_loaded = true;
      forecast_age_ms = 0; // reset age counter

      // testing
      const char* updated = json_doc["properties"]["updated"];  // should be a date-time string like 2019-11-06T23:49:22+00:00
      //float elevation = json_doc["properties"]["elevation"]["value"]; // 192.9384
      forecast_temp_f = json_doc["properties"]["periods"][0]["temperature"];  // TODO: put logic to check temp units first  
      
      Serial.print("Forecast updated: ");
      Serial.println(updated);
      success = true;
    }
    else
    {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.c_str());
    }
  }
  else
  {
    Serial.print("HTTP error: ");
    Serial.println(http_code);
    digitalWrite(LED_1, HIGH);  // red=problem
  }
  client.end();
 
  digitalWrite(LED_3, LOW);
  WiFi.disconnect();
  return success;
}

/*
 * Retrieve the local time from the ESP32 internal RTC
 */
bool readControllerTime(struct ds3231_time_t *read_time)
{
  struct tm timeinfo;
  
  if(getLocalTime(&timeinfo))
  {
    read_time->second = timeinfo.tm_sec;
    read_time->minute = timeinfo.tm_min;
    read_time->hour = timeinfo.tm_hour;
    read_time->dayOfWeek = timeinfo.tm_wday + 1;
    read_time->dayOfMonth = timeinfo.tm_mday;
    read_time->month = timeinfo.tm_mon;
    read_time->year = timeinfo.tm_year % 100;
    return true;
  }
  return false;
}

/*
 * Set the ESP32's internal RTC based on the passed ds3231 time
 */
void setControllerTime(ds3231_time_t set_time)
{
    struct tm t;
    time_t time_now;

    t.tm_year = set_time.year + 100;  // Year - 1900
    t.tm_mon = set_time.month;        // Month, where 0 = jan
    t.tm_mday = set_time.dayOfMonth;  // Day of the month
    t.tm_hour = set_time.hour;
    t.tm_min = set_time.minute;
    t.tm_sec = set_time.second;
    t.tm_isdst = 0;                   // Is DST on? 1 = yes, 0 = no, -1 = unknown
    
    time_now = mktime(&t);

    timeval epoch = {time_now, 0};
    settimeofday((const timeval*)&epoch, 0);

    Serial.println(time_now);

    // testing
    struct ds3231_time_t test_time;
    readControllerTime(&test_time);

    Serial.print(rtc.getDayAbbr(test_time.dayOfWeek));
    Serial.print(", ");
    if (test_time.dayOfMonth < 10) Serial.print("0");
    Serial.print(test_time.dayOfMonth);
    Serial.print(" ");
    Serial.print(rtc.getMonthAbbr(test_time.month));
    Serial.print(" ");
    Serial.print("20");
    Serial.print(test_time.year); 
    Serial.print(" ");
    Serial.print(test_time.hour); 
    Serial.print(":");
    Serial.print(test_time.minute);
    Serial.print(":");
    Serial.println(test_time.second);
}

/*
 * Get the current time from the configured NTP server
 * and use it to set the ESP32 internal and DS3231 RTC time values
 */
bool getNtpTime()
{
  if (!connectToWiFi(net_connect_timeout_sec))
    return false;
  
  configTime(gmt_offset_sec, daylight_offset_sec, ntp_server); // from ESP32 Time library

  if (readControllerTime(&curr_time))
  {
    digitalWrite(LED_3, HIGH); // flash green=success
    rtc.setDS3231time(curr_time);
    digitalWrite(LED_3, LOW);
  }
  else
  {
    digitalWrite(LED_1, HIGH); // red=problem
  }
  WiFi.disconnect();
  return true;
}

/*
 * Initial set up for the Time and Sensor display mode
 */
void beginTimeAndSensor()
{
  tft.fillScreen(COLOR_SCREEN_BGD);
  tft.fillRect(0, 30, 320, 74, COLOR_TIME_BGD);  // time box

  // Initial time display
  tft.setFont(&FreeSansBold18pt7b);
  tft.setTextSize(2);
  tft.setTextColor(COLOR_TIME, COLOR_TIME_BGD);
  tft.setCursor(98, 92);
  tft.print(":");
  tft.setCursor(194, 92);
  tft.print(":");

  // force redraw of all date, time, and sensor values
  for (int i = 0; i < 6; i++)
  {
    prev_time_digits[i] = 11;
  }
  prev_day = 0;
  prev_temp_f = 0;
  prev_temp_c = 0;
  prev_humid = 0;
  prev_pressure = 0;

  ds3231_available = rtc.readDS3231time(&curr_time);

  if (bme280_available)
  {  
    tft.drawLine(0, 180, 320, 180, COLOR_SENSOR_LINES);   // humidity / pressure
    tft.drawLine(160, 180, 160, 240, COLOR_SENSOR_LINES); // seperator lines
  }
  else if (forecast_loaded)
  {
    const char* short_forecast = json_doc["properties"]["periods"][0]["shortForecast"]; 

    curr_temp_f = json_doc["properties"]["periods"][0]["temperature"];
    curr_temp_c = (int16_t)((float)curr_temp_f - 32) / 1.8;
    
    tft.setFont(&FreeMonoBold12pt7b);
    tft.setTextColor(COLOR_FORECAST_BODY);
    tft.setTextSize(1);
    tft.setCursor(0, 200);
    drawWordWrap(short_forecast, 2, 22);  
  } 
}

/*
 * Update the screen for the Time and Sensor display mode
 */
void updateTimeAndSensor()
{
  int16_t x;

  // Get current time and temp values
  if (ds3231_available)
  {
    ds3231_available = rtc.readDS3231time(&curr_time);
  }
  else
  {
    readControllerTime(&curr_time);
  }
  currTimeToDigits();
  
  // Time
  tft.setFont(&FreeSansBold18pt7b);
  tft.setTextSize(2);
  tft.setTextColor(COLOR_TIME, COLOR_TIME_BGD);
  tft.setCursor(20, 92);

  for (uint8_t i = 0; i < 6; i++)
  {
    x = TIME_DIGIT_X(i);
    if (prev_time_digits[i] != curr_time_digits[i])
    {
      tft.fillRect(x, 44, 38, 50, ILI9341_ORANGE);
      tft.setCursor(x, 92);
      tft.print(curr_time_digits[i]);
    }
    prev_time_digits[i] = curr_time_digits[i];
  }

  tft.setFont(&FreeMonoBold12pt7b);
 
  // Date
  if (prev_day != curr_time.dayOfMonth)
  {
    tft.fillRect(0, 0, 320, 29, COLOR_SCREEN_BGD); // clear date
    tft.setTextSize(1);
    tft.setTextColor(COLOR_DATE);
    tft.setCursor(50, 18);
    tft.print(rtc.getDayAbbr(curr_time.dayOfWeek));
    tft.print(", ");
    if (curr_time.dayOfMonth < 10) tft.print("0");
    tft.print(curr_time.dayOfMonth);
    tft.print(" ");
    tft.print(rtc.getMonthAbbr(curr_time.month));
    tft.print(" ");
    tft.print("20");
    tft.print(curr_time.year); 

    prev_day = curr_time.dayOfMonth;
  }

  // Environmental 
  if (bme280_available)
  {
    tft.setTextColor(COLOR_SENSOR_TEMP);
    readBME280(); 
  }
  else
  {
    // just using forecast temp
    tft.setTextColor(COLOR_FORECAST_TEMP);
  }
   
  // Temperature
  if (prev_temp_f != curr_temp_f)
  {
    tft.fillRect(0, 128, 106, 38, COLOR_SCREEN_BGD);   // clear temperature F
    tft.fillRect(170, 128, 106, 38, COLOR_SCREEN_BGD); // clear temperature C

    x = 10 + (curr_temp_f < 100 ? 30 : 0) + (curr_temp_f < 10 ? 30 : 0) - (curr_temp_f < 0 ? 30 : 0) - (curr_temp_f <= -10 ? 30 : 0); 
    
    tft.setTextSize(2);
    tft.setCursor(x, 162);
    tft.print(curr_temp_f);
  
    tft.drawCircle(112, 138, 4, COLOR_SENSOR_TEMP);
    
    tft.setTextSize(1);
    tft.setCursor(124, 162);
    tft.print("F");
    
    prev_temp_f = curr_temp_f;

    if (forecast_loaded && prev_forecast_temp_f != forecast_temp_f)
    {
      if (bme280_available)
      {
        x = 180 + (forecast_temp_f < 100 ? 30 : 0) + (forecast_temp_f < 10 ? 30 : 0) - (forecast_temp_f < 0 ? 30 : 0) - (forecast_temp_f <= -10 ? 30 : 0);
        tft.setTextColor(COLOR_SENSOR_TEMP);
        tft.setTextSize(2);
        tft.setCursor(x, 162);
        tft.print(forecast_temp_f);
      
        tft.drawCircle(284, 138, 4, COLOR_SENSOR_TEMP);
      
        tft.setTextSize(1);
        tft.setCursor(296, 162);
        tft.print("F");
        
        tft.setTextColor(COLOR_SENSOR_TEMP);
        tft.setCursor(124, 126);
        tft.print("in");
      }
      else 
      {
        x = 180 + (curr_temp_c < 100 ? 30 : 0) + (curr_temp_c < 10 ? 30 : 0) - (curr_temp_c < 0 ? 30 : 0) - (curr_temp_c <= -10 ? 30 : 0);
        tft.setTextColor(COLOR_FORECAST_TEMP);
        tft.setTextSize(2);
        tft.setCursor(x, 162);
        tft.print(curr_temp_c);

        tft.drawCircle(284, 138, 4, COLOR_FORECAST_TEMP);
      
        tft.setTextSize(1);
        tft.setCursor(296, 162);
        tft.print("C");

        tft.setCursor(148, 126);
        tft.print("out");      
      }
        
    }
    else if (!forecast_loaded)
    {
      x = 180 + (curr_temp_c < 100 ? 30 : 0) + (curr_temp_c < 10 ? 30 : 0) - (curr_temp_c < 0 ? 30 : 0) - (curr_temp_c <= -10 ? 30 : 0);
      tft.setTextSize(2);
      tft.setCursor(x, 162);
      tft.print(curr_temp_c);
    
      tft.drawCircle(284, 134, 4, COLOR_SENSOR_TEMP);
    
      tft.setTextSize(1);
      tft.setCursor(296, 158);
      tft.print("C");
      
      tft.setCursor(154, 126);
      tft.print("in");
    }
  } 

  if (!bme280_available)
  {
    // no humidity or pressure info available
    // short forecast is shown in this area instead
    return;
  }

  // Humidity
  if (prev_humid != curr_humid)
  {
    tft.fillRect(0, 190, 106, 50, COLOR_SCREEN_BGD);
    x = 10 + (curr_humid < 100 ? 30 : 0) + (curr_humid < 10 ? 30 : 0); 
    tft.setTextColor(COLOR_SENSOR_HUMID);
    tft.setTextSize(2);
    tft.setCursor(x, 230);
    tft.print(curr_humid);
  
    tft.setTextSize(1);
    tft.setCursor(112, 210);
    tft.print("%");
    
    tft.setCursor(112, 232);
    tft.print("Hum");

    prev_humid = curr_humid;
  }
  
  // Barometric pressure
  if (prev_pressure != curr_pressure)
  {
    tft.fillRect(170, 190, 106, 50, COLOR_SCREEN_BGD);

    x = 180 + (curr_pressure < 100 ? 30 : 0) + (curr_pressure < 10 ? 30 : 0); 
    tft.setTextColor(COLOR_SENSOR_PRESSURE);
    tft.setTextSize(2);
    tft.setCursor(x, 230);
    tft.print(curr_pressure);
  
    tft.setTextSize(1);
    tft.setCursor(284, 210);
    tft.print("in");
    tft.setCursor(284, 232);
    tft.print("Hg");

    prev_pressure = curr_pressure;
  }
}

/*
 * Initial set up for the Current Day Forecast display mode
 */
void beginDayForecast() 
{
  tft.fillScreen(COLOR_SCREEN_BGD);

  tft.fillRect(0, 0, 320, 24, COLOR_FORECAST_TITLE_BGD);
  tft.setFont(&FreeMonoBold12pt7b);
  tft.setTextColor(COLOR_FORECAST_TITLE, COLOR_FORECAST_TITLE_BGD);
  tft.setTextSize(1);
  tft.setCursor(0, 40);

  if (!network_available) 
  {
    tft.print("Network not available");
    return;
  }

  if (forecast_loaded == false || forecast_age_ms >= FORECAST_REFRESH_MS)
  {
    
    tft.print("Getting Forecast...");
    
    bool success = getForecast();
    tft.fillRect(0, 0, 320, 60, COLOR_SCREEN_BGD);  // erase msg
  
    if (!success)
    {
      tft.setCursor(0, 40);
      tft.println("Unable to get forecast.");
      tft.println("Check internet connection.");
      return;
    }
  }
  
  const char* period_name = json_doc["properties"]["periods"][0]["name"]; 
  const char* detailed_forecast = json_doc["properties"]["periods"][0]["detailedForecast"]; 

  tft.setCursor(0, 20);
  drawTextUpper(period_name);

  tft.setTextColor(COLOR_FORECAST_BODY);
  tft.setCursor(0, 50);

  drawWordWrap(detailed_forecast, 8, 22);
}


void drawWordWrap(const char* text, uint8_t rows, uint8_t cols)
{
   uint8_t  row = 0;
   uint8_t  col_start = 0;
   uint16_t curr_break, next_break;
  
   uint16_t pos = 0;   
   const char space = ' ';
   char *pch;

   if (text == NULL)
   {
    return;
   }

   if (strlen(text) < cols)
   {
    tft.print(text);
    return;
   }
   
   Serial.println("drawWordWrap");
   Serial.println(text);

   next_break = 0; 
   pch = strchr(text, space);
   if (pch != NULL)
   {
     curr_break = pch - text;
   }
   else
   {
    Serial.println("No space detected");
    tft.print(text);
    return;
   }

   Serial.print("First space: ");
   Serial.println(curr_break);
   Serial.print("Text length: ");
   Serial.println(strlen(text));

    while (pos < strlen(text) - 1 && row < rows)
    {
     Serial.print("Pos: ");
     Serial.println(pos);
    
     if (next_break < strlen(text) - 1)
     {
       pch = strchr(pch + 1, space);
       if (pch != NULL)
       {
         next_break = pch - text;
       }
       else
       {
        next_break = strlen(text) - 1;
       }
     }
     
    Serial.print("Curr break: ");
    Serial.println(curr_break);
    Serial.print("Next break: ");
    Serial.println(next_break);
    Serial.print("Col start: ");
    Serial.println(col_start);
     
     if (next_break - col_start >= cols || curr_break >= strlen(text) - 1)
     {
      // too many characters until next break, use this one
      while (pos <= curr_break && pos < strlen(text) - 1)
      {
        Serial.print(text[pos]);
        tft.print(text[pos]);
        pos += 1;
      }
      Serial.println();
      tft.println();

      while (text[pos] == ' ' && pos < strlen(text) - 1)
      {
         pos += 1;  // skip the next space(s)
      }
      
      col_start = pos + 1;
      row += 1;
     }

     curr_break = next_break;
   }
}

/*
 * Draws the text in uppercase to the display
 */
void drawTextUpper(const char* text)
{
  for (uint16_t i = 0; i < strlen(text); i++)
  {
    tft.print((char)toupper(text[i]));
  }
}

/*
 * Update the screen for the Current Day Forecast display mode
 */
void updateDayForecast() 
{
   
}

/*
 * Initial setup for the Tomorrow Forecast display mode
 */
void beginTomorrowForecast() 
{
  tft.fillScreen(COLOR_SCREEN_BGD);
  
  tft.fillRect(0, 0, 320, 24, COLOR_FORECAST_TITLE_BGD);
  tft.setFont(&FreeMonoBold12pt7b);
  tft.setTextColor(COLOR_FORECAST_TITLE, COLOR_FORECAST_TITLE_BGD);
  tft.setTextSize(1);

  const char* period_name = json_doc["properties"]["periods"][1]["name"]; 
  const char* detailed_forecast = json_doc["properties"]["periods"][1]["detailedForecast"]; 

  tft.setCursor(0, 20);
  drawTextUpper(period_name);

  tft.setTextColor(COLOR_FORECAST_BODY);
  tft.setCursor(0, 50);
  drawWordWrap(detailed_forecast, 8, 22);
}

/*
 * Update the screen for the Tomorrow Forecast display mode
 */
void updateTomorrowForecast() 
{
  
}

/*
 * Initial setup for the manual Set Time display mode 
 */
void beginSetTime() 
{
  tft.fillScreen(COLOR_SCREEN_BGD);

  tft.setFont(&FreeMonoBold12pt7b);
  tft.setTextColor(COLOR_DATE);
  tft.setTextSize(1);
  tft.setCursor(0, 20);
  tft.print("SET TIME");

  tft.setTextColor(COLOR_FORECAST_BODY);
  tft.setCursor(0, 160);
  tft.println("Use dir btns to change");
  tft.println("Press A to confirm");
  tft.println("X or Y to cancel");
  

  tft.fillRect(0, 30, 320, 80, COLOR_TIME_BGD);       // time box

  tft.setFont(&FreeSansBold18pt7b);
  tft.setTextSize(2);
  tft.setTextColor(COLOR_TIME, COLOR_TIME_BGD);

  tft.setCursor(98, 92);
  tft.print(":");
  tft.setCursor(194, 92);
  tft.print(":");

  uint16_t x;
  for (uint8_t i = 0; i < 6; i++)
  {
    x = TIME_DIGIT_X(i);
    tft.fillRect(x, 44, 38, 50, COLOR_TIME_BGD);
    tft.setCursor(x, 92);
    tft.print(curr_time_digits[i]);
  }
}

/*
 * Update the screen for the manual Set Time display mode
 */
void updateSetTime() 
{
  static uint16_t edit_field, prev_edit_field;
  static uint16_t edit_x, edit_y;
  static bool     redraw;

  // TODO:  add date change functionality(?)

  prev_edit_field = edit_field;

  if (btnLeft_released)
  {
    edit_field = (edit_field > 0 ? edit_field - 1 : 0);
    redraw = true;
  }

  if (btnRight_released)
  {
    edit_field = (edit_field < 5 ? edit_field + 1 : 5);
    redraw = true;
  }

  if (btnUp_released)
  {
    curr_time_digits[edit_field] = curr_time_digits[edit_field] + 1;
    redraw = true;
  }

  if (btnDown_released)
  {
    curr_time_digits[edit_field] = curr_time_digits[edit_field] - 1;
    redraw = true;
  }

  if (redraw == true)
  {
    // Erase previous edit field highlight
    tft.fillRect(TIME_DIGIT_X(prev_edit_field), 44, 38, 50, COLOR_TIME_BGD);
    tft.setTextColor(COLOR_TIME, COLOR_TIME_EDIT);
    tft.setCursor(TIME_DIGIT_X(prev_edit_field), 92);
    tft.print(curr_time_digits[prev_edit_field]);

    // Draw new edit field highlight
    tft.setTextColor(COLOR_TIME_EDIT, COLOR_TIME);  // TODO!
    tft.setCursor(TIME_DIGIT_X(edit_field), 92);
    tft.print(curr_time_digits[edit_field]);    
  }

  if (btnA_pressed)
  {
    // Commit the changes go back to time mode
    digitsToCurrTime();
    setControllerTime(curr_time);
    rtc.setDS3231time(curr_time);

    display_mode = MODE_TIME_AND_SENSOR;
  }
  
}

/*
 * Main screen update logic.  Call on each loop.
 */
void updateScreen()
{
  if (prev_display_mode != display_mode)
  {
    // do the initial drawing of the screen for the new display mode
    switch (display_mode)
    {
      case MODE_TIME_AND_SENSOR: 
        beginTimeAndSensor();
        break;
        
      case MODE_DAY_FORECAST: 
        beginDayForecast();
        break;

      case MODE_TOMORROW_FORECAST: 
        beginTomorrowForecast();
        break;

      case MODE_SET_TIME: 
        beginSetTime();
        break;
        
    }
  }

  prev_display_mode = display_mode;
  
  // do the update for the current display mode
  switch (display_mode)
  {
    case MODE_TIME_AND_SENSOR: 
      updateTimeAndSensor();
      break;
      
    case MODE_DAY_FORECAST: 
      updateDayForecast();
      break;

    case MODE_TOMORROW_FORECAST: 
      updateTomorrowForecast();
      break;

    case MODE_SET_TIME: 
      updateSetTime();
      break;
  }
}

/*
 * Main program loop
 */
void loop(void)
{
  unsigned long start_time = millis();

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
    btnB_pressed = false;
  }
  else
  {
    btnB_pressed = false;
  }
  
  updateScreen();
  delay(50);

  forecast_age_ms = forecast_age_ms +  millis() - start_time;
}
