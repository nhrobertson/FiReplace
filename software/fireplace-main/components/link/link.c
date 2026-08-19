#include <stdio.h>
#include "link.h"

static void espnow_recv_callback(const uint8_t *mac_addr, const uint8_t *data, int len) {
  //Only act if the even is of espnow
  
} 

void init_wifi(void) {
  esp_event_loop_create_default();

  wifi_init_config_t wifi_cfg = WIFI_INIT_CONFIG_DEFAULT();

  ESP_ERROR_CHECK(esp_wifi_init(&cfg));
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
  ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
  ESP_ERROR_CHECK(esp_wifi_start());
}

void init_link(void)
{
  espnow_storage_init();
  init_wifi();

  espnow_config_t espnow_config = ESPNOW_INIT_CONFIG_DEFAULT();
  espnow_init(&espnow_config);

  esp_now_register_recv_cb(espnow_recv_callback); //main device only recieves, no need to register a sender callback
}
