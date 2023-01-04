#include "dome.h"
#include "mbed.h"
#include "eerom.h"
#include "gui.h"
#include "mqtt.h"
#include "historic.h"
#include <cstring>

int EncoderPosition = 0;    //posizione attuale cupola in impulsi encoder assoluti 
int QuotaParcheggio = 0;    //quota cui parcheggiare la cupola in impulsi encoder
//int DomePosition = 0;       //posizione cupola 0/360 in gradi
Historic<int> DomePosition;
int DomeMotion = 0;         //flag di cupola in movimento
Historic<int> TelescopePosition;  //posizione telescopio 0/360 in gradi
int TelescopeAlt = 0;       //altezza telescopio
int DomeParking = 0;        //flag di cupola in parcheggio
int DomeAbsTarget = 0;      //target assoluto di movimento cupola in impulsi encoder
int temp;
int DomeOneRotPulses = 12500;       //quanti impulsi encoder per giro di cupola
int MemDir = 0;             //memoria di direzione intrapresa
int DomeShiftZero =0;
int StopRampPulses = 300;     //quanti impulsi impiega la cupola a fermarsi dalla velocità massima
int DomeManMotion = 0;      //movimento manuale in corso

struct {
    int start;
    int finish;
} DomePositionDecelerating; //salva la quota dell'encoder durante la rampa di decelerazione
bool CalibratingSlope = false;

bool isUserOverriding = false;  //true se l'utente sta pilotando la cupola manualmente coi pulsanti
bool isTracking = false;

int TrackingStopAngle = 5;  //Angolo che determina se staccare l'inseguimento se dopo un aggiornamento (ogni secondo) della posizione
                            //del telescopio, questo si è mosso di più dell'angolo

DigitalInOut CwOut(PA_6,PIN_OUTPUT,PullNone,0);
DigitalInOut CcwOut(PA_5,PIN_OUTPUT,PullNone,0);

//InterruptIn ManualMovementButton(PA_7, PullNone);
DigitalInOut CwTrack(PA_8,PIN_OUTPUT,PullNone,0);       //movimento lento di inseguimento

int SlopeCycleCounter = 0;

namespace Dome::API {
    Mail<Command, 10> command_queue;
    Mutex tele_pos_mutex;   //Lock this mutex when writing/reading TelescopeAlt and TelescopePosition
}

void BindMqttCallbacks();
void DomeStopSlopeCalibration();

void InputOverrideISR() {
    //isUserOverriding = true;
}

void InputOverrideStopISR() {
    //isUserOverriding = false;
}

/*
 *  Calcola la distanza in termini assoluti tra due angoli della posizione del telescopio [0, 360]
 *  tenendo in considerazione che potrei superare un giro completo della cupola muovendomi.
 *  Se ad esempio passo da 355° a 5° mi sono mosso solo di 10°
 */
int AngularDelta(Historic<int> x) {
    int more, less;

    if(x.get_current() >= x.get_last()) {
        more = x.get_current();
        less = x.get_last();
    } else {
        more = x.get_last();
        less = x.get_current();
    }

    int cwdelta = abs(more - less);
    int ccwdelta = (360 - cwdelta);

    return cwdelta < ccwdelta ? cwdelta : ccwdelta;
}

void DomeInit(void) {

    //recupera la quota di parcheggio cupola dalla eerom
    eerom_load((char *)&QuotaParcheggio, sizeof(int), EeromParkLoc);
    eerom_load((char *)&StopRampPulses, sizeof(int), EeromStopRampPulsesLoc);

    DomeShiftZero = (DomeOneRotPulses * 2) - 18000;      //shift posizione = metà encoder così a centro encoder avremo posizione = 0 quando verra' reiteratamente ridotto di 12500 , nota che l'algo vuole solo numeri positivi
                                //dato che ne ricaverà un numero 0/360, una while sottrarrà + volte 12500 fino ad essere 0/12500

    // Collega callback alle ISR dei pulsanti
    //ManualMovementButton.rise(InputOverrideISR);
    //ManualMovementButton.rise(InputOverrideISR);

    MQTTController::bind_sub_callbacks_cb = BindMqttCallbacks;
}

