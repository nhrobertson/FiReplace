#ifndef LINK_H
#define LINK_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_mac.h"
#include "esp_crc.h"
#include "esp_now.h"
#include "esp_wifi.h"
#include "esp_random.h"
#include "config.h"

//Largely taken/influenced by: https://github.com/espressif/esp-idf/blob/master/examples/wifi/espnow/main/espnow_example_main.c

#if CONFIG_ESPNOW_WIFI_MODE_STATION
#define ESPNOW_WIFI_MODE WIFI_MODE_STA
#define ESPNOW_WIFI_IF   WIFI_IF_STA
#else
#define ESPNOW_WIFI_MODE WIFI_MODE_AP
#define ESPNOW_WIFI_IF   WIFI_IF_AP
#endif

#define ESPNOW_MAXDELAY 512

#define IS_BROADCAST_ADDR(addr) (memcmp(addr, s_example_broadcast_mac, ESP_NOW_ETH_ALEN) == 0)

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
  espnow_event_type_info_t info;
} espnow_event_t;



void init_link(void);
void link_peer(uint8_t *mac_addr);
void task_espnow_recv(void *pvParameter);


#endif //LINK_H
