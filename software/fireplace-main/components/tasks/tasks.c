#include <stdio.h>
#include "tasks.h"
#include "esp_log.h"

bool override = false;
bool state = false;
float old_temp = 0;

void task_check_heat(void *args) {
  for(;;) {
    if (current_temp >= temp_set) {
      override = true;      
    } else {
      override = false;
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void task_drive_output(void *args) {
  for(;;) {
    if (state != on_cmd_state) {
      //TODO: Drive GPIO output, (I think it needs 6 seconds on, then off)

      state = on_cmd_state;
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
