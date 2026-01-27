#include <Arduino.h>
#include <WiFi.h>
#include <Wire.h>
#include "wifi_manager.h"
#include "utils.h"
#include "ble_manager.h"
#include "http_manager.h"
#include "preferences_manager.h"
#include "mqtt_setup.h"
#include "SHT31.h"
#include "SCD41.h"
#include "BME680.h"
#include "PMS5003.h"
#include "tasks.h"
#include <array>

constexpr uint32_t PAIRING_HOLD_MS = 5000;
volatile bool bootPressed = false;
static bool pairingActive = false;
volatile uint32_t bootPressStart = 0;

void handlePairingModeEnter();

void IRAM_ATTR onBootPressed()
{
  const bool level = digitalRead(BOOT_PIN);

  if (!level)
  {
    bootPressed = true;
    bootPressStart = millis();
  }
  else
  {
    bootPressed = false;
  }
}

void setup()
{
  Serial.begin(9600);
  Serial.printf("Starting Clear Zone Module [ID]: %s \n", CHIP_ID);

  // I2C Init
  Wire.begin(21, 22); // SDA, SCL

  // Wait for sensors to turn on
  delay(1000);

  // Sensors
  initSht31();
  initCOSensor();
  initBME();
  initPMS();

  // FreeRTOS
  initRTOS();

  // Pins setup
  pinMode(BT_LED_PIN, OUTPUT);
  pinMode(BOOT_PIN, INPUT_PULLUP);

  // Interrupts
  attachInterrupt(digitalPinToInterrupt(BOOT_PIN), onBootPressed, FALLING);

  // WIFI Setup
  WiFi.mode(WIFI_STA);

  bool isConnected = autoconnectToWifi();

  // Auth, MQTT Setup, BLE Setup
  if (isConnected)
  {
    delay(2000); // Wait for DNS
    setupWifiClient();
    if (handleAuthInitialization())
    {
      if (mqttConnect())
      {
        uint8_t msg = 1;
        xQueueSend(enableSensorsQueue, &msg, 0);
        xQueueSend(enablePmsPollingQueue, &msg, 0);
      }
    }
  }

  setupBLEServer();
}

void loop()
{
  handlePairingModeEnter();
  delay(50);
}

void handlePairingModeEnter()
{
  if (bootPressed && !pairingActive)
  {
    const uint32_t held = millis() - bootPressStart;
    if (held >= PAIRING_HOLD_MS)
    {
      pairingActive = true;
      bootPressed = false;

      toggleDeviceBLEAdvertising(true);
    }
    else if (!digitalRead(BOOT_PIN))
    {
      delay(10);
    }
    else
    {
      bootPressed = false;
      pairingActive = false;
    }
  }

  if (pairingActive && deviceConnectedToBLE)
  {
    pairingActive = false;
  }
}
