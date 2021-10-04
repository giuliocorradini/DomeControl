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
    extern MQTTClient *client;

    // Creates an MQTTClient and connects to a broker
    void init(char *broker);
    void end();

    void publish(const char *topic, const char *msg, int n);
    void publish(const char *topic, const char *msg);
    void subscribe(const char *topic, MQTTClient::messageHandler callback);
};