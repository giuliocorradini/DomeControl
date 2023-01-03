#include "mbed.h"
#include <cstddef>
#include "string.h"
#include "mqtt.h"
#include "MQTTClient.h"
#include "MQTTSocket.h"
#include "io.h"

MQTT::Client<MQTTSocket, Countdown> *client = nullptr;
MQTTSocket *network = nullptr;

namespace MQTTController {

bool init(char *broker) {
    if(network == nullptr)
        network = new MQTTSocket(net::interfaces::eth0);

    int conn_status = network->connect(broker, 1883);
    if(conn_status != NSAPI_ERROR_OK) {
        debug("[MQTT] Could not establish connection with broker\n");
        return false;
    }

    if(client == nullptr)
        client = new MQTT::Client<MQTTSocket, Countdown>(*network);

    MQTTPacket_connectData connectOptions = MQTTPacket_connectData_initializer;
    connectOptions.keepAliveInterval = 400;
    connectOptions.cleansession = true;
    client->connect(connectOptions);

    return true;
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
    if(!client)
        return;

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
    if(!client)
        return;
    
    client->subscribe(topic, MQTT::QOS0, callback);
}

int yield(int wait_time) {
    if(!client)
        return -1;
    
    return client->yield(wait_time);
}

}


#include "config.h"
#include "mbed_debug.h"


namespace Remote {

Queue<int, 10> BrokerStatus;

void mqtt_thread() {
    
    while(true) {
        if(!client || !client->isConnected())
            MqttInit();

        int conn_status;

        int broker_status;
        BrokerStatus.try_put(&(broker_status = 1)); //assign 1 to broker_status and get its pointer &

        do {
            conn_status = MQTTController::yield(1000);
            if(conn_status == MQTT::FAILURE) {
                debug("[MQTT] Error: disconnected from broker\n");
                ThisThread::sleep_for(500ms);
            }
        } while(conn_status == MQTT::SUCCESS);

        BrokerStatus.try_put(&(broker_status = 0));
    }

    //TODO: reconnect to MQTT broker
}

}

void MqttInit() {
    //Prepare MQTT subscriptions
    debug("[remote] Trying to initialize broker connection\n");
    MQTTController::init(CONFIG_MQTT_BROKER_ADDR);
}
