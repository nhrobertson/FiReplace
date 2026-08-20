#include "link.h"

static QueueHandle_t s_espnow_queue = NULL;

static void espnow_recv_callback(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len) {
  //Only act if the even is of espnow
  espnow_event_t event;
  espnow_recv_cb_t *recv_cb = &event.info.recv_cb;

  uint8_t *src_mac  = recv_cb->src_addr;
  uint8_t *dest_mac = recv_cb->des_addr;

  if (src_mac == NULL || data == NULL || len <= 0) {
    //Inital error
  }
  
  event.id = ESPNOW_RECV_CB;
  memcpy(recv_cb->data, data, len); //Actual moment of copying recieved data
  recv_cb->data_len = len;
  if (xQueueSend(s_espnow_queue, &event, ESPNOW_MAXDELAY) != pdTRUE) {
    //Failure in sending event to the queue
    free(recv_cb->data);
  }
}

static void espnow_send_callback(const esp_now_send_info_t *tx_info, esp_now_send_status_t status) {
  espnow_event_t event;
  espnow_send_cb_t *send_cb = &event.info.send_cb;

  if(tx_info == NULL) {
    //Invalid
    return;
  }

  event.id = ESPNOW_SEND_CB;
  memcpy(send_cb->mac_addr, tx_info->des_addr, ESP_NOW_ETH_ALEN);

  send_cb->status = status;
  if (xQueueSend(s_espnow_queue, &event, ESPNOW_MAXDELAY) != pdTRUE) {
    //Failure in sending event to the queue
  }

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
  
  s_espnow_queue = xQueueCreate(ESPNOW_QUEUE_SIZE, sizeof(espnow_event_t));
  if (s_espnow_queue == NULL) {
    //queue creation failed
    return;
  }

  espnow_config_t espnow_config = ESPNOW_INIT_CONFIG_DEFAULT();
  espnow_init(&espnow_config);

#if FIREPLACE_SENDER_DEV
  esp_now_register_send_cb(espnow_send_callback);
#endif

#if FIREPLACE_RECIEVER_DEV
  esp_now_register_recv_cb(espnow_recv_callback); //main device only recieves, no need to register a sender callback
#endif
}
