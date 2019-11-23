#ifndef _MARQUEE_
#define _MARQUEE_

#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include "esp32_badge.h"

/*
 * Displays two lines of text (i.e. name and company)
 * with chasing light border effect and pulsing LEDs
 */

// Settings for the pulsing LED effect

#define MQ_LED_CNT    3
#define MQ_DUTY_MAX  32
#define MQ_DUTY_MIN -16
#define MQ_DUTY_STEP  4

// Settings for the animated marquee sign and border lights

#define MQ_COLOR_TEXTLINE1 ILI9341_ORANGE
#define MQ_COLOR_TEXTLINE2 ILI9341_RED

#define MQ_LIGHT_CNT       58
#define MQ_LIGHT_RADIUS     6
#define MQ_LIGHT_ORIG_X     8
#define MQ_PHASE_CNT        3
 
class Marquee 
{
	public:
 
    Marquee();
		
		void begin(const char* textLine1, const char* textLine2, Adafruit_ILI9341 *tft);
        
		void update(Adafruit_ILI9341 *tft);
    void updateLEDs();
    void updateLights(Adafruit_ILI9341 *tft);
    void resetLEDs();

  private:       
  
    uint8_t led_pin[MQ_LED_CNT] = { LED_1, LED_2, LED_3 };
    uint8_t led_channel[MQ_LED_CNT] = { 0, 1, 2 };
    int8_t  led_duty[MQ_LED_CNT] = { 16, 0, -16 }; 
    int8_t  led_dir[MQ_LED_CNT] = { MQ_DUTY_STEP, MQ_DUTY_STEP, MQ_DUTY_STEP };
    bool    pwm_started = false;

    int16_t  border[MQ_LIGHT_CNT][2] = {  // screen x,y position of border lights: [0] = x position, [1] = y position 
      { 8, 6 }, { 26, 6 }, { 44, 6 }, { 62, 6 }, { 80, 6 }, { 98, 6 }, { 116, 6 }, { 134, 6 }, { 152, 6 }, { 170, 6 }, { 188, 6 }, { 206, 6 }, { 224, 6 }, { 242, 6 }, { 260, 6 }, { 278, 6 }, { 296, 6 }, { 314, 6 },  // top row
      { 314, 26 }, { 314, 44 }, { 314, 62 }, { 314, 80 }, { 314, 98 }, { 314, 116 }, { 314, 134 }, { 314, 152 }, { 314, 170 }, { 314, 188 }, { 314, 206 }, { 314, 224 },  // right side
      { 296, 224 }, { 278, 224 }, { 260, 224 }, { 242, 224 }, { 224, 224 }, { 206, 224 }, { 188, 224 }, { 170, 224 }, { 152, 224 }, { 134, 224 }, { 116, 224 }, { 98, 224 }, { 80, 224 }, { 62, 224 }, { 44, 224 }, { 26, 224 }, { 8, 224 }, // bottom row
      { 8, 206 }, { 8, 188 }, { 8, 170 }, { 8, 152 }, { 8, 134 }, { 8, 116 }, { 8, 98 }, { 8, 80 }, { 8, 62 }, { 8, 44 }, { 8, 26 } // left side
    };
    uint8_t  light_phase;  // cycles from 0 to MQ_PHASE_CNT - 1
    uint16_t light_color[MQ_PHASE_CNT * 2] { ILI9341_RED, ILI9341_YELLOW, ILI9341_GREEN, ILI9341_RED, ILI9341_YELLOW, ILI9341_GREEN }; // two cycles of border light changes

};

#endif // _MARQUEE_
