#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <inttypes.h>
#include "freertos/projdefs.h"
#include "ssd1306.h"
#include "esp_sleep.h"
#include "esp_timer.h"
#include "io.h"

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
  static int set_temp = 72; //Farenheit default
  static enum TEMP_FORMAT temp_format = FARENHEIT;
  static bool on = false;
  int64_t now;
  int64_t deadline;
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
      }
    } 

    if (now > deadline) {
      esp_deep_sleep_start();
    }
    vTaskDelay(pdMS_TO_TICKS(1));
  }
}
