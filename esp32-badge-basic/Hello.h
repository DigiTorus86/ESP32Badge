#ifndef _HELLO_
#define _HELLO_

#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include "esp32_badge.h"

/*
 * Displays a "Hello, my name is" badge with two lines of text (i.e. attendee name and company)
 */

#define HLO_COLOR_HEADER 	ILI9341_RED
#define HLO_COLOR_TEXTLINE1 ILI9341_BLACK
#define HLO_COLOR_TEXTLINE2 ILI9341_BLACK
 
class Hello 
{
  public:
 
    Hello();
    
    void begin(const char* textLine1, const char* textLine2, const char* textLine3, Adafruit_ILI9341 *tft, const GFXfont *gfxFontHello, const GFXfont *gfxFontMyNameIs, const GFXfont *gfxFontTextLines);
        
    void update(Adafruit_ILI9341 *tft);

  private:       
  
    

};

#endif // _HELLO_