// Coda per processare la lista dei movimenti che dobbiamo compiere (ad esempio centrare prima del tracking)
Queue<int, 20> movement_process_queue;
inline bool GetNextMovement(int &next) {
    return movement_process_queue.try_get(reinterpret_cast<int**>(&next));
}

inline void EmptyProcessQueue() {
    int *dump;
    while(movement_process_queue.try_get(&dump));
}

//richiamato ciclicamente da Main
void DomeMain(void){
    int next_mov;

    if(isTracking && AngularDelta(TelescopePosition) >= TrackingStopAngle)
        DomeMoveStop();
 
    //se siamo in movimento controlliamo l'arrivo in quota
    if (DomeMotion || DomeParking) {

        if (MemDir == Cw) {
            if (EncoderPosition >= DomeAbsTarget)
                DomeMoveStop();
        } else {
            if (EncoderPosition <= DomeAbsTarget)
                DomeMoveStop();
        };

    };

    //Controllo la posta per eseguire nuovi comandi dal modulo MQTT
    using namespace Dome::API;
    Command *cmd;

    if((cmd = command_queue.try_get())) { //Leggo l'ultimo comando

        debug("[dome] Received command %d\n", *cmd);

        switch(*cmd) {
            case CENTER:
                movement_process_queue.try_put((int *)Rollover);
                break;
            case TRACK:
                isTracking = true;
                movement_process_queue.try_put((int *)Rollover);
                movement_process_queue.try_put((int *)Tracking);
                break;
            case STOP:
            case NO_TRACK:
                isTracking = false;
                DomeMoveStop();
                //Pulisce la coda di processo
                EmptyProcessQueue();
                break;
        }

        command_queue.free(cmd);    //Libero spazio nella cassetta postale
    }

    if(CalibratingSlope) {
        SlopeCycleCounter++;

        if(SlopeCycleCounter == 500) {   //dopo cinque secondi stacca il motore
            CwOut = 0;

            DomePositionDecelerating.start = EncoderPosition;

            debug("[slope] Stopping motor\n");
        }

        if(SlopeCycleCounter == 700) {   //aspetta altri due secondi
            DomePositionDecelerating.finish = EncoderPosition;
            DomeStopSlopeCalibration();
        }
    }

    //Se non stai facendo alcun movimento, avvia un nuovo movimento dalla coda di processo
    // Controllando il flag DomeMotion, avviamo un nuovo movimento (con DomeMoveStart) solo quando
    // ci siamo fermati. Ovvero quando abbiamo completato un movimento richeisto precedentemente,
    // visto che DomeMoveStop resetta il flag, oppure se è arrivato il comando di DomeMoveStop.
    else if(!DomeMotion && !CalibratingSlope && GetNextMovement(next_mov)) {
        DomeMoveStart(TelescopePosition, next_mov);
    }

    //trasmetti la quota attuale al broker se questa cambia
    char dome_pos_str[8];

    if(DomePosition.is_changed()) {
        sprintf(dome_pos_str, "%d", static_cast<int>(DomePosition));
        MQTTController::publish("T1/cupola/pos", dome_pos_str, true); //TODO: la pubblicazione accoda il messaggio, non lo invia da questo thread
    }

}

//salva la quota corrente come quota di parcheggio
void DomeParkSave(void){

    QuotaParcheggio = EncoderPosition;
    eerom_store((char *)&QuotaParcheggio, sizeof(int), EeromParkLoc);
}

/*avvia il movimento della cupola alla quota passata in gradi
  se Absolute posiziona alla quota encoder assoluta -360/+360
  se Rollover trova il percorso piu breve per fare 0/360
  se Tracking posiziona alla quota assoluta ma non muove in caso di overtravel limiti encoder
  ritorna 0 se non ha mosso perchè già sul target
  1 se movimento avviato regolarmente
  -1 se impossibile (tracking fuori limiti)*/
