#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLEAdvertising.h>
#include <WiFi.h>
#include <Preferences.h>
#include <string>
#include <iostream>

#define BT_LED_PIN 2

constexpr char BT_SERVICE_ID[] = "dd8b172f-7b6d-489c-a346-5a05a6b5f5a4";
constexpr char WIFI_NETWORKS_CHARACTERISTIC_ID[] = "f9cf5cae-146c-41dd-825d-4d9ee4e9f279";
constexpr char WIFI_CREDS_CHARACTERISTIC_ID[] = "ba2c9d3b-9126-41a1-8cd8-54bee53e9b6b";

bool deviceConnected = false;

BLECharacteristic *wifiNetworksCharacteristic;
BLECharacteristic *wifiCredsCharacteristic;
Preferences prefs;

void setupBLEServer();
void scanWiFi();
void autoconnectToWifi();
void saveCreds(const std::string &ssid, const std::string &pass);
bool loadCreds(std::string &ssid, std::string &pass);
std::pair<std::string, std::string> splitStr(const std::string &in, char sep);

class WifiNetworksCharacteristicCallbacks : public BLECharacteristicCallbacks
{
  void onRead(BLECharacteristic *pCharacteristic)
  {
    scanWiFi();
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

    WiFi.begin(ssid.c_str(), pass.c_str());

    wl_status_t res = (wl_status_t)WiFi.waitForConnectResult(8000);

    if (res == WL_CONNECTED)
    {
      saveCreds(ssid, pass);
    }

    pCharacteristic->setValue(std::to_string(res));
    // pCharacteristic->notify();
  }
};

class MyServerCallbacks : public BLEServerCallbacks
{
  void onConnect(BLEServer *pServer) override
  {
    deviceConnected = true;
    digitalWrite(BT_LED_PIN, HIGH);
  }

  void onDisconnect(BLEServer *pServer) override
  {
    deviceConnected = false;
    digitalWrite(BT_LED_PIN, LOW);

    pServer->startAdvertising();
  }
};

void setup()
{
  Serial.begin(9600);
  pinMode(BT_LED_PIN, OUTPUT);
  Serial.println("Starting ...");

  WiFi.mode(WIFI_STA);

  // autoconnectToWifi();
  setupBLEServer();
  Serial.println("BLE Started");
}

void loop()
{
}

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

  wifiNetworksCharacteristic->setCallbacks(new WifiNetworksCharacteristicCallbacks());
  wifiCredsCharacteristic->setCallbacks(new WifiCredsCharacteristicCallbacks());

  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(BLEUUID(BT_SERVICE_ID));

  BLEDevice::startAdvertising();
}

void scanWiFi()
{
  WiFi.disconnect();
  delay(100);

  String wifiData = "";

  int n = WiFi.scanNetworks();
  if (n == 0)
  {
    wifiData = "NO_NETWORKS";
  }
  else
  {

    for (int i = 0; i < n; ++i)
    {
      wifiData += WiFi.SSID(i) + "," +
                  String(WiFi.RSSI(i)) + "," +
                  String(WiFi.encryptionType(i) != WIFI_AUTH_OPEN ? 1 : 0);
      if (i < n - 1)
        wifiData += ";";
    }

    wifiNetworksCharacteristic->setValue(wifiData.c_str());
    wifiNetworksCharacteristic->notify();
  }
}

void saveCreds(const std::string &ssid, const std::string &pass)
{
  prefs.begin("wifi", false);
  prefs.putString("ssid", ssid.c_str());
  prefs.putString("pass", pass.c_str());
  prefs.end();
}

bool loadCreds(std::string &ssid, std::string &pass)
{
  prefs.begin("wifi", true);
  String s = prefs.getString("ssid", "");
  String p = prefs.getString("pass", "");
  prefs.end();
  if (s.isEmpty() || p.isEmpty())
    return false;
  ssid.assign(s.c_str());
  pass.assign(p.c_str());
  return true;
}

std::pair<std::string, std::string> splitStr(const std::string &in, char sep)
{
  auto pos = in.find(sep);
  if (pos == std::string::npos)
    return {"", ""};
  return {in.substr(0, pos), in.substr(pos + 1)};
}

void autoconnectToWifi()
{
  std::string ssid;
  std::string pass;

  if (loadCreds(ssid, pass))
  {
    WiFi.begin(ssid.c_str(), pass.c_str());

    wl_status_t res = (wl_status_t)WiFi.waitForConnectResult(15000);
    Serial.printf("Connect result: %d\n", res);
    if (res == WL_CONNECTED)
    {
      Serial.print("IP: ");
      Serial.println(WiFi.localIP());
    }
    else
    {
      Serial.printf("Status=%d (6=DISCONNECTED, 4=CONNECT_FAILED, 1=NO_SSID)\n", WiFi.status());
    }
  }
}