#include <Arduino.h>
#include <WiFi.h>
#include "wifi_manager.h"
#include "utils.h"
#include "ble_manager.h"
#include "http_manager.h"
#include "preferences_manager.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

constexpr uint32_t PAIRING_HOLD_MS = 5000;
volatile bool bootPressed = false;
static bool pairingActive = false;
volatile uint32_t bootPressStart = 0;

void handlePairingModeEnter();
void AuthTask(void *param);

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

  authQueue = xQueueCreate(1, sizeof(uint8_t));
  xTaskCreatePinnedToCore(AuthTask, "AuthTask", 8192, nullptr, 1, nullptr, 1);

  pinMode(BT_LED_PIN, OUTPUT);
  pinMode(BOOT_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BOOT_PIN), onBootPressed, FALLING);

  WiFi.mode(WIFI_STA);

  bool isConnected = autoconnectToWifi();

  if (isConnected)
  {
    handleAuthInitialization();
  }

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

void AuthTask(void *param)
{
  uint8_t msg;
  for (;;)
  {
    if (xQueueReceive(authQueue, &msg, portMAX_DELAY) == pdTRUE)
    {
      uint8_t result = handleAuthInitialization();
      sendAuthResult(result);
    }
  }
}
