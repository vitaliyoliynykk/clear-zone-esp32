#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "tasks.h"
#include "http_manager.h"
#include "ble_manager.h"
#include "mqtt_setup.h"
#include "utils.h"
#include "SHT31.h"
#include "SCD41.h"
#include "PMS5003.h"
#include "BME680.h"
#include <array>

const uint16_t READING_INTERVAL = 60000;
const uint16_t PMS_POLLING_INTERVAL = 1000;

const uint8_t AUTH_QUEUE_LEN = 1;
const uint8_t SENSORS_QUEUE_LEN = 1;
const uint8_t PMS_POLLING_QUEUE_LEN = 1;
const uint8_t SHT31_QUEUE_LEN = 4;
const uint8_t C02_QUEUE_LEN = 1;
const uint8_t PMS_QUEUE_LEN = 4;
const uint8_t BME_QUEUE_LEN = 4;
const uint8_t QUEUE_SET_LEN = SHT31_QUEUE_LEN + C02_QUEUE_LEN + PMS_QUEUE_LEN + BME_QUEUE_LEN;

QueueHandle_t authQueue;
QueueHandle_t enableSensorsQueue;
QueueHandle_t enablePmsPollingQueue;
QueueHandle_t sht31Queue;
QueueHandle_t pmsQueue;
QueueHandle_t bmeQueue;
QueueHandle_t co2Queue;

QueueSetHandle_t sensorQueueSet = xQueueCreateSet(QUEUE_SET_LEN);

void MqttTask(void *param);
void AuthTask(void *param);
void SensorsTask(void *param);
void PmsPollingTask(void *param);

void initRTOS()
{
    authQueue = xQueueCreate(AUTH_QUEUE_LEN, sizeof(uint8_t));
    enableSensorsQueue = xQueueCreate(SENSORS_QUEUE_LEN, sizeof(uint8_t));
    enablePmsPollingQueue = xQueueCreate(PMS_POLLING_QUEUE_LEN, sizeof(uint8_t));
    sht31Queue = xQueueCreate(SHT31_QUEUE_LEN,
                              sizeof(std::array<float, 2>));
    pmsQueue = xQueueCreate(PMS_QUEUE_LEN,
                            sizeof(std::array<float, 3>));
    co2Queue = xQueueCreate(C02_QUEUE_LEN, sizeof(float));
    bmeQueue = xQueueCreate(BME_QUEUE_LEN,
                            sizeof(std::array<float, 2>));

    xQueueAddToSet(sht31Queue, sensorQueueSet);
    xQueueAddToSet(co2Queue, sensorQueueSet);
    xQueueAddToSet(pmsQueue, sensorQueueSet);
    xQueueAddToSet(bmeQueue, sensorQueueSet);

    xTaskCreatePinnedToCore(AuthTask, "AuthTask", 4096, nullptr, 1, nullptr, 1);
    xTaskCreatePinnedToCore(SensorsTask, "SensorsTask", 2048, nullptr, 1, nullptr, 1);
    xTaskCreatePinnedToCore(MqttTask, "MqttTask", 4096, nullptr, 1, nullptr, 1);
    xTaskCreatePinnedToCore(PmsPollingTask, "PmsPollingTask", 2048, nullptr, 1, nullptr, 1);
}

void AuthTask(void *param)
{
    uint8_t msg;
    for (;;)
    {
        if (xQueueReceive(authQueue, &msg, portMAX_DELAY) == pdTRUE)
        {
            bool result = handleAuthInitialization();
            sendAuthResult(result);
        }
    }
}

