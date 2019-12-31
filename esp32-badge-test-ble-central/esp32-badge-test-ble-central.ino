/***************************************************
 * ESP32 Badge Test - Bluetooth Low Energy Central/Client
 * by Paul Pagel
 * 
 * Requires:
 * - ESP32 Badge 
 * - Another ESP32 running the esp32-badge-test-ble-peripheral sketch.
 ****************************************************/
#include <BLEDevice.h> 
#include "esp32_badge.h"
#include "codemash_logo.h"

// Specific MAC address of the peripheral/server you're looking for.
// Should be in ff:ff:ff:ff:ff:ff format (6 bytes plus colon separators)
String esp32_peripheral_address = "<ENTER>"; 

#define PAIR_LED_PIN LED_3 
#define DATA_LED_PIN LED_2 

#define XMIT_ADDRESS 111
// UUIDs for the BLE service and characteristic - must match the peripheral
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

bool btnA_pressed, btnB_pressed, btnX_pressed, btnY_pressed;
bool btnUp_pressed, btnDown_pressed, btnLeft_pressed, btnRight_pressed;
bool spkr_on, led1_on, led2_on, led3_on;

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK, TFT_RST, TFT_MISO);

#define PACKET_SIZE   2

uint8_t data_packet[] = 
{
   0x00,  // button states
   0x00,  // TODO: other data
};

static BLERemoteCharacteristic* 
                    remote_characteristic;
BLEScan*            ble_scan; //BLE scanning device 
BLEScanResults      found_devices;
static BLEAddress*  server_ble_address;
String              server_ble_address_str;
bool                server_paired = false;
bool                client_connected = false;
bool                data_received = false;

/*
 * Callback used when scanning for BLE peripheral/server devices
 */
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks
{
    void onResult(BLEAdvertisedDevice advertisedDevice) 
    {
      Serial.printf("Scan Result: %s \n", advertisedDevice.toString().c_str());
      BLEAddress *foundAddress = new BLEAddress(advertisedDevice.getAddress());
      String bleAddr = foundAddress->toString().c_str();

      tft.println(bleAddr);

      if (bleAddr == esp32_peripheral_address)
      {
        Serial.printf("Found specified peripheral\n");
        server_ble_address = new BLEAddress(advertisedDevice.getAddress());
        server_ble_address_str = server_ble_address->toString().c_str();
      }
    }
};

/* 
 * Callbacks used with BLE control device 
 */
class MyClientCallbacks: public BLEClientCallbacks
{
  void onConnect(BLEClient *pClient)
  {
    client_connected = true;
  }
  
  void onDisconnect(BLEClient *pClient)
  {
    client_connected = false;
    server_paired = false;
  }
};

/*
 * Connect to a peripheral BLE control server address that we found during the scan
 */
bool connectToServer(BLEAddress pAddress){
    
    BLEClient*  pClient  = BLEDevice::createClient();
    pClient->setClientCallbacks(new MyClientCallbacks()); 
    Serial.println(" - Created client");

    // Connect to the BLE Server.
    pClient->connect(pAddress);
    Serial.println(" - Connected to ESP32 Peripheral");

    // Obtain a reference to the service we are after in the remote BLE server.
    BLERemoteService* pRemoteService = pClient->getService(SERVICE_UUID);
    if (pRemoteService == nullptr)
    {
      Serial.println(" - Unable to find control data service.");
      return false;
    }
     Serial.println(" - data control service found.");
    
    // Obtain a reference to the characteristic in the service of the remote BLE server.
    remote_characteristic = pRemoteService->getCharacteristic(CHARACTERISTIC_UUID);
    if (remote_characteristic == nullptr)
    {
      Serial.println(" - Unable to find Control characteristic.");
      return false;
    }
    Serial.println(" - Control characteristic found.");
    
    if(remote_characteristic->canNotify())
      remote_characteristic->registerForNotify(notifyCallback);
    
    return true;
}

/*
 * Called every time we get BLE Control data from the peripheral server device
 */
static void notifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) 
{
    Serial.print("Notify callback for characteristic ");
    Serial.print(pBLERemoteCharacteristic->getUUID().toString().c_str());
    Serial.print(" of data length ");
    Serial.println(length);
    Serial.print("data: ");
    Serial.println((char*)pData);
    
    if (length != PACKET_SIZE)
      return;
    
    for (int i = 0; i < length; i++)
    {
        data_packet[i] = pData[i];
        Serial.println(data_packet[i]);
    }
    data_received = true;
}

