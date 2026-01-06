#include <PubSubClient.h>

#pragma once

extern PubSubClient mqtt;
extern String tempTopic;

bool mqttConnect();