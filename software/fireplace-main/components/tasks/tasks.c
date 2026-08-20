#include <stdio.h>
#include "tasks.h"


void task_check_heat(void *args) {

}

void task_drive_output(void *args) {

}

void task_check_espnow_events(void *args) {


}

void init_tasks(void)
{
  xTaskCreate(task_check_heat, "heat", 4096, NULL, 6, NULL);
  xTaskCreate(task_drive_output, "output", 4096, NULL, 6, NULL);
  xTaskCreate(task_check_espnow_events, "espnow", 4096, NULL, 6, NULL);
}
