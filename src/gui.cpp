#include "gui.h"
#include "mbed.h"
#include "lcd.h"
#include "io.h"
#include "dome.h"
#include "touch.h"
#include "mqtt.h"

//ingresso analogico carico motore
AnalogIn LoadIn(A2);

//BusInOut  MyBus(PC_0,PC_1,PC_2,PC_3,PC_4,PC_5,PC_6,PC_7);
NHLCD   lcd(PC_9,PC_10,PC_12,PC_11,PC_8);

DigitalOut BackLight(PA_7);     //retroilluminazione
int BackLitTimeout = 0;         //timer spegnimento retroilluminazione
int KeyEnable = 0;              //memoria abilitazione touch

//definizione di struttura dati pulsanti 
struct btnstruct {
    int Y;
    int X;
    int Y2;
    int X2;
    char testo[17];
};
    
/*definizione pulsanti
 * per ogni pulsante si indica: X, Y, larghezzaX, Altezza Y,
 * X inizio testo, Y inizio testo, testo, colore
 * in alcuni casi si usa solo per getkey, in quel caso il colore è Cnullo
 *   cioè NON viene disegnato alcunchè !
 */
struct btnstruct Btn[54] = {
    //pagina 0
    {5,240,35,315,"CENTRA"},
    {45,240,75,315,"PARCHEGGIA"},
    {85,240,115,315,"LUCE SCHERMO"},
    {125,240,155,315,"SERVICE"},
    //pagina 1 service
    {45,40,85,150,"ESCI"},
    {145,40,185,150,"CALIBRA TOUCH"},
    {45,180,85,280,"DIAGNOSTICA"},
    {145,180,185,280,"QUOTA PARCHEGGIO"},
    {95,40,134,150,"RAMPA DECEL."},
    //pagina 2 calibrazione touchscreen
    {5,240,35,315,"ESCI"},
    {85,240,115,315,"AVVIA"},
    {125,240,155,315,"PULISCI"},
    //pagina 3 esecuzione touchscreen
    {55,122,85,197,"ANNULLA"},
    //pagina 4 conferma valori touch
    {55,122,85,197,"CONFERMA"},
    //pagina 5 diagnostica
    {5,240,35,315,"ESCI"},
    //pagina 6 conferma memorizzazione parcheggio o esci 
    {55,122,85,197,"CONFERMA"},
    {5,240,35,315,"ESCI"},  
    //pagina 7 conferma azione di parcheggio cupola
    {140,70,170,145,"MUOVI"},
    {140,175,170,250,"ESCI"},
    //pagina 8 calcolo della rampa di decelerazione
    {55,122,85,197,"AVVIA"},
    {5,240,35,315,"ESCI"}, 
};

/*definizione numero pulsante min e max per ogni pagina
 * serve per cercare nell'array Btn con GetKey e ButtonDraw*/
const int BtnPageBoundary [9][2] = {
    {0,3},
    {4,8},
    {9,11},
    {12,12},
    {13,13},
    {14,14},
    {15,16},
    {17,18},
    {19,20},
};

//dati grezzi di calibrazione touchscreen
int XM,XP,YM,YP;

int CurrentPage = 0;
int update = 1;           //flag di aggiornamento schermo
char buff[20];
struct btnstruct * B;       //usato un po ovunque per referenziare la truttura pulsanti

DigitalIn BuiltInButton(PC_13);     //pulsante built in della scheda nucleo, usato per boot con calibrazione touchscreen

#define LinkRedraw 2

//prototipi
void ButtonDraw(int page);
void Page0Show();   //principale
void Page1Show();   //service
void Page2Show();   //calibrazione touch
void Page3Show();   //esegui calibrazione touch
void Page4Show();   //conferma calibrazione touch
void Page5Show();   //diagnostica
void Page6Show();   //memorizza quota parcheggio
void Page7Show();   //conferma parcheggio
void Page8Show();   //calcolo rampa di decelarazione

int getkey(void);
void DomeDrawUpdate(void);
void LoadMeter(uint8_t valore);
void OvTrMeter(uint16_t valore);

