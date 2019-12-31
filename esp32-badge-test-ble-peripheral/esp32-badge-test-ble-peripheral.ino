/***************************************************
 * ESP32 Badge Test - Bluetooth Low Energy Peripheral/Server
 * by Paul Pagel
 * 
 * Requires:
 * - ESP32 Badge 
 * - Another ESP32 running the esp32-badge-test-ble-central sketch,
 *   configured with this device's MAC address.
 ****************************************************/

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include "esp32_badge.h"
#include "codemash_logo.h"

#define BLE_DEVICE_ID   "CtrlSrv1"  // Adjust as desired, but keep short

// UUIDs for the BLE service and characteristic - must match the central
// Generate new GUIDs if you modify the data packet size/format
#define SERVICE_UUID        "7b2948a7-1072-4f71-9f99-662c22f6087e"
#define CHARACTERISTIC_UUID "fd93ec01-e537-443a-a57a-68c01bca3823"

// Button state bit flags
#define FLAG_UP     0x01
#define FLAG_DOWN   0x02
#define FLAG_LEFT   0x04
#define FLAG_RIGHT  0x08
#define FLAG_X      0x10
#define FLAG_Y      0x20
#define FLAG_A      0x40
#define FLAG_B      0x80

bool    btnA_pressed, btnB_pressed, btnX_pressed, btnY_pressed;
bool    btnA_released, btnB_released, btnX_released, btnY_released;
bool    btnUp_pressed, btnDown_pressed, btnLeft_pressed, btnRight_pressed;
bool    btnUp_released, btnDown_released, btnLeft_released, btnRight_released;
bool    spkr_on, led1_on, led2_on, led3_on;
uint8_t spkr_channel = 1;

BLECharacteristic*  pCharacteristic;
bool                device_connected = false;

#define PACKET_SIZE 2

uint8_t data_packet[] = 
{
   0x00,  // button states
   0x00,  // TODO: other data
};

class MyServerCallbacks: public BLEServerCallbacks 
{
    void onConnect(BLEServer* pServer) {
      device_connected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      device_connected = false;
    }
};

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK, TFT_RST, TFT_MISO);

/*
 * Use to toggle between normal and inverted text display
 */
void setTextColor(uint16_t color, bool inverted)
{
  if (inverted)
    tft.setTextColor(ILI9341_BLACK, color);
  else
    tft.setTextColor(color, ILI9341_BLACK);
}

/*
 * Initialization and component setup.  Called once at program start. 
 */
void setup() 
{
  Serial.begin(115200);
  Serial.println("ESP32 Badge BLE Controller"); 
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
  tft.setTextSize(3);

  tft.setCursor(0, 0);
  tft.println("BLE Controller");
  
  tft.drawLine(0, 35, 319, 35, ILI9341_BLUE);
  tft.drawLine(0, 180, 319, 180, ILI9341_BLUE);

  delay(100);

  // Set up BLE
  
  BLEDevice::init(BLE_DEVICE_ID);

  Serial.print("MAC Address: ");
  Serial.println(BLEDevice::getAddress().toString().c_str());

  tft.setTextSize(2);
  tft.setCursor(0, 190);
  tft.setTextColor(ILI9341_ORANGE);
  tft.print("ADDR: ");
  tft.print(BLEDevice::getAddress().toString().c_str());

  tft.drawRGBBitmap(250, 50, (uint16_t *)codemash_logo, 57, 67);
    
  // Create the BLE Server
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  Serial.println("Callbacks created.");
  
  // Create the BLE Service
  BLEService *pService = pServer->createService(BLEUUID(SERVICE_UUID));

  // Create a BLE Characteristic
  pCharacteristic = pService->createCharacteristic(
    BLEUUID(CHARACTERISTIC_UUID),
    BLECharacteristic::PROPERTY_READ   |
    BLECharacteristic::PROPERTY_WRITE  |
    BLECharacteristic::PROPERTY_NOTIFY |
    BLECharacteristic::PROPERTY_WRITE_NR
  );
  Serial.println("BLE service characteristic created.");
  
  // https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.descriptor.gatt.client_characteristic_configuration.xml
  // Create a BLE Descriptor
  pCharacteristic->addDescriptor(new BLE2902());

  // Start the service
  pService->start();
  Serial.println("BLE service started.");
  
  // Start advertising
  BLEAdvertising *pAdvertising = pServer->getAdvertising();
  pAdvertising->addServiceUUID(pService->getUUID());
  pAdvertising->start();
  Serial.println("BLE advertising started.");
}

/*
 * Returns the button states as a byte of bit flags
 */
