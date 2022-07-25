#!/bin/bash

docker run --rm -it -p 1883:1883 -v "$(pwd)/mosquitto.conf:/mosquitto/config/mosquitto.conf" eclipse-mosquitto
