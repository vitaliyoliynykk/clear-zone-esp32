#pragma once

bool hasToken();
void saveWifiCreds(const std::string &ssid, const std::string &pass);
bool loadWifiCreds(std::string &ssid, std::string &pass);
void saveModuleCreds(const char *moduleToken, const char *mqttUsername, const char *mqttPass);
bool loadModuleToken(std::string &moduleToken);
bool loadMqttCreds(std::string &mqttUsername, std::string &mqttPass);
void deletePrefs();