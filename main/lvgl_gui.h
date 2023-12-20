#pragma once
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "esp_attr.h"
#include "esp_sleep.h"
#include "protocol_examples_common.h"
#include "esp_sntp.h"

#include "freertos/queue.h"
#include "driver/touch_pad.h"
#include "soc/rtc_periph.h"
#include "soc/sens_periph.h"


#include "esp_wifi.h"
#include "esp_wpa2.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_smartconfig.h"
#include <sys/param.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include "esp_vfs.h"
#include "esp_http_server.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include "esp_timer.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "lvgl.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_check.h"
#include "driver/rmt_tx.h"
#include "led_strip_encoder.h"
#include "esp_dsp.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"
#include "driver/i2c.h"
#include "esp_lcd_touch_gt911.h"
#include <math.h>
#include "esp32s3/rom/ets_sys.h"
#include "driver/i2s.h"
#include "driver/spi_master.h"
#include "soc/gpio_struct.h"
#include "driver/uart.h"
#include "soc/uart_struct.h"
#include "driver/gptimer.h"
#include "sdkconfig.h"
#include "ui.h"
#include "demos/lv_demos.h"



#define SD_CLK       6
#define SD_CMD       5
#define SD_D0        7 
#define SD_D1        8
#define SD_D2        3 
#define SD_D3        4

#define WS        47
#define SCK       39
#define DATA_OUT  48 
#define DATA_IN   38

#define MOUNT_POINT "/sdcard"

#define LINES  32
#define EXAMPLE_I2C_NUM                 0  
#define EXAMPLE_I2C_SCL                 10
#define EXAMPLE_I2C_SDA                 9

#define LCD_H_RES 64
#define LCD_V_RES 32
#define RMT_LED_STRIP_RESOLUTION_HZ 10000000 // 10MHz resolution, 1 tick = 0.1us (led strip needs a high resolution)
#define  LVGL_TICK_PERIOD_MS   10
void guiTask(void *pvParameter);
void strip_init(void);
void led_strip_hsv2rgb(uint32_t h, uint32_t s, uint32_t v, uint32_t *r, uint32_t *g, uint32_t *b);

void initialise_wifi(void);
esp_err_t start_file_server(const char *base_path);

void test_sntp(void);
void touch_init(void *pvParameter);

#define EXAMPLE_LED_NUMBERS  2048
#define LED_CHANNEL_NUM     4
extern rmt_transmit_config_t tx_config;
extern rmt_sync_manager_handle_t synchro ;
extern rmt_encoder_handle_t led_encoder[LED_CHANNEL_NUM];
extern rmt_channel_handle_t led_channels[LED_CHANNEL_NUM];
extern uint8_t led_strip_pixels[EXAMPLE_LED_NUMBERS*3];
extern nvs_handle_t my_handle;

extern lv_obj_t * qr_code ;
extern lv_obj_t * wp;
extern uint8_t flag_mode ;
extern lv_obj_t * ui_Label1 ;