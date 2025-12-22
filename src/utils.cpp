#include <Arduino.h>
#include <string>
#include <iostream>
#include "utils.h"

String CHIP_ID = String((uint32_t)ESP.getEfuseMac(), HEX);
QueueHandle_t authQueue;

const char PROVISION_SECRET[] = "clear-zone-device-sc-dd8b172f-7b6d-489c-a346-5a05a6b5f5a4";
const char PREF_WIFI_KEY[] = "wifi-creds";
const char PREF_AUTH_KEY[] = "auth-creds";

std::pair<std::string, std::string> splitStr(const std::string &in, char sep)
{
  auto pos = in.find(sep);
  if (pos == std::string::npos)
    return {"", ""};
  return {in.substr(0, pos), in.substr(pos + 1)};
}

bool ensureTimeSynced(unsigned long timeoutMs = 10000)
{
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  unsigned long start = millis();
  time_t now;
  do
  {
    delay(200);
    now = time(nullptr);
  } while ((now < 1672531200 /* 2023-01-01 */) && (millis() - start < timeoutMs));
  return now >= 1672531200;
}