#include "lvgl_gui.h"


static const char *TAG = "main";

#define N_SAMPLES 1024
unsigned int N = N_SAMPLES;
// Input test array
int16_t rec_data[N_SAMPLES];
float audio_data[N_SAMPLES];
float x[N_SAMPLES];
// Window coefficients
float  wind[N_SAMPLES];
// working complex array
float fft_output[N_SAMPLES*2];
// Pointers to result arrays
unsigned char circle = 0;
float filter_arry[N_SAMPLES/2]={0};
#define SAMPLE_RATE     44100
#define SAMPLE_BITS     16
#define FFT_SIZE        N_SAMPLES
#define MAX_CIRCLE      3

uint8_t height[64];
uint8_t max_point[64];
uint8_t decrease_circle = 0;
float coeffs_lpf[5];
float w_lpf[5] = {0,0};
 
uint32_t red = 0;
uint32_t green = 0;
uint32_t blue = 0;
uint16_t hue = 0;
uint16_t start_rgb = 0;

uint8_t select_mode = 0;

void IIRfilter( )
{
    memset(led_strip_pixels, 0, sizeof(led_strip_pixels));
    dsps_biquad_f32(audio_data, x, N, coeffs_lpf, w_lpf);
    for (int i=0 ; i< N ; i++)
    {
        fft_output[i*2 + 0] = x[i];
        fft_output[i*2 + 1] = 0;
    }
    dsps_fft2r_fc32_ansi(fft_output, N);
    dsps_bit_rev_fc32_ansi(fft_output, N);
    for (int i = 0 ; i < N/2 ; i++) {
        fft_output[i] = 10 * log10f((fft_output[i * 2 + 0] * fft_output[i * 2 + 0] + fft_output[i * 2 + 1] * fft_output[i * 2 + 1])/N);
    }
    
    decrease_circle=(decrease_circle+1)%MAX_CIRCLE;
    for(unsigned char w = 0;w<64;w++)
    {
        int temp = 0;
        for(unsigned char i = 0 ;i<8;i++)
        {
           // temp = fft_output[w*16+i] > temp ? fft_output[w*16+i] : temp;
            temp += fft_output[w*8+i] ;
        }  
        temp=temp/9;
        if(temp>31)temp=31;
        if(temp>height[w])
        {
            height[w]=temp;
        }
        else if (height[w]>0)
        {
           height[w]--;
        }
       if(temp>max_point[w])max_point[w] = temp;
       else if( max_point[w]>0) max_point[w]= max_point[w]-decrease_circle/(MAX_CIRCLE-1);
       hue = w * 360 / 64 + start_rgb;
       led_strip_hsv2rgb(hue, 100,10, &red, &green, &blue);
       for(unsigned char n = 0 ;n<height[w];n++)
       {
            led_strip_pixels[(n *64+w)*3] =green;
            led_strip_pixels[(n *64+w)*3+1] =red;
            led_strip_pixels[(n *64+w)*3+2] =blue;
       }
        led_strip_pixels[(max_point[w] *64+w)*3] = 85;
        led_strip_pixels[(max_point[w] *64+w)*3+1] = 85;
        led_strip_pixels[(max_point[w] *64+w)*3+2] = 85;
    }
    start_rgb+=1;
    rmt_sync_reset(synchro);
    ESP_ERROR_CHECK(rmt_transmit(led_channels[0], led_encoder[0], led_strip_pixels+1536*3,512*3, &tx_config));
    ESP_ERROR_CHECK(rmt_transmit(led_channels[1], led_encoder[1], led_strip_pixels+1024*3,512*3,&tx_config));
    ESP_ERROR_CHECK(rmt_transmit(led_channels[2], led_encoder[2], led_strip_pixels+512*3, 512*3,&tx_config));
    ESP_ERROR_CHECK(rmt_transmit(led_channels[3], led_encoder[3], led_strip_pixels,       512*3, &tx_config));
    vTaskDelay(pdMS_TO_TICKS(15));
}

