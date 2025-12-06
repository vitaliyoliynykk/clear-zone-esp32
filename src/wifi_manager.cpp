#include <BLECharacteristic.h>
#include <WiFi.h>
#include <Preferences.h>
#include "utils.h"
#include "wifi_manager.h"
#include "preferences_manager.h"

void scanWiFi(BLECharacteristic *notifyNetworks)
{
    WiFi.disconnect();
    delay(100);

    String wifiData = "";

    int n = WiFi.scanNetworks();

    if (n == 0)
    {
        wifiData = "NO_NETWORKS";
    }
    else
    {

        for (int i = 0; i < n; ++i)
        {
            wifiData += WiFi.SSID(i) + "," +
                        String(WiFi.RSSI(i)) + "," +
                        String(WiFi.encryptionType(i) != WIFI_AUTH_OPEN ? 1 : 0);
            if (i < n - 1)
                wifiData += ";";
        }

        notifyNetworks->setValue(wifiData.c_str());
        notifyNetworks->notify();
    }
}
bool autoconnectToWifi()
{
    std::string ssid;
    std::string pass;

    if (loadWifiCreds(ssid, pass))
    {
        WiFi.begin(ssid.c_str(), pass.c_str());

        wl_status_t res = (wl_status_t)WiFi.waitForConnectResult(15000);
        if (res == WL_CONNECTED)
        {
            Serial.print("Connected. IP: ");
            Serial.println(WiFi.localIP());
            return true;
        }
        else
        {
            Serial.printf("Status=%d (6=DISCONNECTED, 4=CONNECT_FAILED, 1=NO_SSID)\n", WiFi.status());
            return false;
        }
    }

    return false;
}

wl_status_t connectToWifi(std::string ssid, std::string pass)
{
    WiFi.begin(ssid.c_str(), pass.c_str());

    wl_status_t res = (wl_status_t)WiFi.waitForConnectResult(8000);

    if (res == WL_CONNECTED)
    {
        saveWifiCreds(ssid, pass);
    }

    return res;
}
