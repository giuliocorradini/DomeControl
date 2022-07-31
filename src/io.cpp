#include "io.h"
#include <stdio.h>
#include <string>
#include "mbed.h"
#include "dome.h"
#include "gui.h"
#include "WIZnetInterface.h"
#include "config.h"


BufferedSerial Pc(USBTX,USBRX);


/* Encoder serial connection */
#define EncMaxTimeout 500   // 500 tick is approximately 5 secs

UnbufferedSerial encoder(PA_11,PA_12);
CircularBuffer<char, 32> encoder_rx_buf;

bool EncStringReceiveEndFlag = false;   // < true if the serial encountered a \n, which is the delimiter of the encoder response


/* Ethernet interface */
const char * MyIP_Addr  = CONFIG_IP_ADDRESS;
const char * IP_Subnet  = CONFIG_NETMASK;
const char * IP_Gateway = CONFIG_GATEWAY_IP;
unsigned char MAC_Addr[6] = {0x00,0x08,0xDC,0x12,0x34,0x56};

WIZnetInterface eth(PB_5, PB_4, PB_3, PB_6, PC_4);    // reset pin is dummy, don't affect any pin of WIZ550io


/*
 *  Interrupt service routine to process incoming data on UnbufferedSerial for encoder.
 *  Read a char and puts it on the ring buffer, if it's not full.
 *  If the buffer is full or \n is reached, forces a flush by setting EncStringReceiveEndFlag to true.
 */
void EncoderOnReceiveISR(void){
    char tmp;
    
    encoder.read(&tmp,1);

    if(tmp == '\n' || encoder_rx_buf.full())    // second condition enables forced flush
            EncStringReceiveEndFlag = true;

    if(!encoder_rx_buf.full()) {
        encoder_rx_buf.push(tmp);
    }
}


/*
 *  Setup connection with encoder via a serial port configured as 19200-8-N-1.
 *  An interrupt is attached to the controller in order to process the incoming bytes.
 */
void SetupEncoderSerial() {
    encoder.baud(19200);
    encoder.format(8, SerialBase::None, 1);
    encoder.attach(EncoderOnReceiveISR, SerialBase::RxIrq);

    encoder.write("ND", 2); // Inizializza l'encoder a trasmettere in decimale e no debug mode
}


void IoInit(void){
    int returnCode = 0;

    SetupEncoderSerial();

    // Inizializza la porta seriale connessa al computer (115200-8-N-1)
    Pc.set_baud(115200);
    Pc.set_format(8, SerialBase::None, 1);
    
    ThisThread::sleep_for(1s); // 1 second for stable state
    debug("[io] Initializing Ethernet if net0\n");
    returnCode = eth.init(MAC_Addr,MyIP_Addr,IP_Subnet,IP_Gateway);
    net::interfaces::eth0 = &eth;
 
    if ( returnCode == 0 ){
        debug("[io]   Ethernet ready\n");
    } else {
        debug("[io]   Could not initialize Ethernet\n");
    }
    
    debug("[io] Ethernet connecting\n");
    returnCode = eth.connect();
    debug("[io]   connecting returned %d\n", returnCode);
    debug("[io]   IP Address is %s\n", eth.get_ip_address());
}


void IoMain(void){
    static int CycleCounter = 0;
    static int EncTimeout = EncMaxTimeout;
    int position;

    static char rxstrbuf[33];
    int elems = 0;

    int success;

    // Extract encoder response char-by-char from the ring buffer
    if (EncStringReceiveEndFlag) {
        EncStringReceiveEndFlag = false;

        elems = encoder_rx_buf.pop(rxstrbuf, 32);   // svuota il buffer appena finisci di ricevere una stringa
        rxstrbuf[elems] = '\0';

        success = sscanf(rxstrbuf, "%d", &position);

        if(success == 1) {  // Se ho scansionato esattamente un elemento correttamente

            debug("[io] decoded position from encoder %d\n", position);

            //siccome l'encoder gira a rovescio invertiamo il numero ottenuto
            position = 36000 - position;
            EncoderPosition = position ;
            position += DomeShiftZero;

            //riduci il valore di un giro fino alla frazione di giro
            while (position >= DomeOneRotPulses)
                position -= DomeOneRotPulses;
            position *= 360;
            position /= DomeOneRotPulses;

            //ora temp e' 0/359, usalo come posizione attuale cupola
            DomePosition = position;
            
            //ricezione corretta: mostra link ok
            GuiEncLinkShow(1);
            EncTimeout = 0;
        }

    } else {
        EncTimeout++;
    }
    
    if(EncTimeout == 500)
        GuiEncLinkShow(0);

    // Update time counter (every second)
    if (++CycleCounter == 50)
        encoder.write("R", 1); //invia una richiesta di posizione all'encoder

    if (CycleCounter == 100)
        CycleCounter = 0;

}