static void process(float* data, int length)
{
    dsps_fft2r_fc32(data, length);
    dsps_bit_rev_fc32(data, length);
    dsps_cplx2reC_fc32(data, length);
    if(circle<50)  
    {
       circle++;
        for (int i = 0 ; i < length/2 ; i++) {
            data[i] =10 * log10f((data[i * 2 + 0] * data[i * 2 + 0] + data[i * 2 + 1] * data[i * 2 + 1])/N);
            if(data[i] <0)filter_arry[i]=data[i]*-1;
            else filter_arry[i]=data[i] ;
        // printf("Frequency: %d Hz, Magnitude: %.2f dB\n", i * (SAMPLE_RATE / FFT_SIZE),  data[i]);
        }
    }
    else
    {
        for (int i = 0 ; i < length/2 ; i++) {
            data[i] =10 * log10f((data[i * 2 + 0] * data[i * 2 + 0] + data[i * 2 + 1] * data[i * 2 + 1])/N);
            // printf("Frequency: %d Hz, Magnitude: %.2f dB\n", i * (SAMPLE_RATE / FFT_SIZE),  data[i]);
            if(data[i]<0)data[i]*=-1;
            data[i]=data[i]- filter_arry[i];
             if(data[i]<0)data[i]*=-1;
        }
        memset(led_strip_pixels, 0, sizeof(led_strip_pixels));
        for(unsigned char w = 0;w<64;w++)
        {
            int temp = 0;
            for(unsigned char i = 0 ;i<16;i++)
            {
                temp += data[w*16+i];
            }
            temp=temp/32;
            temp=temp-3;
            if(temp>31)temp=31;
            if(temp<0)temp=0;
            if(temp>height[w])height[w]=temp;
            else if (height[w]>0)height[w]=height[w]-1;
            for(unsigned char n = 0 ;n<height[w];n++)
            {
                led_strip_pixels[(n *64+w)*3] = 25;
                led_strip_pixels[(n *64+w)*3+1] = 50;
                led_strip_pixels[(n *64+w)*3+2] = 0;
            }
        }
    rmt_sync_reset(synchro);
    ESP_ERROR_CHECK(rmt_transmit(led_channels[0], led_encoder[0], led_strip_pixels+1536*3,512*3, &tx_config));
    ESP_ERROR_CHECK(rmt_transmit(led_channels[1], led_encoder[1], led_strip_pixels+1024*3,512*3,&tx_config));
    ESP_ERROR_CHECK(rmt_transmit(led_channels[2], led_encoder[2], led_strip_pixels+512*3, 512*3,&tx_config));
    ESP_ERROR_CHECK(rmt_transmit(led_channels[3], led_encoder[3], led_strip_pixels,       512*3, &tx_config));
    }
}

