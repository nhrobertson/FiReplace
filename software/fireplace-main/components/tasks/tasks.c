#include <stdio.h>
#include "tasks.h"

bool override = false;
bool state = false;

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

void init_tasks(void)
{
  xTaskCreate(task_check_heat, "heat", 4096, NULL, 6, NULL);
  xTaskCreate(task_drive_output, "output", 4096, NULL, 2, NULL);

  link_peer(sens_mac_addr);
  link_peer(remote_mac_addr);

  xTaskCreate(task_espnow, "espnow_handler", 4096, NULL, 1, NULL);
}
