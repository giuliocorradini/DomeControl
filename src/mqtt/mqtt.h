#pragma once

#include "mbed.h"
#include "MQTTClientMbedOs.h"

namespace MQTTController {
    extern MQTTClient *client;
    void init();
    void end();

    void publish(const char *topic, const char *msg, int n);
    void publish(const char *topic, const char *msg);
    void subscribe(const char *topic, MQTTClient::messageHandler callback);
};