#pragma once
#include <string>

class BLECharacteristic;  // попереднє оголошення

void scanWiFi(BLECharacteristic *notifyNetworks);
void autoconnectToWifi();
void saveCreds(const std::string &ssid, const std::string &pass);
bool loadCreds(std::string &ssid, std::string &pass);