#include <Arduino.h>
#include <WiFi.h>
#include "wifi_manager.h"
#include "utils.h"
#include "ble_manager.h"

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
    Serial.println("Boot pressed");
  }
  else
  {
    bootPressed = false;
  }
}

void setup()
{
  Serial.begin(9600);
  Serial.println("Starting Clear Zone Module");

  pinMode(BT_LED_PIN, OUTPUT);
  pinMode(BOOT_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BOOT_PIN), onBootPressed, FALLING);

  WiFi.mode(WIFI_STA);

  autoconnectToWifi();
  setupBLEServer();
}

void loop()
{
  handlePairingModeEnter();
  delay(10);
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
      Serial.println("Entered pairing");
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
    Serial.println("Exited pairing mode");
    pairingActive = false;
  }
}