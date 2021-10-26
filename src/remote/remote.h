/*
 *  remote.h
 *
 *  Remote control for dome via MQTT and HTTP REST API
 *  by Giulio Corradini - Osservatorio di Cavezzo
 */

#pragma once

#include "mbed.h"

namespace Remote {
    void init();
    void thread_routine();

    extern Queue<int, 10> telescope_position;
};