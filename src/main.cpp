#include "mbed.h"
#include "lcd.h"
#include "touch.h"
#include "gui.h"
#include "io.h"
#include "dome.h"

//I2C
extern I2C i2c;
extern char i2cmembuff[9];      //buffer di scambio dati per i2c
#define I2cMemAddr 0xA0         //indirizzo eerom su bus I2C
#define i2cNoEnd    true
#define i2cEnd      false

//elenco inizio locazioni di memoria EEROM
#define EeromTouchLoc   0   //16 locazioni per calibrazione touchscreen
#define EeromParkLoc    16  //2 locazioni per quota parcheggio

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

