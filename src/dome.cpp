#include "dome.h"
#include "mbed.h"
#include "i2c.h"
#include "eerom.h"
#include "gui.h"

int EncoderPosition = 0;    //posizione attuale cupola in impulsi encoder assoluti 
int QuotaParcheggio = 0;    //quota cui parcheggiare la cupola in impulsi encoder
int DomePosition = 0;       //posizione cupola 0/360 in gradi
int DomeMotion = 0;         //flag di cupola in movimento
int TelescopePosition = 0;  //posizione telescopio 0/360 in gradi
int TelescopeAlt = 0;       //altezza telescopio
int DomeParking = 0;        //flag di cupola in parcheggio
int DomeAbsTarget = 0;      //target assoluto di movimento cupola in impulsi encoder
int temp;
int DomeOneRotPulses;       //quanti impulsi encoder per giro di cupola
int MemDir = 0;             //memoria di direzione intrapresa
int DomeShiftZero =0;
int StopRampPulses = 0;     //quanti impulsi impiega la cupola a fermarsi dalla velocità massima
int DomeManMotion = 0;      //movimento manuale in corso


DigitalInOut CwOut(PA_6,PIN_OUTPUT,PullNone,0);
DigitalInOut CcwOut(PA_5,PIN_OUTPUT,PullNone,0);

namespace Dome::API {
    Mail<Command, 10> command_queue;
    Mutex tele_pos_mutex;   //Lock this mutex when writing/reading TelescopeAlt and TelescopePosition
}

void DomeInit(void) {

    //recupera la quota di parcheggio cupola dalla eerom
    i2cmembuff[0] = EeromParkLoc;              //indirizzo di partenza locazioni eerom
    i2c.write(I2cMemAddr, i2cmembuff, 1, i2cNoEnd); //indirizza la locazione da leggere ma non manda lo stop
    i2c.read(I2cMemAddr, i2cmembuff, 2, i2cEnd);
    temp |= i2cmembuff[1];
    temp <<= 8;
    temp |= i2cmembuff[0];
    QuotaParcheggio = temp;

    DomeOneRotPulses = 12500;        //impulsi per un giro di cupola
    DomeShiftZero = (DomeOneRotPulses * 2) - 18000;      //shift posizione = metà encoder così a centro encoder avremo posizione = 0 quando verra' reiteratamente ridotto di 12500 , nota che l'algo vuole solo numeri positivi
                                //dato che ne ricaverà un numero 0/360, una while sottrarrà + volte 12500 fino ad essere 0/12500
    StopRampPulses = 300;                 //impulsi di rampa frenatura
}

//richiamato ciclicamente da Main
void DomeMain(void){
 
    //se siamo in movimento controlliamo l'arrivo in quota
    if (DomeMotion == 1) {
    
        if (MemDir == Cw){
            if (EncoderPosition >= DomeAbsTarget)
                DomeMoveStop();
        } else {
            if (EncoderPosition <= DomeAbsTarget)
                DomeMoveStop();
        };
    };

    //Controllo la posta per eseguire nuovi comandi dal modulo MQTT
    using namespace Dome;
    API::Command *cmd;

    if(cmd = API::command_queue.try_get()) { //Leggo l'ultimo comando

        debug("[dome] Received command %d\n", cmd->action);

        switch(cmd->action) {
            case API::CENTER:
                DomeMoveStart(cmd->azimuth, Rollover);
                break;
            case API::TRACK:
                DomeMoveStart(cmd->azimuth, Tracking);
                break;
            case API::NO_TRACK:
                DomeMoveStop();
                break;
        }

        API::command_queue.free(cmd);    //Libero spazio nella cassetta postale
    }

}

//salva la quota corrente come quota di parcheggio
void DomeParkSave(void){

    QuotaParcheggio = EncoderPosition;
    i2cmembuff[0] = EeromParkLoc;              //questa eerom come primo byte vuole l'indirizzo di partenza a cui scrivere
    temp = EncoderPosition;
    i2cmembuff[1] = temp & 0xFF;
    temp >>= 8;
    i2cmembuff[2] = temp & 0xFF;
    i2c.write(I2cMemAddr, i2cmembuff, 3, i2cEnd);
}

/*avvia il movimento della cupola alla quota passata in gradi
  se Absolute posiziona alla quota encoder assoluta -360/+360
  se Rollover trova il percorso piu breve per fare 0/360
  se Tracking posiziona alla quota assoluta ma non muove in caso di overtravel limiti encoder
  ritorna 0 se non ha mosso perchè già sul target
  1 se movimento avviato regolarmente
  -1 se impossibile (tracking fuori limiti)*/
