#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "tasks.h"
#include "http_manager.h"
#include "ble_manager.h"
#include "mqtt_setup.h"
#include "wifi_manager.h"
#include "preferences_manager.h"
#include "utils.h"
#include "SHT31.h"
#include "SCD41.h"
#include "PMS5003.h"
#include "BME680.h"
#include <algorithm>
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
const uint16_t MQTT_TASK_STACK_SIZE = 12288;

QueueHandle_t authQueue;
QueueHandle_t enableSensorsQueue;
QueueHandle_t enablePmsPollingQueue;
QueueHandle_t sht31Queue;
QueueHandle_t pmsQueue;
QueueHandle_t bmeQueue;
QueueHandle_t co2Queue;

QueueSetHandle_t sensorQueueSet = xQueueCreateSet(QUEUE_SET_LEN);

namespace
{
constexpr uint32_t WIFI_RECONNECT_INITIAL_MS = 5000;
constexpr uint32_t WIFI_RECONNECT_MAX_MS = 60000;
constexpr uint32_t MQTT_RECONNECT_INITIAL_MS = 2000;
constexpr uint32_t MQTT_RECONNECT_MAX_MS = 30000;
constexpr uint32_t AUTH_RECONNECT_INITIAL_MS = 5000;
constexpr uint32_t AUTH_RECONNECT_MAX_MS = 60000;
constexpr TickType_t CONNECTION_POLL_DELAY = pdMS_TO_TICKS(250);

volatile bool authInProgress = false;
bool sensorsStarted = false;
bool wifiClientReady = false;
uint32_t nextWifiReconnectAt = 0;
uint32_t nextMqttReconnectAt = 0;
uint32_t nextAuthRetryAt = 0;
uint32_t wifiReconnectBackoffMs = WIFI_RECONNECT_INITIAL_MS;
uint32_t mqttReconnectBackoffMs = MQTT_RECONNECT_INITIAL_MS;
uint32_t authReconnectBackoffMs = AUTH_RECONNECT_INITIAL_MS;

bool isRetryDue(uint32_t now, uint32_t retryAt)
{
    return static_cast<int32_t>(now - retryAt) >= 0;
}

void resetReconnect(uint32_t &retryAt, uint32_t &backoffMs, uint32_t initialMs)
{
    retryAt = 0;
    backoffMs = initialMs;
}

void scheduleReconnect(uint32_t now, uint32_t &retryAt, uint32_t &backoffMs, uint32_t maxMs)
{
    retryAt = now + backoffMs;
    backoffMs = std::min(backoffMs * 2U, maxMs);
}

void enableSensorTasksOnce()
{
    if (sensorsStarted)
    {
        return;
    }

    uint8_t msg = 1;
    xQueueSend(enableSensorsQueue, &msg, 0);
    xQueueSend(enablePmsPollingQueue, &msg, 0);
    sensorsStarted = true;
}

void resetConnectionStateAfterWifiLoss()
{
    if (mqtt.connected())
    {
        mqtt.disconnect();
    }

    wifiClientReady = false;
}

bool ensureWifiConnected(uint32_t now)
{
    if (WiFi.status() == WL_CONNECTED)
    {
        return true;
    }

    resetConnectionStateAfterWifiLoss();

    if (!isRetryDue(now, nextWifiReconnectAt))
    {
        return false;
    }

    Serial.println("[WIFI] Attempting reconnect");

    if (autoconnectToWifi())
    {
        resetReconnect(nextWifiReconnectAt, wifiReconnectBackoffMs, WIFI_RECONNECT_INITIAL_MS);
        nextMqttReconnectAt = 0;
        return true;
    }

    Serial.printf("[WIFI] Reconnect failed, status=%d\n", WiFi.status());
    scheduleReconnect(now, nextWifiReconnectAt, wifiReconnectBackoffMs, WIFI_RECONNECT_MAX_MS);
    return false;
}

bool ensureMqttCreds(uint32_t now)
{
    std::string mqttUser;
    std::string mqttPass;

    if (loadMqttCreds(mqttUser, mqttPass))
    {
        return true;
    }

    if (authInProgress || !isRetryDue(now, nextAuthRetryAt))
    {
        return false;
    }

    if (requestAuth())
    {
        Serial.println("[AUTH] Requested MQTT credentials");
        scheduleReconnect(now, nextAuthRetryAt, authReconnectBackoffMs, AUTH_RECONNECT_MAX_MS);
    }

    return false;
}

void requestAuthRefresh(uint32_t now)
{
    if (requestAuth())
    {
        Serial.println("[AUTH] Requested credential refresh");
    }

    scheduleReconnect(now, nextAuthRetryAt, authReconnectBackoffMs, AUTH_RECONNECT_MAX_MS);
}

bool ensureMqttConnected(uint32_t now)
{
    if (mqtt.connected())
    {
        return true;
    }

    if (!wifiClientReady)
    {
        setupWifiClient();
        wifiClientReady = true;
        nextMqttReconnectAt = 0;
    }

    if (!ensureMqttCreds(now) || !isRetryDue(now, nextMqttReconnectAt))
    {
        return false;
    }

    if (mqttConnect())
    {
        resetReconnect(nextMqttReconnectAt, mqttReconnectBackoffMs, MQTT_RECONNECT_INITIAL_MS);
        enableSensorTasksOnce();
        return true;
    }

    const int mqttState = mqtt.state();
    if (mqttState == MQTT_CONNECT_BAD_CREDENTIALS || mqttState == MQTT_CONNECT_UNAUTHORIZED)
    {
        requestAuthRefresh(now);
    }

    scheduleReconnect(now, nextMqttReconnectAt, mqttReconnectBackoffMs, MQTT_RECONNECT_MAX_MS);
    return false;
}

void handleMqttDisconnect(uint32_t now, const char *reason)
{
    Serial.printf("[MQTT] %s, state=%d\n", reason, mqtt.state());
    mqtt.disconnect();
    nextMqttReconnectAt = now;
}

bool publishPayload(const String &topicName, const char *payload)
{
    if (mqtt.publish(topicName.c_str(), payload))
    {
        return true;
    }

    handleMqttDisconnect(millis(), "Publish failed");
    return false;
}
} // namespace

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

    xTaskCreatePinnedToCore(AuthTask, "AuthTask", 8192, nullptr, 1, nullptr, 1);
    xTaskCreatePinnedToCore(SensorsTask, "SensorsTask", 2048, nullptr, 1, nullptr, 1);
    xTaskCreatePinnedToCore(MqttTask, "MqttTask", MQTT_TASK_STACK_SIZE, nullptr, 1, nullptr, 1);
    xTaskCreatePinnedToCore(PmsPollingTask, "PmsPollingTask", 2048, nullptr, 1, nullptr, 1);
}

