#pragma once
#include <string>
#include <iostream>

extern String CHIP_ID;
extern QueueHandle_t authQueue;
extern QueueHandle_t sensorsQueue;
extern QueueHandle_t sht31Queue;

extern const char PROVISION_SECRET[];
extern const char PREF_WIFI_KEY[];
extern const char PREF_AUTH_KEY[];

std::pair<std::string, std::string> splitStr(const std::string &in, char sep);
bool ensureTimeSynced(unsigned long timeoutMs);