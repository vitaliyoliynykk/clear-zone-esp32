#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

// Internal task wiring; application callers use tasks.h.
extern QueueHandle_t authQueue;
extern QueueHandle_t enableSensorsQueue;
extern QueueHandle_t enablePmsPollingQueue;
extern QueueHandle_t sht31Queue;
extern QueueHandle_t pmsQueue;
extern QueueHandle_t bmeQueue;
extern QueueHandle_t co2Queue;
extern QueueSetHandle_t sensorQueueSet;

void MqttTask(void *param);
void AuthTask(void *param);
void SensorsTask(void *param);
void PmsPollingTask(void *param);

bool isAuthInProgress();
// Called synchronously by AuthTask, preserving the existing shared retry state.
void resetRetriesAfterAuthSuccess();
