
#include <Arduino.h>
#include <PubSubClient.h>
#include "mqtt_setup.h"
#include "wifi_manager.h"
#include "preferences_manager.h"
#include "utils.h"

const char *MQTT_HOST = "0a485ffbbe9140d2b244b49d6c3ef435.s1.eu.hivemq.cloud";
const uint16_t MQTT_PORT = 8883;

String topic = "devices/" + CHIP_ID;
String statusTopic = topic + "/status";
String tempTopic = topic + "/temp";
String co2Topic = topic + "/co2";
String pmsTopic = topic + "/pms";

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
        mqtt.setKeepAlive(5);

        const char *willPayload = "{\"status\":\"offline\"}";

        Serial.printf("[MQTT] Connecting to %s ...\n", MQTT_HOST);
        if (!mqtt.connect(CHIP_ID.c_str(), MQTT_USER.c_str(), MQTT_PASS.c_str(), statusTopic.c_str(), 1, true, willPayload))
        {
            Serial.printf("[MQTT] Connect failed, state=%d\n", mqtt.state());
            return false;
        }

        mqtt.publish(statusTopic.c_str(), "{\"status\":\"online\"}");
        Serial.println("[MQTT] Connected!");

        return true;
    }

    return false;
}