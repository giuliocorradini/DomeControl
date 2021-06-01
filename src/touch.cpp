#include "touch.h"
#include "mbed.h"
#include "lcd.h"

//ingressi analogici lettura
AnalogIn analog_row(A0);
AnalogIn analog_col(A1);
//pin di comando matrice resistiva
DigitalInOut RowP(PB_12,PIN_INPUT,PullNone,0);
DigitalInOut RowM(PB_14,PIN_INPUT,PullNone,0);
DigitalInOut ColP(PB_13,PIN_INPUT,PullNone,0);
DigitalInOut ColM(PB_15,PIN_INPUT,PullNone,0);

//variabili globali statiche
char TouchStep;                 //passo di conversione dati TouchScreen
int TouchX;
int TouchY;
int KeyPress;
int KeyPressP;
int KeyPressPriv;
int TouchXg,TouchYg;

struct settaggi Settings ;


void TouchInit(void){
    int temp;

    //recuperiamo i dati di calibrazione touchscreen dalla eerom

    i2cmembuff[0] = EeromTouchLoc;              //indirizzo di partenza locazioni eerom
    i2c.write(I2cMemAddr, i2cmembuff, 1, i2cNoEnd); //indirizza la locazione da leggere ma non manda lo stop
    i2c.read(I2cMemAddr, i2cmembuff, 8, i2cEnd);

    temp = i2cmembuff[3];
    temp <<= 8;
    temp |= i2cmembuff[2];
    temp <<= 8;
    temp |= i2cmembuff[1];
    temp <<= 8;
    temp |= i2cmembuff[0];
    Settings.TouchYoffset = temp;

    temp = i2cmembuff[7];
    temp <<= 8;
    temp |= i2cmembuff[6];
    temp <<= 8;
    temp |= i2cmembuff[5];
    temp <<= 8;
    temp |= i2cmembuff[4];
    Settings.TouchYmax = temp;

    //legge altri 8 bytes, la lettura è sequenziale non paginata come la write
    i2c.read(I2cMemAddr, i2cmembuff, 8, false);

    temp = i2cmembuff[3];
    temp <<= 8;
    temp |= i2cmembuff[2];
    temp <<= 8;
    temp |= i2cmembuff[1];
    temp <<= 8;
    temp |= i2cmembuff[0];
    Settings.TouchXoffset = temp;

    temp = i2cmembuff[7];
    temp <<= 8;
    temp |= i2cmembuff[6];
    temp <<= 8;
    temp |= i2cmembuff[5];
    temp <<= 8;
    temp |= i2cmembuff[4];
    Settings.TouchXmax = temp;


}


