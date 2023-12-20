#include "lvgl_gui.h"

static const char *TAG = "lvgl_gui";
const uint8_t  Gamma[256] =  {0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
                                0,   0,   0,   0,   0,   0,   0,   0,   0,   1,   1,   1,   1,   1,   1,
                                1,   1,   1,   1,   1,   1,   2,   2,   2,   2,   2,   2,   2,   2,   3,
                                3,   3,   3,   3,   3,   4,   4,   4,   4,   5,   5,   5,   5,   5,   6,
                                6,   6,   6,   7,   7,   7,   8,   8,   8,   9,   9,   9,   10,  10,  10,
                                11,  11,  11,  12,  12,  13,  13,  13,  14,  14,  15,  15,  16,  16,  17,
                                17,  18,  18,  19,  19,  20,  20,  21,  21,  22,  22,  23,  24,  24,  25,
                                25,  26,  27,  27,  28,  29,  29,  30,  31,  31,  32,  33,  34,  34,  35,
                                36,  37,  38,  38,  39,  40,  41,  42,  42,  43,  44,  45,  46,  47,  48,
                                49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,
                                64,  65,  66,  68,  69,  70,  71,  72,  73,  75,  76,  77,  78,  80,  81,
                                82,  84,  85,  86,  88,  89,  90,  92,  93,  94,  96,  97,  99,  100, 102,
                                103, 105, 106, 108, 109, 111, 112, 114, 115, 117, 119, 120, 122, 124, 125,
                                127, 129, 130, 132, 134, 136, 137, 139, 141, 143, 145, 146, 148, 150, 152,
                                154, 156, 158, 160, 162, 164, 166, 168, 170, 172, 174, 176, 178, 180, 182,
                                184, 186, 188, 191, 193, 195, 197, 199, 202, 204, 206, 209, 211, 213, 215,
                                218, 220, 223, 225, 227, 230, 232, 235, 237, 240, 242, 245, 247, 250, 252,
                                255};
extern EventGroupHandle_t s_gui_event_group;
SemaphoreHandle_t xGuiSemaphore;
SemaphoreHandle_t xGuiSemaphore_2;
static lv_img_dsc_t IMG1 = {
    .header.cf = LV_IMG_CF_RAW,
    .header.always_zero = 0,
    .header.reserved = 0,
    .header.w = 0,
    .header.h = 0,
    .data_size = 0,
    .data = NULL,
};
/*static bool notify_lvgl_flush_ready(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx)
{
    lv_disp_drv_t *disp_driver = (lv_disp_drv_t *)user_ctx;
    lv_disp_flush_ready(disp_driver);
    return false;
}*/
static void lvgl_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map)
{
   //esp_lcd_panel_handle_t panel_handle = (esp_lcd_panel_handle_t)drv->user_data;
 //   int offsetx1 = area->x1;
 //   int offsetx2 = area->x2;
 //   int offsety1 = area->y1;
//    int offsety2 = area->y2;
 //   ESP_LOGI("DATA", "rgb:%d",(color_map[1024]).full);
     lv_disp_flush_ready(drv);
    for(unsigned int i = 0 ;i<2048;i++)
    {
        led_strip_pixels[i*3+1] = Gamma[(((color_map[i]).full  >> 16 ) &0xFF)]>>3;
        led_strip_pixels[i*3] =  Gamma[(((color_map[i]).full  >> 8 ) &0xFF )]>>3;
        led_strip_pixels[i*3+2] =Gamma[ ((color_map[i]).full & 0xFF)]>>3;
    /*    led_strip_pixels[i*3+1] = (((color_map[i]).full) >> 11) << 3;
        led_strip_pixels[i*3] = ((((color_map[i]).full) << 5) >> 10) << 2;
        led_strip_pixels[i*3+2] = ((((color_map[i]).full) << 11) >> 11) << 3;*/
    }
    rmt_sync_reset(synchro);
    ESP_ERROR_CHECK(rmt_transmit(led_channels[0], led_encoder[0], led_strip_pixels+1536*3,512*3, &tx_config));
    ESP_ERROR_CHECK(rmt_transmit(led_channels[1], led_encoder[1], led_strip_pixels+1024*3,512*3,&tx_config));
    ESP_ERROR_CHECK(rmt_transmit(led_channels[2], led_encoder[2], led_strip_pixels+512*3, 512*3,&tx_config));
    ESP_ERROR_CHECK(rmt_transmit(led_channels[3], led_encoder[3], led_strip_pixels,       512*3, &tx_config));
}
static void increase_lvgl_tick(void *arg)
{
    /* Tell LVGL how many milliseconds has elapsed */
    lv_tick_inc(LVGL_TICK_PERIOD_MS);
}