/*
 * Process the received control data and map it to internal state variables
 */
void processDataPacket()
{
  btnUp_pressed = data_packet[0] & FLAG_UP;
  btnDown_pressed = data_packet[0] & FLAG_DOWN;
  btnLeft_pressed = data_packet[0] & FLAG_LEFT;
  btnRight_pressed = data_packet[0] & FLAG_RIGHT;
  btnX_pressed = data_packet[0] & FLAG_X;
  btnY_pressed = data_packet[0] & FLAG_Y;
  btnA_pressed = data_packet[0] & FLAG_A;
  btnB_pressed = data_packet[0] & FLAG_B;

  led1_on = btnX_pressed;
  led2_on = btnY_pressed;
  led3_on = btnA_pressed;
  spkr_on = btnB_pressed;
}

/*
 * Kick off the process of finding and paring with the Control server.
 * Returns true if successfully found and paired.
 */
bool pairBlePeripheral()
{
  showBleScanScreen();
  server_ble_address_str = "";

  ble_scan = BLEDevice::getScan(); //create new scan
  ble_scan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks()); //Call the class that is defined above
  ble_scan->setActiveScan(true); //active scan uses more power, but get results faster
  
  found_devices = ble_scan->start(3); //Scan for 3 seconds to find the Control Gauntlet

  if (found_devices.getCount() >= 1 && !server_paired)
  {
    if (server_ble_address_str.length() > 0)
    {
      if (connectToServer(*server_ble_address))
      {
        server_paired = true;
        digitalWrite(PAIR_LED_PIN, HIGH);
        tft.println("Control server paired");
        Serial.print("Control server paired: ");
        Serial.println(server_ble_address_str.c_str());
        showControlTestScreen();
        updateControlTestScreen();
        
        return true; // found peripheral controller
      }
    }
  }
  return false;
}

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
 * Shows the BLE scanning and pairing screen
 */
void showBleScanScreen()
{
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextColor(ILI9341_WHITE);  
  tft.setTextSize(2);

  tft.setCursor(0, 0);
  tft.println("BLE Control Central");
}

/*
 * Shows the control test status screen 
 */
void showControlTestScreen()
{
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextColor(ILI9341_WHITE);  
  tft.setTextSize(3);

  tft.setCursor(0, 0);
  tft.println("Badge Central");
  
  tft.drawLine(0, 35, 319, 35, ILI9341_BLUE);
  tft.drawLine(0, 180, 319, 180, ILI9341_BLUE);

  tft.setTextSize(2);
  tft.setCursor(0, 190);
  tft.setTextColor(ILI9341_ORANGE);
  tft.print("");  // TODO: put status info here

  tft.drawRGBBitmap(250, 50, (uint16_t *)codemash_logo, 57, 67);
}

/*
 * Updates the display showing the control test status values
 */
void updateControlTestScreen()
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
 * Initialization and component setup.  Called once at program start. 
 */
void setup() 
{
  Serial.begin(115200); 

  // Set up the speaker and LED output pins
  pinMode(SPKR, OUTPUT);
  pinMode(LED_1, OUTPUT);
  pinMode(LED_2, OUTPUT);
  pinMode(LED_3, OUTPUT);

  // Set up the TFT screen
  tft.begin();
  tft.setRotation(SCREEN_ROT);
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextColor(ILI9341_WHITE);  
  tft.setTextSize(1);

  if (esp32_peripheral_address.length() != 17)
  {
    tft.println("Please configure a valid ESP32 peripheral address!");
    tft.println("Valid fmt: 01:02:03:f1:f2:f3");
    digitalWrite(LED_1, HIGH);  // error!
    while(true);
  }

  // Setup up BLE and find control peripheral to pair with
  
  BLEDevice::init("");
  
  while (!pairBlePeripheral())
  {
    delay(3000);
  }
}


void loop() 
{
  
  if (!client_connected)
  {
    digitalWrite(PAIR_LED_PIN, LOW);
    while (!pairBlePeripheral())
    {
      delay(3000);
    }
  }

  if (data_received)
  {
    digitalWrite(DATA_LED_PIN, HIGH);
    processDataPacket();
    updateControlTestScreen();
    
    // Set LEDs on or off based on button state
    digitalWrite(LED_1, led1_on ? HIGH : LOW);
    digitalWrite(LED_2, led2_on ? HIGH : LOW);
    digitalWrite(LED_3, led3_on ? HIGH : LOW);
  
    data_received = false;
    delay(10);
    digitalWrite(DATA_LED_PIN, LOW);
  }
}
