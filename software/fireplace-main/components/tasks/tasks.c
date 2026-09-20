#include <stdio.h>
#include "tasks.h"
#include "io.h"
#include "esp_log.h"
#include "esp_timer.h"

#define SENSOR_TIMEOUT_US ((int64_t)CONFIG_FIREPLACE_SENSOR_TIMEOUT_SECONDS * 1000000)
#define MAX_ON_US         ((int64_t)CONFIG_FIREPLACE_MAX_ON_SECONDS * 1000000)

bool override = false;
bool sensor_stale = true;
bool timed_out = false;
bool state = false;
float old_temp = 0;
int64_t on_since_us = 0;

void task_check_heat(void *args) {
  for(;;) {
    int64_t now = esp_timer_get_time();

    sensor_stale = last_sensor_us == 0 || now - last_sensor_us > SENSOR_TIMEOUT_US;

    if (!on_cmd_state) {
      on_since_us = 0;
      timed_out = false;
    } else if (on_since_us == 0) {
      on_since_us = now;
    } else if (!timed_out && now - on_since_us > MAX_ON_US) {
      timed_out = true;
      ESP_LOGW("FAILSAFE", "Max on time reached, forcing off");
    }

    if (current_temp >= temp_set) {
      override = true;
    } else if (current_temp <= temp_set - CONFIG_FIREPLACE_TEMP_HYSTERESIS) {
      override = false;
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void task_drive_output(void *args) {
  ESP_ERROR_CHECK(init_output());
  ESP_ERROR_CHECK(pulse_output(false));
  ESP_LOGI("OUTPUT", "Boot off pulse done");

  for(;;) {
    bool target = on_cmd_state && !override && !sensor_stale && !timed_out;
    if (state != target) {
      ESP_ERROR_CHECK(pulse_output(target));
      ESP_LOGI("OUTPUT", "Pulsed %s", target ? "on" : "off");
      state = target;
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void task_eval_temp(void *args) {
  
  for (;;) {
    xEventGroupWaitBits(g_events, TEMP_DATA_RECVD, true, true, portMAX_DELAY);
    ESP_LOGI("TASK EVAL TEMP", "EVENTGROUP DATA RECVD");
    
    
  }
}

void init_tasks(void)
{
  g_events = xEventGroupCreate(); 

  xTaskCreate(task_check_heat, "heat", 4096, NULL, 6, NULL);
  
  ESP_LOGI("TASKS", "Installed task_check_heat");
  xTaskCreate(task_drive_output, "output", 4096, NULL, 2, NULL);
  ESP_LOGI("TASKS", "Installed task_drive_output");

  link_peer(sens_mac_addr);
  link_peer(remote_mac_addr);

  xTaskCreate(task_espnow_recv, "espnow_handler", 4096, NULL, 1, NULL);
  ESP_LOGI("TASKS", "Installed task_espnow_recv");

  xTaskCreate(task_eval_temp, "eval_temp", 2048, NULL, 3, NULL);
}
