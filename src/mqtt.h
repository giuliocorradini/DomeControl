/*
 *  mqtt.h
 *
 *  Provides a global unique MQTT client through the MQTTController namespace.
 *  For higher level usage you should use functions from the Remtoe namespace.
 *  DomeControl - (C) 2021 Osservatorio di Cavezzo 
 */

#pragma once

#include "mbed.h"
#include "MQTTClient.h"

//  Low level setup of MQTT client
namespace MQTTController {
    // Creates an MQTTClient and connects to a broker
    bool init(char *broker_hostname);
    void end();

    void publish(const char *topic, const char *msg, int n, bool retain = false);
    void publish(const char *topic, const char *msg, bool retain = false);

    typedef void (*MessageHandler_t)(MQTT::MessageData &);
    void subscribe(const char *topic, MessageHandler_t callback);

    int yield(int wait_time);

    extern void (*bind_sub_callbacks_cb)(void);
};

namespace Remote {
    //  Handles connection with broker and restarts it automatically if it goes down
    void mqtt_thread();

    extern Queue<int, 10> BrokerStatus;
};

void MqttInit();
