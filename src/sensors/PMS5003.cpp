
#include <Arduino.h>
#include "PMS5003.h"
#include "Adafruit_PM25AQI.h"
#include <optional>
#include <array>

#define PMS_RX_PIN 16
#define PMS_TX_PIN 17

Adafruit_PM25AQI pm25 = Adafruit_PM25AQI();

void initPMS()
{
    Serial2.begin(9600, SERIAL_8N1, PMS_RX_PIN, PMS_TX_PIN);

    if (!pm25.begin_UART(&Serial2))
    {
        Serial.println("Could not find PM 2.5 sensor!");
        while (1)
            delay(10);
    }
}

std::array<uint16_t, 3> readPMS()
{
    PM25_AQI_Data data;

    if (!pm25.read(&data))
    {
        Serial.println("Could not read from AQI");
        return {0, 0, 0};
    }

    return {data.pm10_env, data.pm25_env, data.pm100_env};
}