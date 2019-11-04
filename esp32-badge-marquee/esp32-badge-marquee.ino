/***************************************************
 * ESP32 Badge Marquee
 * - Displays attendee name and company with
 *   chasing light border effect and pulsing LEDs
 *   
 *   by Paul Pagel
 *   http://twobittinker.com 
 *   
 * Requires:
 * - ESP32 Conference Badge
 *
 ****************************************************/
 
#include "esp32_badge.h"
#include "BatteryLevel.h"
#include <Fonts/FreeSansBold24pt7b.h> 

const char* my_name  = "Paul Pagel";
const char* company  = "First Solar";

// Settings for the pulsing LED effect

#define LED_CNT    3
#define DUTY_MAX  32
#define DUTY_MIN -16
#define DUTY_STEP  4
uint8_t led_pin[] = { LED_1, LED_2, LED_3 };
uint8_t led_channel[] = { 0, 1, 2 };
int8_t  led_duty[] = { 16, 0, -16 }; 
int8_t  led_dir[] = { DUTY_STEP, DUTY_STEP, DUTY_STEP };

// Settings for the marquee border lights

#define LIGHT_CNT    58
#define LIGHT_RADIUS  6
#define LIGHT_ORIG_X  8
#define PHASE_CNT     3

int16_t  border[LIGHT_CNT][2] = {  // screen x,y position of border lights: [0] = x position, [1] = y position 
  { 8, 6 }, { 26, 6 }, { 44, 6 }, { 62, 6 }, { 80, 6 }, { 98, 6 }, { 116, 6 }, { 134, 6 }, { 152, 6 }, { 170, 6 }, { 188, 6 }, { 206, 6 }, { 224, 6 }, { 242, 6 }, { 260, 6 }, { 278, 6 }, { 296, 6 }, { 314, 6 },  // top row
  { 314, 26 }, { 314, 44 }, { 314, 62 }, { 314, 80 }, { 314, 98 }, { 314, 116 }, { 314, 134 }, { 314, 152 }, { 314, 170 }, { 314, 188 }, { 314, 206 }, { 314, 224 },  // right side
  { 296, 224 }, { 278, 224 }, { 260, 224 }, { 242, 224 }, { 224, 224 }, { 206, 224 }, { 188, 224 }, { 170, 224 }, { 152, 224 }, { 134, 224 }, { 116, 224 }, { 98, 224 }, { 80, 224 }, { 62, 224 }, { 44, 224 }, { 26, 224 }, { 8, 224 }, // bottom row
  { 8, 206 }, { 8, 188 }, { 8, 170 }, { 8, 152 }, { 8, 134 }, { 8, 116 }, { 8, 98 }, { 8, 80 }, { 8, 62 }, { 8, 44 }, { 8, 26 } // left side
};
uint8_t  light_phase;  // cycles from 0 to PHASE_CNT - 1
uint16_t light_color[PHASE_CNT * 2] { ILI9341_RED, ILI9341_YELLOW, ILI9341_GREEN, ILI9341_RED, ILI9341_YELLOW, ILI9341_GREEN }; // two cycles of border light changes

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK, TFT_RST, TFT_MISO);
BatteryLevel battery = BatteryLevel(BATT_LVL, 5000);

/*
 * Initial Setup - called once
 */
void setup() 
{
  Serial.begin(115200);
  Serial.println("ESP32 Badge Marquee"); 
  delay(100);

  for (int i = 0; i < LED_CNT; i++)
  {
    ledcSetup(led_channel[i], 5000, 8);  // 12 kHz max, 8 bit resolution
    ledcAttachPin(led_pin[i], led_channel[i]);
    pinMode(led_pin[i], OUTPUT);
  }

  delay(100);
  
  // Set up the TFT screen
  tft.begin();
  tft.setRotation(SCREEN_ROT);
    tft.fillScreen(ILI9341_BLACK);
  tft.setFont(&FreeSansBold24pt7b);
  tft.setTextSize(1);

  int16_t  x1, y1; 
  uint16_t wd, ht;
  tft.getTextBounds(my_name, 10, 50, &x1, &y1, &wd, &ht);
   
  tft.setCursor(160 - (wd / 2), 42 + ht);
  tft.setTextColor(ILI9341_ORANGE);
  tft.print(my_name);

  tft.getTextBounds(company, 10, 50, &x1, &y1, &wd, &ht);
  tft.setCursor(160 - (wd / 2), 160);
  tft.setTextColor(ILI9341_RED);
  tft.print(company);

  battery.checkLevel();
  battery.drawLevel(280, 204, &tft);
}

/*
 * Update the LED pulsing effect by changing the duty cycle
 */
void updateLEDs()
{
  for (int i = 0; i < LED_CNT; i++)
  {
    led_duty[i] += led_dir[i];

    if (led_duty[i] >= 0)
      ledcWrite(led_channel[i], led_duty[i]);
    
    if (led_duty[i] >= DUTY_MAX)
      led_dir[i] = -DUTY_STEP; 
    if (led_duty[i] <= DUTY_MIN)
      led_dir[i] = DUTY_STEP; 
  }
}

/*
 * Update the marquee border light chase effect
 */
void updateLights()
{
  light_phase = (light_phase + 1) % PHASE_CNT;

  for (int i = 0; i < LIGHT_CNT; i += PHASE_CNT)
  {
    for (int j = 0; j < PHASE_CNT; j++)
      tft.fillCircle(border[i + j][0], border[i + j][1] , LIGHT_RADIUS, light_color[light_phase + j]);
      
  }
}

/*
 * Main program loop - called repeatedly
 */
void loop(void) 
{
  
  updateLEDs();
  updateLights();

  if (battery.isTimeToCheck())
  {
    battery.checkLevel();
    battery.drawLevel(280, 204, &tft);
  }
  delay(10);
}