void  guiTask(void *pvParameter)
{
   // vTaskDelay(pdMS_TO_TICKS(500));
    xGuiSemaphore = xSemaphoreCreateMutex();
    static lv_disp_draw_buf_t disp_buf; // contains internal graphic buffer(s) called draw buffer(s)
    static lv_disp_drv_t disp_drv;      // contains callback functions
    ESP_LOGI(TAG, "Initialize LVGL library");
    lv_init();
    lv_color_t *buf1 = NULL;
    lv_color_t *buf2 = NULL;
    // buf1 = heap_caps_aligned_alloc(PSRAM_DATA_ALIGNMENT, LCD_H_RES * LINES * sizeof(lv_color_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    // buf2 = heap_caps_aligned_alloc(PSRAM_DATA_ALIGNMENT, LCD_H_RES * LINES * sizeof(lv_color_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    buf1 = heap_caps_malloc(LCD_H_RES * LINES * sizeof(lv_color_t), MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    buf2 = heap_caps_malloc(LCD_H_RES * LINES * sizeof(lv_color_t), MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    assert(buf1);
    assert(buf2);
    ESP_LOGI(TAG, "buf1@%p, buf2@%p", buf1, buf2);
    // initialize LVGL draw buffers
    lv_disp_draw_buf_init(&disp_buf, buf1, buf2, LCD_H_RES * LINES);
    ESP_LOGI(TAG, "Register display driver to LVGL");
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = LCD_H_RES;
    disp_drv.ver_res = LCD_V_RES;
    disp_drv.flush_cb = lvgl_flush_cb;
    disp_drv.full_refresh = 1;
    disp_drv.draw_buf = &disp_buf;
    lv_disp_t *disp = lv_disp_drv_register(&disp_drv);
    lv_disp_drv_register(&disp_drv);
    lv_fs_fatfs_init();
/*
    ESP_LOGI(TAG, "Register touch driver to LVGL");
    static lv_indev_drv_t indev_drv;    // Input device driver (Touch)
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.disp = disp;
    indev_drv.read_cb = example_lvgl_touch_cb;
    indev_drv.user_data = tp;
    lv_indev_drv_register(&indev_drv);
*/
    ESP_LOGI(TAG, "Install LVGL tick timer");
    const esp_timer_create_args_t lvgl_tick_timer_args = {
        .callback = &increase_lvgl_tick,
        .name = "lvgl_tick"};
    esp_timer_handle_t lvgl_tick_timer = NULL;
    ESP_ERROR_CHECK(esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(lvgl_tick_timer, LVGL_TICK_PERIOD_MS * 1000));
  /*lv_fs_file_t f;
    lv_fs_res_t res;
	uint32_t readbytes;
	char * buf=heap_caps_malloc(1024,MALLOC_CAP_SPIRAM);
    memset(buf, 0, 1024);
    lv_fs_dir_open(&f,"S:/");
    res = lv_fs_dir_read(&f,(char *)buf);
	ESP_LOGI(TAG,"lv_fs_dir_read   res : %d Content : %s",res,(char *)buf);
	res = lv_fs_dir_close(&f);
    ESP_ERROR_CHECK(lv_fs_open(&f, "S:/hello.txt", LV_FS_MODE_WR));
    lv_fs_write(&f,"ADASDSADSADSADSADSADSADSADN你哦好阿德撒旦",1,&readbytes);
    lv_fs_close(&f);
    ESP_ERROR_CHECK(lv_fs_open(&f, "S:/hello.txt", LV_FS_MODE_RD));
    lv_fs_read(&f, buf, 1024, &readbytes);
	ESP_LOGI(TAG,"lv_fs_read ReadByte : %ld Content : %s",readbytes,buf);
*/
   // lv_demo_benchmark_set_max_speed(true);
  // lv_demo_benchmark();
   // lv_demo_widgets();
    /*FILE *pFile;
    long lSize;

    pFile = fopen("/sdcard/640.jpg", "rb");
    ESP_LOGI(TAG, "open file");
    //获取文件大小 
    fseek(pFile, 0, SEEK_END);
    lSize = ftell(pFile);
    rewind(pFile);

    uint8_t *buffer = heap_caps_malloc(lSize, MALLOC_CAP_SPIRAM);

    //将文件拷贝到buffer中
    fread(buffer, 1, lSize, pFile);
    fclose(pFile);

    IMG1.data_size = lSize;
    IMG1.data = buffer;

    ESP_LOG_BUFFER_HEX(TAG, buffer, 10);*/

    //ui_Screen1_screen_init(scr);
   // lv_demo_benchmark();
   // lv_demo_music();
    //lv_demo_keypad_encoder();
    //lv_demo_stress();
   // lv_demo_widgets();
    ui_init();
   /* lv_obj_t * ui_Label2= lv_label_create(ui_Screen1);
    lv_obj_set_width(ui_Label2, 64);
    lv_obj_set_height(ui_Label2, 32);
    lv_obj_set_x(ui_Label2, 0);
    lv_obj_set_y(ui_Label2, 16);
    lv_obj_set_align(ui_Label2, LV_ALIGN_CENTER);
    lv_label_set_long_mode(ui_Label2, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_label_set_text(ui_Label2, "buf");
    lv_obj_set_style_text_font(ui_Label2, &chinese_16, LV_PART_MAIN | LV_STATE_DEFAULT);
    */
   /* lv_obj_t *img = lv_img_create(scr);
    lv_img_set_src(img, &IMG1);
    lv_obj_set_align(img, LV_ALIGN_CENTER);*/
 /*   lv_obj_t *scr = lv_scr_act();
    lv_obj_set_style_bg_img_src(scr, &IMG1, LV_PART_MAIN);
    lv_obj_set_style_bg_img_opa(scr, LV_OPA_COVER, LV_PART_MAIN); */
    while (1)
    {
        if(1)
        {
            vTaskDelay(pdMS_TO_TICKS(5));
            lv_timer_handler();
        }
    }
}