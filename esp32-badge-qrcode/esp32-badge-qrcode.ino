/*
 *  ESP32 Badge QRCode 
 *  
 *  Button B = Show big QR code for scanning
 *  Button A = Show normal badge mode
 *
 * Copyright (c) 2019 Paul Pagel
 * This is free software; see the license.txt file for more information.
 * There is no warranty; not even for merchantability or fitness for a particular purpose.
 */

#include "esp32_badge.h"
#include <Fonts/FreeSerifBold24pt7b.h>  // FreeSerifBold24pt7bBitmaps[]
#include <WiFi.h>
#include "qrcode.h"
#include "headshot.h"
#include "codemash_logo.h"


#define QR_VERSION 3  	// v3 = 29x29 bits
#define QR_SIZE 	29	
#define QR_BORDER  4   // quiet zone

#define HIDE_LOGO 50
#define SHOW_LOGO 100

// TODO:  Change values as appropriate
const char* my_name  = "";  
const char* company  = "";
const char* ssid     = "";
const char* password = "";

bool btnA_pressed, btnB_pressed, btnX_pressed, btnY_pressed;
bool btnA_released, btnB_released, btnX_released, btnY_released;
bool btnUp_pressed, btnDown_pressed, btnLeft_pressed, btnRight_pressed;
bool btnUp_released, btnDown_released, btnLeft_released, btnRight_released;
bool spkr_on, led1_on, led2_on, led3_on;

uint8_t curr_font, font_offset = 0;
String font_name;

uint8_t spkr_channel = 1;

uint8_t    qr_scale;
int16_t    qr_x, qr_y, qr_pixels;

String   url; 

WiFiServer server(80);

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK, TFT_RST, TFT_MISO);
 
void setup() 
{
  Serial.begin(115200);
  Serial.println("ESP32 Badge QR Code"); 
     
	// Initialize TFT
	tft.begin();
  tft.setRotation(SCREEN_ROT);
  
	drawBadge();
  
  ledcSetup(spkr_channel, 12000, 8);  // 12 kHz max, 8 bit resolution
  ledcAttachPin(SPKR, spkr_channel);

  // Set up user input pins
  // Pins 24, 35, 36, & 39 do NOT have internal pullups, so need 4k7 physical resistors for these
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
  
  // initialize WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
        digitalWrite(LED_2, HIGH); // flash yellow while connecting
        delay(500);
        Serial.print(".");
        digitalWrite(LED_2, LOW);
  }
  digitalWrite(LED_1, HIGH); // flash green when connected
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  server.begin();
  digitalWrite(LED_1, LOW);
    
	// assemble URL for QR code
  url = String("http://" + WiFi.localIP().toString() + "/");
  Serial.println(url);
	 
  drawQrCode(210, 130, 3, url.c_str(), false);
}


void drawBadge()
{
  tft.fillScreen(ILI9341_BLACK);
  tft.setFont(&FreeSerifBold24pt7b);
  tft.setTextSize(1);

  int16_t  x1, y1; 
  uint16_t wd, ht;
  tft.getTextBounds(my_name, 10, 50, &x1, &y1, &wd, &ht);
  Serial.println("x1, y1, w, h");
  Serial.println((int)x1);
  Serial.println((int)y1);
  Serial.println((int)wd);
  Serial.println((int)ht);
  
  tft.setCursor(160 - (wd / 2), ht);
  tft.setTextColor(ILI9341_ORANGE);
  tft.print(my_name);

  tft.getTextBounds(company, 10, 50, &x1, &y1, &wd, &ht);
  tft.setCursor(160 - (wd / 2), 100);
  tft.setTextColor(ILI9341_RED);
  tft.print(company);

  tft.drawRGBBitmap(0, 130, (uint16_t *)headshot, 120, 120);

  tft.drawRGBBitmap(133, 144, (uint16_t *)codemash_logo, 57, 67);
}


void drawQrCode(int16_t x, int16_t y, uint8_t scale, const char *data, bool erasePrevious)
{
  if (erasePrevious)
  {
    tft.fillRect(qr_x, qr_y, qr_pixels, qr_pixels, ILI9341_BLACK);
  }
  
  QRCode qrcode;
  uint8_t qrcodeData[qrcode_getBufferSize(QR_VERSION)];  
  qrcode_initText(&qrcode, qrcodeData, 3, 0, data);
  qr_scale = scale;
  qr_x = x;
  qr_y = y;
  qr_pixels = (QR_SIZE + QR_BORDER * 2) * qr_scale;
  
  // show a filled white square 
  tft.fillRect(qr_x, qr_y, qr_pixels, qr_pixels, ILI9341_WHITE);
 
  for (uint8_t y = 0; y < qrcode.size; y++) 
  {
    // Each horizontal module
    for (uint8_t x = 0; x < qrcode.size; x++) 
    {
      if (qrcode_getModule(&qrcode, x, y))
      {
        tft.fillRect(qr_x + (QR_BORDER + x) * qr_scale, qr_y + (QR_BORDER + y) * qr_scale, qr_scale, qr_scale, ILI9341_BLACK);
      }
    }
  }
}

 
void loop() 
{
  // pressing A draws normal badge
  if(digitalRead(BTN_A) == LOW)
  {
    Serial.println("Button A pressed");
    drawBadge();
    drawQrCode(210, 130, 3, url.c_str(), false);
  }
  
  // pressing B draws a full-screen QR code
  if(digitalRead(BTN_B) == LOW)
  {
    //dumpScreen(0, 0, 320, 240, tft); // TESTING!!
    Serial.println("Button B pressed");
    tft.fillScreen(ILI9341_BLACK);
    drawQrCode(0, 0, 7, url.c_str(), false);
  }

  WiFiClient client = server.available();   // listen for incoming clients

  if (client) {                             // if you get a client,
    
    Serial.println("New Client.");           // print a message out the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected()) {            // loop while the client's connected
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        if (c == '\n') {                    // if the byte is a newline character

          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();

            // Display the HTML web page
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            // CSS to style the on/off buttons 
            // Feel free to change the background-color and font-size attributes to fit your preferences
            client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
            client.println(".button { background-color: #4CAF50; border: none; color: white; padding: 16px 40px;");
            client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
            client.println(".button2 {background-color: #555555;}</style></head>");
            
            // Web Page Heading
            client.println("<body><h1>ESP32 Web Server</h1>");

            // Display current state, and ON/OFF buttons for Green LED  
            // If the led1_on is false, it displays the ON button       
            if (led3_on) 
            {
              client.println("<p>Green LED - State ON</p>");
              client.println("<p><a href=\"/led3/off\"><button class=\"button button2\">OFF</button></a></p>");
            } 
            else 
            {
              client.println("<p>Green LED - State OFF</p>");
              client.println("<p><a href=\"/led3/on\"><button class=\"button\">ON</button></a></p>");
            } 

            // The HTTP response ends with another blank line:
            client.println();
            // break out of the while loop:
            break;
          } else {    // if you got a newline, then clear currentLine:
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }

        // Check to see if the client request was "GET /led/xx
        if (currentLine.endsWith("GET /led3/on")) {
          digitalWrite(LED_3, HIGH);   
          led3_on = true;
          Serial.println("\nHIGH cmd detected");
        }
        if (currentLine.endsWith("GET /led3/off")) {
          digitalWrite(LED_3, LOW);          
          led3_on = false;
          Serial.println("\nLOW cmd detected");
        }
      }
    }
    // close the connection:
    client.stop();
    Serial.println("Client Disconnected.");
  } 
  
}
 
