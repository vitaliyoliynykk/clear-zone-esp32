#pragma once
#include <Arduino.h>

extern QueueHandle_t authQueue;
extern QueueHandle_t enableSensorsQueue;
extern QueueHandle_t enablePmsPollingQueue;
extern QueueHandle_t sht31Queue;
extern QueueHandle_t co2Queue;

void initRTOS();
bool requestAuth();
