/*
 *  remote.h
 *
 *  Remote control for dome via MQTT and HTTP REST API
 *  by Giulio Corradini - Osservatorio di Cavezzo
 */

#pragma once

namespace Remote {
    void Init();
    
    enum available_services {
        MQTT,
        REST,
        TOTAL
    };

    void Enable(int services);
};