int DomeMoveStart(int target, int type) {
    debug("[dome] Avviato un movimento in %d al target %d\n", type, target);
    debug("[dome] DomePosition=%d\n", DomePosition.get_current());

    int moto = 0;
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
        if (moto > 359)
            moto -= 360;        
        else if (moto < -359)
            moto += 360;
        
        break;
    case Absolute:
    case Tracking:
        //moto assoluto -360+360 
        //rendendo eventualmente possibile fare +720 !
        moto = target;    
        break;
    };

    debug("[dome] moto=%d\n", moto);

    //esce se non dobbiamo muovere
    if (moto == 0)
        return 0;    

    //converti il moto incrementale appena calcolato in impulsi encoder
    // sto calcolando la seguente proporzione
    // <Impulsi da fare per raggiungere target> : <Impulsi in un giro di cupola> = <moto> : 360
    // che diventa <impulsi target> = <moto> * <Impulsi giro> / 360
    moto *= DomeOneRotPulses;
    moto /= 360;

    //Adesso moto è sempre l'angolo relativo da percorrere per arrivare al target, ma espresso
    //in impulsi dell'encoder

    /*fai una previsione sul target finale e controlla che non sia 
    finito oltre i limiti dell'encoder 0/36000
    questo per evitare disastrosi rollover
    con un po di tolleranza per tenere conto delle rampe di accelerazione poco precise
    */
    PrevisioneMovimento = EncoderPosition + moto;   //dove dovrei arrivare alla fine del movimento
    debug("[dome] PrevisioneMovimento=%d, EncoderPosition=%d\n", PrevisioneMovimento, EncoderPosition);
    switch (type){
    case Rollover:
    case Absolute:
        if (PrevisioneMovimento > 35000)    //35000 è la tolleranza per evitare i rollover
            moto -= DomeOneRotPulses;
        if (PrevisioneMovimento < 1000)     //per evitare un "rollunder"
            moto += DomeOneRotPulses;
        break;
    case Tracking:  //non abilitare il tracking se rischio il roll
        if (PrevisioneMovimento > 35000 || PrevisioneMovimento < 1000)
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

/*
 *  Avvia il movimento pilotando le uscite ai pulsanti dell'inverter. Calcola la posizione
 *  assoluta del target tenendo conto delle rampe di accelerazione e decelerazione.
 *  
 *  @param Dist2Go: la quota incrementale espressa come numero di impulsi dell'encoder
 *  che rappresenta l'angolo relativo alla posizione corrente da percorrere.
 *  Un angolo positivo indica un movimento orario, negativo antiorario.
 * 
 *  @return -1 se la distanza da percorrere è 0, altrimenti 1
 */
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

    if(isUserOverriding) {
        // se l'utente sta pilotando manualmente dai pulsanti ferma ogni movimento
        DomeMoveStop();
        return -1;
    }
    // Eventuali spike di segnale dovuti a un .write(1) con un conseguente .write(0) vengono filtrati
    // dall'hardware e comunque l'uscita è già a 1

    return 1;    
}


