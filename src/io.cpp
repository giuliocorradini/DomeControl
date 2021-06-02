#include "io.h"
#include <stdio.h>
#include <string>
#include "mbed.h"
#include "dome.h"
#include "gui.h"
#include "WIZnetInterface.h"

BufferedSerial Pc(USBTX,USBRX);
//RawSerial server(PA_9,PA_10);
UnbufferedSerial encoder(PA_11,PA_12);
FILE *encoder_file = fdopen(&encoder, "w+");

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


const char * MyIP_Addr    = "192.168.0.22";
const char * IP_Subnet  = "255.255.255.0";
const char * IP_Gateway = "192.168.0.1";
unsigned char MAC_Addr[6] = {0x00,0x08,0xDC,0x12,0x34,0x56};

//SPI spi(PB_5, PB_4, PB_3); // mosi, miso, sclk
WIZnetInterface eth(PB_5, PB_4, PB_3, PB_6, PC_4);    // reset pin is dummy, don't affect any pin of WIZ550io

//UDPSocket is imported with mbed.h
UDPSocket RxUdp;
SocketAddress RxEndpoint;
UDPSocket TxUdp;
SocketAddress TxEndpoint;
//per porta web
TCPSocket WebSrv;
TCPSocket *WebClient;
bool serverIsListened = false;
bool clientIsConnected = false;

//prototipi
void encrx(void);
int ParseNumber(char * ptrdato);
char SrvRxCharGet(void);
uint8_t SrvRxCharPending(void);
void srvrx(void);
int32_t SrvValGet(char * ptr);      //parsing stringa alla ricerca di un numero decimale 



void IoInit(void){
    int returnCode = 0;

    encoder.baud(19200);
    encoder.format(8, SerialBase::None, 1);
    encoder.attach(&encrx, SerialBase::RxIrq);
    EncRxPtr = EncBuff;
    fputs("ND", encoder_file);    //inizializza l'encoder a trasmettere in decimale e no debug mode

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

    /* Configurazione endpoint UDP */
    RxUdp.open(&eth);   //Apro socket UDP su interfaccia Ethernet
    //if (RxUdp.init() == 0)
    //    printf("init socket udp in ricezione OK\r\n");
    if (RxUdp.bind(1032) == NSAPI_ERROR_OK)
        printf("bind RX socket 1032 udp OK\r\n");
    else
        exit(-1);
    RxEndpoint.set_ip_address(MyIP_Addr);
    RxEndpoint.set_port(1032);
    RxUdp.set_blocking(false);

    //if (TxUdp.init() == 0)
    //    printf("init socket udp in trasmissione OK\r\n");
    TxUdp.open(&eth);
    if (TxUdp.bind(1031) == 0)
        printf("bind TX socket udp 1031 OK\r\n");
    //TxEndpoint.set_address("192.168.0.20", 1031);
    TxEndpoint.set_ip_address("192.168.0.102");
    TxEndpoint.set_port(1050);
    TxUdp.set_blocking(false);

    //configura ed apre la porta web
    WebSrv.set_blocking(false);

    //setup tcp socket
    if(WebSrv.bind(80) < 0) {
        printf("web 80 port tcp server bind failed.\n\r");
        return;
    } else {
        printf("tcp server port 80 bind successed.\n\r");
        serverIsListened = true;
    }
 
    if(WebSrv.listen(1) < 0) {
        printf("web server listen failed.\n\r");
        return;
    } else {
        printf("web server is listening...\n\r");
    }       
}

