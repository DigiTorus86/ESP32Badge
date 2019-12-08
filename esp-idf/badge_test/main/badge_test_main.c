/***************************************************
ESP32 Badge test app

by Paul Pagel

Requires:
 - ESP32 Devkit v1 
 - ILI9431 SPI TFT  
 - Passive buzzer/speaker
 - 1k trim pot
 - (8) momentary buttons
 - (4) 4k7 pullup resistors
 - (3) 220R LED resistors
 ****************************************************/
 
#include <stdio.h>
#include <string.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <driver/adc.h>
#include <esp_adc_cal.h>
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_system.h"
#include "esp_heap_caps.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_event.h"
#include "nvs_flash.h"

#include "tftspi.h"
#include "tft.h"
#include "spiffs_vfs.h"
#include "esp32_badge.h"
#include "codemash_logo.h"

/* Source and docs:  https://github.com/loboris/ESP32_TFT_library
   - had to change PIN_NUM_xxx values in tftspi.h
*/

#define WIFI_SCAN_LIST_SIZE 	12    // Max # of access points to detect 
#define SPI_BUS 		TFT_HSPI_HOST
#define V_REF 			1100  // ADC reference voltage

void init_adc();
void init_gpio();
void init_spkr();
void init_tft();

static const char *TAG = "scan";

// LEDC used for configuring PWM and generating speaker tones
ledc_timer_config_t   ledc_timer;
ledc_channel_config_t ledc_channel = {0};

int btnA_pressed, btnB_pressed, btnX_pressed, btnY_pressed;
int btnUp_pressed, btnDown_pressed, btnLeft_pressed, btnRight_pressed;
bool spkr_on, led1_on, led2_on, led3_on;

uint8_t spkr_channel = 1;

void setTextColor(color_t color, int invert)
{
	if (invert)
	{
		_fg = TFT_BLACK;
		_bg = color;
	}
	else
	{
		_fg = color;
		_bg = TFT_BLACK;
	}
}

/*
 * Draws an RGB bitmap to the screen.  The bitmap must be a pixel array in RGB565 format.
 * TODO:  move this to a custom TFT library
 */
void draw_rgb_bitmap(int16_t x, int16_t y, uint16_t *bitmap, int16_t width, int16_t height)
{
    //startWrite();
    uint16_t bmp_color;
    color_t tft_color = {0, 0, 0};

    for(int16_t j = 0; j < height; j++, y++) 
    {
        for(int16_t i = 0; i < width; i++ ) 
        {
		bmp_color = bitmap[j * width + i];
	        tft_color.r = (bmp_color >> 8) & 0xF8;
         	tft_color.g = (bmp_color >> 3) & 0xFC;
		tft_color.b = (bmp_color << 3) & 0xF8;
		TFT_drawPixel(x + i, y, tft_color, 1); // TODO: only sel on first and last
    	}    
    }
    //endWrite();
}

/*
 * Check the button press states
 */
void check_buttons(void)
{
	btnA_pressed = 1 - gpio_get_level(BTN_A);
	btnB_pressed = 1 - gpio_get_level(BTN_B);
	btnX_pressed = 1 - gpio_get_level(BTN_X);
	btnY_pressed = 1 - gpio_get_level(BTN_Y);

	btnUp_pressed = 1 - gpio_get_level(BTN_UP);
	btnDown_pressed = 1 - gpio_get_level(BTN_DOWN);
	btnLeft_pressed = 1 - gpio_get_level(BTN_LEFT);
	btnRight_pressed = 1 - gpio_get_level(BTN_RIGHT);

	led1_on = btnX_pressed;
	led2_on = btnY_pressed;
	led3_on = btnA_pressed;
	spkr_on = btnB_pressed;

	gpio_set_level(LED_1, led1_on);
	gpio_set_level(LED_2, led2_on);
	gpio_set_level(LED_3, led3_on);
}

/*
 * Draws the little battery level icon
 */
