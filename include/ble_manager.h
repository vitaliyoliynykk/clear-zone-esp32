#pragma once
#include <string>
#include <iostream>
#include <BLEServer.h>

#define BT_LED_PIN 2

extern BLECharacteristic *wifiNetworksCharacteristic;
extern BLECharacteristic *wifiCredsCharacteristic;
extern bool deviceConnectedToBLE;

void setupBLEServer();