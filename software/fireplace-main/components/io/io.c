#include <stdio.h>
#include "io.h"
#include "include/io.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define DEBOUNCE_SAMPLES 5

#define OUT_IN1_GPIO ((gpio_num_t)CONFIG_FIREPLACE_OUT_IN1_GPIO)
#define OUT_IN2_GPIO ((gpio_num_t)CONFIG_FIREPLACE_OUT_IN2_GPIO)

static void debounce_input(debounce_t *db, int raw) {
  if (!db->init) {
    db->candidate = db->stable = raw;
    db->count = DEBOUNCE_SAMPLES;
    db->init = true;
  }

  if (raw != db->candidate) {
    db->candidate = raw;
    db->count = 1;
  } 
  else if (db->count < DEBOUNCE_SAMPLES) {
    if (++db->count == DEBOUNCE_SAMPLES) {
      db->stable = raw;
    }
  }
}

//Raw read
esp_err_t read_gpio(gpio_num_t GPIO_NUM, int *level) {
  esp_err_t ret;
  *level = gpio_get_level(GPIO_NUM);
  //ESP_LOGI(TAG, "Read GPIO: %d to %d", GPIO_NUM, (int)level);

  if (*level == -1) {
    ret = ESP_ERR_INVALID_RESPONSE;
  } else {
    ret = ESP_OK;
  }

  return ret;
}

//Auto uses debounce logic
esp_err_t read_input(gpio_t *gpio) {
  esp_err_t ret;
  int raw = 0;
  ret = read_gpio(gpio->GPIO_NUM, &raw);
  debounce_input(&gpio->db, raw);

  gpio->state = gpio->db.stable;
  return ret;
}

esp_err_t init_output(void) {
  gpio_config_t out_conf = {
    .pin_bit_mask = (1ULL << OUT_IN1_GPIO) | (1ULL << OUT_IN2_GPIO),
    .mode = GPIO_MODE_OUTPUT,
    .pull_up_en = GPIO_PULLUP_DISABLE,
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    .intr_type = GPIO_INTR_DISABLE,
  };
  ESP_ERROR_CHECK(gpio_set_level(OUT_IN1_GPIO, 0));
  ESP_ERROR_CHECK(gpio_set_level(OUT_IN2_GPIO, 0));
  return gpio_config(&out_conf);
}

esp_err_t pulse_output(bool on) {
  ESP_ERROR_CHECK(gpio_set_level(OUT_IN1_GPIO, on ? 1 : 0));
  ESP_ERROR_CHECK(gpio_set_level(OUT_IN2_GPIO, on ? 0 : 1));
  vTaskDelay(pdMS_TO_TICKS(CONFIG_FIREPLACE_PULSE_MS));
  ESP_ERROR_CHECK(gpio_set_level(OUT_IN1_GPIO, 0));
  return gpio_set_level(OUT_IN2_GPIO, 0);
}
