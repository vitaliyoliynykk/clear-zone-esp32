#include "tasks.h"
#include "tasks/task_context.h"
#include "tasks/sensor_messages.h"

const uint8_t AUTH_QUEUE_LEN = 1;
const uint8_t SENSORS_QUEUE_LEN = 1;
const uint8_t PMS_POLLING_QUEUE_LEN = 1;
const uint8_t SHT31_QUEUE_LEN = 4;
const uint8_t C02_QUEUE_LEN = 1;
const uint8_t PMS_QUEUE_LEN = 4;
const uint8_t BME_QUEUE_LEN = 4;
const uint8_t QUEUE_SET_LEN = SHT31_QUEUE_LEN + C02_QUEUE_LEN + PMS_QUEUE_LEN + BME_QUEUE_LEN;
const uint16_t MQTT_TASK_STACK_SIZE = 12288;

QueueHandle_t authQueue;
QueueHandle_t enableSensorsQueue;
QueueHandle_t enablePmsPollingQueue;
QueueHandle_t sht31Queue;
QueueHandle_t pmsQueue;
QueueHandle_t bmeQueue;
QueueHandle_t co2Queue;

QueueSetHandle_t sensorQueueSet = xQueueCreateSet(QUEUE_SET_LEN);

void initRTOS()
{
    authQueue = xQueueCreate(AUTH_QUEUE_LEN, sizeof(uint8_t));
    enableSensorsQueue = xQueueCreate(SENSORS_QUEUE_LEN, sizeof(uint8_t));
    enablePmsPollingQueue = xQueueCreate(PMS_POLLING_QUEUE_LEN, sizeof(uint8_t));
    sht31Queue = xQueueCreate(SHT31_QUEUE_LEN, sizeof(TemperatureHumidityReading));
    pmsQueue = xQueueCreate(PMS_QUEUE_LEN, sizeof(ParticulateReading));
    co2Queue = xQueueCreate(C02_QUEUE_LEN, sizeof(float));
    bmeQueue = xQueueCreate(BME_QUEUE_LEN, sizeof(BmeAverages));

    xQueueAddToSet(sht31Queue, sensorQueueSet);
    xQueueAddToSet(co2Queue, sensorQueueSet);
    xQueueAddToSet(pmsQueue, sensorQueueSet);
    xQueueAddToSet(bmeQueue, sensorQueueSet);

    xTaskCreatePinnedToCore(AuthTask, "AuthTask", 8192, nullptr, 1, nullptr, 1);
    xTaskCreatePinnedToCore(SensorsTask, "SensorsTask", 2048, nullptr, 1, nullptr, 1);
    xTaskCreatePinnedToCore(MqttTask, "MqttTask", MQTT_TASK_STACK_SIZE, nullptr, 1, nullptr, 1);
    xTaskCreatePinnedToCore(PmsPollingTask, "PmsPollingTask", 2048, nullptr, 1, nullptr, 1);
}
