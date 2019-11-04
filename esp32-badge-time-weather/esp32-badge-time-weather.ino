/***************************************************
ESP32 Badge test app
  By Paul Pagel
  http://twobittinker.com/

Requires:
 - DS3231 Real Time Clock breakout
 - BME280 Temperature, pressure, & humidity breakout

 Controls:
 - Y: connect to NTP server to get local time 
 ****************************************************/

#include <Wire.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <Fonts/FreeMonoBold12pt7b.h> 
#include <Fonts/FreeSansBold18pt7b.h> 
#include <WiFi.h>
#include "time.h"
#include "esp32_badge.h"
#include "DS3231_RTC.h"

const char* ssid     = "";
const char* password = ""; 

const char* ntp_server = "pool.ntp.org";
const long  gmt_offset_sec = -18000;      // EST GMT-5
const int   daylight_offset_sec = -14400; // EDT GMT-4 

struct ds3231_time_t curr_time;
DS3231_RTC rtc = DS3231_RTC();

uint8_t curr_time_digits[6];
uint8_t prev_time_digits[6];
uint8_t prev_day;

int16_t curr_temp_f, curr_temp_c, curr_humid, curr_pressure;
int16_t prev_temp_f, prev_temp_c, prev_humid, prev_pressure;

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK, TFT_RST, TFT_MISO);

void setup() 
{
  Serial.begin(115200);
  Serial.println("ESP32 Badge RTC Test"); 
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

  tft.fillRect(0, 30, 320, 80, ILI9341_ORANGE);       // time box
  
  tft.drawLine(0, 180, 320, 180, ILI9341_DARKGREY);   // humidity / pressure
  tft.drawLine(160, 180, 160, 240, ILI9341_DARKGREY); // seperator lines

  // Initial time display
  tft.setFont(&FreeSansBold18pt7b);
  tft.setTextSize(2);
  tft.setTextColor(ILI9341_BLACK, ILI9341_ORANGE);
  tft.setCursor(98, 92);
  tft.print(":");
  tft.setCursor(194, 92);
  tft.print(":");

  int16_t x;
  for (uint8_t i = 0; i < 6; i++)
  {
    x = getTimeDigitX(i);
    tft.setCursor(x, 92);
    tft.print(curr_time_digits[i]);
  }

  Wire.begin();  // fire up I2C
}

int16_t getTimeDigitX(uint8_t digit_idx)
{
  return 20 + digit_idx * 38 + (digit_idx > 1 ? 20 : 0) + (digit_idx > 3 ? 20 : 0);
}

void currTimeToDigits()
{
  curr_time_digits[0] = curr_time.hour / 10;
  curr_time_digits[1] = curr_time.hour % 10;
  curr_time_digits[2] = curr_time.minute / 10;
  curr_time_digits[3] = curr_time.minute % 10;
  curr_time_digits[4] = curr_time.second / 10;
  curr_time_digits[5] = curr_time.second % 10;
}


void readBME280()
{
  static uint16_t cntr = 0;

  if (cntr == 0)
  {
    curr_temp_f = random(1, 120);
    curr_temp_c = (curr_temp_f - 32) / 1.8; 
    curr_humid = random(00, 100);
    curr_pressure = random(90, 120);
  }
  cntr = (cntr + 1) % 100;
}


void readNtpTime()
{
  //connect to WiFi
  Serial.printf("Connecting to %s ", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) 
  {
    digitalWrite(LED_2, HIGH);
    delay(250);
    Serial.print(".");
    digitalWrite(LED_2, LOW);
    delay(250);
  }
  Serial.println(" CONNECTED");
  
  configTime(gmt_offset_sec, daylight_offset_sec, ntp_server);

  struct tm timeinfo;
  if(getLocalTime(&timeinfo))
  {
    digitalWrite(LED_3, HIGH); 
    curr_time.second = timeinfo.tm_sec;
    curr_time.minute = timeinfo.tm_min;
    curr_time.hour = timeinfo.tm_hour;
    curr_time.dayOfWeek = timeinfo.tm_wday + 1;
    curr_time.dayOfMonth = timeinfo.tm_mday;
    curr_time.month = timeinfo.tm_mon;
    curr_time.year = timeinfo.tm_year % 100;

    rtc.setDS3231time(curr_time);
    digitalWrite(LED_3, LOW);
  }
  else
  {
    digitalWrite(LED_1, HIGH);
  }
  WiFi.disconnect();
}