//***************** INIZIO ********************
void GuiInit(void){
    update = 1;
    CurrentPage=0;   
    lcd.Init();
    
    //se il pulsante blu user che c'è nella scheda nucleo è premuto
    //entriamo direttamente nella calibrazione touchscreen
    if (BuiltInButton.read() == 0)
        CurrentPage = 3; 
    
    BackLight.write(1);             //avviamo con la backlight accesa
    KeyEnable = 1;


}

void GuiMain(void) {
    
    if (BackLitTimeout++ == 3000){
        //BackLitTimeout = 0;       //incrementerà fino a 2miliardi ma sono 248 giorni !!!
        BackLight.write(0);         //spegne la retroilluminazione
        KeyEnable = 0;              //disabilita il touchscreen
    };
    if (KeyPressP == 1 && BackLitTimeout > 100){            //se è stato premuto un pulsante
        BackLitTimeout = 100;       //così rispondiamo da subito al prossimo tocco
                                    //mentre lo spegnimento luce da pulsante conta da zero
        BackLight.write(1);         //riaccende la retroilluminazione
        KeyEnable = 1;              //riabilita il touchescreen
    };
    
    //aggiorna lo stato del touchscreen
    keyread();
    
    /* gestione pagine */
    switch (CurrentPage) {
    case 0:             //principale
        Page0Show();
        break;
    case 1:             //service
        Page1Show();
        break;
    case 2:             //calibrazione touchscreen
        Page2Show();
        break;
    case 3:             //esecuzione calibrazione touch
        Page3Show();
        break;
    case 4:             //conferma calibrazione touch
        Page4Show();
        break;
    case 5:             //diagnostica
        Page5Show();
        break;
    case 6:             //memorizza quota parcheggio
        Page6Show();
        break;
    case 7:             //conferma azione parcheggio
        Page7Show();
        break;
    case 8:
        Page8Show();
        break;
    };
}

/*************************************************************************
 PAGINA 0
 mostra la posizione corrente di cupola e telescopio
**************************************************************************/
void Page0Show() {
    static int prevang = 0;   //-1 per forzare l'aggiornamento
    static int prevalt = 0;
    static uint8_t LoadRefreshRate=0;
    static uint32_t LoadMetAcc = 0;
    int tmp;    //usata per scartare la prima lettura ADC

    if(!Remote::BrokerStatus.empty())
        update = 1;

    if (update) {
        lcd.clearScreen();

        //pulsanti sottomenu per pagina 0
        ButtonDraw(0);
        
        //punti cardinali
        lcd.graph_text("S",18,118,1);
        lcd.graph_text("N",216,118,1);
        lcd.graph_text("E",118,19,1);
        lcd.graph_text("W",118,216,1);

        // Etichette dei dati di posizione di telescopio e cupola
        lcd.graph_text("cupola",214,6,nero);
        lcd.graph_text("telescopio Az",226,6,nero);
        lcd.graph_text("Alt",226,120,nero);

        // Link con encoder
        lcd.Rect(195,237,81,19,1);  //finestrella link encoder
        GuiEncLinkShow(LinkRedraw);

        // Link con broker
        int *broker_status;
        if(Remote::BrokerStatus.try_get(&broker_status)) {
            GuiBrokerLinkShow(*broker_status);
        } else {
            GuiBrokerLinkShow(LinkRedraw);
        }
        lcd.Rect(220,237,81,19,1);  //finestrella link host
        
        lcd.Rect(3,6,67,10,1);  //finestrella power meter
        lcd.graph_text("carico",15,22,nero);
        lcd.Rect(3,157,73,10,1);    //finestrella monitor oltrecorsa
        lcd.graph_text("oltrecorsa",15,163,nero);
        
    };

    //aggiorniamo la posizione cupola in base a nuovo angolo o ad un aggiornamento schermo
    if (DomePosition != prevang || update) {
        DomeDrawUpdate();
        lcd.fillRect(214,48,8,24,bianco);
        sprintf(buff,"%d\x7F", DomePosition);   // \x7f=127 cioe' ° nella nostra tabella
        lcd.graph_text(buff,214,48,nero);
        prevang = DomePosition;
    };

    if (TelescopePosition.is_changed() || update) {
        TelescopeDrawUpdate(TelescopePosition);
    }

    //aggiorniamo l'altezza telescopio se cambia
    if (TelescopeAlt != prevalt || update) {
        lcd.fillRect(226,148,8,18,bianco);
        sprintf(buff,"%d\x7F",TelescopeAlt);   // \x7f=127 cioe' ° nella nostra tabella
        lcd.graph_text(buff,226,148,nero);
        prevalt = TelescopeAlt;
    };
    
    update = 0;

    switch (getkey()) {
    case 1:                 //pulsante centraggio cupola sul telescopio
        if (DomeParking != 0)   //se stiamo parcheggiando disabilita i pulsanti
            break;
        if (DomeMotion == 1){
            DomeMoveStop();
            //NB DomeMoveStop ripristina anche le scritte sui pulsanti !
        } else {
            //centra la cupola sul telescopio secondo il percorso piu breve
            DomeMoveStart(TelescopePosition,Rollover);
            //NB il pulsante verra' aggiornato da domemovestart tramite guipage0stop0btn
        };
        break;
    case 2:                 //parcheggia cupola
        if (DomeMotion != 0)    //durante un movimento normale non permettere il parcheggio
            break;
        if (DomeParking == 0) {
            CurrentPage = 7;
            update = 1;
        } else {
            DomeMoveStop();
            //NB DomeMoveStop ripristina anche le scritte sui pulsanti !
        };
        break;
    case 3:                 //spegne backlight lcd
        BackLight.write(0);         //spegne la retroilluminazione
        KeyEnable = 0;              //disabilita il touchscreen
        BackLitTimeout = 0;     //così abbiamo una immunità di un secondo prima di riaccendere la backlight, vedi il >100 piu sopra
        break;
    case 4:                 //pulsante pagina di calibrazione
        CurrentPage = 1;
        update = 1;
        break;
    };

    //aggiorna il loadmeter
    if (LoadRefreshRate < 5)
        tmp = LoadIn.read_u16();            //scartando la prima misura la lettura è molto piu stabile !!!
        LoadMetAcc += LoadIn.read_u16()>>8;
    if (LoadRefreshRate++ == 20) {
        LoadMetAcc /= 5;
        LoadMeter(LoadMetAcc);
        LoadRefreshRate = 0;
        LoadMetAcc=0;
    };
    
    OvTrMeter(EncoderPosition);

}