//interrompe un movimento automatico della cupola
void DomeMoveStop(void) {
    DomeMotion = 0;
    DomeParking = 0;
    DomeManMotion = 0;
    isTracking = false;
    CwOut.write(0);
    CcwOut.write(0);
    MemDir = 0;
    GuiPage0BtnRestore();       //toglie eventuali pulsanti FERMA
    
    debug("[dome] Movimento fermato\n");
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


//Possibili stati della cupola
enum status_t {
    TrackingOff = 1 << 0,
    TrackingOn  = 1 << 1,
    NotMoving   = 1 << 2,
    Moving      = 1 << 3
};

//Codifica lo stato corrente della cupola
status_t encode_status() {
    using namespace Remote;
    int s = 0;

    s |= (isTracking) ? TrackingOn : TrackingOff;

    if(DomeMotion || DomeManMotion)
        s |= Moving;
    else if(!DomeMotion && !DomeManMotion)
        s |= NotMoving;

    return (status_t)s;
}

Queue<int, 10> telescope_position;

/*
 *  Manda il comando ricevuto da MQTT (Track, Center, Stop etc.) alla coda
 *  di processamento che DomeMain legge a ogni iterazione.
 */
void PassReceivedCommandOn(Dome::API::Command action) {
    using namespace Dome::API;
    Command *cmd = command_queue.try_alloc();
    if(cmd) {
        *cmd = action;
        command_queue.put(cmd);
    }
}

/*
 *  Aggiorna il valore di TelescopePosition (in modo thread-safe) e se la nuova posizione differisce
 *  da quella attuale di più di 5°, allora ferma ogni movimento automatico della cupola e stacca
 *  l'inseguimento.
 */
void TelescopePositionUpdate(int new_pos) {
    if(abs(new_pos - TelescopePosition) >= TELESCOPE_POSITION_DELTA_THRESHOLD) {
        DomeMoveStop();
    }

    SAFE_TELESCOPE_UPDATE(TelescopePosition = new_pos);
}

void MqttAzimuthCallback(MQTT::MessageData &msg) {
    debug("[MQTT] Received telescope azimuth update\n");

    static char buffer[4];
    strncpy(buffer, (char *)msg.message.payload, msg.message.payloadlen > 3 ? 3 : msg.message.payloadlen);

    int azimuth;

    if(sscanf(buffer, "%d", &azimuth)) {
        azimuth = azimuth > 0 ? (azimuth < 360 ? azimuth : 359) : 0; //Clamp in [0, 360) without branching
        TelescopePositionUpdate(azimuth);
    } else {
        debug("[MQTT] Error while parsing telescope azimuth");
    }
}

void MqttAltitudeCallback(MQTT::MessageData &msg) {
    debug("[MQTT] Received telescope altitude update\n");

    static char buffer[4];
    strncpy(buffer, (char *)msg.message.payload, msg.message.payloadlen > 2 ? 2 : msg.message.payloadlen);

    int altitude;
    if(sscanf((char *)buffer, "%d", &altitude)) {
        altitude = altitude > 0 ? (altitude <= 90 ? altitude : 90) : 0; //Clamp in [0, 90] without branching
        SAFE_TELESCOPE_UPDATE(TelescopeAlt = altitude);
    } else {
        debug("[MQTT] Error while parsing telescope altitude");
    }
}

void MqttCommandCallback(MQTT::MessageData &msg) {
    debug("[MQTT] Received command\n");

    char *command_str = (char *)msg.message.payload;

    using namespace Dome::API;
    Command action;

    if(strncmp(command_str, "Centra", 6) == 0) {
        action = CENTER;
    } else if (strncmp(command_str, "Insegui", 7) == 0) {
        action = TRACK;
    } else if (strncmp(command_str, "NoInsegui", 10) == 0) {
        action = NO_TRACK;
    } else if (strncmp(command_str, "Stop", 4) == 0) {
        action = STOP;
    } else {
        debug("[MQTT] Unrecognised command received\n");
        return;
    }

    PassReceivedCommandOn(action);
    debug("[remote] Enqueued command\n");
}

/*  
 *  This function binds callbacks with the MQTT client for required subscription
 *  Right now those functions are enabled:
 *      - T1/telescopio/az: azimuth update from mount controller
 *      - T1/telescopio/alt: receive current altitude from mount controller
 *      - T1/cupola/cmd: receive control commands
 * 
 *  MQTT Client initialisation should already be done when calling this function
 *  (you may start Remote::mqtt_thread)
 */
void BindMqttCallbacks() {
    //Receive telescope coordinates
    MQTTController::subscribe("T1/telescopio/az", MqttAzimuthCallback);
    MQTTController::subscribe("T1/telescopio/alt", MqttAltitudeCallback);

    //Commands
    MQTTController::subscribe("T1/cupola/cmd", MqttCommandCallback);
}

void DomeStartSlopeCalibration() {
    debug("[dome][slope] Starting slope calibration\n");
    EmptyProcessQueue();
    DomeMoveStop();

    DomePositionDecelerating = {0, 0};

    CalibratingSlope = true;

    SlopeCycleCounter = 0;
    CwOut = 1;
}

void DomeStopSlopeCalibration() {
    CalibratingSlope = false;
    StopRampPulses = abs(DomePositionDecelerating.finish - DomePositionDecelerating.start);

    ScreenUpdate();

    debug("[dome][slope] Calibration stopped. Ramp pulses: %d\n", StopRampPulses);

    eerom_store((char *)&StopRampPulses, sizeof(int), EeromStopRampPulsesLoc);
}

void DomeResetSlopeCalibration() {
    CalibratingSlope = false;
    debug("[dome][slope] Calibration canceled. Reloading slope value from memory\n");
    eerom_load((char *)&StopRampPulses, sizeof(int), EeromStopRampPulsesLoc);
}