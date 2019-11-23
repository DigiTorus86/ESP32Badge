#ifndef _BATTERYLEVEL_
#define _BATTERYLEVEL_

#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>

/*
 * A utility class for reading and displaying the battery level from an analog pin.
 */
class BatteryLevel 
{
    public:
        BatteryLevel(int pin, int check_interval_ms);

        /*
         * Returns true if it is time to check the battery level based on the configured check interval
         */
        bool  isTimeToCheck();

        /*
         * Reads the battery level from the voltage divider pin
         * Returns the estimated percent battery life remaining (0 - 100)
         */
        int   checkLevel();

        /*
         * Compares the current battery level against the warning and danger limits
         * and turns on the associated LED.  Optionally plays a tone on the speaker
         * if speaker_channel is non-negative.
         */
        bool  checkAlarm(int warning_led, int danger_led, int spkr_channel);

        /*
         * Draws the battery level icon to the screen at the specified coordinates.
         */
        void  drawLevel(int16_t x, int16_t y, Adafruit_ILI9341 *tft);

        /*
         * Returns the underlying raw analog read value.  Useful for calibration.
         */
        int   getRawLevel();     

    private:
        int   _pin;
        int   _check_interval_ms;
        unsigned long _last_checked;
        int   _raw_level;
        int   _percent_remaining;
};     

#endif // _BATTERYLEVEL_
