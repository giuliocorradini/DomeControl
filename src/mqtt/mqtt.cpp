#include "mbed.h"
#include "string.h"
#include "mqtt/mqtt.h"
#include "MQTTClient.h"
#include "MQTTSocket.h"
#include "io.h"

MQTT::Client<MQTTSocket, Countdown> *client;
MQTTSocket *network;

namespace MQTTController {

void init(char *broker) {
    network = new MQTTSocket(net::interfaces::eth0);
    network->connect(broker, 1883);
    client = new MQTT::Client<MQTTSocket, Countdown>(*network);

    MQTTPacket_connectData connectOptions = MQTTPacket_connectData_initializer;
    connectOptions.keepAliveInterval = 400;
    connectOptions.cleansession = true;
    client->connect(connectOptions);
}

void end() {
    client->disconnect();
    network->disconnect();
}

/*
 *  Publish a message on a given topic.
 *  Payload can be anything. Expected size in bytes.
 */
void publish(const char *topic, const char *msg, int n, bool retain) {
    using namespace MQTT;
    Message mqtt_msg = {
        .retained = retain,
        .payload = (void *)msg,
        .payloadlen = n
    };
    
    client->publish(topic, mqtt_msg);
}

/*
 *  Publish a message on given topic.
 *  Message is a C string (NULL terminated).
 */
void publish(const char *topic, const char* msg, bool retain) {
    publish(topic, msg, strlen(msg), retain);
}

void subscribe(const char *topic, MessageHandler_t callback) {
    client->subscribe(topic, MQTT::QOS0, callback);
}

int yield(int wait_time) {
    return client->yield(wait_time);
}

}