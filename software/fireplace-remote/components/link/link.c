#include "link.h"
#include "esp_mac.h"
#include "esp_log.h"

static QueueHandle_t s_espnow_queue = NULL;
static SemaphoreHandle_t s_send_done = NULL;

static void espnow_recv_callback(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len) {
  //Only act if the even is of espnow
  espnow_event_t event = {0};
  espnow_recv_cb_t *recv_cb = &event.info.recv_cb;

  uint8_t *src_mac  = recv_info->src_addr;
  uint8_t *dest_mac = recv_info->des_addr;

  if (src_mac == NULL || data == NULL || len <= 0) {
    //Inital error
  }
 
  event.id = ESPNOW_RECV_CB;
  memcpy(recv_cb->mac_addr, src_mac, ESP_NOW_ETH_ALEN);
  recv_cb->data = malloc(len);
  if (recv_cb->data == NULL) {
    return;
  }

  memcpy(recv_cb->data, data, len); //Actual moment of copying recieved data
  recv_cb->data_len = len;
  if (xQueueSend(s_espnow_queue, &event, ESPNOW_MAXDELAY) != pdTRUE) {
    //Failure in sending event to the queue
    free(recv_cb->data);
  }
}

static void espnow_send_callback(const esp_now_send_info_t *tx_info, esp_now_send_status_t status) {
  xSemaphoreGive(s_send_done);
}

bool wait_for_send(TickType_t timeout) {
  return xSemaphoreTake(s_send_done, timeout) == pdTRUE;
}

void link_peer(uint8_t *mac_addr) {
  esp_now_peer_info_t *peer = malloc(sizeof(esp_now_peer_info_t));
  if (peer == NULL) {
    //error
  }
  memset(peer, 0, sizeof(esp_now_peer_info_t));
  peer->channel = CONFIG_ESPNOW_CHANNEL;
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
  send_param->broadcast = true;
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

  ESP_ERROR_CHECK(esp_wifi_init(&wifi_cfg));
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
  ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
  ESP_ERROR_CHECK(esp_wifi_start());
}

void init_link(void)
{
  init_wifi();
  
  s_send_done = xSemaphoreCreateBinary();
  s_espnow_queue = xQueueCreate(ESPNOW_QUEUE_SIZE, sizeof(espnow_event_t));
  if (s_espnow_queue == NULL) {
    //queue creation failed
    return;
  }

  esp_now_init();
  
  uint8_t mac[ESP_NOW_ETH_ALEN];
  esp_err_t ret = esp_read_mac(mac, ESP_MAC_WIFI_STA);

  if (ret == ESP_OK) {
      ESP_LOGI("MAC", "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  }

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

void task_espnow_recv(void *pvParameter) {
  espnow_event_t event;
  uint8_t recv_state = 0;
  uint16_t recv_seq = 0;
  uint32_t recv_magic = 0;
  bool is_broadcast = false;
  int ret;

#if FIREPLACE_SENDER_DEV 
  fireplace_espnow_send_param_t *send_param = pvParameter;
#endif
  vTaskDelay(5000 / portTICK_PERIOD_MS);

  while(xQueueReceive(s_espnow_queue, &event, portMAX_DELAY) == pdTRUE) {
    switch(event.id) {
      case ESPNOW_RECV_CB:
#if FIREPLACE_RECIEVER_DEV
        ESP_LOGI("ESPNOW", "DATA RECIEVED");
        uint8_t *recv_data = event.info.recv_cb.data;
        handle_data(recv_data); //Device Specific, include from an intermediatery spot
        break;
#else
        //If it is not a reciever device and recieved a packet, ignore
        break;
#endif
      case ESPNOW_SEND_CB:
#if FIREPLACE_SENDER_DEV
        break;
#else
        break;
#endif
    }
  }
}
