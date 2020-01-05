/***************************************************
ESP32 Badge Image Slideshow App 

Based on examples from the SDFat and Adafruit_ImageReader libraries

Requires: 
 - ESP32 Badge v1.2 with 38-pin ESP32-DevKitC 
 - Formatted SD card containing bitmap files in the root directory.
 - Bitmaps should be 320x240 pixels, 24-bit color with 8.3 format filenames (i.e. "PICTURE1.BMP") 
 
  Controls:
  - B button:  immediately move to next slide

Copyright (c) 2019 Paul Pagel
This is free software; see the license.txt file for more information.
There is no warranty; not even for merchantability or fitness for a particular purpose.
****************************************************/

#include <SdFat.h> // Adafruit fork
#include <Adafruit_ImageReader.h> 
#include <Adafruit_SPIFlash.h>
#include "esp32_badge.h"

#define SLIDE_DELAY_SEC  10

SdFat                sd;         // SD card filesystem
SdFile               root;
SdFile               file;
Adafruit_ImageReader reader(sd); // Image-reader object, pass in SD filesys

// IMPORTANT!  The SD card SPI drivers will cause the TFT to stop working
// if you specify the MISO/MOSI/CLK pins in the TFT constructor!
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

/*
 * Set up the board
 */
void setup() 
{
  Serial.begin(115200);
  Serial.println("ESP32 Badge Image Slideshow"); 
  delay(100);

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

  tft.fillScreen(ILI9341_BLACK);
  tft.setTextColor(ILI9341_WHITE);  
  tft.setTextSize(1);
  tft.setCursor(0, 0);

  delay(200);

  // Initialize the SD file system before trying using the reader
  // ESP32 requires 25 MHz SPI clock limit. Breakouts may require 10 MHz limit due to longer wires
  // See SDFat/src/SdCard/SdInfo.h for list of error codes

  Serial.print(F("Initializing filesystem..."));
  if(!sd.begin(SD_CS, SD_SCK_MHZ(10))) 
  { 
    Serial.println(F("SD begin() failed"));
    digitalWrite(LED_1, HIGH);
    for(;;); // Fatal error, do not continue
  }

  if (!root.open("/")) 
  {
    digitalWrite(LED_1, HIGH);
    sd.errorHalt("open root failed");
  }
}

/*
 * Reads the next image from the SD card and displays it to the screen
 * Returns true on success, false on failure or rewind
 */
bool showNextImage()
{
  char            file_name[20];
  size_t          name_size = 20;
  ImageReturnCode stat;         // Status from image-reading functions
  int32_t         width  = 0,   // BMP image dimensions
                  height = 0;
  
  if (file.openNext(&root, O_RDONLY)) 
  {
    file.printFileSize(&Serial);
    Serial.write(' ');
    file.printModifyDateTime(&Serial);
    Serial.write(' ');
    file.printName(&Serial);
    Serial.println();
    
    if (!file.isDir()) 
    {
      file.getName(file_name, name_size);

      stat = reader.bmpDimensions(file_name, &width, &height);
      reader.printStatus(stat);   
      if(stat == IMAGE_SUCCESS) 
      { 
        Serial.print(F("Image dimensions: "));
        Serial.print(width);
        Serial.write('x');
        Serial.println(height);

        stat = reader.drawBMP(file_name, tft, 0, 0);
        reader.printStatus(stat); 
      }
    }
    
    file.close();
  }
  else
  {
    // Start from the beginning next time
    root.rewind();  
    return false;
  }

  return true;
}

/*
 * The main program loop
 */
void loop(void) 
{
  showNextImage();

  // Wait until delay time has expired or B button is pressed
  
  for (uint16_t i = 0; i < SLIDE_DELAY_SEC * 100; i++)
  {
    if(digitalRead(BTN_B) == LOW)
    {
      return; 
    }
    
    delay(10);
  }
}