/*************************************************************************
 PAGINA 1
 menu di service
**************************************************************************/
void Page1Show() {
   
    if (update) {
        lcd.clearScreen();
        //pulsanti sottomenu per pagina 0
        ButtonDraw(1);
        
    };
    update = 0;

    switch (getkey()) {
    case 1:                 //pulsante Exit
        CurrentPage = 0;
        update = 1;
        break;
    case 2:                 //calibrazione touchscreen
        CurrentPage = 2;
        update = 1;
        break;
    case 3:                 //diagnostica
        CurrentPage = 5;
        update = 1;
        break;
    case 4:                 // memorizza la quota encoder attuale come quota di parcheccio
        CurrentPage = 6;
        update = 1;
        break;
    case 5:
        CurrentPage = 8;    // calcola rampa decelerazione
        update = 1;
        break;
    };

}

/*************************************************************************
 PAGINA 2
calibrazione touchscreen 
**************************************************************************/
void Page2Show() {
    int dimX,dimY;
    
    if (update) {
        lcd.clearScreen();
        //pulsanti sottomenu per pagina 0
        ButtonDraw(2);
        lcd.graph_text("Premi il tasto AVVIA",20,20,nero);
        lcd.graph_text("poi dovrai premere in successione",30,20,nero);
        lcd.graph_text("i 4 quadrati neri che ti verranno",40,20,nero);
        lcd.graph_text("proposti negli spigoli",50,20,nero);
        lcd.graph_text("dello schermo.",60,20,nero);
        lcd.graph_text("Puoi provare la calibrazione",70,20,nero);
        lcd.graph_text("e ripeterla se non e' soddisfacente",80,20,nero);
        lcd.graph_text("toccando un punto qualsiasi",90,20,nero);
        lcd.graph_text("in questa stessa pagina gia' da ora.",100,20,nero);
    };
    update = 0;

    //mostra dati attuali touch
    sprintf(buff,"X %d Y %d", TouchXg,TouchYg);
    lcd.fillRect(200, 120, 7, 90, bianco);
    lcd.graph_text(buff,200,120,nero);
    sprintf(buff,"X %d Y %d", TouchX,TouchY);
    lcd.fillRect(210, 120, 7, 100, bianco);
    lcd.graph_text(buff,210,120,nero);

    switch (getkey()) {
    case 1:                 //pulsante Exit
        CurrentPage = 0;
        update = 1;
        break;
    case 2:
        CurrentPage = 3;
        update = 1;
        break;
    case 3:                 //pulsante Exit
        lcd.fillRect(0, 0, 240, 320, nogrigio);
        break;
    };

    //disegna un quadratino dove premiano il touch
    if (KeyPressP && TouchY < 240 && TouchX < 320 && TouchX > 0 && TouchY > 0){
        if (TouchX < 5) dimX = 10-TouchX;
        else dimX = 10;
        if (TouchY < 5) dimY = 10-TouchY;
        else dimY = 10;
        lcd.fillRect(TouchY-5,TouchX-5,dimY,dimX,grigio);
    };

}


