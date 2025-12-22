#include <Arduino.h>
#include <NimBLEDevice.h>
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

NimBLECharacteristic *wifiNetworksCharacteristic;
NimBLECharacteristic *wifiCredsCharacteristic;
NimBLECharacteristic *deviceIdCharacteristic;
NimBLECharacteristic *statusCharacteristic;

bool deviceConnectedToBLE = false;

class WifiNetworksCharacteristicCallbacks : public NimBLECharacteristicCallbacks
{
  void onRead(NimBLECharacteristic *pCharacteristic, NimBLEConnInfo &connInfo) override
  {
    scanWiFi(wifiNetworksCharacteristic);
  }
};

class DeviceIdCharacteristicCallbacks : public NimBLECharacteristicCallbacks
{
  void onRead(NimBLECharacteristic *pCharacteristic, NimBLEConnInfo &connInfo) override
  {
    std::string value = CHIP_ID.c_str();

    pCharacteristic->setValue(value);
    pCharacteristic->notify();
  }
};

class WifiCredsCharacteristicCallbacks : public NimBLECharacteristicCallbacks
{
  void onWrite(NimBLECharacteristic *pCharacteristic, NimBLEConnInfo &connInfo) override
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
      xQueueSend(authQueue, &msg, 0);
    }
  }
};

class MyServerCallbacks : public NimBLEServerCallbacks
{
  void onConnect(NimBLEServer *pServer, NimBLEConnInfo &connInfo) override
  {
    deviceConnectedToBLE = true;
    digitalWrite(BT_LED_PIN, HIGH);
  }

  void onDisconnect(NimBLEServer *pServer, NimBLEConnInfo &connInfo, int reason) override
  {
    deviceConnectedToBLE = false;
    digitalWrite(BT_LED_PIN, LOW);

    NimBLEDevice::startAdvertising();
  }
};

void setupBLEServer()
{
  NimBLEDevice::init("Clear Zone Module");
  NimBLEDevice::setSecurityAuth(true, false, true);
  NimBLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT);
  NimBLEDevice::setSecurityInitKey(BLE_SM_PAIR_KEY_DIST_ENC);
  NimBLEDevice::setSecurityRespKey(BLE_SM_PAIR_KEY_DIST_ENC);

  NimBLEServer *pServer = NimBLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  NimBLEService *pService = pServer->createService(BT_SERVICE_ID);

  wifiNetworksCharacteristic = pService->createCharacteristic(
      WIFI_NETWORKS_CHARACTERISTIC_ID,
      NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::READ);

  wifiCredsCharacteristic = pService->createCharacteristic(
      WIFI_CREDS_CHARACTERISTIC_ID,
      NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::READ);

  deviceIdCharacteristic = pService->createCharacteristic(
      DEVICE_ID_CHARACTERISTIC,
      NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::READ);

  statusCharacteristic = pService->createCharacteristic(
      STATUS_CHARACTERISTIC_ID,
      NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::READ);

  deviceIdCharacteristic->setCallbacks(new DeviceIdCharacteristicCallbacks());
  wifiNetworksCharacteristic->setCallbacks(new WifiNetworksCharacteristicCallbacks());
  wifiCredsCharacteristic->setCallbacks(new WifiCredsCharacteristicCallbacks());

  pService->start();

  NimBLEAdvertising *pAdvertising = NimBLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(NimBLEUUID(BT_SERVICE_ID));
  pAdvertising->enableScanResponse(true);
}

void toggleDeviceBLEAdvertising(bool isAdvertising)
{
  if (isAdvertising)
  {
    NimBLEDevice::startAdvertising();
  }
  else
  {
    NimBLEDevice::stopAdvertising();
  }
}

void sendAuthResult(bool result)
{
  statusCharacteristic->setValue(result ? "REGISTERED" : "REGISTRATION_ERROR");
  statusCharacteristic->notify();
}