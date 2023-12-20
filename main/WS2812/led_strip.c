#include "lvgl_gui.h"
static const char *TAG = "led_strip";
uint8_t led_strip_pixels[EXAMPLE_LED_NUMBERS*3];
rmt_transmit_config_t tx_config;
rmt_channel_handle_t led_channels[LED_CHANNEL_NUM] = {NULL};
rmt_encoder_handle_t led_encoder[LED_CHANNEL_NUM] =  {NULL};
rmt_sync_manager_handle_t synchro = NULL;

void led_strip_hsv2rgb(uint32_t h, uint32_t s, uint32_t v, uint32_t *r, uint32_t *g, uint32_t *b)
{
    h %= 360;
    uint32_t rgb_max = v * 2.55f;
    uint32_t rgb_min = rgb_max * (100 - s) / 100.0f;

    uint32_t i = h / 60;
    uint32_t diff = h % 60;
    uint32_t rgb_adj = (rgb_max - rgb_min) * diff / 60;

    switch (i) {
    case 0:
        *r = rgb_max;
        *g = rgb_min + rgb_adj;
        *b = rgb_min;
        break;
    case 1:
        *r = rgb_max - rgb_adj;
        *g = rgb_max;
        *b = rgb_min;
        break;
    case 2:
        *r = rgb_min;
        *g = rgb_max;
        *b = rgb_min + rgb_adj;
        break;
    case 3:
        *r = rgb_min;
        *g = rgb_max - rgb_adj;
        *b = rgb_max;
        break;
    case 4:
        *r = rgb_min + rgb_adj;
        *g = rgb_min;
        *b = rgb_max;
        break;
    default:
        *r = rgb_max;
        *g = rgb_min;
        *b = rgb_max - rgb_adj;
        break;
    }
}

void strip_init(void)
{
    ESP_LOGI(TAG, "Create RMT TX channel");
    led_strip_encoder_config_t encoder_config = {.resolution = RMT_LED_STRIP_RESOLUTION_HZ,};
    int tx_gpio_number[8] = {15,18,11,12,17,16,14,13};
    for (int i = 0; i < LED_CHANNEL_NUM; i++) {
        rmt_tx_channel_config_t tx_chan_config = {
            .clk_src = RMT_CLK_SRC_DEFAULT,       // 选择时钟源
            .gpio_num = tx_gpio_number[i],    // GPIO 编号
            .mem_block_symbols = 48,          // 内存块大小，即 64 * 4 = 256 字节
            .resolution_hz =RMT_LED_STRIP_RESOLUTION_HZ, // 1 MHz 分辨率
            .trans_queue_depth = 4,           // 设置可以在后台挂起的事务数量
        };
        ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_chan_config, &led_channels[i]));
        ESP_ERROR_CHECK(rmt_enable(led_channels[i]));
        ESP_ERROR_CHECK(rmt_new_led_strip_encoder(&encoder_config, &led_encoder[i]));
        ESP_LOGW(TAG,"Init LED Strip Channel[%d] -> GPIO:%d",i,tx_gpio_number[i]);
    }
    // 安装同步管理器
    rmt_sync_manager_config_t synchro_config = {
        .tx_channel_array = led_channels,
        .array_size = sizeof(led_channels) / sizeof(led_channels[0]),
    };
    ESP_ERROR_CHECK(rmt_new_sync_manager(&synchro_config, &synchro));
    tx_config.loop_count =0; // no transfer loop
} 