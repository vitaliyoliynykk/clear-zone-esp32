
#include <Arduino.h>
#include "PMS5003.h"
#include "Adafruit_PM25AQI.h"
#include <optional>
#include <array>

#define PMS_RX_PIN 16
#define PMS_TX_PIN 17

portMUX_TYPE pmsMux = portMUX_INITIALIZER_UNLOCKED;
Adafruit_PM25AQI pm25 = Adafruit_PM25AQI();
static PM25_AQI_Data lastData;
static bool hasData = false;

bool initPMS()
{
    Serial2.begin(9600, SERIAL_8N1, PMS_RX_PIN, PMS_TX_PIN);

    return pm25.begin_UART(&Serial2);
}

void pollPMS()
{
    PM25_AQI_Data data;
    if (pm25.read(&data))
    {
        portENTER_CRITICAL(&pmsMux);
        lastData = data;
        hasData = true;
        portEXIT_CRITICAL(&pmsMux);
    }
}

bool readPMS(std::array<uint16_t, 3> &out)
{
    portENTER_CRITICAL(&pmsMux);
    if (!hasData)
    {
        portEXIT_CRITICAL(&pmsMux);
        return false;
    }

    out = {
        lastData.pm10_env,
        lastData.pm25_env,
        lastData.pm100_env};

    hasData = false;
    portEXIT_CRITICAL(&pmsMux);

    return true;
}