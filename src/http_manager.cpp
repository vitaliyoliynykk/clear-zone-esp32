
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "utils.h"
#include "http_manager.h"
#include "preferences_manager.h"

const char *API_URL = "https://clear-zone-api.duckdns.org";

String apiPath(String path);
void handleRegistrationResponse(const String &response);

bool registerHttpsRequest()
{
    WiFiClientSecure client;
    client.setInsecure();

    HTTPClient http;

    if (!http.begin(client, apiPath("/module-auth/register-module")))
    {
        Serial.println("HTTP begin failed");
        return false;
    }

    http.addHeader("Content-Type", "application/json");

    String payload = "{\"deviceId\":\"" + CHIP_ID + "\",\"secret\":\"" + PROVISION_SECRET + "\"}";

    int httpCode = http.POST(payload);

    Serial.printf("HTTP POST code: %d\n", httpCode);

    if (httpCode == 200)
    {
        String response = http.getString();
        handleRegistrationResponse(response);

        http.end();

        return true;
    }

    http.end();

    return false;
}

bool checkModuleRegistrationRequest(std::string moduleToken)
{
    WiFiClientSecure client;
    client.setInsecure();

    HTTPClient http;

    if (!http.begin(client, apiPath("/module-auth/me")))
    {
        Serial.println("HTTP begin failed");
        return false;
    }

    http.addHeader("Content-Type", "application/json");

    String payload = "{\"deviceId\":\"" + CHIP_ID + "\",\"moduleToken\":\"" + moduleToken.c_str() + "\"}";

    int httpCode = http.POST(payload);

    http.end();

    return httpCode == 200;
}

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

    saveModuleCreds(moduleToken, mqttUsername, mqttPass);
}

int handleAuthInitialization()
{
    std::string moduleToken;
    Serial.println("Checking for registration...");

    if (loadModuleToken(moduleToken) &&
        checkModuleRegistrationRequest(moduleToken))
    {
        Serial.println("Device has already been registered");
        Serial.println("Doing stuff");
        return 200;
    }

    Serial.println("Trying to register...");

    if (registerHttpsRequest())
    {
        Serial.println("Device is registered");
        Serial.println("Doing stuff");

        return 200;
    }

    return 400;
}