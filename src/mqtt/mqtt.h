#pragma once

#include "mbed.h"
#include "MQTTClientMbedOs.h"

class MQTTController {
    public:
        static MQTTController& getInstance();

        void publish(const char *topic, const char *msg, int n);
        void publish(const char *topic, const char *msg);

        void subscribe(const char *topic, MQTTClient::messageHandler callback);

    private:
        TCPSocket sock;
        MQTTClient *client;

        MQTTController();
        ~MQTTController();

        MQTTController(const MQTTController&)   = delete;
        void operator= (const MQTTController&)  = delete;
        
};