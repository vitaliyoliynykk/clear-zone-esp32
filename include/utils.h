#pragma once
#include <string>
#include <iostream>

extern String CHIP_ID;
extern const char PROVISION_SECRET[];


std::pair<std::string, std::string> splitStr(const std::string &in, char sep);