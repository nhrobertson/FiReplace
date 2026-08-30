#include <stdio.h>
#include "i2cdev.h"
#include "sht4x.h"
#include "nvs_flash.h"
#include "link.h"
#include "config.h"
#include "esp_err.h"
#include "esp_sleep.h"
#include "esp_log.h"
#include "esp_mac.h"

#define SDA_GPIO_NUM GPIO_NUM_4
#define SCL_GPIO_NUM GPIO_NUM_5

#define SLEEP_TIME 60 //seconds

static const char *TAG = "FiReplace - Sensor";

void app_main(void)
{
  esp_err_t nvs_ret = nvs_flash_init();
  if (nvs_ret == ESP_ERR_NVS_NO_FREE_PAGES || nvs_ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    nvs_ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(nvs_ret);
  //Inital Setup
  static float temperature = 10;
  static float humidity = 10;
  
  //static sht4x_t sht40;
  
  //ESP_ERROR_CHECK(i2cdev_init());

  //ESP_ERROR_CHECK(sht4x_init_desc(&sht40, 0, SDA_GPIO_NUM, SCL_GPIO_NUM));

  //ESP_ERROR_CHECK(sht4x_init(&sht40));
  
  ESP_ERROR_CHECK(esp_sleep_enable_timer_wakeup(SLEEP_TIME * 1000000ULL));
  
  uint8_t mac[ESP_NOW_ETH_ALEN];
  esp_err_t ret = esp_read_mac(mac, ESP_MAC_WIFI_STA);

  if (ret == ESP_OK) {
      ESP_LOGI("MAC", "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  }
  
  init_link();

  link_peer(controller_mac_addr);

  //Main Loop
  for (;;) {
    //ESP_ERROR_CHECK(sht4x_measure(&sht40, &temperature, &humitidy));
    //ESP_LOGI(TAG, "temperature: %.3f, humitidy: %.3f", temperature, humitidy);
     
    //TODO: Send uplink
    fireplace_payload_t payload = {0};
    payload.dev_id = FIREPLACE_DEV_SENSOR;
    payload.payload_u.sensor.temperature = temperature;
    payload.payload_u.sensor.humidity = humidity;

    esp_err_t err = esp_now_send(controller_mac_addr, (uint8_t *)&payload, sizeof(payload));
    if (err != ESP_OK) {
      ESP_LOGI(TAG, "ESPNOW SEND ERR");
    } else {
      ESP_LOGI(TAG, "ESPNOW SEND DATA");
    }

    esp_deep_sleep_start();
  }
}