bool requestAuth()
{
    if (authInProgress)
    {
        return false;
    }

    uint8_t msg = 1;
    if (xQueueSend(authQueue, &msg, 0) != pdTRUE)
    {
        return false;
    }

    authInProgress = true;
    return true;
}

void AuthTask(void *param)
{
    uint8_t msg;
    for (;;)
    {
        if (xQueueReceive(authQueue, &msg, portMAX_DELAY) == pdTRUE)
        {
            bool isRegistered = handleAuthInitialization();
            sendAuthResult(isRegistered);
            authInProgress = false;

            if (isRegistered)
            {
                resetReconnect(nextAuthRetryAt, authReconnectBackoffMs, AUTH_RECONNECT_INITIAL_MS);
                nextMqttReconnectAt = 0;
            }
        }
    }
}

void MqttTask(void *param)
{
    QueueSetMemberHandle_t activated;
    char payload[84];

    for (;;)
    {
        const uint32_t now = millis();

        if (!ensureWifiConnected(now) || !ensureMqttConnected(now))
        {
            vTaskDelay(CONNECTION_POLL_DELAY);
            continue;
        }

        if (!mqtt.loop())
        {
            handleMqttDisconnect(now, "Loop failed");
            vTaskDelay(CONNECTION_POLL_DELAY);
            continue;
        }

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

                publishPayload(tempTopic, payload);
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

                publishPayload(co2Topic, payload);
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

                publishPayload(pmsTopic, payload);
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
                    "{\"gas_resistance\":%.2f,\"sensor_humidity\":%.2f,\"uptime_seconds\":%llu}",
                    bmeReading[0], bmeReading[1], getBmeUptimeSec());

                publishPayload(bmeTopic, payload);
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
