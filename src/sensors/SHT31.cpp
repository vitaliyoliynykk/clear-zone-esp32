#include <Adafruit_SHT31.h>
#include <array>
#include "SHT31.h"

Adafruit_SHT31 sht31 = Adafruit_SHT31();

bool isSht31Init = false;

void initSht31()
{
    if (!sht31.begin(0x44))
    {
        Serial.println("SHT3x not found");
        isSht31Init = false;
        return;
    }

    isSht31Init = true;
}

std::array<float, 2> mesureSht31()
{
    return {
        sht31.readTemperature(),
        sht31.readHumidity()};
}