#ifndef TASKS_CONFIG_H
#define TASKS_CONFIG_H

#include <stdio.h>
#include "freertos/FreeRTOS.h"

#define HTTP_SERVER_TASK_STACK_SIZE			8192
#define HTTP_SERVER_TASK_PRIORITY			4
#define HTTP_SERVER_TASK_CORE_ID			0

extern uint32_t sensor_read;
extern SemaphoreHandle_t xSemaphore;
#endif