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

void link_peer(uint8_t *mac_addr) {
  esp_now_peer_info_t peer = malloc(sizeof(esp_now_send_info_t));
  if (peer == NULL) {
    //error
  }
  memset(peer, 0, sizeof(esp_now_peer_info_t));
  peer->info = CONFIG_ESPNOW_CHANNEL;
  peer->ifidx = ESPNOW_WIFI_IF;
  peer->encrypt = false;
  memcpy(peer->peer_addr, mac_addr, ESP_NOW_ETH_ALEN);
  esp_now_add_peer(peer);
  free(peer);
}

fireplace_espnow_send_param_t* install_send_parameters(uint8_t *broadcaster_mac_addr) {
  fireplace_espnow_send_param_t *send_param;

  send_param = malloc(sizeof(fireplace_espnow_send_param_t));
  if (send_param == NULL) {
    //Error;
  }
  memset(send_param, 0, sizeof(fireplace_espnow_send_param_t));
  send_param->unicast = false;
  send_param->broadcase = true;
  send_param->state = 0;
  send_param->magic = esp_random();
  send_param->count = CONFIG_ESPNOW_SEND_COUNT;
  send_param->delay = CONFIG_ESPNOW_SEND_DELAY;
  send_param->len   = CONFIG_ESPNOW_SEND_LEN;
  send_param->buffer = malloc(CONFIG_ESPNOW_SEND_LEN);
  if (send_param->buffer == NULL) {
    //Error;
    free(send_param);
  }
  memcpy(send_param->dest_mac, broadcaster_mac_addr, ESP_NOW_ETH_ALEN);

  return(send_param);
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


#if FIREPLACE_RECIEVER_DEV
  esp_now_register_recv_cb(espnow_recv_callback); //main device only recieves, no need to register a sender callback
#endif

#if FIREPLACE_SENDER_DEV
  esp_now_register_send_cb(espnow_send_callback);
#endif

#if CONFIG_ESPNOW_ENABLE_POWER_SAVE
  ESP_ERROR_CHECK( esp_now_set_wake_window(CONFIG_ESPNOW_WAKE_WINDOW) );
  ESP_ERROR_CHECK( esp_wifi_connectionless_module_set_wake_interval(CONFIG_ESPNOW_WAKE_INTERVAL) );
#endif
}

void task_espnow(void *pvParameter) {
  espnow_event_t event;
  uint8_t recv_state = 0;
  uint16_t recv_seq = 0;
  uint32_t recv_magic = 0;
  bool is_broadcast = false;
  int ret;

  vTaskDelay(5000 / portTICK_PERIOD_MS);

  while(xQueueRecieve(s_espnow_queue, &event, portMAX_DELAY) == pdTRUE) {
    switch(event.id) {
      case ESPNOW_RECV_CB:
#if FIREPLACE_RECIEVER_DEV
        uint8_t *recv_data = event->info.recv_cb.data;
        handle_data(recv_data); //Device Specific, include from an intermediatery spot
        break;
#else
        //If it is not a reciever device and recieved a packet, ignore
        break;
#endif
      case ESPNOW_SEND_CB:
#if FIREPLACE_SENDER_DEV
        //Acknowledges that the data has been recieved, if needed, send more data.
        
        //Example handling broadcast vs unicast
        espnow_send_cb_t *send_cb = &event.info.send_cb;
        is_broadcast = IS_BROADCAST_ADDR(send_cb->mac_addr);

        if (is_broadcast && (send_param->broadcast == false)) {
          //Was a broadcast message, no need to 
          break;
        }

        if (!is_broadcast) {
          send_param->count--;
          if (send_param->count == 0) {
            ESP_LOGI(TAG, "Send done");
            example_espnow_deinit(send_param);
            vTaskDelete(NULL);
          }
        }
        break;
#else
        break;
#endif
    }
  }
}
