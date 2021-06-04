#include "remote.h"
#include "string.h"
#include <regex>
#include "mbed.h"
#include "MQTTClientMbedOs.h"
#include "mqtt/mqtt.h"
#include "dome.h"

using namespace std;

void Remote::Enable(int services) {
    if (services & Remote::MQTT) {

        //Callback function for targeted movement instructions
        MQTTController::subscribe("dome/1/position", [](MQTT::MessageData &msg) {
            printf("MQTT Received targeted movement instruction\n");

            char payload[256];
            strncpy(payload, (char*)msg.message.payload, msg.message.payloadlen > 255 ? 255 : msg.message.payloadlen);
            cmatch matches;

            //Absolute position
            const static regex target_re("^goto ([+-]?\\d+)");
            if(regex_match(payload, matches, target_re)) {
                int target_position = stoi(matches[1]);
                printf("Move dome to absolute position: %d\n", target_position);
            }

            //Relative movement
            const static regex relative_re("^move ([+-]?\\d+)");
            if(regex_match((char *)payload, matches, relative_re)) {
                int correction = stoi(matches[1].str());
                printf("Move dome to relative position: %d\n", correction);
            }
        });

        //Simple movement instructions responder
        MQTTController::subscribe("dome/1/movement", [](MQTT::MessageData &msg) {
            printf("MQTT Received movement instruction\n");

            char payload[256];
            strncpy(payload, (char*)msg.message.payload, msg.message.payloadlen > 255 ? 255 : msg.message.payloadlen);

            if(strcmp("start", payload) == 0) {
                DomeManStart(Dome::MovementDirection::Clockwise);
            } else if (strcmp("stop", payload) == 0) {
                DomeManStop();
            } else if (strcmp("park", payload) == 0) {
                DomePark();
            }
        });
    }
}