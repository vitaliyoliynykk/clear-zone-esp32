#include <Preferences.h>
#include "preferences_manager.h"
#include "utils.h"

Preferences prefs;

bool hasToken()
{
    prefs.begin(PREF_AUTH_KEY, true);

    String token = prefs.getString("moduleToken", "");
    prefs.end();

    if (token.isEmpty())
        return false;

    return true;
}

void saveWifiCreds(const std::string &ssid, const std::string &pass)
{
    prefs.begin(PREF_WIFI_KEY, false);

    prefs.putString("ssid", ssid.c_str());
    prefs.putString("pass", pass.c_str());

    prefs.end();
}

bool loadWifiCreds(std::string &ssid, std::string &pass)
{
    prefs.begin(PREF_WIFI_KEY, true);

    String s = prefs.getString("ssid", "");
    String p = prefs.getString("pass", "");

    prefs.end();

    if (s.isEmpty() || p.isEmpty())
        return false;

    ssid.assign(s.c_str());
    pass.assign(p.c_str());

    return true;
}