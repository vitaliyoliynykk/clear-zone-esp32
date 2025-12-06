
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include "utils.h"
#include "http_manager.h"

const char *API_URL = "https://reason-moses-profiles-placing.trycloudflare.com";

// Preferences prefs;

String apiPath(String path);
void handleRegistrationResponse(const String &response);

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
        handleRegistrationResponse(response);
    }

    http.end();
}

// bool checkModuleRegistrationRequest()
// {
//     WiFiClientSecure client;
//     client.setInsecure();

//     HTTPClient http;

//     if (!http.begin(client, apiPath("/module-auth/me")))
//     {
//         Serial.println("HTTP begin failed");
//         return false;
//     }
// }

String apiPath(String path)
{
    return String(API_URL) + path;
}

void handleRegistrationResponse(const String &response)
{
    JsonDocument doc;
    deserializeJson(doc, response);

    const char *moduleToken = doc["moduleToken"];
    const char *mqttUsername = doc["mqttUsername"];
    const char *mqttPass = doc["mqttPass"];

    // prefs.begin(PREF_AUTH_KEY, true);

    // prefs.putString("moduleToken", moduleToken);
    // prefs.putString("mqttUsername", mqttUsername);
    // prefs.putString("mqttPass", mqttPass);
    // prefs.end();
}
