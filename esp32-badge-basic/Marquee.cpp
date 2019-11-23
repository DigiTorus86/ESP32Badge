/*
 * Displays two lines of text (i.e. name and company)
 * with chasing light border effect and pulsing LEDs
 */
 
#include "Marquee.h"

Marquee::Marquee()
{
}

/*
 * Do the initial setup and screen draw
 */
void Marquee::begin(const char* textLine1, const char* textLine2, Adafruit_ILI9341 *tft)
{
  // Set up the PWM pins and channels for the LEDs 

  for (int i = 0; i < MQ_LED_CNT; i++)
  {
    ledcSetup(led_channel[i], 5000, 8);  // 12 kHz max, 8 bit resolution
    ledcAttachPin(led_pin[i], led_channel[i]);
    pinMode(led_pin[i], OUTPUT);
  }
  pwm_started = true;

  // Display the text lines
 
  int16_t  x1, y1; 
  uint16_t wd, ht;
  tft->getTextBounds(textLine1, 10, 50, &x1, &y1, &wd, &ht);

  tft->fillScreen(ILI9341_BLACK);
  tft->setCursor(SCREEN_WD / 2 - (wd / 2), 42 + ht);
  tft->setTextColor(MQ_COLOR_TEXTLINE1);
  tft->print(textLine1);

  tft->getTextBounds(textLine2, 10, 50, &x1, &y1, &wd, &ht);
  tft->setCursor(SCREEN_WD / 2 - (wd / 2), 160);
  tft->setTextColor(MQ_COLOR_TEXTLINE2);
  tft->print(textLine2);
}

/*
 * Update the badge lights and display
 */
void Marquee::update(Adafruit_ILI9341 *tft)
{
 updateLEDs();
 updateLights(tft);
}

/*
 * Update the LED pulsing effect by changing the duty cycle
 */
void Marquee::updateLEDs()
{
  for (int i = 0; i < MQ_LED_CNT; i++)
  {
    led_duty[i] += led_dir[i];

    if (led_duty[i] >= 0)
      ledcWrite(led_channel[i], led_duty[i]);
    
    if (led_duty[i] >= MQ_DUTY_MAX)
      led_dir[i] = -MQ_DUTY_STEP; 
    if (led_duty[i] <= MQ_DUTY_MIN)
      led_dir[i] = MQ_DUTY_STEP; 
  }
}

/*
 * Turn the LEDs off (set duty cycle to zero)
 */
void Marquee::resetLEDs()
{
  if (pwm_started)
  {
    for (int i = 0; i < MQ_LED_CNT; i++)
    {
      //ledc_stop(1, led_channel[i], 0);
      ledcWrite(led_channel[i], 0);
    }
  }
}

/*
 * Update the marquee border light chase effect
 */
void Marquee::updateLights(Adafruit_ILI9341 *tft)
{
  light_phase = (light_phase + 1) % MQ_PHASE_CNT;

  for (int i = 0; i < MQ_LIGHT_CNT; i += MQ_PHASE_CNT)
  {
    for (int j = 0; j < MQ_PHASE_CNT; j++)
      tft->fillCircle(border[i + j][0], border[i + j][1] , MQ_LIGHT_RADIUS, light_color[light_phase + j]);
  }
}
