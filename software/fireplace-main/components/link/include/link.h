#ifndef LINK_H
#define LINK_H

#include <stdio.h>

#include "espnow.h"
#include "espnow_storage.h"
#include "espnow_utils.h"
#include "esp_wifi.h"
#include "config.h"

const uint8_t sens_mac_addr[ESP_NOW_ETH_ALEN];
const uint8_t remote_mac_addr[ESP_NOW_ETH_ALEN];
const uint8_t controller_mac_addr[ESP_NOW_ETH_ALEN];


typedef struct {
  uint8_t type;                           //Broadcast or unicast ESPNOW data.
  uint8_t state;                          //Indicate that if has received broadcast ESPNOW data or not.
  uint16_t seq_num;                       //Sequence number of ESPNOW data.
  uint16_t crc;                           //CRC16 value of ESPNOW data.
  uint32_t magic;                         //Magic number which is used to determine which device to send unicast ESPNOW data.
  uint8_t  payload[0];                    //Real payload of ESPNOW data.
} __attribute__((packed)) fireplace_espnow_data_t;

/* Parameters of sending ESPNOW data. */
typedef struct {
    bool unicast;                         //Send unicast ESPNOW data.
    bool broadcast;                       //Send broadcast ESPNOW data.
    uint8_t state;                        //Indicate that if has received broadcast ESPNOW data or not.
    uint32_t magic;                       //Magic number which is used to determine which device to send unicast ESPNOW data.
    uint16_t count;                       //Total count of unicast ESPNOW data to be sent.
    uint16_t delay;                       //Delay between sending two ESPNOW data, unit: ms.
    int len;                              //Length of ESPNOW data to be sent, unit: byte.
    uint8_t *buffer;                      //Buffer pointing to ESPNOW data.
    uint8_t dest_mac[ESP_NOW_ETH_ALEN];   //MAC address of destination device.
} fireplace_espnow_send_param_t;


//Payloads come from the Remote and Sensor-
//Remote transmits command for I/O output, aswell as setting a temperature threshold
//Sensor transmits temp and humidity - originally floats
typedef struct payload {
  uint8_t dev_id; //<-- Needs to be valid for the FIREPLACE_DEV enum. 
  union {
    struct remote {
      uint8_t status;
      uint8_t temp_threshold;
    } remote_payload_t;
    struct sensor {
      uint8_t temperature; //Need to clamp the temperature to a uint8_t 0-255 decode, maybe use celsius internally?
      uint8_t humidity;    //Probably won't be using this
    } sensor_payload_t;
  } payload_u;
} __attribute__((packed)) fireplace_payload_t; //Forcefully ensure no padding

typedef enum event_id {
  ESPNOW_RECV_CB,
  ESPNOW_SEND_CB
} espnow_event_type_id_t;

typedef struct send_callback {
  uint8_t mac_addr[ESP_NOW_ETH_ALEN];
  esp_now_send_status_t status;
} espnow_send_cb_t;

typedef struct recieve_callback {
  uint8_t mac_addr[ESP_NOW_ETH_ALEN];
  uint8_t *data;
  int data_len;
} espnow_recv_cb_t;

typedef union event_info {
  espnow_send_cb_t send_cb;
  espnow_recv_cb_t recv_cb;
} espnow_event_type_info_t;

typedef struct event_struct {
  espnow_event_type_id_t id;
  espnow_event_tyoe_info_t info;
} espnow_event_t

typedef enum {
  FIREPLACE_DEV_REMOTE,
  FIREPLACE_DEV_SENSOR,
  FIREPLACE_DEV_MAIN
} FIREPLACE_DEV;


void init_link(void);
void task_espnow_recv(void);


#endif //LINK_H
