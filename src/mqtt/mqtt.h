/*
 *  mqtt.h
 *
 *  MQTT remote control for dome
 *  DomeControl - (C) 2021 Osservatorio di Cavezzo 
 */

#pragma once

#include "mbed.h"
#include "MQTTClient.h"

namespace MQTTController {
    // Creates an MQTTClient and connects to a broker
    void init(char *broker);
    void end();

    void publish(const char *topic, const char *msg, int n, bool retain = false);
    void publish(const char *topic, const char *msg, bool retain = false);

    typedef void (*MessageHandler_t)(MQTT::MessageData &);
    void subscribe(const char *topic, MessageHandler_t callback);

    int yield(int wait_time);
};