void IoMain(void){
    static int CycleCounter = 0;
    int temp;
    #define HostMaxTimeout 500
    static int HostTimeout = HostMaxTimeout;    //forza un aggiornmento al primo update
    #define EncMaxTimeout 500
    static int EncTimeout = EncMaxTimeout;
    char * SrvBufPtr;
    char WebBuffer[1024] = {};
    
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
        fputs("R", encoder_file);
    
    //ogni secondo
    if (CycleCounter == 100){
        //trasmetti la quota attuale verso il server
        SrvBufPtr = SrvRxBuffer;
        SrvBufPtr += sprintf(SrvBufPtr,"RP%d ",EncoderPosition);     //NB i due spazi dietro il %d verranno 'divorati' dal numero a 2 e tre cifre !
        *SrvBufPtr++ = ' ';
        SrvBufPtr += sprintf(SrvBufPtr,"DP%d\n",DomePosition);
        TxUdp.sendto(TxEndpoint, SrvRxBuffer, strlen(SrvRxBuffer));
        CycleCounter = 0;
    };
    
    //legge dati dal socket udp di ricezione
    temp = RxUdp.recvfrom(&RxEndpoint, SrvRxBuffer, SrvRxBufSize);
    if (temp>0){
        SrvRxBuffer[temp] = '\n';
        SrvRxBuffer[temp+1] = 0;
        //printf("ricevuto %s\n",SrvRxBuffer);
        SrvBufPtr = SrvRxBuffer;
        switch (*SrvBufPtr++) {
            case 'D':    // D : Display posizione del telescopio in gradi da mostrare a schermo
                //prima arriva l'AZ
                temp = SrvValGet(SrvBufPtr);
                if (temp >= 0 || temp <360){
                    TelescopePosition = temp;
                    TelescopeDrawUpdate(temp);
                    //considera attivo il link con il telescopio solo se ne arriva ciclicamente la quota attuale
                    if (HostTimeout == HostMaxTimeout){
                        GuiHostLinkShow(1);
                    };
                    HostTimeout = 0;
                    HostLinkOk = 1;
                };
                //poi arriva l'Alt
                temp = SrvValGet((char *) -1);   //prosegue dalla posizione precedente, il cast serve per evitare errori
                if (temp >= 0 || temp <91)
                    TelescopeAlt = temp;
                break;
            case 'T':   //Tracking , muove il telescopio in modo incrementale, usata per il tracciamento
                temp = SrvValGet(SrvBufPtr);
                if (temp >= 0 || temp <360){
                    //avvia il posizionamento
                    DomeMoveStart(temp,Tracking);
                };
                break;
            case 'C':   //Centra la cupola sul telescopio
                DomeMoveStart(TelescopePosition,Rollover);
                break;
                
        };
    } else {    //nessun carattere arrivato
        if (HostTimeout == HostMaxTimeout -1){
            HostLinkOk = 0;
            GuiHostLinkShow(0);
        }
        if (HostTimeout++ == HostMaxTimeout)
            HostTimeout--;  //forzalo a restare 500
        
    };
    
    //Pagine WEB
    if (serverIsListened) {
        //NON blocking mode
        nsapi_size_or_error_t WebSrvError;
        WebClient = WebSrv.accept(&WebSrvError);
        if(WebSrvError < 0) {
            //printf("failed to accept connection.\n\r");
        } else {
            //printf("connection success!\n\rIP: %s\n\r",WebClient.get_address());
            clientIsConnected = true;
            WebClient->set_blocking(true);  //importante ! questa causa il nonblocking
            WebClient->set_timeout(50);
 
            while(clientIsConnected) {
                switch(WebClient->recv(WebBuffer, 1023)) {
                    case 0:
                        //printf("recieved buffer is empty.\n\r");
                        clientIsConnected = false;
                        break;
                    case -1:
                        //printf("failed to read data from client.\n\r");
                        clientIsConnected = false;
                        break;
                    default:
                        //printf("Recieved Data: %d\n\r\n\r%.*s\n\r",strlen(WebBuffer),strlen(WebBuffer),WebBuffer);
                        if(WebBuffer[0] == 'G' && WebBuffer[1] == 'E' && WebBuffer[2] == 'T' ) {
                            //spezza la stringa nei 3 token
                            //una stringa GET somiglierà a "GET /path HTTP1.1"
                            // il secondo pezzo è quello che serve a noi
                            char* pString = WebBuffer;
                            char* pField;
                            char* pFields[2];
    
                            for(int i=0; i<2; i++){
                                pField = strtok(pString, " ");
 
                                if(pField != NULL){
                                    pFields[i] = pField;
                                } else {
                                    pFields[i] = "";
                                }
 
                                pString = NULL; //to make strtok continue parsing the next field rather than start again on the original string (see strtok documentation for more details)
                            }

                            char echoHeader[256] = {};
                            //sprintf(echoHeader,"HTTP/1.1 200 OK\n\rContent-Length: %d\n\rContent-Type: text\n\rConnection: Close\n\r\n\r",strlen(WebBuffer));
                            sprintf(echoHeader,"HTTP/1.1 200 OK\r\nContent-Type: text; charset=UTF-8\r\nHost: 192.168.0.22:80\r\nConnection: Close\r\n\r\n");
                            WebClient->send(echoHeader,strlen(echoHeader));

                            if(strcmp(pFields[1],"/") == 0 ) {
                                //printf("GET pagina Home\r\n");
                                //setup http response header & data
                                sprintf(WebBuffer,"<head><title>NATPC Cupola</title></head>");
                                WebClient->send(WebBuffer,strlen(WebBuffer));
                                sprintf(WebBuffer,"<body bgcolor=\"#0099ff\"><center><h1>Automazione Cupola NATPC\r\n<br></h1>Posizione cupola: %d\n\r<br>Posizione Encoder: %d\r\n",DomePosition,EncoderPosition);
                                WebClient->send(WebBuffer,strlen(WebBuffer));
                                sprintf(WebBuffer,"<br>Azimut Telescopio: %d\r\n</center></body>",TelescopePosition);
                                WebClient->send(WebBuffer,strlen(WebBuffer));
                                sprintf(WebBuffer,"<br>Altezza Telescopio: %d\r\n</center></body>",TelescopeAlt);
                                WebClient->send(WebBuffer,strlen(WebBuffer));
                            }
                            //dati per app tablet
                            if (strcmp(pFields[1],"/GetData.php") == 0 ){
                                int TpAz;
                                int TpAlt;
                                if (HostLinkOk == 1){
                                    TpAz = TelescopePosition;
                                    TpAlt = TelescopeAlt;
                                } else {
                                    TpAz = 999;
                                    TpAlt = 0;
                                };
                                sprintf(WebBuffer,"{ \"AzTelescopio\":%d , \"AltTelescopio\":%d , \"PosCupola\":%d , \"Oltrecorsa\":%d , \"Movimento\":%d}\n",TpAz,TpAlt,DomePosition,EncoderPosition/360,\
                                    DomeMotion == 1 || DomeManMotion == 1 ? 1 : 0);
                                WebClient->send(WebBuffer,strlen(WebBuffer));
                            }
                            //centra la cupola sul telescopio
                            //ma solo se il link con il tlescopio è ok
                            if (strcmp(pFields[1],"/center") == 0 && HostLinkOk == 1){
                                DomeMoveStart(TelescopePosition,Rollover);
                            }
                            //Start Movimento manuale orario
                            if (strcmp(pFields[1],"/cwon") == 0 ){
                                DomeManStart(Cw);
                            }
                            if (strcmp(pFields[1],"/ccwon") == 0 ){
                                DomeManStart(Ccw);
                            }
                            if (strcmp(pFields[1],"/moveoff") == 0 ){
                                DomeMoveStop(); //DomeManStop();
                            }
                            
                            clientIsConnected = false;
                            //printf("echo back done.\n\r");
                        }
                        break;
                }
            }
            //printf("chiusura connessione\n\r");
            WebClient->close();
        }
    }
        
}