void i2s_fft_task(void *pvParameters)
{
    // 配置I2S接口
    i2s_config_t i2s_config = {
        .mode = I2S_MODE_MASTER | I2S_MODE_RX ,
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT,
        .communication_format = I2S_COMM_FORMAT_I2S_MSB,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 3,
        .dma_buf_len = 1024,
        .use_apll = false,
        .tx_desc_auto_clear = false,
        .fixed_mclk = 0,
    };
    i2s_pin_config_t pin_config = {
        .bck_io_num = SCK,
        .ws_io_num = WS,
        .data_out_num = DATA_OUT,
        .data_in_num = DATA_IN,
    };
    i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
    i2s_set_pin(I2S_NUM_0, &pin_config);

    // 初始化FFT库
    esp_err_t ret;
    ESP_LOGI(TAG, "*** Start Example. ***");
    ret = dsps_fft2r_init_fc32(NULL, N_SAMPLES);
    if (ret  != ESP_OK)
    {
        ESP_LOGE(TAG, "Not possible to initialize FFT. Error = %i", ret);
        return;
    }
  // strip_init();
  //  dsps_wind_hann_f32(wind, N);
    dsps_biquad_gen_hpf_f32(coeffs_lpf, 0.0005,  0.0016);  
    memset(led_strip_pixels, 0, sizeof(led_strip_pixels));
    memset(max_point, 0, sizeof(max_point));
    while (1) {
       size_t bytes_read = 0;
        i2s_read(I2S_NUM_0,rec_data, N , &bytes_read, 1000);
        //i2s_write(I2S_NUM_0, &rec_data, N , &bytes_read, 5);
       //用基本函数将两个输入向量转换成一个复向量
        for(unsigned int  i = 0 ;i<bytes_read;i++)
        {
            audio_data[i]=rec_data[i];
           // fft_output[i*2 + 0] = audio_data[i]*wind[i];
           //fft_output[i*2 + 1] = 0;
        }
      //  dsps_mul_f32(audio_data, wind, fft_output, N, 1, 1, 2); // Multiply input array with window and store as real part
       // dsps_mulc_f32(&fft_output[1], &fft_output[1], N, 0, 2, 2); // Clear imaginary part of the complex signal
      // process(fft_output, N);
        IIRfilter();

    }
}
void sdcard_init()
{
    esp_err_t ret;
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };
    sdmmc_card_t *card;
    const char mount_point[] = MOUNT_POINT;
    ESP_LOGI(TAG, "Initializing SD card");
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    slot_config.width = 4;
    slot_config.clk = SD_CLK;
    slot_config.cmd = SD_CMD;
    slot_config.d0 = SD_D0;
    slot_config.d1 = SD_D1;
    slot_config.d2 = SD_D2;
    slot_config.d3 = SD_D3;
    slot_config.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;
    ESP_LOGI(TAG, "Mounting filesystem");
    ret = esp_vfs_fat_sdmmc_mount(mount_point, &host, &slot_config, &mount_config, &card);
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount filesystem. "
                     "If you want the card to be formatted, set the EXAMPLE_FORMAT_IF_MOUNT_FAILED menuconfig option.");
        } else {
            ESP_LOGE(TAG, "Failed to initialize the card (%s). "
                     "Make sure SD card lines have pull-up resistors in place.", esp_err_to_name(ret));
        }
        return;
    }
    ESP_LOGI(TAG, "Filesystem mounted");
    sdmmc_card_print_info(stdout, card);
}
void i2c_init()
{
    ESP_LOGI(TAG, "Initialize I2C");
    const i2c_config_t i2c_conf = {
    .mode = I2C_MODE_MASTER,
    .sda_io_num = EXAMPLE_I2C_SDA,
    .scl_io_num = EXAMPLE_I2C_SCL,
    .sda_pullup_en = GPIO_PULLUP_ENABLE,
    .scl_pullup_en = GPIO_PULLUP_ENABLE,
    .master.clk_speed = 400000,
    };
    ESP_ERROR_CHECK(i2c_param_config(EXAMPLE_I2C_NUM, &i2c_conf));  
    ESP_ERROR_CHECK(i2c_driver_install(EXAMPLE_I2C_NUM, i2c_conf.mode, 0, 0, 0));
}

void app_main()
{
  gpio_set_direction(20,GPIO_MODE_INPUT);
  gpio_set_direction(19,GPIO_MODE_INPUT);
  i2c_init();
  sdcard_init();
  gpio_set_direction(0,GPIO_MODE_INPUT);
  gpio_set_pull_mode(0,GPIO_PULLUP_ENABLE);
 /* const char *file_hello = MOUNT_POINT"/hello.txt";
  ESP_LOGI(TAG, "Opening file %s", file_hello);
  FILE *f = fopen(file_hello, "w");
  fprintf(f, "Hello %s!\n", "你好");
  fclose(f);
  ESP_LOGI(TAG, "File written");*/
  
  strip_init();
  //xTaskCreatePinnedToCore(i2s_fft_task, "i2s_fft_task", 1024 * 24, NULL, 5, NULL, 0);  
  initialise_wifi();
  xTaskCreatePinnedToCore(guiTask, "guiTask", 1024 * 12, NULL, 5, NULL,0) ;
  xTaskCreatePinnedToCore(touch_init, "touch_pad_init", 1024 * 12, NULL, 5, NULL,1) ;
}
/*
static void print_rtos_status(void *pvParameters)
{
    // char buf[1024];
    while (1)
    {
        size_t size = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
        size_t size_internal = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
        ESP_LOGW(TAG, "%d, %d", size, size_internal);
        // vTaskList(buf);
        // ESP_LOGI(TAG, "\n%s", buf);
        vTaskDelay(pdMS_TO_TICKS(1500));
    }
}
*/