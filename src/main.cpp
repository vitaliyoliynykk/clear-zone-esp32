#include <Arduino.h>
#include <WiFi.h>
#include "wifi_manager.h"
#include "utils.h"
#include "ble_manager.h"

void setup()
{
  Serial.begin(9600);
  Serial.println("Starting Clear Zone Module");
  
  pinMode(BT_LED_PIN, OUTPUT);

  WiFi.mode(WIFI_STA);

  autoconnectToWifi();
  setupBLEServer();
}

void loop()
{
}