void updateScreen()
{
  int16_t x;
  
  // Time
  tft.setFont(&FreeSansBold18pt7b);
  tft.setTextSize(2);
  tft.setTextColor(ILI9341_BLACK, ILI9341_ORANGE);
  tft.setCursor(20, 92);

  for (uint8_t i = 0; i < 6; i++)
  {
    x = getTimeDigitX(i);
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
    tft.fillRect(0, 0, 320, 29, ILI9341_BLACK); // clear date
    tft.setTextSize(1);
    tft.setTextColor(ILI9341_ORANGE);
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

  // Temperature
  if (prev_temp_f != curr_temp_f)
  {
    tft.fillRect(0, 124, 106, 38, ILI9341_BLACK);   // clear temperature F
    tft.fillRect(170, 124, 106, 38, ILI9341_BLACK); // clear temperature C

    x = 10 + (curr_temp_f < 100 ? 30 : 0) + (curr_temp_f < 10 ? 30 : 0) - (curr_temp_f < 0 ? 30 : 0) - (curr_temp_f <= -10 ? 30 : 0); 
    tft.setTextColor(ILI9341_GREEN);
    tft.setTextSize(2);
    tft.setCursor(x, 158);
    tft.print(curr_temp_f);
  
    tft.drawCircle(112, 134, 4, ILI9341_GREEN);
    
    tft.setTextSize(1);
    tft.setCursor(124, 158);
    tft.print("F");

    x = 180 + (curr_temp_c < 100 ? 30 : 0) + (curr_temp_c < 10 ? 30 : 0) - (curr_temp_c < 0 ? 30 : 0) - (curr_temp_c <= -10 ? 30 : 0);; 
    tft.setTextSize(2);
    tft.setCursor(x, 158);
    tft.print(curr_temp_c);
  
    tft.drawCircle(284, 134, 4, ILI9341_GREEN);
  
    tft.setTextSize(1);
    tft.setCursor(296, 158);
    tft.print("C");

    prev_temp_f = curr_temp_f;
  } 
  
  // Humidity
  if (prev_humid != curr_humid)
  {
    tft.fillRect(0, 190, 106, 50, ILI9341_BLACK);
    x = 10 + (curr_humid < 100 ? 30 : 0) + (curr_humid < 10 ? 30 : 0); 
    tft.setTextColor(ILI9341_CYAN);
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
    tft.fillRect(170, 190, 106, 50, ILI9341_BLACK);

    x = 180 + (curr_pressure < 100 ? 30 : 0) + (curr_pressure < 10 ? 30 : 0); 
    tft.setTextColor(ILI9341_BLUE);
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


void loop(void) 
{
  if(digitalRead(BTN_UP) == LOW)
  {
    btnUp_pressed = true;
  }
  else
  {
    btnUp_pressed = false;
  }

  if(digitalRead(BTN_DOWN) == LOW)
  {
    btnDown_pressed = true;
  }
  else
  {
    btnDown_pressed = false;
  }

  if(digitalRead(BTN_LEFT) == LOW)
  {
    btnLeft_pressed = true;
  }
  else
  {
    btnLeft_pressed = false;
  }

  if(digitalRead(BTN_RIGHT) == LOW)
  {
    btnRight_pressed = true;
  }
  else
  {
    btnRight_pressed = false;
  }
  
  
  if(digitalRead(BTN_X) == LOW)
  {
    btnX_pressed = true;
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
    readNtpTime();
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
  else
  {
    btnA_pressed = false;
  }
  
  if(digitalRead(BTN_B) == LOW)
  {
    btnB_pressed = true;
    
  }
  else
  {
    btnB_pressed = false;
  }
  
  readBME280();
  rtc.readDS3231time(&curr_time);
  currTimeToDigits();
  updateScreen();
  
  delay(100);
}
