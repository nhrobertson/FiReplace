#include "config.h"
#include "esp_log.h"
#include "esp_timer.h"

const uint8_t sens_mac_addr[ESP_NOW_ETH_ALEN] = { 0x82, 0xfd, 0x49, 0x44, 0x3f, 0x04 };
const uint8_t remote_mac_addr[ESP_NOW_ETH_ALEN] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
const uint8_t controller_mac_addr[ESP_NOW_ETH_ALEN] = { 0x30, 0x30, 0xf9, 0x5d, 0xd3, 0xd0 };

EventGroupHandle_t g_events;

#if FIREPLACE_DEV_ID == 0

//Fireplace Main
float temp_set        = 72;
float current_temp    = 0;
float current_humid   = 0;
bool on_cmd_state     = false;
int64_t last_sensor_us = 0;

#elif FIREPLACE_DEV_ID == 1

//Fireplace Remote
float temp_set        = 72;
bool is_faren         = true;
bool on_cmd_state     = false;

#elif FIREPLACE_DEV_ID == 2

//Fireplace Sensor
float current_temp    = 0;
float current_humid   = 0;

#else
//INVALID CONFIGURATION
#endif


void handle_data(uint8_t *data) {
#if FIREPLACE_DEV_ID == 0
  fireplace_payload_t *payload = malloc(sizeof(fireplace_payload_t));
  memcpy(payload, data, sizeof(fireplace_payload_t));
  switch (payload->dev_id) {
    case FIREPLACE_DEV_MAIN:
      //Should be impossible
      break;
    case FIREPLACE_DEV_REMOTE:
      //Carries state and temp setting
      temp_set = payload->payload_u.remote.temp_threshold;
      on_cmd_state = payload->payload_u.remote.status;
      break;
    case FIREPLACE_DEV_SENSOR:
      //Carries current temperature and humidity reading
      current_temp  = payload->payload_u.sensor.temperature;
      current_humid = payload->payload_u.sensor.humidity;
      last_sensor_us = esp_timer_get_time();
      ESP_LOGI("handle_data", "temp: %f, humid: %f", current_temp, current_humid);
      xEventGroupSetBits(g_events, TEMP_DATA_RECVD);
  }
  free(payload);
#elif FIREPLACE_DEV_ID == 1
//Should not be recieving data
#elif FIREPLACE_DEV_IF == 2
//Should not be recieving data
#else 
//Should be impossible to land here
#endif
}