void draw_battery_level(int battLevel, int x, int y)
{
	color_t fill_color = TFT_GREEN;
	float batt_percent = ((battLevel - BATT_DIRE) / (BATT_FULL - BATT_DIRE)) * 100.0; // TODO:  fix!!
	batt_percent = (batt_percent < 100 ? batt_percent : 100);
	batt_percent = (batt_percent > 0 ? batt_percent : 0);

	if (batt_percent < 25)
	{ 
		gpio_set_level(LED_1, 1);
		//ledcWrite(spkr_channel, 128);
		//ledcWriteTone(spkr_channel, 440);
	}
	else if (batt_percent < 50)
	{
		gpio_set_level(LED_2, 1);
		//ledcWrite(spkr_channel, 128);
		//ledcWriteTone(spkr_channel, 440);
	}

	if (batt_percent <= 50) fill_color = TFT_YELLOW;

	if (batt_percent <= 25) fill_color = TFT_RED;

	// draw outline of battery
	TFT_fillRect(x, y + 2, 3, 6, TFT_DARKGREY);
	TFT_drawRect(x + 3, y, 24, 10, TFT_DARKGREY);

	for (int i = 0; i < 4; i++)
	{
		if (batt_percent < 12 + i * 25)
		fill_color = TFT_BLACK;
		TFT_fillRect(x + 6 + (i * 5), y + 2, 4, 6, fill_color);
	}

	gpio_set_level(LED_1, 0);
	gpio_set_level(LED_2, 0);
}

/*
 * Checks the battery level and displays it to the screen
 */
void check_battery(void)
{
  // 220 on USB
  // 180 on full batt
  // 935 = 3.9v

  static uint8_t cntr = 0;

  if (cntr == 0)
  {
    adc1_config_width(ADC_WIDTH_BIT_10);
    int batt_raw = adc1_get_raw(ADC1_GPIO32_CHANNEL);
  
    setTextColor(TFT_ORANGE, 0);
    TFT_print("Battery:", 0, 210);
    char str[10];
    sprintf(str, "%d", batt_raw);
    TFT_print(str, 140, 210);

    draw_battery_level(batt_raw, 200, 214);
  }

  cntr = (cntr + 1) % 100; // check every 100th time
}

/* 
 * Initialize WiFi as STA and runs a WiFi access scan.
 * Returns the number of WiFi access points detected.
 */
static uint16_t scan_wifi(void)
{
    uint16_t ap_count = 0;

    esp_netif_init();
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    uint16_t number = WIFI_SCAN_LIST_SIZE;
    wifi_ap_record_t ap_info[WIFI_SCAN_LIST_SIZE];
    
    memset(ap_info, 0, sizeof(ap_info));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_scan_start(NULL, true));
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&number, ap_info));
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));
    ESP_LOGI(TAG, "Total APs scanned = %u", ap_count);
    for (int i = 0; (i < WIFI_SCAN_LIST_SIZE) && (i < ap_count); i++) 
    {
        ESP_LOGI(TAG, "SSID \t\t%s", ap_info[i].ssid);
        ESP_LOGI(TAG, "RSSI \t\t%d", ap_info[i].rssi);
        //print_auth_mode(ap_info[i].authmode);
        if (ap_info[i].authmode != WIFI_AUTH_WEP) 
        {
            //print_cipher_type(ap_info[i].pairwise_cipher, ap_info[i].group_cipher);
        }
        ESP_LOGI(TAG, "Channel \t\t%d\n", ap_info[i].primary);
    }
    return ap_count;
}

/*
 * Check and display the WiFi stations 
 */ 
void check_wifi(void)
{
  	setTextColor(TFT_ORANGE, 0);
  	TFT_print("WiFi Nets: ", 0, 190);

	int  wifi_count;
  	wifi_count = scan_wifi();

	char str[10];
    	sprintf(str, "%d", wifi_count);
  	TFT_print(str, 140, 190);
}

/*
 * Update the screen showing which buttons are pressed
 */
void update_screen(void)
{
	// Direction buttons
	setTextColor(TFT_CYAN, btnUp_pressed);
	TFT_print("UP", 45, 50); 

	setTextColor(TFT_CYAN, btnLeft_pressed);
	TFT_print("<-LT", 0, 70); 

	setTextColor(TFT_CYAN, btnRight_pressed);
	TFT_print("RT->", 74, 70);

	setTextColor(TFT_CYAN, btnDown_pressed);
	TFT_print("DN", 45, 90);

	uint16_t x = 140;

	// Action buttons
	setTextColor(TFT_CYAN, btnY_pressed);
	TFT_print("Y", x+30, 50); 

	setTextColor(TFT_CYAN, btnX_pressed);
	TFT_print("X", x, 70); 

	setTextColor(TFT_CYAN, btnB_pressed);
	TFT_print("B", x+60, 70);

	setTextColor(TFT_CYAN, btnA_pressed);
	TFT_print("A", x+30, 90);


	setTextColor(TFT_LIGHTGREY, spkr_on);
	TFT_print("(( SPKR ))", 20, 140);

	setTextColor(TFT_RED, led1_on);
	TFT_print("LED1", 140, 140);

	setTextColor(TFT_YELLOW, led2_on);
	TFT_print("LED2", 200, 140);

	setTextColor(TFT_GREEN, led3_on);
	TFT_print("LED3", 260, 140);
}