uint8_t getButtonStatus()
{
  uint8_t btn_status = 0x00;

  if (btnUp_pressed) btn_status |= FLAG_UP;
  if (btnDown_pressed) btn_status |= FLAG_DOWN;
  if (btnLeft_pressed) btn_status |= FLAG_LEFT;
  if (btnRight_pressed) btn_status |= FLAG_RIGHT;
  if (btnX_pressed) btn_status |= FLAG_X;  
  if (btnY_pressed) btn_status |= FLAG_Y;  
  if (btnA_pressed) btn_status |= FLAG_A;  
  if (btnB_pressed) btn_status |= FLAG_B;  

  return btn_status;
}

/*
 * Sends a "control change" message
 */
void sendControlChange()
{
  if (device_connected) 
  {
    digitalWrite(LED_2, HIGH);
    data_packet[0] = getButtonStatus();
    data_packet[1] = 0x00; // TODO
    pCharacteristic->setValue(data_packet, 2); 
    pCharacteristic->notify();
    digitalWrite(LED_2, LOW);
  }
}

/*
 * Updates the screen based on which buttons are being pressed
 */
void updateScreen()
{
  // Direction buttons
  tft.setCursor(45, 50);
  setTextColor(ILI9341_CYAN, btnUp_pressed);
  tft.print("UP"); 

  tft.setCursor(0, 70);
  setTextColor(ILI9341_CYAN, btnLeft_pressed);
  tft.print("<-LT"); 

  tft.setCursor(60, 70);
  setTextColor(ILI9341_CYAN, btnRight_pressed);
  tft.print("RT->");
 
  tft.setCursor(45, 90);
  setTextColor(ILI9341_CYAN, btnDown_pressed);
  tft.print("DN");

  int x = 140;

  // Action buttons
  tft.setCursor(x+30, 50);
  setTextColor(ILI9341_CYAN, btnY_pressed);
  tft.print("Y"); 

  tft.setCursor(x, 70);
  setTextColor(ILI9341_CYAN, btnX_pressed);
  tft.print("X"); 
 
  tft.setCursor(x+60, 70);
  setTextColor(ILI9341_CYAN, btnB_pressed);
  tft.print("B");

  tft.setCursor(x+30, 90);
  setTextColor(ILI9341_CYAN, btnA_pressed);
  tft.print("A");

  tft.setCursor(0, 140);
  setTextColor(ILI9341_LIGHTGREY, spkr_on);
  tft.print("(( SPKR ))");

  tft.setCursor(140, 140);
  setTextColor(ILI9341_RED, led1_on);
  tft.print("LED1");

  tft.setCursor(200, 140);
  setTextColor(ILI9341_YELLOW, led2_on);
  tft.print("LED2");

  tft.setCursor(260, 140);
  setTextColor(ILI9341_GREEN, led3_on);
  tft.print("LED3");
  
}

/*
 * Main program loop.  Called continuously after setup.
 */
void loop(void) 
{
  digitalWrite(LED_3, device_connected ? HIGH : LOW);
  
  if(digitalRead(BTN_UP) == LOW)
  {
    Serial.println("Button Up pressed");
    btnUp_pressed = true;
  }
  else
  {
    btnUp_pressed = false;
  }

  if(digitalRead(BTN_DOWN) == LOW)
  {
    Serial.println("Button Down pressed");
    btnDown_pressed = true;
  }
  else
  {
    btnDown_pressed = false;
  }

  if(digitalRead(BTN_LEFT) == LOW)
  {
    Serial.println("Button Left pressed");
    btnLeft_pressed = true;
  }
  else
  {
    btnLeft_pressed = false;
  }

  if(digitalRead(BTN_RIGHT) == LOW)
  {
    Serial.println("Button Right pressed");
    btnRight_pressed = true;
  }
  else
  {
    btnRight_pressed = false;
  }
  
  
  if(digitalRead(BTN_X) == LOW)
  {
    Serial.println("Button X pressed");
    btnX_pressed = true;
    digitalWrite(LED_1, HIGH);
    led1_on = true;
  }
  else
  {
    btnX_pressed = false;
    digitalWrite(LED_1, LOW);
    led1_on = false;
  }

  if(digitalRead(BTN_Y) == LOW)
  {
    Serial.println("Button Y pressed");
    btnY_pressed = true;
    digitalWrite(LED_2, HIGH);
    led2_on = true;
  }
  else
  {
    btnY_pressed = false;
    digitalWrite(LED_2, LOW);
    led2_on = false;
  }

  if(digitalRead(BTN_A) == LOW)
  {
    Serial.println("Button A pressed");
    btnA_pressed = true;
    //digitalWrite(LED_3, HIGH);
    led3_on = true;
  }
  else
  {
    btnA_pressed = false;
    //digitalWrite(LED_3, LOW);
    led3_on = false;
  }
  
  if(digitalRead(BTN_B) == LOW)
  {
    Serial.println("Button B pressed");
    btnB_pressed = true;
    spkr_on = true;
  }
  else
  {
    btnB_pressed = false;
    spkr_on = false;
  }
  
  updateScreen();
  sendControlChange();
  delay(100);
}
