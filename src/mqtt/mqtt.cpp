#include "mqtt.h"
#include "mbed.h"
#include "string.h"
#include "MQTTClientMbedOs.h"
#include "io.h"

MQTTController::MQTTController() {
    this->sock.open(net::interfaces::eth0);

    SocketAddress mqtt_server("192.168.0.27", 1883);
    this->sock.connect(mqtt_server);

    this->client = new MQTTClient(&sock);

    MQTTSNPacket_connectData connectOptions;
    connectOptions.duration = 20;
    connectOptions.cleansession = true;
    this->client->connect(connectOptions);
}

MQTTController::~MQTTController() {
    this->client->disconnect();
    this->sock.close();
}

MQTTController& MQTTController::getInstance() {
    static MQTTController *instance = new MQTTController();
    return *instance;
}

void MQTTController::publish(const char *topic, const char *msg, int n) {
    using namespace MQTT;
    Message mqtt_msg = {
        .payload = (void *)msg,
        .payloadlen = n
    };
    
    this->client->publish(topic, mqtt_msg);
}

void MQTTController::publish(const char *topic, const char* msg) {
    this->publish(topic, msg, strlen(msg));
}

void MQTTController::subscribe(const char *topic, MQTTClient::messageHandler callback) {
    this->client->subscribe(topic, MQTT::QOS0, callback);
}