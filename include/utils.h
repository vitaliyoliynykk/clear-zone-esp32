#pragma once
#include <Arduino.h>
#include <cmath>
#include <iostream>
#include <string>
#include <utility>

struct WindowAgg
{
    float sum = 0;
    float max = -INFINITY;
    uint16_t count = 0;

    void add(float v)
    {
        sum += v;
        if (v > max)
            max = v;
        count++;
    }

    float avg() const
    {
        return count ? sum / count : NAN;
    }

    void reset()
    {
        sum = 0;
        max = -INFINITY;
        count = 0;
    }
};

extern String CHIP_ID;
// extern QueueHandle_t authQueue;
// extern QueueHandle_t enableSensorsQueue;
// extern QueueHandle_t sht31Queue;
// extern QueueHandle_t co2Queue;

extern const char PROVISION_SECRET[];
extern const char PREF_WIFI_KEY[];
extern const char PREF_AUTH_KEY[];

std::pair<std::string, std::string> splitStr(const std::string &in, char sep);
bool ensureTimeSynced(unsigned long timeoutMs);
