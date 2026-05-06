#include <Adafruit_SHT31.h>
#include <array>
#include "SHT31.h"

Adafruit_SHT31 sht31 = Adafruit_SHT31();

void initSht31()
{
    if (!sht31.begin(0x44))
    {
        Serial.println("SHT3x not found");
        return;
    }
}

std::array<float, 2> mesureSht31()
{
    return {
        sht31.readTemperature(),
        sht31.readHumidity()};
}
