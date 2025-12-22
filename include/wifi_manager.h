#pragma once
#include <string>
#include <WiFi.h>
#include <WiFiClientSecure.h>

extern WiFiClientSecure wifiClient;

class NimBLECharacteristic;

void scanWiFi(NimBLECharacteristic *notifyNetworks);
bool autoconnectToWifi();
wl_status_t connectToWifi(std::string ssid, std::string pass);
void saveCreds(const std::string &ssid, const std::string &pass);
bool loadCreds(std::string &ssid, std::string &pass);
void setupWifiClient();
