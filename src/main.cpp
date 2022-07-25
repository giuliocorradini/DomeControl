#include "mbed.h"
#include "lcd.h"
#include "touch.h"
#include "gui.h"
#include "io.h"
#include "dome.h"
#include "i2c.h"
#include "mqtt.h"

Ticker tick;
 
DigitalOut led(LED1);

volatile int FlagTick=0;

void timeout(){
    FlagTick = 1;
}

Thread remote_conn_thread;

int main() {

    GuiInit();
    IoInit();
    TouchInit();    //touchscreen
    MqttInit();
    DomeInit();     //cupola

    /* setup remote connections (MQTT) */
    remote_conn_thread.start(callback(
        Remote::mqtt_thread
    ));

    tick.attach(&timeout, 10ms);
   
    while(1) {
        GuiMain();
        IoMain();
        DomeMain();
        led = !led;
        //attende lo scadere dei 10msec
        while (FlagTick == 0);
        FlagTick = 0;
    
    }

    remote_conn_thread.terminate();
}

