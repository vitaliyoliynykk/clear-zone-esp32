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
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include <array>

struct SensorData
{
  float temp;
  float hum;
};

constexpr uint32_t PAIRING_HOLD_MS = 5000;
volatile bool bootPressed = false;
static bool pairingActive = false;
volatile uint32_t bootPressStart = 0;

void handlePairingModeEnter();
void AuthTask(void *param);
void SensorsTask(void *param);
void MqttTask(void *param);

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

  // Sensors
  initSht31();
  initCOSensor();

  // FreeRTOS
  authQueue = xQueueCreate(1, sizeof(uint8_t));
  enableSensorsQueue = xQueueCreate(1, sizeof(uint8_t));
  sht31Queue = xQueueCreate(4,
                            sizeof(std::array<float, 2>));
  co2Queue = xQueueCreate(1, sizeof(uint16_t));
  xTaskCreatePinnedToCore(AuthTask, "AuthTask", 4096, nullptr, 1, nullptr, 1);
  xTaskCreatePinnedToCore(SensorsTask, "SensorsTask", 2048, nullptr, 1, nullptr, 1);
  xTaskCreatePinnedToCore(MqttTask, "MqttTask", 4096, nullptr, 1, nullptr, 1);

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
      }
    }
  }

  setupBLEServer();
}

void loop()
{
  mqtt.loop();
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

void AuthTask(void *param)
{
  uint8_t msg;
  for (;;)
  {
    if (xQueueReceive(authQueue, &msg, portMAX_DELAY) == pdTRUE)
    {
      bool result = handleAuthInitialization();
      sendAuthResult(result);
    }
  }
}

void SensorsTask(void *param)
{
  uint8_t msg;

  if (xQueueReceive(enableSensorsQueue, &msg, portMAX_DELAY) == pdTRUE)
  {
    for (;;)
    {
      auto sht31Reading = mesureSht31();
      uint16_t co2Reading = readCO2();

      xQueueSend(sht31Queue, &sht31Reading, 0);
      xQueueSend(co2Queue, &co2Reading, 0);

      vTaskDelay(pdMS_TO_TICKS(15000));
    }
  }
}

void MqttTask(void *param)
{
  std::array<float, 2> msg;
  uint16_t co2Reding;
  char payload[64];

  for (;;)
  {
    if (xQueueReceive(sht31Queue, &msg, portMAX_DELAY))
    {
      snprintf(
          payload,
          sizeof(payload),
          "{\"temp\":%.2f,\"hum\":%.2f}",
          msg[0],
          msg[1]);

      mqtt.publish(tempTopic.c_str(), payload);
    }
    vTaskDelay(pdMS_TO_TICKS(100));

    if (xQueueReceive(co2Queue, &co2Reding, portMAX_DELAY))
    {
      snprintf(
          payload,
          sizeof(payload),
          "{\"co2\":%d}",
          co2Reding);

      mqtt.publish(co2Topic.c_str(), payload);
    }
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}