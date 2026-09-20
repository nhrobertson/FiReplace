#ifndef IO_H
#define IO_H

#include "driver/gpio.h"
#include "esp_err.h"
#include <inttypes.h>
#include <stdbool.h>

typedef struct debounce_t {
  int candidate;
  uint8_t count;
  int stable;
  bool init;
} debounce_t;

typedef struct gpio_t {
  gpio_num_t GPIO_NUM;
  debounce_t db;
  int state;
} gpio_t;

//Raw read
esp_err_t read_gpio(gpio_num_t GPIO_NUM, int *level);

//Auto uses debounce logic
esp_err_t read_input(gpio_t *gpio);

esp_err_t init_output(void);
esp_err_t pulse_output(bool on);


#endif //IO_H