/*************************************************************************
 PAGINA 3
 esecuzione calibrazione tuochscreen
**************************************************************************/
void Page3Show(void) {
   static int level;
   
    if (update) {
        lcd.clearScreen();
        //pulsanti sottomenu per pagina 0
        ButtonDraw(3);
        level = 0;
        update = 0;
    };

    switch (level){
        case 0:
            lcd.fillRect(115,0,10,10,nero);
            lcd.graph_text("premi il quadratino",115,15,nero);
            level = 1;
            break;
        case 1:
            if (KeyPressP){
                XM = TouchXg;
                level = 2;
            };
            break;
        case 2:
            lcd.clearScreen();
            lcd.fillRect(0,155,10,10,nero);
            lcd.graph_text("premi il quadratino",15,103,nero);
            ButtonDraw(3);
            level = 3;
            break;
        case 3:
            if (KeyPressP){
                YM = TouchYg;
                level = 4;
            };
            break;
        case 4:
            lcd.clearScreen();
            lcd.fillRect(115,310,10,10,nero);
            lcd.graph_text("premi il quadratino",115,191,nero);
            ButtonDraw(3);
            level = 5;
            break;
        case 5:
            if (KeyPressP){
                XP = TouchXg;
                level = 6;
            };
            break;
        case 6:
            lcd.clearScreen();
            lcd.fillRect(230,155,10,10,nero);
            lcd.graph_text("premi il quadratino",221,103,nero);
            ButtonDraw(3);
            level = 7;
            break;
        case 7:
            //fine ! andiamo alla pagina di conferma
            if (KeyPressP){
                YP = TouchYg;
                CurrentPage = 4;
                update = 1;
            };
            break;
    };

    switch (getkey()) {
    case 1:                 //pulsante Exit
        CurrentPage = 2;
        update = 1;
        break;
    };

}



/*************************************************************************
 PAGINA 4
 conferma calibrazione tuochscreen
**************************************************************************/
void Page4Show(void) {
   static int contaripeti;
   
    if (update) {
        lcd.clearScreen();
        //pulsanti sottomenu per pagina 0
        ButtonDraw(4);
        lcd.graph_text("premi CONFERMA entro 10 secondi",100,70,nero);
        lcd.graph_text("altrimenti la calibrazione",110,82,nero);
        lcd.graph_text("verra' ripetuta !",120,112,nero);
        sprintf(buff,"XP %d XM %d", XP,XM);
        lcd.graph_text(buff,140,112,nero);
        sprintf(buff,"YP %d YM %d", YP,YM);
        lcd.graph_text(buff,150,112,nero);
        update = 0;
        contaripeti = 0;
        TouchSetngsUpdate(YM, YP, XM, XP);      //attiva subito i nuovi valori di matrice touchscreen ma solo in RAM
    };

    //timeout di 10 secondi ... può voler dire che non riesce a premere conferma !!!
    if (contaripeti++ == 1000){
        CurrentPage = 3;
        update = 1;
    };
    
    switch (getkey()) {
    case 1:                 //pulsante Conferma
        TouchSetngsSave();  //salva i nuovi settaggi in eerom
        CurrentPage = 2;
        update = 1;
        break;
    };

}

