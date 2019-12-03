/*
 * Helper functions for displaying text on the ILI9341 TFT display
 */

#ifndef _TFTHELPER_
#define _TFTHELPER_

#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>

/*
 * Draws the text to the display, dowing word wrap on the spaces.
 * Assumes a fixed-width font
 */
void TFT_drawWordWrap(const char* text, uint8_t rows, uint8_t cols, Adafruit_ILI9341 *tft);

/*
 * Draws the text in uppercase to the display
 */
void TFT_drawTextUpper(const char* text, uint8_t cols, Adafruit_ILI9341 *tft);


/*
 * Draws the text to the display, dowing word wrap on the spaces.
 * Assumes a fixed-width font
 */
void TFT_drawWordWrap(const char* text, uint8_t rows, uint8_t cols, Adafruit_ILI9341 *tft)
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
    tft->print(text);
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
    tft->print(text);
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
        tft->print(text[pos]);
        pos += 1;
      }
      Serial.println();
      tft->println();

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
void TFT_drawTextUpper(const char* text, Adafruit_ILI9341 *tft)
{
  for (uint16_t i = 0; i < strlen(text); i++)
  {
    tft->print((char)toupper(text[i]));
  }
}

#endif // _TFTHELPER_