void MqttTask(void *param)
{
    QueueSetMemberHandle_t activated;
    char payload[84];

    for (;;)
    {
        mqtt.loop();
        activated = xQueueSelectFromSet(sensorQueueSet, pdMS_TO_TICKS(100));

        if (activated == sht31Queue)
        {
            std::array<float, 2> msg;
            if (xQueueReceive(sht31Queue, &msg, 0))
            {
                snprintf(
                    payload,
                    sizeof(payload),
                    "{\"temp\":%.2f,\"hum\":%.2f}",
                    msg[0],
                    msg[1]);

                mqtt.publish(tempTopic.c_str(), payload);
            }
        }
        else if (activated == co2Queue)
        {
            float co2Reding;

            if (xQueueReceive(co2Queue, &co2Reding, 0))
            {
                snprintf(
                    payload,
                    sizeof(payload),
                    "{\"co2\":%.2f}",
                    co2Reding);

                mqtt.publish(co2Topic.c_str(), payload);
            }
        }
        else if (activated == pmsQueue)
        {
            std::array<float, 3> pmsReading;

            if (xQueueReceive(pmsQueue, &pmsReading, 0))
            {
                snprintf(
                    payload,
                    sizeof(payload),
                    "{\"pm10\":%.2f,\"pm25\":%.2f,\"pm100\":%.2f}",
                    pmsReading[0], pmsReading[1], pmsReading[2]);

                mqtt.publish(pmsTopic.c_str(), payload);
            }
        }
        else if (activated == bmeQueue)
        {
            std::array<float, 2> bmeReading;

            if (xQueueReceive(bmeQueue, &bmeReading, 0))
            {
                snprintf(
                    payload,
                    sizeof(payload),
                    "{\"gas_resistance\":%.2f,\"sensor_humidity\":%.2f,\"uptime_seconds\":%d}",
                    bmeReading[0], bmeReading[1], getBmeUptimeSec());

                mqtt.publish(bmeTopic.c_str(), payload);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void SensorsTask(void *param)
{
    uint8_t msg;

    WindowAgg co2Agg;

    WindowAgg tempAgg;
    WindowAgg humAgg;

    WindowAgg gasResistandeAgg;
    WindowAgg gasSensorHumidityAgg;

    WindowAgg pm10Agg;
    WindowAgg pm25Agg;
    WindowAgg pm100Agg;

    unsigned long readingWindowStart;

    if (xQueueReceive(enableSensorsQueue, &msg, portMAX_DELAY) == pdTRUE)
    {
        readingWindowStart = millis();
        for (;;)
        {
            if (millis() - readingWindowStart >= READING_INTERVAL)
            {
                float co2Avg = co2Agg.avg();

                float tempAvg = tempAgg.avg();
                float humAvg = humAgg.avg();

                float pm10Avg = pm10Agg.avg();
                float pm25Avg = pm25Agg.avg();
                float pm100Avg = pm100Agg.avg();

                float gasResistanceAvg = gasResistandeAgg.avg();
                float gasSensorHumidityAvg = gasSensorHumidityAgg.avg();

                std::array<float, 2> sht31Avg = {tempAvg, humAvg};
                std::array<float, 3> pmsAvg = {pm10Avg, pm25Avg, pm100Avg};
                std::array<float, 2> bmeAvg = {gasResistanceAvg, gasSensorHumidityAvg};

                xQueueSend(co2Queue, &co2Avg, 0);
                xQueueSend(sht31Queue, &sht31Avg, 0);
                xQueueSend(pmsQueue, &pmsAvg, 0);
                if (getIsBMEReady())
                {
                    xQueueSend(bmeQueue, &bmeAvg, 0);
                }

                co2Agg.reset();
                tempAgg.reset();
                humAgg.reset();
                pm10Agg.reset();
                pm25Agg.reset();
                pm100Agg.reset();

                readingWindowStart = millis();
            }
            std::array<uint16_t, 3> pmReading;
            std::pair<uint32_t, float> bmeReading;

            auto sht31Reading = mesureSht31();
            uint16_t co2Reading = readCO2();

            if (readPMS(pmReading))
            {
                pm10Agg.add(pmReading[0]);
                pm25Agg.add(pmReading[1]);
                pm100Agg.add(pmReading[2]);
            }

            if (getIsBMEReady() && readGas(bmeReading))
            {
                uint32_t gasResistance = bmeReading.first;
                float humidity = bmeReading.second;

                gasResistandeAgg.add(gasResistance);
                gasSensorHumidityAgg.add(humidity);
            }

            tempAgg.add(sht31Reading[0]);
            humAgg.add(sht31Reading[1]);
            co2Agg.add(co2Reading);

            vTaskDelay(pdMS_TO_TICKS(5000));
        }
    }
}

void PmsPollingTask(void *param)
{
    uint8_t msg;
    if (xQueueReceive(enablePmsPollingQueue, &msg, portMAX_DELAY) == pdTRUE)
    {
        for (;;)
        {
            pollPMS();
            vTaskDelay(pdMS_TO_TICKS(250));
        }
    }
}