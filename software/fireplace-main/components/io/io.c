#include <stdio.h>
#include "io.h"
#include "include/io.h"

#define DEBOUNCE_SAMPLES 5

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
