#ifndef TASKS_H
#define TASKS_H

#include "FreeRTOS/FreeRTOS.h"

void task_check_heat(void *args);
void task_drive_output(void *args);

#endif //TASKS_H
