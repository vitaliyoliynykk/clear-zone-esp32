#include <PubSubClient.h>

#pragma once

extern PubSubClient mqtt;
extern String tempTopic;
extern String co2Topic;
extern String pmsTopic;
extern String bmeTopic;

bool mqttConnect();