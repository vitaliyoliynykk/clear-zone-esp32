#pragma once

bool hasToken();
void saveWifiCreds(const std::string &ssid, const std::string &pass);
bool loadWifiCreds(std::string &ssid, std::string &pass);