/*************************************************************************
 PAGINA 5
 diagnostica
**************************************************************************/
void Page5Show() {
    static int CycleCounter = 0;
    
    if (update) {
        lcd.clearScreen();
        //pulsanti sottomenu per pagina 5
        ButtonDraw(5);
        lcd.graph_text("Posizione encoder",10,10,nero);
        lcd.graph_text("Posizione cupola ",20,10,nero);
        lcd.graph_text("Quota parcheggio ",30,10,nero);
        lcd.graph_text("Rampa di decelerazione ",40,10,nero);

    };
    update = 0;
    
    //aggiorniamo lo schermo 5 volte/sec
    if (CycleCounter++ == 20){
        CycleCounter = 0;

        lcd.fillRect(10, 124, 7, 36, bianco);
        sprintf(buff,"%d", EncoderPosition);
        lcd.graph_text(buff,10,124,nero);

        lcd.fillRect(20, 124, 7, 36, bianco);
        sprintf(buff,"%d", DomePosition);
        lcd.graph_text(buff,20,124,nero);

        lcd.fillRect(30, 124, 7, 36, bianco);
        sprintf(buff," %d     ", QuotaParcheggio);
        lcd.graph_text(buff,30,124,nero);
        
        lcd.fillRect(40, 150, 7, 36, bianco);
        sprintf(buff," %d     ", StopRampPulses);
        lcd.graph_text(buff,40,150,nero);
    };


    switch (getkey()) {
    case 1:                 //pulsante Exit
        CurrentPage = 0;
        update = 1;
        break;
    };

}

/*************************************************************************
 PAGINA 6
 memorizza quota attuale encoder come quota di parcheggio
**************************************************************************/
void Page6Show(void) {
   
    if (update) {
        lcd.clearScreen();
        //pulsanti sottomenu per pagina 6
        ButtonDraw(6);
        lcd.graph_text("se premi CONFERMA",100,109,nero);
        lcd.graph_text("la posizione attuale della cupola",110,61,nero);
        lcd.graph_text("verra' usata come quota di parcheggio",120,49,nero);
        lcd.graph_text("cioe' la cupola tornera' in questa",130,58,nero);
        lcd.graph_text("posizione ogni volta che premerai",140,61,nero);
        lcd.graph_text("il tasto PARCHEGGIA",150,103,nero);
        update = 0;
    };

    switch (getkey()) {
        case 1:                 //pulsante Conferma
            DomeParkSave();     //salva la quota corrente come quota di parcheggio
            CurrentPage = 0;
            update = 1;
            break;
        case 2:                 //pulsante esci
            CurrentPage = 0;
            update = 1;
            break;
    };

}


/*************************************************************************
 PAGINA 7
 chiede conferma prima di parcheggiare
**************************************************************************/
void Page7Show(void) {
   
    if (update) {
        //LCD.clearScreen();
        //pulsanti sottomenu per pagina 7
        lcd.fillRect(50, 50, 140, 220, bianco);
        lcd.fillRect(70, 240, 90, 30, nogrigio);    //cancella anche le righe grigine dei vecchi pulsanti
        lcd.Rect(55,55,210,130,nero);
        lcd.Rect(56,56,208,128,nero);
        lcd.Rect(57,57,206,126,nero);
        lcd.Rect(61,61,198,118,nero);
        lcd.Rect(62,62,196,116,nero);
        lcd.Rect(63,63,194,114,nero);
        lcd.graph_text("Stai per PARCHEGGIARE",70,97,nero);
        lcd.graph_text("la cupola nella posizione",80,85,nero);
        lcd.graph_text("di riposo.",90,130,nero);
        lcd.graph_text("Premi MUOVI per confermare",100,82,nero);
        lcd.graph_text("ESCI per annullare",110,112,nero);
        ButtonDraw(7);
        update = 0;
    };

    switch (getkey()) {
        case 1:                 //pulsante Conferma
            //cambia a pag.0 il pulsante parcheggia in FERMA
            //NB da ora in poi questo viene fatto da domepark tramite guipage0btn1stop
            //B = &Btn[1];
            DomePark();
            //strcpy(B->testo,"FERMA");
            CurrentPage = 0;
            update = 1;
            break;
        case 2:                 //pulsante esci
            CurrentPage = 0;
            update = 1;
            break;
    };

}

/*
 *  Mostra la pagina per avviare il calcolo della rampa di decelerazione.
 */
