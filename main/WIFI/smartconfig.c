
#include "lvgl_gui.h"
uint8_t GET_WIFI_CONFIG_BIT = 0 ;
uint8_t connect_cycles = 0 ;
nvs_handle_t my_handle;
gptimer_handle_t gptimer = NULL;
gptimer_config_t timer_config = {
    .clk_src = GPTIMER_CLK_SRC_DEFAULT,
    .direction = GPTIMER_COUNT_UP,
    .resolution_hz = 1000000, // 1KHz, 1 tick = 1ms
};
static const char *TAG = "smartconfig";
uint16_t Get_nvs_wifi(char *ssid, char *password);
void Set_nvs_wifi(char *ssid, char *password);
void my_timer_init()
{
  ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &gptimer));
}
uint64_t millis()
{
    static uint64_t count;
    gptimer_get_raw_count(gptimer, &count);
    return count;
}
static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        if(GET_WIFI_CONFIG_BIT<=0)
        {
            ESP_ERROR_CHECK( esp_smartconfig_set_type(SC_TYPE_ESPTOUCH) );
            smartconfig_start_config_t cfg = SMARTCONFIG_START_CONFIG_DEFAULT();
            ESP_ERROR_CHECK( esp_smartconfig_start(&cfg) );
        }
        else
        {
            ESP_LOGI("WIFI", "WIFI Auto Connect from NVS Wificonfig");
            esp_wifi_connect();
        }
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {  //网络未连接
       esp_wifi_connect();
       ESP_LOGI("WIFI", "WIFI Retry to connect.....");
       if(connect_cycles<255)
       {    connect_cycles++;
            if(connect_cycles>=10 && GET_WIFI_CONFIG_BIT>=1)
            {
                    ESP_LOGW("WIFI", " Auto Connect from NVS Wificonfig Failed\nStart Smartconfig !");
                    ESP_ERROR_CHECK( esp_event_handler_register(SC_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL) );
                    ESP_ERROR_CHECK( esp_smartconfig_set_type(SC_TYPE_ESPTOUCH) );
                    smartconfig_start_config_t cfg = SMARTCONFIG_START_CONFIG_DEFAULT();
                    ESP_ERROR_CHECK( esp_smartconfig_start(&cfg) );
            }
       }
    }else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {          //网络连接成功
        connect_cycles=0;
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        ESP_ERROR_CHECK(start_file_server(MOUNT_POINT));
      //  ESP_LOGI(TAG, "got ip:%d", esp_ip4_addr1(&event->ip_info.ip));
        char ip_addr[25];
        sprintf(ip_addr,"http://%d%s%d%s%d%s%d",esp_ip4_addr1(&event->ip_info.ip),".",esp_ip4_addr2(&event->ip_info.ip),".",esp_ip4_addr3(&event->ip_info.ip),".",esp_ip4_addr4(&event->ip_info.ip)); 
        ESP_LOGI(TAG, "got ip:%s", ip_addr);
        ESP_LOGI(TAG, "File server started");
        lv_qrcode_update(qr_code, ip_addr,strlen(ip_addr));
      //   test_sntp();
    } else if (event_base == SC_EVENT && event_id == SC_EVENT_SCAN_DONE) {
        ESP_LOGI(TAG, "Scan done");
    } else if (event_base == SC_EVENT && event_id == SC_EVENT_FOUND_CHANNEL) {
        ESP_LOGI(TAG, "Found channel");
    } else if (event_base == SC_EVENT && event_id == SC_EVENT_GOT_SSID_PSWD) {
        ESP_LOGI(TAG, "Got SSID and password");
        smartconfig_event_got_ssid_pswd_t *evt = (smartconfig_event_got_ssid_pswd_t *)event_data;
        wifi_config_t wifi_config;
        char ssid[64] = { 0 };
        char password[64] = { 0 };
        char rvd_data[33] = { 0 };
        bzero(&wifi_config, sizeof(wifi_config_t));
        memcpy(wifi_config.sta.ssid, evt->ssid, sizeof( evt->ssid));
        memcpy(wifi_config.sta.password, evt->password, sizeof(evt->password));
        wifi_config.sta.bssid_set = evt->bssid_set;
        if (wifi_config.sta.bssid_set == true) {
            memcpy(wifi_config.sta.bssid, evt->bssid, sizeof(wifi_config.sta.bssid));
        }
        memcpy(ssid, evt->ssid, sizeof(evt->ssid));
        memcpy(password, evt->password, sizeof(evt->password));
        ESP_LOGI(TAG, "SSID:%s", ssid);
        ESP_LOGI(TAG, "PASSWORD:%s", password);
        ESP_ERROR_CHECK( esp_wifi_disconnect() );
        ESP_ERROR_CHECK( esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
        Set_nvs_wifi(ssid,password);
        esp_wifi_connect();
    } else if (event_base == SC_EVENT && event_id == SC_EVENT_SEND_ACK_DONE) {  //自动配网结束
        esp_smartconfig_stop();
    }
}            
void initialise_wifi(void)
{
    //my_timer_init();
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL) );
    ESP_ERROR_CHECK( esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL) );
    char read_ssid[64] = { 0 };
    char read_password[64] = { 0 };
    GET_WIFI_CONFIG_BIT=Get_nvs_wifi(read_ssid,read_password);
    if(GET_WIFI_CONFIG_BIT)
    {
        wifi_config_t wifi_config;
        ESP_LOGI("WIFI","Read WIFI Config -> SSID:%s  PASSWORD:%s\nStart Connect....",read_ssid,read_password);
        bzero(&wifi_config, sizeof(wifi_config_t));
        memcpy(wifi_config.sta.ssid,read_ssid, sizeof(wifi_config.sta.ssid));
        memcpy(wifi_config.sta.password, read_password, sizeof(wifi_config.sta.password));
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
        ESP_ERROR_CHECK(esp_wifi_start() );
    }
    else
    {
        ESP_LOGI("WIFI:","Not Found WIFI config,Start Smart Config....");
        ESP_ERROR_CHECK( esp_event_handler_register(SC_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL) );
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
        ESP_ERROR_CHECK(esp_wifi_start() );
    }
}
uint16_t Get_nvs_wifi(char *ssid, char *password)
{
    size_t read_size = 0;
    unsigned char flag = 0 ;
    unsigned char flag_2 = 0 ;
    esp_err_t err;
    
    err = nvs_open("wificonfig", NVS_READWRITE, &my_handle);
    if (err != ESP_OK)
    {
        ESP_LOGI("WIFI","Error (%s) opening NVS handle!", esp_err_to_name(err));
    } 
    else 
    {
        ESP_LOGI("WIFI","Opening NVS handle successful!");
    }
    read_size = 64;
    err = nvs_get_str(my_handle, "ssid", ssid, &read_size);
    switch (err) {
        case ESP_OK:
                ESP_LOGI(TAG, "Get ssid success!");
             //   ESP_LOGI(TAG, "ssid=%s", ssid);
              //  ESP_LOGI(TAG, "ssid_len=%d", read_size);
                flag = 1;
            break;
        case ESP_ERR_NVS_NOT_FOUND:
                ESP_LOGI(TAG, "get err =0x%x", err);
                ESP_LOGI(TAG, "Get ssid fail!");
            break;
        default :
                ESP_LOGI(TAG, "Get ssid fail!");
                break;
    }
    read_size = 64;
    err = nvs_get_str(my_handle, "password", password, &read_size);
    switch (err) {
        case ESP_OK:
            ESP_LOGI(TAG, "Get password success!");
           // ESP_LOGI(TAG, "password=%s", password);
           // ESP_LOGI(TAG, "password_len=%d", read_size);
            flag_2 = 1;
            break;
        case ESP_ERR_NVS_NOT_FOUND:
            ESP_LOGI(TAG, "get err =0x%x", err);
            ESP_LOGI(TAG, "Get password fail!");
            break;
        default :
            ESP_LOGI(TAG, "Get password fail!");
            break;
    }   

    nvs_close(my_handle);
    return flag & flag_2;
}
void Set_nvs_wifi(char *ssid, char *password)
{
    esp_err_t err;
    err = nvs_open("wificonfig", NVS_READWRITE, &my_handle);
    if (err != ESP_OK)
    {
        ESP_LOGI(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
    }
    else
    {
        ESP_LOGI(TAG, "Set ssid and pass to NVS ... ");
        err = nvs_set_str(my_handle, "ssid", ssid);
        if (err == ESP_OK) ESP_LOGI(TAG, "set ssid success!");
        else ESP_LOGI(TAG, "set ssid fail!");
        err = nvs_set_str(my_handle, "password", password);
        if (err == ESP_OK) ESP_LOGI(TAG, "set password success!");
        else ESP_LOGI(TAG, "set password fail!");
        err = nvs_commit(my_handle);
        if (err == ESP_OK)
            ESP_LOGI(TAG, "nvs_commit Done");
        else
            ESP_LOGI(TAG, "nvs_commit Failed!");
    }
    nvs_close(my_handle);
}
