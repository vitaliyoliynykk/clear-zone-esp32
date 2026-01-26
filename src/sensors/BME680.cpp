#include <Adafruit_BME680.h>
#include <array>
#include <BME680.h>

Adafruit_BME680 bme;

void initBME()
{
    if (!bme.begin(0x76))
    {
        Serial.println("BME680 not found");
        while (1)
            ;
    }

    bme.setGasHeater(320, 150); // °C, ms
}

std::pair<uint32_t, float> readGas()
{
    if (bme.performReading())
    {
        return {
            bme.gas_resistance,
            bme.humidity};
    }

    return {0, 0.0f};
}