void Page8Show() {
    if(update) {
        lcd.clearScreen();
        strcpy(Btn[19].testo, CalibratingSlope ? "FERMA" : "AVVIA");

        ButtonDraw(8);

        lcd.graph_text("Il controller muovera' la cupola per 10 secondi",100,20,nero);
        lcd.graph_text("nel senso di marcia che non fa andare in rollover",110,15,nero);
        lcd.graph_text("l'encoder, poi stacca il segnale e misura da",120,20,nero);
        lcd.graph_text("questo momento per quanti impulsi",130,20,nero);
        lcd.graph_text("la cupola continua a muoversi",140,20,nero);
    
        update = 0;
    }

    switch(getkey()) {
        case 1: //avvia movimento
            if(!CalibratingSlope)
                DomeStartSlopeCalibration();
            else
                DomeResetSlopeCalibration();
            update = 1;
            break;

        case 2: //esci
            if(CalibratingSlope)
                DomeResetSlopeCalibration();

            CurrentPage = 1;
            update = 1;
            break;
    }
}


//****************************************************************
//aggiorna lo schermo
void ScreenUpdate(void) {
    
    update = 1;

}


//****************************************************************
/* restituisce (come impulso dato che usa KeyPressP) il numero di tasto premuto per pagina
 */
int getkey(void) {
    
    int num,inizio,fine; //,Xe,Ye;
    struct btnstruct * B;
    int cntr = 1;
    
    inizio = BtnPageBoundary[CurrentPage][0];
    fine = BtnPageBoundary[CurrentPage][1];
    
    if (KeyPressP && KeyEnable) {
        for (num=inizio;num<=fine;num++) {
            B = &Btn[num];              //il warning è perchè manca la CONST ... fa lo stesso
            //Xe = B->X + B->L;
            //Ye = B->Y + B->H;
            if (TouchX >= B->X && TouchX <= B->X2) 
                if (TouchY >= B->Y && TouchY <= B->Y2){
                    return cntr;
                };
            cntr += 1;
        };
    };
    
    return 0;
}


//****************************************************************
//disegna tutti i pulsanti della pagina passata
void ButtonDraw(int page){
    int n,inizio,fine;
    struct btnstruct * B;

    inizio = BtnPageBoundary[CurrentPage][0];
    fine = BtnPageBoundary[CurrentPage][1];

    for (n=inizio;n<=fine;n++) {
        B = &Btn[n];                //il warning è perchè manca la CONST ... fa lo stesso
        //if (B->colore != Cnullo){   //disegna il pulsante solo se esiste un colore
            lcd.BtnDraw(B->Y, B->X, B->Y2, B->X2, B->testo);
        //};
    };
    
}


// usato per diagnostica varia...
void dummy(char * buff) {
        //sprintf(buff,"XP %d XM %d", Settings.TouchYoffset,Settings.TouchYmax);
        lcd.graph_text(buff,140,112,nero);
        //sprintf(buff,"YP %d YM %d", Settings.TouchXoffset,Settings.TouchXmax);
        //LCD.graph_text(buff,150,112,nero);
}

void DomeDrawUpdate(void){
   int angBegin,angEnd;

    //cerchio cupola
    lcd.DisegnaCerchio();
    //LCD.DrawCircle(120, 120, 90, 1);        
    //LCD.DrawCircle(120, 120, 91, 1);        
    //LCD.DrawCircle(120, 120, 89, 1);
    //LCD.DrawCircle(120, 120, 88, 1);
    if (DomePosition < 20)
        angBegin = 340 + DomePosition;
    else angBegin = DomePosition -20;
    if (DomePosition > 339)
        angEnd = DomePosition - 340;
    else angEnd = DomePosition + 20;
    //sbianca l'arco corrispondente alla posizione del telescopio
    lcd.DrawCircleArc(120, 120, 84, angBegin, angEnd, bianco);
    lcd.DrawCircleArc(120, 120, 86, angBegin, angEnd, bianco);
    //LCD.DrawCircleArc(120, 120, 86, angBegin, angEnd, bianco);
    lcd.DrawCircleArc(120, 120, 88, angBegin, angEnd, bianco);
    //LCD.DrawCircleArc(120, 120, 88, angBegin, angEnd, bianco);
    
}

