#include "remote.h"
#include "string.h"
#include <regex>
#include "mbed.h"
#include "dome.h"
#include "config.h"
#include "mbed_debug.h"
#include "MQTTClient.h"

using namespace std;

void Remote::init() {
    //Prepare MQTT subscriptions
    MQTTController::init(CONFIG_MQTT_BROKER_ADDR);

    //Receive telescope coordinates
    MQTTController::subscribe("T1/telescopio/az", [](MQTT::MessageData &msg) {
        debug("[MQTT] Received telescope azimuth update");

        int azimuth;
        if(!sscanf((char *)msg.message.payload, "%d", &azimuth)) {
            azimuth = azimuth > 0 ? (azimuth <= 90 ? azimuth : 90) : 0; //Clamp in [0, 90] without branching
        } else {
            debug("[MQTT] Error while parsing telescope azimuth");
        }
        
    });

    MQTTController::subscribe("T1/telescopio/alt", [](MQTT::MessageData &msg) {
        debug("[MQTT] Received telescope altitude update");

        int altitude;
        if(sscanf((char *)msg.message.payload, "%d", &altitude)) {
            altitude = altitude > 0 ? (altitude <= 90 ? altitude : 90) : 0; //Clamp in [0, 90] without branching
        } else {
            debug("[MQTT] Error while parsing telescope altitude");
        }
        
    });
    
    //Commands
    MQTTController::subscribe("T1/cupola/cmd", [](MQTT::MessageData &msg) {
        debug("[MQTT] Received command");

        char *command = (char *)msg.message.payload;

        if(strcmp(command, "centra")) {

        } else if (strcmp(command, "insegui")) {

        } else if (strcmp(command, "no_insegui")) {

        } else {
            debug("[MQTT] Unrecognised command received");
        }
    });
}