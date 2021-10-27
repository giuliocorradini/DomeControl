#include "io.h"
#include <stdio.h>
#include <string>
#include "mbed.h"
#include "dome.h"
#include "gui.h"
#include "WIZnetInterface.h"
#include "config.h"

BufferedSerial Pc(USBTX,USBRX);

UnbufferedSerial encoder(PA_11,PA_12);

int EncRxEndFlag = 0;


char EncBuff[10];
char * EncRxPtr;
char * EncReadPtr;

//definizioni comunicazione con il server
uint8_t SrvRxBufHead = 0;
uint8_t SrvRxBufTail = 0;
#define SrvRxBufSize 20
char SrvRxBuffer[SrvRxBufSize];
//char SrvTxBuffer[20];
int HostLinkOk = 0;


const char * MyIP_Addr  = CONFIG_IP_ADDRESS;
const char * IP_Subnet  = CONFIG_NETMASK;
const char * IP_Gateway = CONFIG_GATEWAY_IP;
unsigned char MAC_Addr[6] = {0x00,0x08,0xDC,0x12,0x34,0x56};

//SPI spi(PB_5, PB_4, PB_3); // mosi, miso, sclk
WIZnetInterface eth(PB_5, PB_4, PB_3, PB_6, PC_4);    // reset pin is dummy, don't affect any pin of WIZ550io


//prototipi
void encrx(void);
int ParseNumber(char * ptrdato);
char SrvRxCharGet(void);
uint8_t SrvRxCharPending(void);
void srvrx(void);
int32_t SrvValGet(char * ptr);      //parsing stringa alla ricerca di un numero decimale 
void WebServerThread();


void IoInit(void){
    int returnCode = 0;

    encoder.baud(19200);
    encoder.format(8, SerialBase::None, 1);
    encoder.attach(&encrx, SerialBase::RxIrq);
    EncRxPtr = EncBuff;
    encoder.write("ND", 2);    //inizializza l'encoder a trasmettere in decimale e no debug mode

    /*server.baud(9600);
    server.format(8,server.None,1);
    server.attach(&srvrx,server.RxIrq);*/

    Pc.set_baud(115200);
    Pc.set_format(8, SerialBase::None, 1);
    
    //inizializza ethernet
    //spi.format(8,0); // 8bit, mode 0  handled by WIZnet library internally
    //spi.frequency(15000000); // 7MHz
    ThisThread::sleep_for(1s); // 1 second for stable state
    printf("\r\ninitializing Ethernet\r\n");
    returnCode = eth.init(MAC_Addr,MyIP_Addr,IP_Subnet,IP_Gateway);
    net::interfaces::eth0 = &eth;
 
    if ( returnCode == 0 ){
        printf(" - Ethernet ready\r\n");
    } else {
        printf(" - Could not initialize Ethernet - ending\r\n");
    }
    
    printf("Ethernet.connecting \r\n");
    returnCode = eth.connect();
    printf(" - connecting returned %d \r\n", returnCode);
    printf("IP Address is %s\r\n", eth.get_ip_address());
}

void IoMain(void){
    static int CycleCounter = 0;
    int temp;
    #define HostMaxTimeout 500
    static int HostTimeout = HostMaxTimeout;    //forza un aggiornmento al primo update
    #define EncMaxTimeout 500
    static int EncTimeout = EncMaxTimeout;
    char * SrvBufPtr;



    //lettura stringa ricevuta dall'encoder per decodificare la posizione
    if (EncRxEndFlag == 1) {
        EncReadPtr = EncBuff;
        temp = 0;
        while (1) {
            if (*EncReadPtr == ' ' || *EncReadPtr == '\r')
                break;
            if (*EncReadPtr < '0' || *EncReadPtr > '9')
                goto EncDecErr;
            temp *= 10;
            temp += *EncReadPtr++ -48;
        };
        //siccome l'encoder gira a rovescio invertiamo il numero ottenuto
        temp = 36000 - temp;
        EncoderPosition = temp ;
        temp += DomeShiftZero;
        //riduci il valore di un giro fino alla frazione di giro
        while (temp >= DomeOneRotPulses)
            temp -= DomeOneRotPulses;
        temp *= 360;
        temp /= DomeOneRotPulses;
        //ora temp e' 0/359, usalo come posizione attuale cupola
        DomePosition = temp;
        
        //ricezione corretta: mostra link ok
        if (EncTimeout == EncMaxTimeout){
            GuiEncLinkShow(1);
        };
        EncTimeout = 0;
        
        EncDecErr:
        EncRxEndFlag = 0;
    } else {
        if (EncTimeout == EncMaxTimeout -1)
            GuiEncLinkShow(0);
        if (EncTimeout++ == EncMaxTimeout)
            EncTimeout--;  //forzalo a restare 500
    };


    //ogni secondo...
    if (++CycleCounter == 50)
        //invia una richiesta di posizione all'encoder
        encoder.write("R", 1);
    
    //ogni secondo
    if (CycleCounter == 100){
        //trasmetti la quota attuale verso il broker
        SrvBufPtr = SrvRxBuffer;
        SrvBufPtr += sprintf(SrvBufPtr,"RP%d ",EncoderPosition);     //NB i due spazi dietro il %d verranno 'divorati' dal numero a 2 e tre cifre !
        *SrvBufPtr++ = ' ';
        SrvBufPtr += sprintf(SrvBufPtr,"DP%d\n",DomePosition);
        //TODO: send here to broker
        CycleCounter = 0;
    };

}

//routine in arrivo dall'interrupt seriale ricezione
//riceviamo una stringa fino al newline e solo se abbiamo finito di leggere
//la precedente, non implementiamo buffer circolare ect etc dato che dobbiamo leggere
//un solo tipo di dati !
void encrx(void){
    char tmp;
    static int EncRxcnt = 0;    
    
    encoder.read(&tmp,1);
    if (tmp == '\n') {
        if (EncRxcnt < 10)
            EncRxEndFlag = 1;
        EncRxPtr = EncBuff;
        EncRxcnt = 0;
    } else {
        if (EncRxEndFlag == 0 && ++EncRxcnt < 10)
            *EncRxPtr++ = tmp;
    };
}



int ParseNumber(char * ptrdato){
    int n,tmp;
    int result = 0;
        
    for (n=0;n<10;n++) {
        tmp = *ptrdato++;
        if (tmp >= '0' && tmp <='9') {
            result *= 10;
            result += tmp - 48;
        } else 
            break;
    };
    return result;
}

//riceve un dato decimale fino al newline o CR o separatore ';'
//spazi e altri caratteri non ammessi sono ignorati
//se riceve -1 prosegue dal puntatore precedente
int32_t SrvValGet(char * ptr) {
    int32_t temp=0;
    int32_t minusflag = 1;
    char dato;
    static char * LastPtr = 0;
    
    //se riceviamo -1 continuiamo con il puntatore precedente
    if ((int)ptr == -1) ptr = LastPtr;
    
    while (1) {
        dato = *ptr++;
        if (dato == '\n' || dato == '\r' || dato == ';'){
            LastPtr = ptr;  //nota che all'assegnazione di dato ptr e' stato incrementato
            return temp * minusflag;
        };
        if (dato == '-') minusflag = -1;
        if (dato >= '0' && dato <= '9') {
            temp *= 10;
            temp += dato - '0';
        };
    };
}

