#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <inttypes.h>
#include <math.h>
#include "freertos/projdefs.h"
#include "driver/i2c.h"
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

#define OLED_SDA_GPIO         GPIO_NUM_4
#define OLED_SCL_GPIO         GPIO_NUM_5
#define OLED_I2C_PORT         I2C_NUM_0
#define OLED_I2C_FREQ_HZ      400000

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

//set_temp is always stored in Farenheit; convert for display when in Celsius mode
static int32_t temp_for_display(int32_t set_temp, enum TEMP_FORMAT format)
{
  if (format == CELSIUS) {
    return (int32_t)lroundf((set_temp - 32) * 5.0f / 9.0f);
  }
  return set_temp;
}

static void update_display(ssd1306_handle_t oled, int32_t set_temp, enum TEMP_FORMAT format, bool on)
{
  char temp_str[16];
  int32_t display_temp = temp_for_display(set_temp, format);
  snprintf(temp_str, sizeof(temp_str), "%ld%c", (long)display_temp, format == CELSIUS ? 'C' : 'F');

  ssd1306_clear_screen(oled, 0x00);
  ssd1306_draw_string(oled, 0, 0, (const uint8_t *)(on ? "ON" : "OFF"), 16, 1);
  ssd1306_draw_string(oled, 0, 24, (const uint8_t *)"Set Temp:", 16, 1);
  ssd1306_draw_string(oled, 0, 48, (const uint8_t *)temp_str, 16, 1);
  ssd1306_refresh_gram(oled);
}

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

  //SSD1306 display
  i2c_config_t i2c_conf = {
    .mode = I2C_MODE_MASTER,
    .sda_io_num = OLED_SDA_GPIO,
    .scl_io_num = OLED_SCL_GPIO,
    .sda_pullup_en = GPIO_PULLUP_ENABLE,
    .scl_pullup_en = GPIO_PULLUP_ENABLE,
    .master.clk_speed = OLED_I2C_FREQ_HZ,
  };
  ESP_ERROR_CHECK(i2c_param_config(OLED_I2C_PORT, &i2c_conf));
  ESP_ERROR_CHECK(i2c_driver_install(OLED_I2C_PORT, i2c_conf.mode, 0, 0, 0));
  ssd1306_handle_t oled = ssd1306_create(OLED_I2C_PORT, SSD1306_I2C_ADDRESS);
  update_display(oled, set_temp, (enum TEMP_FORMAT)temp_format, on);

  uint64_t gpio_mask = STATE_BTN_GPIO | TEMP_UP_BTN_GPIO | TEMP_DOWN_BTN_GPIO | TEMP_FORMAT_BTN_GPIO;
  esp_deep_sleep_enable_gpio_wakeup(gpio_mask, ESP_GPIO_WAKEUP_GPIO_HIGH);
  deadline = esp_timer_get_time() + (TIME_TILL_SLEEP * 1000000ULL);
  

  for (;;) {
    now = esp_timer_get_time();
    
    for (int i = 0; i < NUM_BTNS; ++i) {
      read_input(&btns[i]);
      if (btns[i].state) {
        //Button pressed
        bool display_dirty = false;
        switch (btns[i].GPIO_NUM) {
          case (STATE_BTN_GPIO):
            on = !on;
            display_dirty = true;
            break;
          case (TEMP_UP_BTN_GPIO):
            set_temp++;
            display_dirty = true;
            break;
          case (TEMP_DOWN_BTN_GPIO):
            set_temp--;
            display_dirty = true;
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
            display_dirty = true;
            break;
          default:
            break;
        }

        if (display_dirty) {
          update_display(oled, set_temp, (enum TEMP_FORMAT)temp_format, on);
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
