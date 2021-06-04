#include "mqtt.h"
#include "mbed.h"
#include "string.h"
#include "MQTTClientMbedOs.h"
#include "io.h"

using namespace MQTTController;

static TCPSocket sock;
MQTTClient *MQTTController::client;

void init() {
    sock.open(net::interfaces::eth0);

    SocketAddress mqtt_server("192.168.0.27", 1883);
    sock.connect(mqtt_server);

    client = new MQTTClient(&sock);

    MQTTSNPacket_connectData connectOptions;
    connectOptions.duration = 20;
    connectOptions.cleansession = true;
    client->connect(connectOptions);
}

void end() {
    client->disconnect();
    sock.close();
}

/*
 *  Publish a message on a given topic.
 *  Payload can be anything. Expected size in bytes.
 */
void MQTTController::publish(const char *topic, const char *msg, int n) {
    using namespace MQTT;
    Message mqtt_msg = {
        .payload = (void *)msg,
        .payloadlen = n
    };
    
    client->publish(topic, mqtt_msg);
}

/*
 *  Publish a message on given topic.
 *  Message is a C string (NULL terminated).
 */
void MQTTController::publish(const char *topic, const char* msg) {
    publish(topic, msg, strlen(msg));
}

void MQTTController::subscribe(const char *topic, MQTTClient::messageHandler callback) {
    client->subscribe(topic, MQTT::QOS0, callback);
}