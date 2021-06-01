#include "mbed.h"
#include "lcd.h"
#include "touch.h"
#include "gui.h"
#include "io.h"
#include "dome.h"
#include "i2c.h"

//elenco inizio locazioni di memoria EEROM
#define EeromTouchLoc   0   //16 locazioni per calibrazione touchscreen
#define EeromParkLoc    16  //2 locazioni per quota parcheggio

Ticker tick;
 
DigitalOut led(LED1);

volatile int FlagTick=0;

void timeout(){
    FlagTick = 1;
}


int main() {

    GuiInit();
    IoInit();
    TouchInit();    //touchscreen
    DomeInit();     //cupola

    tick.attach(&timeout,0.01);
   
    while(1) {
        
        GuiMain();
        IoMain();
        DomeMain();

        //attende lo scadere dei 10msec
        while (FlagTick == 0);
        FlagTick = 0;
    
    }
}

