#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <inttypes.h>
#include "freertos/projdefs.h"
#include "ssd1306.h"
#include "esp_sleep.h"
#include "esp_timer.h"
#include "esp_now.h"
#include "io.h"
#include "nvs_flash.h"

#include "link.h"

#define STATE_BTN_GPIO        GPIO_NUM_0
#define TEMP_UP_BTN_GPIO      GPIO_NUM_1
#define TEMP_DOWN_BTN_GPIO    GPIO_NUM_2
#define TEMP_FORMAT_BTN_GPIO  GPIO_NUM_3
#define TIME_TILL_SLEEP       30 //s

#define NUM_BTNS              4

static gpio_t btns[NUM_BTNS] = {
  { .GPIO_NUM = STATE_BTN_GPIO, .db = {0}, .state = 0 },
  { .GPIO_NUM = TEMP_UP_BTN_GPIO, .db = {0}, .state = 0 },
  { .GPIO_NUM = TEMP_DOWN_BTN_GPIO, .db = {0}, .state = 0 },
  { .GPIO_NUM = TEMP_FORMAT_BTN_GPIO, .db = {0}, .state = 0 },
};


enum TEMP_FORMAT {
  FARENHEIT,
  CELSIUS
} TMEP_FORMAT;

void app_main(void)
{
  esp_err_t err; 
  err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    err = nvs_flash_init();
  }

  nvs_handle nvs;

  err = nvs_open("storage", NVS_READWRITE, &nvs);

  //Static Variables
  int32_t set_temp = 0; //Farenheit default
  nvs_get_i32(nvs, "set_temp", &set_temp);

  int32_t temp_format = 0;
  nvs_get_i32(nvs, "temp_format", &temp_format);
  enum TEMP_FORMAT e_temp_format = (enum TEMP_FORMAT)temp_format;
  
  int32_t on_cmd = 0;
  nvs_get_i32(nvs, "on_cmd", &on_cmd);
  
  bool on = on_cmd ? true : false;

  //Unstatic Variables
  int64_t now;
  int64_t deadline;
  
  //ESP-NOW for communication
  init_link();

  uint64_t gpio_mask = STATE_BTN_GPIO | TEMP_UP_BTN_GPIO | TEMP_DOWN_BTN_GPIO | TEMP_FORMAT_BTN_GPIO;
  esp_deep_sleep_enable_gpio_wakeup(gpio_mask, ESP_GPIO_WAKEUP_GPIO_HIGH);
  deadline = esp_timer_get_time() + (TIME_TILL_SLEEP * 1000000ULL);
  

  for (;;) {
    now = esp_timer_get_time();
    
    for (int i = 0; i < NUM_BTNS; ++i) {
      read_input(&btns[i]);
      if (btns[i].state) {
        //Button pressed
        switch (btns[i].GPIO_NUM) {
          case (STATE_BTN_GPIO):
            on = !on;
            break;
          case (TEMP_UP_BTN_GPIO):
            set_temp++;
            break;
          case (TEMP_DOWN_BTN_GPIO):
            set_temp--;
            break;
          case (TEMP_FORMAT_BTN_GPIO):
            switch (temp_format) {
              case (FARENHEIT):
                temp_format = CELSIUS;
                break;
              case (CELSIUS):
                temp_format = FARENHEIT;
                break;
            }
            break;
          default:
            break;
        }
        deadline += (TIME_TILL_SLEEP * 1000000ULL);

        fireplace_payload_t payload = {0};
        payload.dev_id = FIREPLACE_DEV_REMOTE;
        on_cmd = on ? 1 : 0;
        payload.payload_u.remote.status = on_cmd;
        payload.payload_u.remote.temp_threshold = set_temp;

        esp_now_send(controller_mac_addr, (uint8_t *)&payload, sizeof(payload));
      }
    }

    if (now > deadline) {
      nvs_set_i32(nvs, "set_temp", set_temp);
      nvs_set_i32(nvs, "temp_format", temp_format);
      nvs_set_i32(nvs, "on_cmd", on_cmd);
      nvs_commit(nvs);
      esp_deep_sleep_start();
    }
    vTaskDelay(pdMS_TO_TICKS(1));
  }
}
