#include <stdbool.h>
#include <stdlib.h>
#include <string.h>


#include "esp_now.h"
#include "freertos/FreeRTOS.h"

#ifndef FIREPLACE_CONFIG_H
#define FIREPLACE_CONFIG_H

//DEVICE CONFIGURATION -------------------------------------------------------------------------

#define FIREPLACE_SENDER_DEV        0
#define FIREPLACE_RECIEVER_DEV      1

#define TEMP_CHANGE_THRESHOLD       20

//Include this file in all devices

#define FIREPLACE_STATUS_KEY        "fireplace_key"

#define ESPNOW_QUEUE_SIZE           6

#define FIREPLACE_DEV_ID            0 //0 - main, 1 - remote, 2 - sensor

//SHARED DATA STRUCTURES ---------------------------------------------------------------------

extern const uint8_t sens_mac_addr[ESP_NOW_ETH_ALEN];
extern const uint8_t remote_mac_addr[ESP_NOW_ETH_ALEN];
extern const uint8_t controller_mac_addr[ESP_NOW_ETH_ALEN];

#define TEMP_DATA_RECVD BIT0
extern EventGroupHandle_t g_events;

//Align with CONFIG ID
typedef enum {
  FIREPLACE_DEV_MAIN,
  FIREPLACE_DEV_REMOTE,
  FIREPLACE_DEV_SENSOR,
} FIREPLACE_DEV;

//Payloads come from the Remote and Sensor-
//Remote transmits command for I/O output, aswell as setting a temperature threshold
//Sensor transmits temp and humidity - originally floats
typedef struct payload {
  uint8_t dev_id; //<-- Needs to be valid for the FIREPLACE_DEV enum. 
  union {
    struct remote {
      uint8_t status; // 0, off, 1, on
      uint8_t temp_threshold;
    } remote;
    struct sensor {
      uint8_t temperature; //Need to clamp the temperature to a uint8_t 0-255 decode, maybe use celsius internally?
      uint8_t humidity;    //Probably won't be using this
    } sensor;
  } payload_u;
} __attribute__((packed)) fireplace_payload_t; //Forcefully ensure no padding

//DEVICE INDEPENDENT SHARED VARIABLES AND FUNCTIONS --------------------------------------------

void handle_data(uint8_t *data); //Handler function, device specific
                                 
#if FIREPLACE_DEV_ID == 0

//Fireplace Main
extern float temp_set;
extern float current_temp;
extern float current_humid;
extern bool on_cmd_state;
extern int64_t last_sensor_us;

#elif FIREPLACE_DEV_ID == 1

//Fireplace Remote
extern float temp_set;
extern bool is_faren;
extern bool on_cmd_state;

#elif FIREPLACE_DEV_ID == 2

//Fireplace Sensor
extern float current_temp;
extern float current_humid;

#else
//INVALID CONFIGURATION
#endif

#endif
