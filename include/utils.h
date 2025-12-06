#pragma once
#include <string>
#include <iostream>

extern String CHIP_ID;
extern const char PROVISION_SECRET[];
extern const char PREF_WIFI_KEY[];
extern const char PREF_AUTH_KEY[];

std::pair<std::string, std::string> splitStr(const std::string &in, char sep);