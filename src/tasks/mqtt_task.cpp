#include <Arduino.h>
#include "tasks.h"
#include "utils.h"
#include "task_context.h"
#include "sensor_messages.h"
#include "retry_backoff.h"
#include "mqtt_setup.h"
#include "wifi_manager.h"
#include "preferences_manager.h"
#include "BME680.h"

namespace
{
    constexpr uint32_t WIFI_RECONNECT_INITIAL_MS = 5000;
    constexpr uint32_t WIFI_RECONNECT_MAX_MS = 60000;
    constexpr uint32_t MQTT_RECONNECT_INITIAL_MS = 2000;
    constexpr uint32_t MQTT_RECONNECT_MAX_MS = 30000;
    constexpr uint32_t AUTH_RECONNECT_INITIAL_MS = 5000;
    constexpr uint32_t AUTH_RECONNECT_MAX_MS = 60000;
    constexpr TickType_t CONNECTION_POLL_DELAY = pdMS_TO_TICKS(250);

    bool sensorsStarted = false;
    bool wifiClientReady = false;
    RetryBackoff wifiRetry{WIFI_RECONNECT_INITIAL_MS, WIFI_RECONNECT_MAX_MS};
    RetryBackoff mqttRetry{MQTT_RECONNECT_INITIAL_MS, MQTT_RECONNECT_MAX_MS};
    RetryBackoff authRetry{AUTH_RECONNECT_INITIAL_MS, AUTH_RECONNECT_MAX_MS};

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

        if (!wifiRetry.isDue(now))
        {
            return false;
        }

        Serial.println("[WIFI] Attempting reconnect");

        if (autoconnectToWifi())
        {
            wifiRetry.reset();
            mqttRetry.setDeadline(0);
            return true;
        }

        Serial.printf("[WIFI] Reconnect failed, status=%d\n", WiFi.status());
        wifiRetry.schedule(now);
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

        if (isAuthInProgress() || !authRetry.isDue(now))
        {
            return false;
        }

        if (requestAuth())
        {
            Serial.println("[AUTH] Requested MQTT credentials");
            authRetry.schedule(now);
        }

        return false;
    }

    void requestAuthRefresh(uint32_t now)
    {
        if (requestAuth())
        {
            Serial.println("[AUTH] Requested credential refresh");
        }

        authRetry.schedule(now);
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
            mqttRetry.setDeadline(0);
        }

        if (!ensureMqttCreds(now) || !mqttRetry.isDue(now))
        {
            return false;
        }

        if (mqttConnect())
        {
            mqttRetry.reset();
            enableSensorTasksOnce();
            return true;
        }

        const int mqttState = mqtt.state();
        if (mqttState == MQTT_CONNECT_BAD_CREDENTIALS || mqttState == MQTT_CONNECT_UNAUTHORIZED)
        {
            requestAuthRefresh(now);
        }

        mqttRetry.schedule(now);
        return false;
    }

    void handleMqttDisconnect(uint32_t now, const char *reason)
    {
        Serial.printf("[MQTT] %s, state=%d\n", reason, mqtt.state());
        mqtt.disconnect();
        mqttRetry.setDeadline(now);
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

    void publishTemperatureHumidity()
    {
        char payload[256];
        TemperatureHumidityReading msg;
        if (xQueueReceive(sht31Queue, &msg, 0))
        {
            snprintf(
                payload,
                sizeof(payload),
                "{\"temp\":%s,\"hum\":%s}",
                jsonNumber(msg.temperature).data(),
                jsonNumber(msg.humidity).data());

            publishPayload(tempTopic, payload);
        }
    }

    void publishCo2()
    {
        char payload[256];
        float co2Reding;

        if (xQueueReceive(co2Queue, &co2Reding, 0))
        {
            snprintf(
                payload,
                sizeof(payload),
                "{\"co2\":%s}",
                jsonNumber(co2Reding).data());

            publishPayload(co2Topic, payload);
        }
    }

    void publishParticulates()
    {
        char payload[256];
        ParticulateReading pmsReading;

        if (xQueueReceive(pmsQueue, &pmsReading, 0))
        {
            snprintf(
                payload,
                sizeof(payload),
                "{\"pm10\":%s,\"pm25\":%s,\"pm100\":%s}",
                jsonNumber(pmsReading.pm1).data(),
                jsonNumber(pmsReading.pm2_5).data(),
                jsonNumber(pmsReading.pm10).data());

            publishPayload(pmsTopic, payload);
        }
    }

    void publishBme()
    {
        char payload[256];
        BmeAverages bmeReading;

        if (xQueueReceive(bmeQueue, &bmeReading, 0))
        {
            snprintf(
                payload,
                sizeof(payload),
                "{\"gas_resistance\":%lu,\"sensor_humidity\":%s,\"uptime_seconds\":%llu}",
                static_cast<unsigned long>(bmeReading.gasResistance), jsonNumber(bmeReading.sensorHumidity).data(), getBmeUptimeSec());

            publishPayload(bmeTopic, payload);
        }
    }

    void publishNextReading(TickType_t timeout)
    {
        const auto activated = xQueueSelectFromSet(sensorQueueSet, timeout);

        if (activated == sht31Queue)
        {
            publishTemperatureHumidity();
        }
        else if (activated == co2Queue)
        {
            publishCo2();
        }
        else if (activated == pmsQueue)
        {
            publishParticulates();
        }
        else if (activated == bmeQueue)
        {
            publishBme();
        }
    }
} // namespace

void resetRetriesAfterAuthSuccess()
{
    authRetry.reset();
    mqttRetry.setDeadline(0);
}

void MqttTask(void *param)
{
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

        publishNextReading(pdMS_TO_TICKS(100));

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
