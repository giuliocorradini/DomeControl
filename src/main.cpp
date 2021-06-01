#include "main.h"

Ticker tick;
 
DigitalOut led(LED1);

volatile int FlagTick=0;

//I2C per eerom
I2C i2c(PB_9, PB_8);
char i2cmembuff[9];             //buffer di scambio dati per i2c
 
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

