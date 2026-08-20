#ifndef TASKS_H
#define TASKS_H

#include "FreeRTOS/FreeRTOS.h"
#include "config.h"
#include "link.h"

void init_tasks();

void task_check_heat(void *args);
void task_drive_output(void *args);
void task_check_espnow_events(void *args);

#endif //TASKS_H
