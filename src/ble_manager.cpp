#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLEAdvertising.h>
#include <BLESecurity.h>
#include <BLE2902.h>
#include "ble_manager.h"
#include "wifi_manager.h"
#include "http_manager.h"
#include "utils.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

constexpr char BT_SERVICE_ID[] = "dd8b172f-7b6d-489c-a346-5a05a6b5f5a4";
constexpr char WIFI_NETWORKS_CHARACTERISTIC_ID[] = "f9cf5cae-146c-41dd-825d-4d9ee4e9f279";
constexpr char WIFI_CREDS_CHARACTERISTIC_ID[] = "ba2c9d3b-9126-41a1-8cd8-54bee53e9b6b";
constexpr char DEVICE_ID_CHARACTERISTIC[] = "caa6bfc4-2149-496d-8aa1-7e1d3945d386";
constexpr char STATUS_CHARACTERISTIC_ID[] = "6e8327de-10cb-41ec-a222-8f472c83fc80";

BLECharacteristic *wifiNetworksCharacteristic;
BLECharacteristic *wifiCredsCharacteristic;
BLECharacteristic *deviceIdCharacteristic;
BLECharacteristic *statusCharacteristic;

bool deviceConnectedToBLE = false;

class WifiNetworksCharacteristicCallbacks : public BLECharacteristicCallbacks
{
  void onRead(BLECharacteristic *pCharacteristic)
  {
    scanWiFi(wifiNetworksCharacteristic);
  }
};

class DeviceIdCharacteristicCallbacks : public BLECharacteristicCallbacks
{
  void onRead(BLECharacteristic *pCharacteristic)
  {
    std::string value = CHIP_ID.c_str();

    pCharacteristic->setValue(value);
    pCharacteristic->notify();
  }
};

class WifiCredsCharacteristicCallbacks : public BLECharacteristicCallbacks
{
  void onWrite(BLECharacteristic *pCharacteristic)
  {
    std::string value = pCharacteristic->getValue();

    auto result = splitStr(value, ':');
    std::string ssid = result.first;
    std::string pass = result.second;

    wl_status_t res = connectToWifi(ssid, pass);

    pCharacteristic->setValue(std::to_string(res));
    pCharacteristic->notify();

    if (res == WL_CONNECTED)
    {
      toggleDeviceBLEAdvertising(false);
      uint8_t msg = 1;
      xQueueSend(authQueue, &msg, 0); // тільки сигнал, без блокування BLE

      // int result = handleAuthInitialization();
      // statusCharacteristic->setValue(result == 200 ? "REGISTERED" : "REGISTRATION_ERROR");
      // statusCharacteristic->notify();
    }
  }
};

class MyServerCallbacks : public BLEServerCallbacks
{
  void onConnect(BLEServer *pServer) override
  {
    deviceConnectedToBLE = true;
    digitalWrite(BT_LED_PIN, HIGH);
  }

  void onDisconnect(BLEServer *pServer) override
  {
    deviceConnectedToBLE = false;
    digitalWrite(BT_LED_PIN, LOW);

    pServer->startAdvertising();
  }
};

void setupBLEServer()
{
  BLEDevice::init("Clear Zone Module");
  BLEDevice::setEncryptionLevel(ESP_BLE_SEC_ENCRYPT);

  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService(BT_SERVICE_ID);

  wifiNetworksCharacteristic = pService->createCharacteristic(
      WIFI_NETWORKS_CHARACTERISTIC_ID,
      BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_READ);

  wifiCredsCharacteristic = pService->createCharacteristic(
      WIFI_CREDS_CHARACTERISTIC_ID,
      BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_READ);

  deviceIdCharacteristic = pService->createCharacteristic(
      DEVICE_ID_CHARACTERISTIC,
      BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_READ);

  statusCharacteristic = pService->createCharacteristic(
      STATUS_CHARACTERISTIC_ID,
      BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_READ);

  deviceIdCharacteristic->setCallbacks(new DeviceIdCharacteristicCallbacks());
  wifiNetworksCharacteristic->setCallbacks(new WifiNetworksCharacteristicCallbacks());
  wifiCredsCharacteristic->setCallbacks(new WifiCredsCharacteristicCallbacks());

  statusCharacteristic->addDescriptor(new BLE2902());

  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(BLEUUID(BT_SERVICE_ID));
}

void toggleDeviceBLEAdvertising(bool isAdvertising)
{
  if (isAdvertising)
  {
    BLEDevice::startAdvertising();
  }
  else
  {
    BLEDevice::stopAdvertising();
  }
}