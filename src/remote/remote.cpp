#include "remote.h"
#include "string.h"
#include "mbed.h"
#include "dome.h"
#include "gui.h"
#include "config.h"
#include "mbed_debug.h"
#include "mqtt/mqtt.h"

using namespace std;


namespace Remote {
Queue<int, 10> telescope_position;
Queue<int, 10> BrokerStatus;
void init() {
    //Prepare MQTT subscriptions
    MQTTController::init(CONFIG_MQTT_BROKER_ADDR);
    debug("[remote] Initialised MQTT client\n");

    //Receive telescope coordinates
    MQTTController::subscribe("T1/telescopio/az", [](MQTT::MessageData &msg) {
        debug("[MQTT] Received telescope azimuth update\n");

        int azimuth;
        if(sscanf((char *)msg.message.payload, "%d", &azimuth)) {
            azimuth = azimuth > 0 ? (azimuth <= 90 ? azimuth : 90) : 0; //Clamp in [0, 90] without branching
            TelescopePosition = azimuth;
            TelescopeDrawUpdate(azimuth);
        } else {
            debug("[MQTT] Error while parsing telescope azimuth");
        }
        
    });

    MQTTController::subscribe("T1/telescopio/alt", [](MQTT::MessageData &msg) {
        debug("[MQTT] Received telescope altitude update\n");

        int altitude;
        if(sscanf((char *)msg.message.payload, "%d", &altitude)) {
            altitude = altitude > 0 ? (altitude <= 90 ? altitude : 90) : 0; //Clamp in [0, 90] without branching
            TelescopeAlt = altitude;    //non thread-safe!
        } else {
            debug("[MQTT] Error while parsing telescope altitude");
        }
        
    });
    
    //Commands
    MQTTController::subscribe("T1/cupola/cmd", [](MQTT::MessageData &msg) {
        debug("[MQTT] Received command\n");

        char *command_str = (char *)msg.message.payload;

        using namespace Dome;

        API::cmd_actions action;

        if(strncmp(command_str, "Centra", 6) == 0) {
            action = API::CENTER;
        } else if (strncmp(command_str, "Insegui", 7) == 0) {
            action = API::TRACK;
        } else if (strncmp(command_str, "NoInsegui", 10) == 0) {
            action = API::NO_TRACK;
        } else if (strncmp(command_str, "Stop", 4) == 0) {
            action = API::STOP;
        } else {
            debug("[MQTT] Unrecognised command received\n");
            return;
        }

        API::Command *cmd = API::command_queue.alloc();
        cmd->action = action;
        API::command_queue.put(cmd);
        debug("[remote] Enqueued command\n");
    });
}

void thread_routine() {
    init();

    int conn_status;

    int broker_status;
    BrokerStatus.try_put(&(broker_status = 1)); //assign 1 to broker_status and get its pointer &

    do {
        conn_status = MQTTController::yield(1000);
        if(conn_status == MQTT::FAILURE) {
            debug("[MQTT] Error: disconnected from broker");
        }

        int *pos;
        char pos_str[16];
        if(telescope_position.try_get(&pos)) {
            sprintf(pos_str, "%d", *pos);
            MQTTController::publish("T1/cupola/pos", pos_str, true);
        }

    } while(conn_status == MQTT::SUCCESS);

    BrokerStatus.try_put(&(broker_status = 0));

    //TODO: reconnect to MQTT broker
    
}

}