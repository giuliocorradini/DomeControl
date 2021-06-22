/*
 *  remote.h
 *
 *  Remote control for dome via MQTT and HTTP REST API
 *  by Giulio Corradini - Osservatorio di Cavezzo
 */

#pragma once

namespace Remote {
    enum available_services {
        MQTT,
        TOTAL_SERVICES
    };

    void Init(enum available_services services);
};