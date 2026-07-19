#include <stdio.h>
#include "i2cdev.h"
#include "sht4x.h"
#include "link.h"
#include "esp_err.h"
#include "esp_sleep.h"
#include "esp_log.h"

#define SDA_GPIO_NUM GPIO_NUM_4
#define SCL_GPIO_NUM GPIO_NUM_5

#define SLEEP_TIME 60 //seconds

static const char *TAG = "FiReplace - Sensor";

void app_main(void)
{
  //Inital Setup
  static float temperature = 0;
  static float humitidy = 0;
  
  static sht4x_t sht40;
  
  ESP_ERROR_CHECK(i2cdev_init());

  ESP_ERROR_CHECK(sht4x_init_desc(&sht40, 0, SDA_GPIO_NUM, SCL_GPIO_NUM));

  ESP_ERROR_CHECK(sht4x_init(&sht40));
  
  ESP_ERROR_CHECK(esp_sleep_enable_timer_wakeup(SLEEP_TIME * 1000000ULL));
  //Main Loop
  for (;;) {
    ESP_ERROR_CHECK(sht4x_measure(&sht40, &temperature, &humitidy));
    ESP_LOGI(TAG, "temperature: %.3f, humitidy: %.3f", temperature, humitidy);
    
    //TODO: Send uplink
    
    
    esp_deep_sleep_start();
  }
}
