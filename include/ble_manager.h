#pragma once
#include <string>
#include <iostream>
#include <NimBLEDevice.h>

#define BT_LED_PIN 2
#define BOOT_PIN 0

extern NimBLECharacteristic *wifiNetworksCharacteristic;
extern NimBLECharacteristic *wifiCredsCharacteristic;
extern NimBLECharacteristic *statusCharacteristic;
extern bool deviceConnectedToBLE;

void setupBLEServer();
void toggleDeviceBLEAdvertising(bool isAdvertising);
void sendAuthResult(bool result);
