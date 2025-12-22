
#include <Arduino.h>
#include <PubSubClient.h>
#include "mqtt_setup.h"
#include "wifi_manager.h"
#include "preferences_manager.h"
#include "utils.h"

const char *MQTT_HOST = "0a485ffbbe9140d2b244b49d6c3ef435.s1.eu.hivemq.cloud";
const uint16_t MQTT_PORT = 8883;

PubSubClient mqtt(wifiClient);

void onMqttMessage(char *topic, byte *payload, unsigned int len)
{
    Serial.printf("[MQTT] %s => ", topic);
    for (unsigned i = 0; i < len; i++)
        Serial.print((char)payload[i]);
    Serial.println();
}

bool mqttConnect()
{
    std::string MQTT_USER;
    std::string MQTT_PASS;

    if (loadMqttCreds(MQTT_USER, MQTT_PASS))
    {
        mqtt.setServer(MQTT_HOST, MQTT_PORT);
        mqtt.setCallback(onMqttMessage);

        String clientId = "esp32-" + CHIP_ID;
        String pubTopic = "devices/" + CHIP_ID + "/hello";
        String subTopic = "devices/" + CHIP_ID + "/cmd";

        Serial.printf("[MQTT] Connecting to %s ...\n", MQTT_HOST);
        if (!mqtt.connect(clientId.c_str(), MQTT_USER.c_str(), MQTT_PASS.c_str()))
        {
            Serial.printf("[MQTT] Connect failed, state=%d\n", mqtt.state());
            return false;
        }
        Serial.println("[MQTT] Connected!");

        mqtt.subscribe(subTopic.c_str(), 1);

        String payload = String("{\"msg\":\"hello from ESP32\",\"chip\":\"") + CHIP_ID + "\"}";
        mqtt.publish(pubTopic.c_str(), payload.c_str(), true); // retain=true для зручності
        Serial.printf("[MQTT] Published to %s: %s\n", pubTopic.c_str(), payload.c_str());
        return true;
    }

    return false;
}