//aggiorna la posizione attuale del telescopio a schermo
void TelescopeDrawUpdate(int posizione){
    static int PrevTelescope = 0;
    
    if (CurrentPage == 0){
        lcd.DrawTelescope(PrevTelescope, bianco);
        lcd.DrawTelescope(posizione, nero);
        PrevTelescope = posizione;
        lcd.fillRect(226,90,8,24,bianco);
        sprintf(buff,"%d\x7f", posizione);   // \x7f = 127 cioe' ° nella nostra tabella
        lcd.graph_text(buff,226,90,nero);
    };
}

//mostra stato link encoder
void GuiEncLinkShow(int Ok){
    static uint8_t LastState = 0;

    if (Ok == LinkRedraw)
        Ok = (int) LastState;
    if (CurrentPage == 0){
        if (Ok == 1){
            lcd.fillRect(196, 238, 18, 80, bianco);
            //277 e' il centro della finestrella
            lcd.graph_text("encoder OK",201,247,nero);
        } else {
            lcd.fillRect(196, 238, 18, 80, nero);
            lcd.graph_text("encoder DOWN",201,241,bianco);        
        };
    };
    LastState = (uint8_t) Ok;
    
}

//Mostra stato della connessione MQTT col broker
void GuiBrokerLinkShow(int Ok) {
    static uint8_t LastState = 0;

    if (Ok == LinkRedraw)
        Ok = (int) LastState;

    if (CurrentPage == 0){
        if (Ok == 1){
            lcd.fillRect(221, 238, 18, 80, bianco);
            lcd.graph_text("broker OK",226,250,nero);
        } else {
            lcd.fillRect(221, 238, 18, 80, nero);
            lcd.graph_text("broker DOWN",226,245,bianco);        
        };
    };

    LastState = (uint8_t) Ok;
}

//al fermo cupola ripristina le scritte CENTRA o PARCHEGGIA sui pulsanti
//richiamata da domeStop in dome.cpp
void GuiPage0BtnRestore(void){
    
    B = &Btn[0];
    strcpy(B->testo,"CENTRA");
    B = &Btn[1];
    strcpy(B->testo,"PARCHEGGIA");

    if (CurrentPage == 0){
        update = 1;         //rinfreschiamo tutto lo schermo che facciamo prima !
    };
}

//allo start cupola da gui o dal server
//aggiorna il pulsante 0 di centra in FERMA
void GuiPage0Btn0Stop(void) {
    B = &Btn[0];
    lcd.fillRect(16,260,8,36,bianco);
    lcd.graph_text("FERMA",16,263,1);
    strcpy(B->testo,"FERMA");
}
//idem per il pulsante parcheggia
void GuiPage0Btn1Stop(void) {
    B = &Btn[1];
    strcpy(B->testo,"FERMA");
}

//aggiorna il valore di loadmeter in pagina 1
//LCD.Rect(3,6,68,10,1);  //finestrella power meter
void LoadMeter(uint8_t valore){
    static uint8_t preval=0;    //valore alla chiamata precedente

    // 68 -4 di bordo = 64 pixel orizzontali
    valore = valore >> 2;   //265->64
    if (valore > preval)    //incrementa barra
        lcd.fillRect(5,8+preval,7,valore-preval,1);  //disegna l'aumento della barra
    if (valore < preval)    //decrementa barra
        lcd.fillRect(5,8+valore,7,preval-valore,0);  //sbianca la parte di cui abbiamo calato
    //se valore e preval sono == non fare nulla !
    preval = valore;
}

//aggiorna il valore della barra di oltrecorsa in pagina 1
//LCD.Rect(3,157,73,10,1);  //finestrella meter
void OvTrMeter(uint16_t valore){
    static uint16_t preval=0;    //valore alla chiamata precedente

    // 68 -4 di bordo = 64 pixel orizzontali
    valore = valore >> 9;   //36000->70
    if (valore > preval)    //incrementa barra
        lcd.fillRect(5,159+preval,7,valore-preval,1);  //disegna l'aumento della barra
    if (valore < preval)    //decrementa barra
        lcd.fillRect(5,159+valore,7,preval-valore,0);  //sbianca la parte di cui abbiamo calato
    //se valore e preval sono == non fare nulla !
    preval = valore;
}
