#include <Arduino.h>
#include <WiFi.h>
#include <Wire.h>
#include "wifi_manager.h"
#include "utils.h"
#include "ble_manager.h"
#include "preferences_manager.h"
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
  esp_reset_reason_t reason = esp_reset_reason();
  Serial.begin(9600);
  Serial.printf("Starting Clear Zone Module [ID]: %s \n", CHIP_ID);
  Serial.printf("Reset reason: %d \n", reason);

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
  autoconnectToWifi();

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

      deletePrefs();
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
