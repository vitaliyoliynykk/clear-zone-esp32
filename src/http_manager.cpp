#include "http_manager.h"
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include "utils.h"

const char *API_URL = "https://wake-wishes-guest-parent.trycloudflare.com";

String apiPath(String path);

void registerHttpsRequest()
{
    WiFiClientSecure client;
    client.setInsecure();

    HTTPClient http;

    if (!http.begin(client, apiPath("/module-auth/register-module")))
    {
        Serial.println("HTTP begin failed");
        return;
    }

    http.addHeader("Content-Type", "application/json");

    String payload = "{\"deviceId\":\"" + CHIP_ID + "\",\"secret\":\"" + PROVISION_SECRET + "\"}";

    int httpCode = http.POST(payload);

    Serial.printf("HTTP POST code: %d\n", httpCode);

    if (httpCode > 0)
    {
        String response = http.getString();
        Serial.println("Response:");
        Serial.println(response);
    }

    http.end();
}

String apiPath(String path)
{
    return String(API_URL) + path;
}