/*
 * The main application entry point
 */
void app_main(void)
{
	// Initialize Non Volatile Storage 
	// Required by the WiFi scanning
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) 
	{
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK( ret );

	printf("ESP32 Badge Test\n");

	init_adc();
	init_gpio();
	init_spkr();
	init_tft();

	// Initial screen draw
	_fg = TFT_YELLOW;
	_bg = TFT_BLUE;
	TFT_setFont(UBUNTU16_FONT, NULL);
	TFT_fillRect(0, 0, _width-1, TFT_getfontheight()+8, TFT_BLUE);
	TFT_drawRect(0, 0, _width-1, TFT_getfontheight()+8, TFT_CYAN);
	TFT_print("ESP32 Badge Test", CENTER, 4);
	TFT_drawLine(0, 176, 319, 176, TFT_BLUE);

	draw_rgb_bitmap(250, 50, (uint16_t *)codemash_logo, 57, 67);	

	update_screen();
	check_wifi();

	printf("Setup done!\r\n");

	while(1)
	{
		check_buttons();
		check_battery();
		update_screen();
		
		
		if (spkr_on)
		{
			ledc_set_duty(ledc_channel.speed_mode, ledc_channel.channel, 128);
		    	ledc_update_duty(ledc_channel.speed_mode, ledc_channel.channel);
		}
		else
		{
			ledc_set_duty(ledc_channel.speed_mode, ledc_channel.channel, 0);
		    	ledc_update_duty(ledc_channel.speed_mode, ledc_channel.channel);
		}

		vTaskDelay(100 / portTICK_PERIOD_MS);
	}
}

/*
 * Initialize the analog to digital converter
 */
void init_adc()
{
	// Configure ADC
	adc1_config_width(ADC_WIDTH_12Bit);
	adc1_config_channel_atten(ADC1_GPIO32_CHANNEL, ADC_ATTEN_11db);

	// Calculate ADC characteristics i.e. gain and offset factors
	esp_adc_cal_characteristics_t characteristics;
	esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_11db, ADC_WIDTH_BIT_12, V_REF, &characteristics);

	// Read ADC and obtain result in mV
	uint32_t voltage = esp_adc_cal_raw_to_voltage(ADC1_GPIO32_CHANNEL, &characteristics);
	printf("%d mV\n",voltage);

	adc1_config_width(ADC_WIDTH_BIT_10);
    	adc1_config_channel_atten(ADC1_GPIO32_CHANNEL, ADC_ATTEN_DB_11);  // GPIO 32, attenuation for max voltage limited by VDD_A
}

/*
 * Initialize the GPIO pins
 */
void init_gpio()
{
	// Set up user input pins
	gpio_set_direction(BTN_UP, GPIO_MODE_INPUT);
	gpio_set_pull_mode(BTN_UP, GPIO_PULLUP_ONLY);

	gpio_set_direction(BTN_DOWN, GPIO_MODE_INPUT);
	gpio_set_pull_mode(BTN_DOWN, GPIO_PULLUP_ONLY); 

	gpio_set_direction(BTN_LEFT, GPIO_MODE_INPUT);  
	gpio_set_pull_mode(BTN_LEFT, GPIO_PULLUP_ONLY);

	gpio_set_direction(BTN_RIGHT, GPIO_MODE_INPUT);
	gpio_set_pull_mode(BTN_RIGHT, GPIO_PULLUP_ONLY);

	gpio_set_direction(BTN_A, GPIO_MODE_INPUT);
	gpio_set_pull_mode(BTN_A, GPIO_PULLUP_ONLY);

	gpio_set_direction(BTN_B, GPIO_MODE_INPUT);
	gpio_set_pull_mode(BTN_B, GPIO_PULLUP_ONLY);

	gpio_set_direction(BTN_X, GPIO_MODE_INPUT);
	gpio_set_pull_mode(BTN_X, GPIO_PULLUP_ONLY);

	gpio_set_direction(BTN_Y, GPIO_MODE_INPUT);

	gpio_set_pull_mode(BTN_Y, GPIO_PULLUP_ONLY);
	
	// Set up the speaker and LED output pins
	gpio_pad_select_gpio(SPKR);
	gpio_set_direction(SPKR, GPIO_MODE_OUTPUT);
	gpio_pad_select_gpio(LED_1);
	gpio_set_direction(LED_1, GPIO_MODE_OUTPUT);
	gpio_pad_select_gpio(LED_2);
	gpio_set_direction(LED_2, GPIO_MODE_OUTPUT);
	gpio_pad_select_gpio(LED_3);
	gpio_set_direction(LED_3, GPIO_MODE_OUTPUT);
}

