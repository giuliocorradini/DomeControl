#include "remote.h"
#include "string.h"
#include "mbed.h"
#include "dome.h"
#include "config.h"
#include "mbed_debug.h"
#include "mqtt/mqtt.h"

using namespace std;


namespace Remote {
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

        if(strncmp(command_str, "centra", 6) == 0) {
            action = API::CENTER;
        } else if (strncmp(command_str, "insegui", 7) == 0) {
            action = API::TRACK;
        } else if (strncmp(command_str, "no_insegui", 10) == 0) {
            action = API::NO_TRACK;
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

    do {
        conn_status = MQTTController::yield(1000);
        if(conn_status == MQTT::FAILURE) {
            debug("[MQTT] Error: disconnected from broker");
        }
    } while(conn_status == MQTT::SUCCESS);

    //TODO: reconnect to MQTT broker
    
}

}