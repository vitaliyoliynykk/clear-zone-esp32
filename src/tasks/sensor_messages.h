#pragma once
#include <cstdint>

struct TemperatureHumidityReading
{
    float temperature;
    float humidity;
};

struct ParticulateReading
{
    float pm1;
    float pm2_5;
    float pm10;
};

struct BmeAverages
{
    uint32_t gasResistance;
    float sensorHumidity;
};