int DomeMoveStart(int target, int type) {
    int moto;
    int PrevisioneMovimento;
    
    //accettiamo comandi di movimento solo se non ne stiamo già effettuando uno !
    if (DomeMotion == 1 || DomeManMotion == 1)
        return -1;
        
    //se dobbiamo prendere la strada piu corta guardiamo se abbiamo sforato i 180°
    //e in quel caso applichiamo un algoritmo adatto
    switch (type){
    case Rollover:
        //calcoliamo quanto manca al target
        moto = target - DomePosition;
        //aggiunge un offset in modo da posizionarsi un po piu a destra
        //del tubo telescopio e favorire il moto sidereo
        moto += 5;          //in gradi...
        /*scorciatoia per la posizione 
        se il percorso assoluto da effettuare e' > di mezzo giro 
        vuol dire che dobbiamo andare dalla parte opposta
        siccome poi il valore di posizione cupola e di target possono essere solo positivi
        questo caso puo capitare solo se dobbiamo passare per lo zero
        abbiamo poi due casistiche: se il moto da compiere e' positivo o negativo
        in quel caso dobbiamo sottrarre duepigredo o sommarlo a seconda...
        */
        if (moto > 359 || moto < 359){
            if (moto > 0)
                moto -= 360;
            else
                moto += 360;
        };
        /* vecchio algoritmo if (moto > 180) 
            moto = 360 - target + DomePosition;
        if (moto < -180)
            moto = 360 + target - DomePosition;*/
        break;
    case Absolute:
    case Tracking:
        //moto assoluto -360+360 
        //rendendo eventualmente possibile fare +720 !
        moto = target;    
        break;
    };

    //esce se non dobbiamo muovere
    if (moto == 0)
        return 0;    

    //converti il moto incrementale appena calcolato in impulsi encoder
    moto *= DomeOneRotPulses;
    moto /= 360;

    /*fai una previsione sul target finale e controlla che non sia 
    finito oltre i limiti dell'encoder 0/36000
    questo per evitare disastrosi rollover
    con un po di tolleranza per tenere conto delle rampe di accelerazione poco precise
    */
    PrevisioneMovimento = EncoderPosition + moto;
    switch (type){
    case Rollover:
    case Absolute:
        if (PrevisioneMovimento > 35000)
            moto -= DomeOneRotPulses;
        if (PrevisioneMovimento < 1000)
            moto += DomeOneRotPulses;
        break;
    case Tracking:
        if (PrevisioneMovimento > 35000)
            return -1;
        if (PrevisioneMovimento < 1000)
            return -1;
        break;
    };
        

    //avvia il movimento necessario
    if (MotionStart(moto) == -1)
        return 0;     //esci se non muoviamo

    DomeMotion = 1;
    GuiPage0Btn0Stop();     //aggiorna scritta pulsante page0 da centra a ferma

    return 1;   //1 se avviamento ok
}

void DomePark(void){
    int TargetDiff;

    TargetDiff = QuotaParcheggio - EncoderPosition;   //distanza da percorrere

    if (MotionStart(TargetDiff) == -1)
        return;
        
    DomeParking = 1;
    GuiPage0Btn1Stop();     //aggiorna scritta pulsante page0 da parcheggia a ferma
    
}

//avvia il movimento in base alla quota incrementale passata in impulsi
//tiene conto delle rampe
//torna -1 se non ha avviato il movimento
int MotionStart( int Dist2Go ){
    //usciamo se non dobbiamo fare nulla
    if (Dist2Go == 0)
        return -1;

    DomeAbsTarget = EncoderPosition + Dist2Go;
        
    if (Dist2Go > 0){
        CwOut.write(1);     //avvia il movimento +
        MemDir = Cw;
        if (Dist2Go < (StopRampPulses *2))
            DomeAbsTarget -= Dist2Go /2;
        else
            DomeAbsTarget -= StopRampPulses;
    };
    if (Dist2Go < 0){
        CcwOut.write(1);    //avvia il movimento -
        MemDir = Ccw;
        Dist2Go *= -1;
        if (Dist2Go < (StopRampPulses *-2))
            DomeAbsTarget += Dist2Go /2;
        else
            DomeAbsTarget += StopRampPulses;
    };

    return 1;    
}


//interrompe un movimento automatico della cupola
void DomeMoveStop(void) {
    
    DomeMotion = 0;
    DomeParking = 0;
    DomeManMotion = 0;
    CwOut.write(0);
    CcwOut.write(0);
    MemDir = 0;
    GuiPage0BtnRestore();       //toglie eventuali pulsanti FERMA
}

//avvia un movimento manuale in orario o antiorario 
void DomeManStart(int dir) {

    //solo se non ci sono movimenti in corso
    if (DomeMotion == 0 || DomeManMotion == 0) {
        //scegli la direzione
        if (dir == Cw){
            CcwOut.write(1);    //avvia il movimento -
        } else {
            CwOut.write(1);    //avvia il movimento +
        };
        DomeManMotion = 1;
    };
}

//ferma un movimento manuale
//usiamo questa anzichè DomeStop per non fermare eventuali movimenti automatici in corso
void DomeManStop(void) {
    if (DomeManMotion == 1)
        DomeMoveStop();
}
