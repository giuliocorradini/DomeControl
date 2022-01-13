#include "mbed.h"
#include "string.h"
#include "mqtt.h"
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


#include "config.h"
#include "mbed_debug.h"


namespace Remote {

void init() {
    //Prepare MQTT subscriptions
    MQTTController::init(CONFIG_MQTT_BROKER_ADDR);
    debug("[remote] Initialised MQTT client\n");
}

Queue<int, 10> BrokerStatus;
void mqtt_thread() {
    init();

    int conn_status;

    int broker_status;
    BrokerStatus.try_put(&(broker_status = 1)); //assign 1 to broker_status and get its pointer &

    do {
        conn_status = MQTTController::yield(1000);
        if(conn_status == MQTT::FAILURE) {
            debug("[MQTT] Error: disconnected from broker");
        }
    } while(conn_status == MQTT::SUCCESS);

    BrokerStatus.try_put(&(broker_status = 0));

    //TODO: reconnect to MQTT broker
}
}