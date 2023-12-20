/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include "lvgl_gui.h"
static const char *TAG = "Touch pad";

static QueueHandle_t que_touch = NULL;
typedef struct touch_msg {
    touch_pad_intr_mask_t intr_mask;
    uint16_t pad_num;
    uint16_t pad_status;
    uint32_t pad_val;
} touch_event_t;

#define TOUCH_BUTTON_NUM    2
#define TOUCH_BUTTON_WATERPROOF_ENABLE 1
#define TOUCH_BUTTON_DENOISE_ENABLE    1
#define TOUCH_CHANGE_CONFIG            1

static const touch_pad_t button[TOUCH_BUTTON_NUM] = {
    TOUCH_PAD_NUM1,     // 'SELECT' button.
    TOUCH_PAD_NUM2,     // 'MENU' button.
    // If this pad be touched, other pads no response.
};

/*
 * Touch threshold. The threshold determines the sensitivity of the touch.
 * This threshold is derived by testing changes in readings from different touch channels.
 * If (raw_data - benchmark) > benchmark * threshold, the pad be activated.
 * If (raw_data - benchmark) < benchmark * threshold, the pad be inactivated.
 */
static const float button_threshold[TOUCH_BUTTON_NUM] = {
    0.2, // 20%.
    0.2, // 20%.
    0.2, // 20%.
    0.1, // 10%.
};

/*
  Handle an interrupt triggered when a pad is touched.
  Recognize what pad has been touched and save it in a table.
 */
static void touchsensor_interrupt_cb(void *arg)
{
    int task_awoken = pdFALSE;
    touch_event_t evt;

    evt.intr_mask = touch_pad_read_intr_status_mask();
    evt.pad_status = touch_pad_get_status();
    evt.pad_num = touch_pad_get_current_meas_channel();

    xQueueSendFromISR(que_touch, &evt, &task_awoken);
    if (task_awoken == pdTRUE) {
        portYIELD_FROM_ISR();
    }
}

static void tp_example_set_thresholds(void)
{
    uint32_t touch_value;
    for (int i = 0; i < TOUCH_BUTTON_NUM; i++) {
        touch_pad_read_benchmark(button[i], &touch_value);
        touch_pad_set_thresh(button[i], touch_value * button_threshold[i]);
        ESP_LOGI(TAG, "touch pad [%d] base %ld, thresh %d",button[i], touch_value, (uint16_t)(touch_value * button_threshold[i]));
    }
}

static void touchsensor_filter_set(touch_filter_mode_t mode)
{
    /* Filter function */
    touch_filter_config_t filter_info = {
        .mode = mode,           // Test jitter and filter 1/4.
        .debounce_cnt = 1,      // 1 time count.
        .noise_thr = 0,         // 50%
        .jitter_step = 4,       // use for jitter mode.
        .smh_lvl = TOUCH_PAD_SMOOTH_IIR_2,
    };
    touch_pad_filter_set_config(&filter_info);
    touch_pad_filter_enable();
    ESP_LOGI(TAG, "touch pad filter init");
}
void touch_init(void *pvParameter)
{
    if (que_touch == NULL) {
        que_touch = xQueueCreate(TOUCH_BUTTON_NUM, sizeof(touch_event_t));
    }
    ESP_LOGI(TAG, "Initializing touch pad");
    touch_pad_init();
    for (int i = 0; i < TOUCH_BUTTON_NUM; i++) {
        touch_pad_config(button[i]);
    }
    touchsensor_filter_set(TOUCH_PAD_FILTER_IIR_16);
    touch_pad_timeout_set(true, SOC_TOUCH_PAD_THRESHOLD_MAX);
    touch_pad_isr_register(touchsensor_interrupt_cb, NULL, TOUCH_PAD_INTR_MASK_ALL);
    touch_pad_intr_enable(TOUCH_PAD_INTR_MASK_ACTIVE | TOUCH_PAD_INTR_MASK_INACTIVE | TOUCH_PAD_INTR_MASK_TIMEOUT);
    touch_pad_set_fsm_mode(TOUCH_FSM_MODE_TIMER);
    touch_pad_fsm_start();
    vTaskDelay(100);
    touch_event_t evt = {0};
    tp_example_set_thresholds();
    while (1) {
        int ret = xQueueReceive(que_touch, &evt, (TickType_t)portMAX_DELAY);
        if (ret != pdTRUE) {
            continue;
        }
        if (evt.intr_mask & TOUCH_PAD_INTR_MASK_ACTIVE) {
            ESP_LOGI(TAG, "TouchSensor [%d] be activated, status mask 0x%x", evt.pad_num, evt.pad_status);
        }
        if (evt.intr_mask & TOUCH_PAD_INTR_MASK_INACTIVE) {
           ESP_LOGI(TAG, "TouchSensor [%d] be inactivated, status mask 0x%x", evt.pad_num, evt.pad_status);
           if(evt.pad_num==button[1])
           {
            lv_obj_add_flag(qr_code, LV_OBJ_FLAG_HIDDEN);   
         //   lv_obj_clear_flag(ui_Label1, LV_OBJ_FLAG_HIDDEN); 
           }
           if(evt.pad_num==button[0])
           {
            lv_obj_clear_flag(qr_code, LV_OBJ_FLAG_HIDDEN);  
         //   lv_obj_add_flag(ui_Label1, LV_OBJ_FLAG_HIDDEN);  
            
           }
        }
    }
}