/*
 * Initialize the speaker to play tones
 */
void init_spkr()
{
	// Configure the LED control timer
	ledc_timer.duty_resolution = LEDC_TIMER_8_BIT; 	// resolution of PWM duty
	ledc_timer.freq_hz = 4000;                      // frequency of PWM signal
	ledc_timer.speed_mode = LEDC_HIGH_SPEED_MODE;   // timer mode
	ledc_timer.timer_num = LEDC_TIMER_0;            // timer index
	ledc_timer.clk_cfg = LEDC_AUTO_CLK;             // Auto select the source clock
	ledc_timer_config(&ledc_timer);

	// Configure the LED control channel
        ledc_channel.channel    = LEDC_CHANNEL_0;
        ledc_channel.duty       = 0;
        ledc_channel.gpio_num   = SPKR;
        ledc_channel.speed_mode = LEDC_HIGH_SPEED_MODE;
        ledc_channel.hpoint     = 0;
        ledc_channel.timer_sel  = LEDC_TIMER_0;

	ledc_channel_config(&ledc_channel);
}

/*
 * Initialize the ILI9341 TFT display
 */
void init_tft()
{
	esp_err_t ret;	

	_width = 240;
	_height = 320;
	tft_disp_type = DISP_TYPE_ILI9341;

	TFT_PinsInit();

	// Configure SPI devices

	spi_lobo_device_handle_t spi;
	
	spi_lobo_bus_config_t buscfg =
	{
		.miso_io_num = TFT_MISO,		// set SPI MISO pin
		.mosi_io_num = TFT_MOSI,		// set SPI MOSI pin
		.sclk_io_num = TFT_CLK,			// set SPI CLK pin
		.quadwp_io_num = -1,
		.quadhd_io_num = -1,
		.max_transfer_sz = 6 * 1024,
	};
	spi_lobo_device_interface_config_t devcfg =
	{
		.clock_speed_hz = 8000000,            // Initial clock out at 8 MHz
		.mode = 0,                            // SPI mode 0
		.spics_io_num = -1,                   // we will use external CS pin
		.spics_ext_io_num = TFT_CS,           // external CS pin (PIN_NUM_CS)
		.flags = LB_SPI_DEVICE_HALFDUPLEX,    // ALWAYS SET  to HALF DUPLEX MODE!! for display spi
	};

	ret = spi_lobo_bus_add_device(SPI_BUS, &buscfg, &devcfg, &spi);
    	assert(ret == ESP_OK);
	printf("SPI: display device added to spi bus (%d)\r\n", SPI_BUS);
	disp_spi = spi;

	// ==== Test select/deselect ====
	ret = spi_lobo_device_select(spi, 1);
    	assert(ret==ESP_OK);
	ret = spi_lobo_device_deselect(spi);
    	assert(ret == ESP_OK);

	printf("SPI: attached display device, speed=%u\r\n", spi_lobo_get_speed(spi));
	printf("SPI: bus uses native pins: %s\r\n", spi_lobo_uses_native_pins(spi) ? "true" : "false");

	printf("SPI: display init...\r\n");
	TFT_display_init();
    	printf("OK\r\n");

	// ---- Detect maximum read speed ----
	max_rdclock = find_rd_speed();
	printf("SPI: Max rd speed = %u\r\n", max_rdclock);

    	// ==== Set SPI clock used for display operations ====
	spi_lobo_set_speed(spi, DEFAULT_SPI_CLOCK);
	printf("SPI: Changed speed to %u\r\n", spi_lobo_get_speed(spi));

	TFT_setGammaCurve(DEFAULT_GAMMA_CURVE);
	TFT_setRotation(LANDSCAPE); // LANDSCAPE (v1.1) or LANDSCAPE_FLIP (v1.2)
	TFT_fillScreen(TFT_BLACK);
	TFT_resetclipwin();
}
