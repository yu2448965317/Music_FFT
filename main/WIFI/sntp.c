/* LwIP SNTP example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include "lvgl_gui.h"
static const char *TAG = "SNTP";
#ifndef INET6_ADDRSTRLEN
#define INET6_ADDRSTRLEN 48
#endif

RTC_DATA_ATTR static int boot_count = 0;
static void obtain_time(void);
static void initialize_sntp(void);
#ifdef CONFIG_SNTP_TIME_SYNC_METHOD_CUSTOM
void sntp_sync_time(struct timeval *tv)
{
   settimeofday(tv, NULL);
   ESP_LOGI(TAG, "Time is synchronized from custom code");
   sntp_set_sync_status(SNTP_SYNC_STATUS_COMPLETED);
}
#endif

void time_sync_notification_cb(struct timeval *tv)
{
    ESP_LOGI(TAG, "Notification of a time synchronization event");
}

void test_sntp(void)
{
    struct tm timeinfo;
    time_t now; 
    time(&now);
    localtime_r(&now, &timeinfo);
    if (timeinfo.tm_year < (2016 - 1900)) {
        ESP_LOGI(TAG, "Time is not set yet. Connecting to WiFi and getting time over NTP.");
        obtain_time();
        time(&now);
    }
    //char strftime_buf[64];
    setenv("TZ", "CST-8", 1);
    tzset();
    while(1)
    {;
        time(&now); 
        localtime_r(&now, &timeinfo);
        ESP_LOGI(TAG, "周%d,%d年:%d月:%d日,%d:%d:%d", timeinfo.tm_wday, timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        //strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
        //ESP_LOGI(TAG, "The current date/time in Shanghai is: %s", strftime_buf);
    }

  //  const int deep_sleep_sec = 10;
 //   ESP_LOGI(TAG, "Entering deep sleep for %d seconds", deep_sleep_sec);
 //   esp_deep_sleep(1000000LL * deep_sleep_sec);
}

static void obtain_time(void)
{
    initialize_sntp();
    time_t now = 0;
    struct tm timeinfo = { 0 };
    int retry = 0;
    const int retry_count = 15;
    while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && ++retry < retry_count) {
        ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
    time(&now);
    localtime_r(&now, &timeinfo);
}

static void initialize_sntp(void)
{
    ESP_LOGI(TAG, "Initializing SNTP");
    sntp_stop();
    sntp_enabled();
    esp_sntp_setoperatingmode(ESP_SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_setservername(3, "cn.pool.ntp.org"); // 中国区NTP服务的虚拟集群
    sntp_setservername(1, "210.72.145.44"); // 国家授时中心服务器 IP 地址
    sntp_setservername(2, "ntp1.aliyun.com");
    sntp_set_time_sync_notification_cb(time_sync_notification_cb);
    esp_sntp_init();
}
