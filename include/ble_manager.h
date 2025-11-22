#pragma once
#include <string>
#include <iostream>
#include <BLEServer.h>

#define BT_LED_PIN 2
#define BOOT_PIN 0

extern BLECharacteristic *wifiNetworksCharacteristic;
extern BLECharacteristic *wifiCredsCharacteristic;
extern bool deviceConnectedToBLE;

void setupBLEServer();
void toggleDeviceBLEAdvertising(bool isAdvertising);