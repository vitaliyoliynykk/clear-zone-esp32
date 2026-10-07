#include <Arduino.h>
#include <array>
#include <cmath>
#include "task_context.h"
#include "sensor_messages.h"
#include "utils.h"
#include "SHT31.h"
#include "SCD41.h"
#include "PMS5003.h"
#include "BME680.h"

namespace
{
    const uint16_t READING_INTERVAL = 60000;
    const uint16_t PMS_POLLING_INTERVAL = 1000;

    struct SensorWindow
    {
        WindowAgg co2Agg;

        WindowAgg tempAgg;
        WindowAgg humAgg;

        WindowAgg gasResistandeAgg;
        WindowAgg gasSensorHumidityAgg;

        WindowAgg pm10Agg;
        WindowAgg pm25Agg;
        WindowAgg pm100Agg;
    };

    void collectReadings(SensorWindow &window)
    {
        std::array<uint16_t, 3> pmReading;
        std::pair<uint32_t, float> bmeReading;

        auto sht31Reading = mesureSht31();
        Serial.println("[DEBUG] sht31Reading");
        uint16_t co2Reading = readCO2();
        Serial.println("[DEBUG] co2Reading");

        if (readPMS(pmReading))
        {
            window.pm10Agg.add(pmReading[0]);
            window.pm25Agg.add(pmReading[1]);
            window.pm100Agg.add(pmReading[2]);
        }
        Serial.println("[DEBUG] pmReading");

        if (getIsBMEReady() && readGas(bmeReading))
        {
            uint32_t gasResistance = bmeReading.first;
            float humidity = bmeReading.second;

            window.gasResistandeAgg.add(gasResistance);
            window.gasSensorHumidityAgg.add(humidity);
        }
        Serial.println("[DEBUG] bmeReading");

        Serial.println("[DEBUG] ------END READING-------");

        window.tempAgg.add(sht31Reading[0]);
        window.humAgg.add(sht31Reading[1]);
        window.co2Agg.add(co2Reading);
    }

    void enqueueAverages(const SensorWindow &window)
    {
        float co2Avg = window.co2Agg.avg();

        float tempAvg = window.tempAgg.avg();
        float humAvg = window.humAgg.avg();

        float pm10Avg = window.pm10Agg.avg();
        float pm25Avg = window.pm25Agg.avg();
        float pm100Avg = window.pm100Agg.avg();

        uint32_t gasResistanceAvg = static_cast<uint32_t>(std::lround(window.gasResistandeAgg.avg()));
        float gasSensorHumidityAvg = window.gasSensorHumidityAgg.avg();
        Serial.println("[DEBUG] Readings averaged");

        TemperatureHumidityReading sht31Avg = {tempAvg, humAvg};
        ParticulateReading pmsAvg = {pm10Avg, pm25Avg, pm100Avg};
        BmeAverages bmeAvg = {gasResistanceAvg, gasSensorHumidityAvg};

        xQueueSend(co2Queue, &co2Avg, 0);
        xQueueSend(sht31Queue, &sht31Avg, 0);
        xQueueSend(pmsQueue, &pmsAvg, 0);
        if (getIsBMEReady())
        {
            xQueueSend(bmeQueue, &bmeAvg, 0);
        }
    }

    void resetWindow(SensorWindow &window)
    {
        window.co2Agg.reset();
        window.tempAgg.reset();
        window.humAgg.reset();
        window.pm10Agg.reset();
        window.pm25Agg.reset();
        window.pm100Agg.reset();
        // Preserve the existing BME accumulation across reading windows.
    }

} // namespace

void SensorsTask(void *param)
{
    uint8_t msg;

    SensorWindow window;

    unsigned long readingWindowStart;

    if (xQueueReceive(enableSensorsQueue, &msg, portMAX_DELAY) == pdTRUE)
    {
        Serial.println("[DEBUG] Sensors enabled");
        readingWindowStart = millis();
        for (;;)
        {
            if (millis() - readingWindowStart >= READING_INTERVAL)
            {
                enqueueAverages(window);
                resetWindow(window);

                readingWindowStart = millis();

                Serial.println("[DEBUG] Readings reset and sent");
                Serial.println("[DEBUG] --------END CYCLE--------");
            }
            collectReadings(window);

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
