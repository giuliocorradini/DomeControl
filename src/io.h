#pragma once

#include "mbed.h"

void IoInit(void);
void IoMain(void);

namespace net::interfaces {
    inline NetworkInterface *eth0;
}
