#ifndef _HELLOROTATE_
#define _HELLOROTATE_

#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include "esp32_badge.h"

/*
 * Displays a "Hello, my name is" badge with two lines of text (i.e. attendee name and company)
 */

#define HLOR_MESSAGE_CNT    7
#define HLOR_LINESPACE      15
#define HLOR_COLOR_HEADER 	ILI9341_BLUE
#define HLOR_COLOR_TEXT     ILI9341_BLACK
 
class HelloRotate 
{
  public:
 
    HelloRotate(uint32_t rotate_ms);
    
    void begin(Adafruit_ILI9341 *tft, const GFXfont *gfxFontHello);
        
    void update(Adafruit_ILI9341 *tft, const GFXfont *gfxFontText);

  private:  
  	uint32_t    _rotate_ms = 10000; // 10 sec default
  	uint32_t    _last_rotate_ms;
    uint8_t     _message_idx = 0;
  	const char* _message[HLOR_MESSAGE_CNT][3] = 
  	{ 
      { "An Amazing", "Software", "Developer" },
      { "Just Happy", "To Be Here", "" },  
  	  { "", "BATMAN", "" }, 
  	  { "", "Awesome", "" },
  	  { "", "Feeling Spicy", "" },
  	  { "from the", "FUTURE!", "" },
  	  { "Silently", "Judging You", "" }
  	};
    
};

#endif // _HELLOROTATE_
