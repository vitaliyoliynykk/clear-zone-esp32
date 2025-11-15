#include <BLECharacteristic.h>
#include "wifi_manager.h"
#include <WiFi.h>
#include <Preferences.h>

Preferences prefs;

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
void autoconnectToWifi()
{
    std::string ssid;
    std::string pass;

    if (loadCreds(ssid, pass))
    {
        WiFi.begin(ssid.c_str(), pass.c_str());

        wl_status_t res = (wl_status_t)WiFi.waitForConnectResult(15000);
        if (res == WL_CONNECTED)
        {
            Serial.print("Connected. IP: ");
            Serial.println(WiFi.localIP());
        }
        else
        {
            Serial.printf("Status=%d (6=DISCONNECTED, 4=CONNECT_FAILED, 1=NO_SSID)\n", WiFi.status());
        }
    }
}

wl_status_t connectToWifi(std::string ssid, std::string pass)
{
    WiFi.begin(ssid.c_str(), pass.c_str());

    wl_status_t res = (wl_status_t)WiFi.waitForConnectResult(8000);

    if (res == WL_CONNECTED)
    {
        saveCreds(ssid, pass);
    }

    return res;
}

void saveCreds(const std::string &ssid, const std::string &pass)
{
    prefs.begin("wifi", false);

    prefs.putString("ssid", ssid.c_str());
    prefs.putString("pass", pass.c_str());

    prefs.end();
}

bool loadCreds(std::string &ssid, std::string &pass)
{
    prefs.begin("wifi", true);

    String s = prefs.getString("ssid", "");
    String p = prefs.getString("pass", "");

    prefs.end();

    if (s.isEmpty() || p.isEmpty())
        return false;

    ssid.assign(s.c_str());
    pass.assign(p.c_str());

    return true;
}