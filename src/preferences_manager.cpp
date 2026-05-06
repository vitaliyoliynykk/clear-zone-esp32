#include <Preferences.h>
#include "preferences_manager.h"
#include "utils.h"

bool hasToken()
{
    Preferences prefs;
    prefs.begin(PREF_AUTH_KEY, true);

    if (!prefs.isKey("moduleToken"))
    {
        prefs.end();
        return false;
    }

    String token = prefs.getString("moduleToken", "");
    prefs.end();

    if (token.isEmpty())
        return false;

    return true;
}

bool loadModuleToken(std::string &moduleToken)
{
    Preferences prefs;
    prefs.begin(PREF_AUTH_KEY, true);

    if (!prefs.isKey("moduleToken"))
    {
        prefs.end();
        return false;
    }

    String token = prefs.getString("moduleToken", "");

    prefs.end();

    if (token.isEmpty())
        return false;

    moduleToken.assign(token.c_str());

    return true;
}

void saveWifiCreds(const std::string &ssid, const std::string &pass)
{
    Preferences prefs;
    prefs.begin(PREF_WIFI_KEY, false);

    prefs.putString("ssid", ssid.c_str());
    prefs.putString("pass", pass.c_str());

    prefs.end();
}

bool loadWifiCreds(std::string &ssid, std::string &pass)
{
    Preferences prefs;
    prefs.begin(PREF_WIFI_KEY, true);

    if (!prefs.isKey("ssid") || !prefs.isKey("pass"))
    {
        prefs.end();
        return false;
    }

    String s = prefs.getString("ssid", "");
    String p = prefs.getString("pass", "");

    prefs.end();

    if (s.isEmpty() || p.isEmpty())
        return false;

    ssid.assign(s.c_str());
    pass.assign(p.c_str());

    return true;
}

void saveModuleCreds(const char *moduleToken, const char *mqttUsername, const char *mqttPass)
{
    Preferences prefs;
    prefs.begin(PREF_AUTH_KEY, false);

    prefs.putString("moduleToken", moduleToken);
    prefs.putString("mqttUsername", mqttUsername);
    prefs.putString("mqttPass", mqttPass);

    prefs.end();
}

void deletePrefs()
{
    Preferences prefs;
    prefs.begin(PREF_AUTH_KEY, false);

    prefs.remove("moduleToken");
    prefs.remove("mqttUsername");
    prefs.remove("mqttPass");

    prefs.end();

    prefs.begin(PREF_WIFI_KEY, false);

    prefs.remove("ssid");
    prefs.remove("pass");

    prefs.end();

    Serial.println("Prefs deleted");
}

bool loadMqttCreds(std::string &mqttUsername, std::string &mqttPass)
{
    Preferences prefs;
    prefs.begin(PREF_AUTH_KEY, true);

    if (!prefs.isKey("mqttUsername") || !prefs.isKey("mqttPass"))
    {
        prefs.end();
        return false;
    }

    String username = prefs.getString("mqttUsername", "");
    String password = prefs.getString("mqttPass", "");

    prefs.end();

    if (username.isEmpty() || password.isEmpty())
        return false;

    mqttUsername.assign(username.c_str());
    mqttPass.assign(password.c_str());

    return true;
}