//****************************************************************
//scansione touschscreen resistivo
//setta KeyPress, KeyPressP impulsiva, TouchX e TouchY
void keyread(void) {

    static int TouchXpre,TouchYpre = 500;       //valori grezzi di posizione tocco
    //static int TouchWait;         //ritardo sensing tocco pulsanti
    //char buff[5]; per diagnosi da sistemare
    int TouchXrange,TouchYrange;
    
    KeyPressP = 0;
    
    switch (TouchStep) {
    case 0:
        //legge la riga
        TouchXg = analog_row.read_u16();
        /*abilita flusso di corrente nella riga
            ed ingresso nella colonna */
        RowP.output();
        RowM.output();
        //ColP.input();
        //ColM.input();
        GPIOB->MODER |= 0xCC000000;     //analog high inpedance input
        GPIOB->PUPDR &= ~0xCC000000;    //pullupp none
        RowP.write(1);
        RowM.write(0);
        break;
    case 1:
        //legge la colonna
        TouchYg = analog_col.read_u16();
        /*abilita flusso di corrente nella colonna
            ed ingresso nella riga */
        ColP.output();
        ColM.output();
        //RowM.input();
        //RowP.input();
        GPIOB->MODER |= 0x33000000;     //analog high inpedance input
        GPIOB->PUPDR &= ~0x33000000;    //pullupp none
        ColP.write(1);
        ColM.write(0);

        //2 letture consecutive prima di decretare vittoria !
        //soglie : inizialmente a 60k ... poi mi accorgo che X legge 59100 e Y 60300 !!
        //causando sporadiche letture spurie se Y scende sotto i 60k (già normalmente è instabile)
        //ho pensato anche di mettere la soglia max pari a TouchXmax o Ymax ma poi diventa impossibile
        //fare la calibrazione dato che la quota per calibrare la prende da qui !!!
        //quindi dato che allo spigolo in basso a dx legge x32k e Y 23k si sono messe le soglie a 40k
        if (TouchXg < 40000 && TouchYg < 40000){
            if ((TouchXg +100) >= TouchXpre && (TouchXpre +100) >= TouchXg){
                if ((TouchYg +100) >= TouchYpre && (TouchYpre +100) >= TouchYg){
                    if (KeyPress == 0) {
                        TouchXrange = (Settings.TouchXmax - Settings.TouchXoffset);
                        TouchYrange = (Settings.TouchYmax - Settings.TouchYoffset);
                        TouchX = TouchXg - Settings.TouchXoffset;
                        TouchX = (TouchX * LCD_XDOTS) / TouchXrange; 
                        TouchY = TouchYg - Settings.TouchYoffset;
                        TouchY = (TouchY * LCD_YDOTS) / TouchYrange; 
/*
diagnosi da sistemare...
                        buff[0]=(char)TouchXg;      //cast a char sennò andiamo ad arare !
                        buff[1]=(char)(TouchX & 0xFF);
                        buff[2]=(char)(TouchX >> 8);
                        buff[3]=(char)TouchYg;
                        buff[4]=(char)(TouchY & 0xFF);
                        buff[5]=(char)(TouchY >> 8);
                        SplayerSendBuff(buff, 6,SpTouchXRaw);*/
                        KeyPressP = 1;
                        KeyPress = 1;
                    };
                };
            };
        } else {
            KeyPress = 0;
        };
        TouchXpre = TouchXg;
        TouchYpre = TouchYg;
        break;

    };
    
    TouchStep += 1;
    if (TouchStep == 2)
        TouchStep = 0;
    
}

//aggiorna i settatti matrice touchscreen, ma solo in RAM   
void TouchSetngsUpdate(int ymin, int ymax, int xmin, int xmax){
    
    Settings.TouchYoffset = ymin;
    Settings.TouchYmax = ymax;
    Settings.TouchXoffset = xmin;
    Settings.TouchXmax = xmax;
}

//salva in EErom i settaggi touchscreen
void TouchSetngsSave(void){
    int temp;
    
    i2cmembuff[0] = EeromTouchLoc;              //questa eerom come primo byte vuole l'indirizzo di partenza a cui scrivere
    temp = Settings.TouchYoffset;
    i2cmembuff[1] = temp & 0xFF;
    temp >>= 8;
    i2cmembuff[2] = temp & 0xFF;
    temp >>= 8;
    i2cmembuff[3] = temp & 0xFF;
    temp >>= 8;
    i2cmembuff[4] = temp;
    temp = Settings.TouchYmax;
    i2cmembuff[5] = temp & 0xFF;
    temp >>= 8;
    i2cmembuff[6] = temp & 0xFF;
    temp >>= 8;
    i2cmembuff[7] = temp & 0xFF;
    temp >>= 8;
    i2cmembuff[8] = temp;
    i2c.write(I2cMemAddr, i2cmembuff, 9, false);

    wait(0.01);                     //la eerom impiega almeno 5msec a scriversi
    
    i2cmembuff[0] = 8;              //la eerom vuole come primo byte la locazione cui scrivere
    temp = Settings.TouchXoffset;
    i2cmembuff[1] = temp & 0xFF;
    temp >>= 8;
    i2cmembuff[2] = temp & 0xFF;
    temp >>= 8;
    i2cmembuff[3] = temp & 0xFF;
    temp >>= 8;
    i2cmembuff[4] = temp;
    temp = Settings.TouchXmax;
    i2cmembuff[5] = temp & 0xFF;
    temp >>= 8;
    i2cmembuff[6] = temp & 0xFF;
    temp >>= 8;
    i2cmembuff[7] = temp & 0xFF;
    temp >>= 8;
    i2cmembuff[8] = temp;
    i2c.write(I2cMemAddr, i2cmembuff, 9, false);
    
}
    