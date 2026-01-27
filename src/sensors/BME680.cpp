#include <Adafruit_BME680.h>
#include <array>
#include <BME680.h>

Adafruit_BME680 bme;
static uint64_t bmeStartUs = 0;
bool isBMEReady;

void initBME()
{
    if (!bme.begin(0x76))
    {
        isBMEReady = false;
        Serial.println("BME680 not found");
    }

    bme.setGasHeater(320, 150); // °C, ms
    bmeStartUs = esp_timer_get_time();
    isBMEReady = true;
}

bool readGas(std::pair<uint32_t, float> &out)
{
    if (bme.performReading())
    {
        out = {
            bme.gas_resistance,
            bme.humidity};

        return true;
    }

    return false;
}

uint64_t getBmeUptimeSec()
{
    return (esp_timer_get_time() - bmeStartUs) / 1000000ULL;
}

bool getIsBMEReady()
{
    return isBMEReady;
}