//riceviamo una stringa fino al newline e solo se abbiamo finito di leggere
//la precedente, non implementiamo buffer circolare ect etc dato che dobbiamo leggere
//un solo tipo di dati !
void encrx(void){
    char tmp;
    static int EncRxcnt = 0;    
    
    tmp = fgetc(encoder_file);
    //Pc.putc(tmp);
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

/*NON USATO

ricezione dal server di automazione in buffer circolare
void srvrx(void){
    uint8_t temp;
    temp = SrvRxBufHead;
    SrvRxBuffer[SrvRxBufHead] = server.getc();
    SrvRxBufHead++;
    if (SrvRxBufHead == SrvRxBufSize)           //buffer circolare
        SrvRxBufHead = 0;
    if (SrvRxBufHead == SrvRxBufTail)         //overrun !
        SrvRxBufHead = temp;
    
}

//ritorna un carattere dal buffer di ricezione seriale server
char SrvRxCharGet(void) {
    char temp;
    uint32_t timeout = 0;
    while (SrvRxBufHead == SrvRxBufTail){
        if (timeout++ == 200000)
            return '\n';
    };     //attende che nel buffer ci sia almeno un carattere
    temp = SrvRxBuffer[SrvRxBufTail];
    SrvRxBufTail++;
    if (SrvRxBufTail == SrvRxBufSize)
        SrvRxBufTail = 0;
    return temp;
}

//restituisce 1 se c'è almeno un carattere in attesa di essere letto
//dal buffer seriale dal server
uint8_t SrvRxCharPending(void) {

    if (SrvRxBufHead == SrvRxBufTail)
        return 0;
    else
        return 1;

}*/


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

