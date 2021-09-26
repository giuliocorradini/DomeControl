/* mbed Newhaven LCD Library
 * Copywrite (c) 2011, Paul Evans

 * mods 2015-05-03: implementiamo il colore grigio nello schermo 2 della ram lcd
   per evitare conflitti con lo screenbuffer, e vista la limitatezza dell'uso del grigio
   usiamo solo per il grigio la vram stessa rileggendola invece che usare il framebuffer

 */

#include "mbed.h"
#include "lcd.h"

NHLCD::NHLCD(PinName PIN_RD,PinName PIN_WR,PinName PIN_A0,PinName PIN_CS,PinName PIN_Busy)
    : RD(PIN_RD),WR(PIN_WR),A0(PIN_A0),CS(PIN_CS),BUSY(PIN_Busy){}


#define LCD_XDOTS 320
#define LCD_YDOTS 240
#define GRAPHIC_AREA 40     //bytes in una riga

const char TEXT [96][5] ={ {0x00, 0x00, 0x00, 0x00, 0x00 }, // SPACE
                         {0x00, 0x00, 0x5F, 0x00, 0x00}, // !
                         {0x00, 0x03, 0x00, 0x03, 0x00}, // "
                         {0x14, 0x3E, 0x14, 0x3E, 0x14}, // #
                         {0x24, 0x2A, 0x7F, 0x2A, 0x12}, // $
                         {0x43, 0x33, 0x08, 0x66, 0x61}, // %
                         {0x36, 0x49, 0x55, 0x22, 0x50}, // &
                         {0x00, 0x05, 0x03, 0x00, 0x00}, // '
                         {0x00, 0x1C, 0x22, 0x41, 0x00}, // (
                         {0x00, 0x41, 0x22, 0x1C, 0x00}, // )
                         {0x14, 0x08, 0x3E, 0x08, 0x14}, // *
                         {0x08, 0x08, 0x3E, 0x08, 0x08}, // +
                         {0x00, 0x50, 0x30, 0x00, 0x00}, // ,
                         {0x08, 0x08, 0x08, 0x08, 0x08}, // -
                         {0x00, 0x60, 0x60, 0x00, 0x00}, // .
                         {0x20, 0x10, 0x08, 0x04, 0x02}, // /
                         {0x3E, 0x51, 0x49, 0x45, 0x3E}, // 0
                         {0x04, 0x02, 0x7F, 0x00, 0x00}, // 1
                         {0x42, 0x61, 0x51, 0x49, 0x46}, // 2
                         {0x22, 0x41, 0x49, 0x49, 0x36}, // 3
                         {0x18, 0x14, 0x12, 0x7F, 0x10}, // 4
                         {0x27, 0x45, 0x45, 0x45, 0x39}, // 5
                         {0x3E, 0x49, 0x49, 0x49, 0x32}, // 6
                         {0x01, 0x01, 0x71, 0x09, 0x07}, // 7
                         {0x36, 0x49, 0x49, 0x49, 0x36}, // 8
                         {0x26, 0x49, 0x49, 0x49, 0x3E}, // 9
                         {0x00, 0x36, 0x36, 0x00, 0x00}, // :
                         {0x00, 0x56, 0x36, 0x00, 0x00}, // ;
                         {0x08, 0x14, 0x22, 0x41, 0x00}, // <
                         {0x14, 0x14, 0x14, 0x14, 0x14}, // =
                         {0x00, 0x41, 0x22, 0x14, 0x08}, // >
                         {0x02, 0x01, 0x51, 0x09, 0x06}, // ?
                         {0x3E, 0x41, 0x59, 0x55, 0x5E}, // @
                         {0x7E, 0x09, 0x09, 0x09, 0x7E}, // A
                         {0x7F, 0x49, 0x49, 0x49, 0x36}, // B
                         {0x3E, 0x41, 0x41, 0x41, 0x22}, // C
                         {0x7F, 0x41, 0x41, 0x41, 0x3E}, // D
                         {0x7F, 0x49, 0x49, 0x49, 0x41}, // E
                         {0x7F, 0x09, 0x09, 0x09, 0x01}, // F
                         {0x3E, 0x41, 0x41, 0x49, 0x3A}, // G
                         {0x7F, 0x08, 0x08, 0x08, 0x7F}, // H
                         {0x00, 0x41, 0x7F, 0x41, 0x00}, // I
                         {0x30, 0x40, 0x40, 0x40, 0x3F}, // J
                         {0x7F, 0x08, 0x14, 0x22, 0x41}, // K
                         {0x7F, 0x40, 0x40, 0x40, 0x40}, // L
                         {0x7F, 0x02, 0x0C, 0x02, 0x7F}, // M
                         {0x7F, 0x02, 0x04, 0x08, 0x7F}, // N
                         {0x3E, 0x41, 0x41, 0x41, 0x3E}, // O
                         {0x7F, 0x09, 0x09, 0x09, 0x06}, // P
                         {0x1E, 0x21, 0x21, 0x21, 0x5E}, // Q
                         {0x7F, 0x09, 0x09, 0x09, 0x76}, // R
                         {0x26, 0x49, 0x49, 0x49, 0x32}, // S
                         {0x01, 0x01, 0x7F, 0x01, 0x01}, // T
                         {0x3F, 0x40, 0x40, 0x40, 0x3F}, // U
                         {0x1F, 0x20, 0x40, 0x20, 0x1F}, // V
                         {0x7F, 0x20, 0x10, 0x20, 0x7F}, // W
                         {0x41, 0x22, 0x1C, 0x22, 0x41}, // X
                         {0x07, 0x08, 0x70, 0x08, 0x07}, // Y
                         {0x61, 0x51, 0x49, 0x45, 0x43}, // Z
                         {0x00, 0x7F, 0x41, 0x00, 0x00}, // [
                         {0x02, 0x04, 0x08, 0x10, 0x20}, // backslash
                         {0x00, 0x00, 0x41, 0x7F, 0x00}, // ]
                         {0x04, 0x02, 0x01, 0x02, 0x04}, // ^
                         {0x40, 0x40, 0x40, 0x40, 0x40}, // _
                         {0x00, 0x01, 0x02, 0x04, 0x00}, // `
                         {0x20, 0x54, 0x54, 0x54, 0x78}, // a
                         {0x7F, 0x44, 0x44, 0x44, 0x38}, // b
                         {0x38, 0x44, 0x44, 0x44, 0x44}, // c
                         {0x38, 0x44, 0x44, 0x44, 0x7F}, // d
                         {0x38, 0x54, 0x54, 0x54, 0x18}, // e
                         {0x04, 0x04, 0x7E, 0x05, 0x05}, // f
                         {0x08, 0x54, 0x54, 0x54, 0x3C}, // g
                         {0x7F, 0x08, 0x04, 0x04, 0x78}, // h
                         {0x00, 0x44, 0x7D, 0x40, 0x00}, // i
                         {0x20, 0x40, 0x44, 0x3D, 0x00}, // j
                         {0x7F, 0x10, 0x28, 0x44, 0x00}, // k
                         {0x00, 0x41, 0x7F, 0x40, 0x00}, // l
                         {0x7C, 0x04, 0x78, 0x04, 0x78}, // m
                         {0x7C, 0x08, 0x04, 0x04, 0x78}, // n
                         {0x38, 0x44, 0x44, 0x44, 0x38}, // o
                         {0x7C, 0x14, 0x14, 0x14, 0x08}, // p
                         {0x08, 0x14, 0x14, 0x14, 0x7C}, // q
                         {0x00, 0x7C, 0x08, 0x04, 0x04}, // r
                         {0x48, 0x54, 0x54, 0x54, 0x20}, // s
                         {0x04, 0x04, 0x3F, 0x44, 0x44}, // t
                         {0x3C, 0x40, 0x40, 0x20, 0x7C}, // u
                         {0x1C, 0x20, 0x40, 0x20, 0x1C}, // v
                         {0x3C, 0x40, 0x30, 0x40, 0x3C}, // w
                         {0x44, 0x28, 0x10, 0x28, 0x44}, // x
                         {0x0C, 0x50, 0x50, 0x50, 0x3C}, // y
                         {0x44, 0x64, 0x54, 0x4C, 0x44}, // z
                         {0x00, 0x08, 0x36, 0x41, 0x41}, // {
                         {0x00, 0x00, 0x7F, 0x00, 0x00}, // |
                         {0x41, 0x41, 0x36, 0x08, 0x00}, // }
                         {0x02, 0x01, 0x02, 0x04, 0x02}, // ~
                         {0x00, 0x02, 0x05, 0x02, 0x00} };// °

#ifdef Schermo180
const char maskbit[8] = {0x0,0x80,0xC0,0xE0,0xF0,0xF8,0xFC,0xFE};
#else
const char maskbit[9] = {0x0,0x1,0x3,0x7,0xF,0x1F,0x3F,0x7F,0x7F};
#endif

void delay(volatile unsigned int n){
    volatile unsigned int i,j;
    for (i=0;i<n;i++)
          for (j=0;j<350;j++)
              {;}
}

//26-09-2021 con il nuovo compilaote il volatile è molto importante
//altrimenti questa delay NON funziona correttamente
void delay1(volatile unsigned int i){
    while(i--);
}

void swap(int* a, int* b){
    int temp = *a;
    *a = *b;
    *b = temp;
}


//****************************************************************
//attende il display libero controllandone il busy bit
void NHLCD::Busy(void) {
    //char data = 64;
    //A0 deve essere basso per la lettura del busy
    A0 = 0;
    GPIOC->MODER &= 0xFFFF0000; //porta display come ingressi
    //attiva CS abbassandolo

    while (1) {
        //lettura dal chip RA8835
        CS = 0 ;
        RD = 0;
        delay1(3);
        //legge dato dal bus
        if ((GPIOC->IDR & 64) == 64)break;
        //disattiva Rd rialzandolo
        RD = 1;
        CS = 1;
        delay1(10);
    };
    //disattiva CS rialzandolo
    RD = 1;
    CS = 1;
    A0 = 1;
    delay1(3);

}


// send commands to the LCD
void NHLCD::comm_out(unsigned char j){
    //Busy();
    GPIOC->BSRR = 0x00FF0000;     //bus dati prima a zero ...
    GPIOC->BSRR = j;       //e poi con il dato
    GPIOC->MODER |= 0x5555; //porta display come uscita
    A0 = 1;                 // set for a command output
    delay1(1);              // stabilizza bus
    CS = 0;                 // active LOW chip select
    WR = 0;                 // set to 0 for write operation
    delay1(2);              // wait for operation to complete
    WR = 1;
//    CS = 1;   //dopo un comando c'e' SEMPRE un'altro dato quindi NON rialziamo il CS
}

// send data to the LCD
void NHLCD::data_out(unsigned char j){
    GPIOC->BSRR = 0x00FF0000;     //bus dati prima a zero ...
    GPIOC->BSRR = j;       //e poi con il dato
    GPIOC->MODER |= 0x5555; //porta display come uscita
    delay1(1);              // stabilizza bus
    A0 = 0;                 // set for a data output
    CS = 0;                 // active LOW chip select
    WR = 0;                 // set to 0 for write operation
    delay1(2);               // wait for operation to complete
    WR = 1;
    CS = 1;
    delay1(1);               // sicurezza
    GPIOC->BSRR = 0x00FF0000;       //rimetti il bus dati a zero per vecchi problemi di errore al RA8835
}


//****************************************************************
//legge da una locazione della videoRAM
char NHLCD::VramRd(unsigned int Loc) {
    char data;
  
    RamSelect(Loc);     //punta alla locazione VRAM su cui lavorare

    Busy();          //attende display libero

    comm_out(0x43);    //comando lettura ram video
    
    //A0 deve essere alto per la lettura dati !!! lcdcmdwr lo abbassava
    A0 = 1;

    GPIOC->MODER &= 0xFFFF0000; //porta display come ingressi
    delay1(2);
    //attiva CS abbassandolo
    CS = 0 ;
    //lettura dal chip RA8835
    RD = 0;
    delay1(2);
    //legge dato dal bus
    data = GPIOC->IDR;
    //disattiva Rd rialzandolo
    RD = 1;
    //disattiva CS rialzandolo
    CS = 1;

    return data;
}

//****************************************************************
//seleziona una locazione di memoria in videoRAM
void NHLCD::RamSelect(unsigned int Loc) {

    //comando scrittura posizione cursore
    comm_out(0x46);
    //ora dobbiamo solo inviare i due byte di posizione cursore
    data_out(Loc & 0xff) ;
    //mette sul bus la locazione H
    data_out(Loc >> 8);

}

// clears the entire screen
void NHLCD::clearScreen(){
    int n;
    comm_out(0x46);         // command to set cursor location
    data_out(0); //xB0);         // 0x4B0 is the start of drawing screen
    data_out(0); //x04);
    comm_out(0x42);         // command to write data
    for(n=0;n<19200;n++){    // 9600 total byte locations, x2 dato che abbiamo contigua l'area del grigio
        data_out(0x00);     // set each to 0
    }
    for(n = 0; n < 40*240; n++)
    screenBuffer[n] = 0;

}

// write text on the screen
//da usare SOLO con display a carattere !!!, quindi NON in questo progetto
void NHLCD::text(char *text, char row, char col){
    
    
    int c = row*40+col;                         // gets the correct address for the cursor
    comm_out(0x46);                             // command to set cursor location
    data_out((unsigned char)(c&0xFF));          // lower 8 bits of address
    data_out((unsigned char)((c&0xFF00)>>8));   // upper 8 bits of address
    comm_out(0x42);                             // command to write data to screen
    while(*text != 0) {                         // write until you hit a null terminator
        data_out(*text);                        // write the current character to the screen
        text++;                                 // move to the next character
    }
}

//****************************************************************
//scrive testo con font 7x5 alla loc spec. in pixel
void NHLCD::graph_text(char * textptr, unsigned int row, unsigned int col, char color)
{
   unsigned int i, j, k;                     // Loop counters
   char pixelData[5];                     // Stores character data
   char * PixeldataPtr;
   char * SrcdataPtr;
   
   for(i=0; textptr[i] != '\0'; ++i, ++col) // Loop through the passed string
   {
       /* in 320 pixel NON ci possono stare + di 53 caratteri
          se la stringa è errata e manca il \0 alla fine in questo modo
          ci cauteliano trancando la visualizz. al 53° carattere
          qst puo accadere se qualche puntatore a stringa stà 'arando'*/
       if (i > 53) break;
       PixeldataPtr = pixelData;
       SrcdataPtr = (char * )TEXT[textptr[i]-' '];
       for (j=0; j<5 ; j++)
           *PixeldataPtr++ = *SrcdataPtr++ ;
           
      if(col+5 >= LCD_XDOTS)          // Performs character wrapping
      {
         col = 0;                           // Set x at far left position
         row += 7 + 1;                 // Set y at next position down
      }
      for(j=0; j<5; ++j, col+=1)         // Loop through character byte data
      {
         for(k=0; k<7; ++k)          // Loop through the vertical pixels
         {
            if(pixelData[j] & 1<<k) // Check if the pixel should be set
            {
               setPixel(row+k ,col ,color); // Draws the pixel
            }
         }
      }
   }


}
    

/* set an individual pixel on the screen.
 * pixels are grouped in bytes, so you must isolate a particular pixel.
 */
void NHLCD::setPixel(int row, int col, int color){
    int loc = 0;
    char dato = 0;

    //questa per lo schermo girato di 180°
#ifdef Schermo180
    int c = (GRAPHIC_AREA * LCD_YDOTS) - 1  - (row*40+(col/8));                         // get location in buffer
    char x = 1<<(col%8);                        // sets the bit to alter
#else
    //questa per lo schermo normale
    int c = row*40+(col/8);                         // get location in buffer
    char x = 1<<(7-(col%8));                        // sets the bit to alter
#endif
    switch (color ) {                               
        case nero:                                     // to mark the bit
            screenBuffer[c] |= x;                   // marks the bit
            loc = c; //+0x4B0;                      // get location on drawing screen
            dato = screenBuffer[c];
            break;
        case bianco:                                     // to clear the bit
            screenBuffer[c] &= ~x;                  // clears the bit
            loc = c; //+0x4B0;                      // get location on drawing screen
            dato = screenBuffer[c];
            break;
        case grigio:
            loc = c + 9600;     //se colore grigio, scrivi nella ceconda area grafica che mostra il grigio
            dato =  VramRd(loc);
            RamSelect(loc);
            dato |= x;                               //SENZA screenbuffer
            break;
        case nogrigio:
            loc = c + 9600;     //se colore grigio, scrivi nella ceconda area grafica che mostra il grigio
            dato =  VramRd(loc);
            RamSelect(loc);
            dato &= ~x;                               //SENZA screenbuffer
            break;
    };
    comm_out(0x46);                                 // command to set cursor location
    data_out((unsigned char)(loc&0xFF));            // lower 8 bits of address
    data_out((unsigned char)((loc&0xFF00)>>8));     // upper 8 bits of address
    Busy();
    comm_out(0x42);                                 // command to write data
    data_out(dato);                                 // write byte to the screen
}

// draw a line -- Bresenham's line algorithm
void NHLCD::drawLine(int r1, int c1, int r2, int c2, int color){
    int w = r2 - r1;                            // calculate width of line
    int h = c2 - c1;                            // calculate height of line
    double ratio;
    int x, y;
    int upright = 0;                            // assumes the line is "wide"
    if (abs(h) > abs(w)) {                      // checks if height is greater than width
        upright = 1;                            // if it is, the line is "tall"            
        swap(&c1, &r1);                         // mirrors coordinates over the line y=x to make it "wide"            
        swap(&c2, &r2);
        swap(&w, &h);
    }
    if (r1 > r2) {                              // if the first row is on the right, swaps start and end coordinates
        swap(&c1, &c2);                         // now the line is always drawn from "bottom-left" to "top-right"
        swap(&r1, &r2);
    }
    ratio = ((double) h) / w;                   // calculates ratio of height to width
    for (x = r1; x < r2; x++) {                 // iterates through the rows
        y = c1 + (x - r1) * ratio;              // calculates the height at this particular row
        if (upright) setPixel(y, x, color);     // depending on if the line is "wide" or "tall", you have to 
        else setPixel(x, y, color);             //      call setPixel differently
    }
}

//****************************************************************
//disegna una linea verticale
void NHLCD::Vline (unsigned int row, unsigned int col, char h, char color) {
 
    while ( h-- ) {
        setPixel(row ,col ,color);
        row++;    
    };   
}


//****************************************************************
//disegna una linea orizzontale
void NHLCD::Hline (unsigned int row, unsigned int col, unsigned int w, char color) {

  unsigned int addr;//,addr2;
  char mask, mask2,tmp;
  unsigned int b;
    //questa per lo schermo girato di 180°
#ifdef Schermo180
  addr = (GRAPHIC_AREA * LCD_YDOTS) - 1 -((GRAPHIC_AREA * row) + (col >> 3));
#else
  addr = (GRAPHIC_AREA * row) + (col >> 3);
#endif
  mask = col & 0x07;
  if (color == grigio || color == nogrigio) addr += 9600;     //se colore grigio, scrivi nella ceconda area grafica
  if (mask) {
    //il primo byte deve essere parzialmente scritto
    b = 8 - mask;    //numero di pixel da riempire dal MSB
    //calcola la maschera per modificare i bit meno significativi
    mask = maskbit[b];
    if ( w < b) {
      //linea cortissima non tutti i bit a dx sono da riempire
      b -= w;
      mask2 = maskbit[b];
      mask &= ~mask2;
      w = 0;
    } else 
      w -= b;
    
    //modifica il byte nello screenbuffer e/o schermo
    switch (color) {                                      // to mark the bit
        case nero:
            screenBuffer[addr] |= mask;                       // marks the bit
            RamSelect(addr);
            Busy();
            comm_out(0x42);                                 // command to write data
#ifdef Schermo180
            data_out(screenBuffer[addr--]);                      // write byte to the screen  
#else
            data_out(screenBuffer[addr++]);                      // write byte to the screen  
#endif
            break;
        case bianco:                                        // to clear the bit
            screenBuffer[addr] &= ~mask;                      // clears the bit
            RamSelect(addr);
            Busy();
            comm_out(0x42);                                 // command to write data
#ifdef Schermo180
            data_out(screenBuffer[addr--]);                      // write byte to the screen  
#else
            data_out(screenBuffer[addr++]);                      // write byte to the screen  
#endif
            break;
        case grigio:
            tmp =  VramRd(addr);
            RamSelect(addr);
            Busy();
            comm_out(0x42);                                 // command to write data
            data_out(tmp|mask);                 //per il grigio NON usiamo lo screenbuffer
#ifdef Schermo180
            addr--;
#else
            addr++;
#endif
            break;
        case nogrigio:
            tmp =  VramRd(addr);
            RamSelect(addr);
            Busy();
            comm_out(0x42);                                 // command to write data
            data_out(tmp & ~mask);                 //per il grigio NON usiamo lo screenbuffer
#ifdef Schermo180
            addr--;
#else
            addr++;
#endif
            break;
    };
  };
  
  //scrive w>>3 byte consecutivi in vram
  for (b=0;b!=(w>>3);b++) {
    RamSelect(addr);    //in teoria questo NON serve, in pratica non disegniamo correttamente se lo omettiamo...
    Busy();
    comm_out(0x42);                                 // command to write data
    switch ( color ){
        case nero:
            data_out(255);                                // write byte to the screen  
#ifdef Schermo180
            screenBuffer[addr--] = 255;                   //e tieni aggiornato anche il buffer
#else
            screenBuffer[addr++] = 255;                   //e tieni aggiornato anche il buffer
#endif
            break;
        case bianco:
            data_out(0);
#ifdef Schermo180
            screenBuffer[addr--] = 0;
#else
            screenBuffer[addr++] = 0;
#endif
            break;
        case grigio:
            data_out(255);
#ifdef Schermo180
            addr--;
#else
            addr++;
#endif
            break;
        case nogrigio:
            data_out(0);
#ifdef Schermo180
            addr--;
#else
            addr++;
#endif
            break;
    };    
  };
  
  w &= 0x07;
  
  //l'ultimo byte deve essere parzialmente scritto
  if (w) {
    //legge il byte da modificare dalla vram
    mask = maskbit[8-w];        //maschera di bit da modificare
    //aggiorna il byte in ram
    switch (color) {
        case bianco:
            screenBuffer[addr] &= mask;
            b = screenBuffer[addr];
            break;
        case nero:
            screenBuffer[addr] |= ~mask;
            b = screenBuffer[addr];
            break;
        case grigio:
            tmp = VramRd(addr);                  //con il grigio NON modifichiamo il framebuffer
            b = tmp | ~mask;
            RamSelect(addr);
           break;
        case nogrigio:                //con il grigio NON modifichiamo il framebuffer
            b = VramRd(addr) & mask;
            RamSelect(addr);
            break;
    };            
    RamSelect(addr);    //in teoria questo NON serve, in pratica non disegniamo correttamente se lo omettiamo...
    Busy();
    comm_out(0x42);                   // command to write data
    data_out(b);                      // write byte to the screen  


  };    

}

// draw and fill a rectangle
void NHLCD::fillRect(int row, int col, int height, int width, int color){
    for (int r = 0; r < height; r++)        // iterate through each row
        Hline(row+r, col, width, color);
}

//disegna un rettangolo con il punto in alto a sinistra row-col largo w alto h
void NHLCD::Rect ( unsigned int row, unsigned int col, unsigned int w, unsigned int h, unsigned short color) {

    Hline(row, col, w, color);
    Hline(row+h, col, w+1, color);
    Vline(row, col, h, color);
    Vline(row, col+w, h, color);

}

//disegna un pulsante
void NHLCD::BtnDraw(int row1, int col1, int row2, int col2, char * text) {
    int colmid,rowmid;
    
    Rect(row1,col1,col2-col1,row2-row1,1);
    Rect(row1-3,col1-3,col2+3-(col1-3),row2+3-(row1-3),1);
    Hline(row2+2,col1-2,col2+2-(col1-2),2);
    Hline(row2+1,col1-1,col2+1-(col1-1),2);
    Vline(row1-1,col2+1,row2+1-(row1-1),2);
    Vline(row1-2,col2+2,row2+2-(row1-2),2);
    setPixel(row1-1, col1-1, 1);
    setPixel(row1-2, col1-2, 1);
    setPixel(row1-1, col2+1, 1);
    setPixel(row1-2, col2+2, 1);
    setPixel(row2+1, col1-1, 1);
    setPixel(row2+2, col1-2, 1);
    setPixel(row2+1, col2+1, 1);
    setPixel(row2+2, col2+2, 1);
    
    //calcola centro testo
    colmid = (col2-col1)/2 + col1 +1;
    rowmid = (row2-row1)/2 + row1;
    colmid -= strlen(text)*3;
    rowmid -= 4;
    graph_text(text, rowmid, colmid, nero);

}      


//tabelle di INIT inutilizzate dal vecchio progetto di supersveglia
//per SYSTEMSET
//const char LcdInitTable1[8] = {0x30,0x87,0x07,0x27,0x3F,0xEF,0x28,0x00};
//per SCROLL
//const char LcdInitTable2[10] = {0,0,0xF0,0x80,0x25,0xF0,0,0,0,0};

// initialize the LCD
void NHLCD::Init(void){

    GPIOC->OSPEEDR = 0x01555555;  //uscite fast speed
    GPIOC->MODER |= 0x01550000;
    GPIOC->MODER &= 0xFFFF0000;
    GPIOC->PUPDR = 0x0000AAAA;      //bus dati tutti in pulldown
    WR = 1;
    RD = 1;
    CS = 1;
    delay(10000);
    
    comm_out(0x40); // system set command
    delay(5);
    data_out(0x30); // P1 parameters
    data_out(0x87); // P2 FX horizontal character size (0x80 = 1) MUST BE SUBMULTIPLE OF 320
    data_out(0x07); // P3 FY vertical character size (0x00 = 1)  MUST BE SUBMULTIPLE OF 240
    data_out(0x27); //40); // P4 C/R addresses per line
    data_out(0x3F); //80); // P5 TC/R words per line
    data_out(0xEF); // P6 L/F 240 displace lines
    data_out(0x28); // P7 APL virtual address 1
    data_out(0x00); // P8 APH virtual address 2
    
    comm_out(0x44); // scroll
    data_out(0x00); // start address 1      inizio schermo 0:0
    data_out(0x00); // start address 2
    data_out(0xEF); // 240 lines
    data_out(0x80); // 2nd screen start1    inizio schermo 1:9600
    data_out(0x25); // 2nd screen start2
    data_out(0xEF); // 2nd screen 240 lines
    data_out(0x00); // 3rd screen address1
    data_out(0x00); // 3rd screen address2
    data_out(0x00); // 4th screen address1
    data_out(0x00); // 4th screen address2
    
    comm_out(0x5A); // hdot scr
    data_out(0x00); // horizontal pixel shift = 0
    
    comm_out(0x5B); // overlay
    data_out(0x0C); // OR + layer1 graphic
    
    comm_out(0x58); // set display
    data_out(0x56); 
    
    comm_out(0x5D); // cursor form
    data_out(0x04); // 5 pixels wide
    data_out(0x86); // 7 pixels tall
    
    comm_out(0x4C); // cursor direction = right
    
    comm_out(0x59); // disp on/off
    data_out(0x34); // screen1 ON, 2 flashing
    
    clearScreen();
    



}

/**
 * Draw a rectangle with rounded corners
 *
 * @param x the x coordinate of the upper left corner of the rectangle
 * @param y the y coordinate of the upper left corner of the rectangle
 * @param width width of the rectangle
 * @param height height of the rectangle
 * @param radius radius of rounded corners
 * @param color 
 *
 * Draw a rectangle with rounded corners.
 * Radius is a value from 1 to half the smaller of height or width of the rectangle.
 * The upper left corner at x,y and the lower right
 * corner at x+width-1, y+width-1.\n
 * The left and right edges of the rectangle are at x and x+width-1.\n
 * The top and bottom edges of the rectangle ar at y and y+height-1.\n
 *
 */

void NHLCD::DrawRoundRect(int y, int x, int width, int height, int radius, char color)
{
    int tSwitch; 
    int x1 = 0, y1 = radius;
    tSwitch = 3 - 2 * radius;
    
    while (x1 <= y1)
    {
        // upper left corner
        setPixel(y+radius - y1, x+radius - x1, color); // upper half
        setPixel(y+radius - x1, x+radius - y1, color); // lower half

        
        // upper right corner
        setPixel(y+radius - y1, x+width-radius-1 + x1, color); // upper half
        setPixel(y+radius - x1, x+width-radius-1 + y1, color); // lower half

        // lower right corner
        setPixel(y+height-radius-1 + y1, x+width-radius-1 + x1, color); // lower half
        setPixel(y+height-radius-1 + x1, x+width-radius-1 + y1, color); // upper half

        // lower left corner
        setPixel(y+height-radius-1 + y1, x+radius - x1, color); // lower half
        setPixel(y+height-radius-1 + x1, x+radius - y1, color); // upper half

        if (tSwitch < 0)
        {
            tSwitch += (4 * x1 + 6);
        }
        else
        {
            tSwitch += (4 * (x1 - y1) + 10);
            y1--;
        }
        x1++;
    }
        
    Hline(y, x+radius, width-(2*radius), color);          // top
    Hline(y+height-1, x+radius, width-(2*radius), color); // bottom
    Vline(y+radius, x, height-(2*radius), color);         // left
    Vline(y+radius, x+width-1, height-(2*radius), color); // right
}

/**
 * Draw a Circle
 *
 * @param xCenter X coordinate of the center of the circle
 * @param yCenter Y coordinate of the center of the circle
 * @param radius radius of circle
 * @param color
 *
 * Draw a circle of the given radius extending out radius pixels
 * from the center pixel.
 * The circle will fit inside a rectanglular area bounded by
 * x-radius,y-radius and x+radius,y+radius
 *
 * The diameter of the circle is radius*2 +1 pixels.
 *
 * Color is optional and defaults to ::PIXEL_ON.
 *
 */
void NHLCD::DrawCircle(int yCenter, int xCenter, int radius, int color)
{
    DrawRoundRect(yCenter-radius, xCenter-radius, 2*radius+1, 2*radius+1, radius, color);
}

//disegna un'arco di cerchio, la usiamo per sbiancare la bocca del telescopio
//per evitare errori di sottocampionamento che possano lasciare puntini neri
//al posto di ogni punto disegnamo un quadrato di 3x3
void NHLCD::DrawCircleArc(int yCenter, int xCenter, int radius, int angleBegin, int angleEnd, int color) {
    int ang;
    float angexec;
    float x,y;
    int xint,yint;
    if (angleBegin > angleEnd) 
        angleEnd += 360;
    angexec = (float)angleBegin;
    angexec *= 0.017453292; // 2pi/360
    for(ang=angleBegin; ang<angleEnd; ang++){
        x = sin(angexec);
        x *= radius;
        x += xCenter;
        y = cos(angexec);
        y *= radius;
        y = yCenter - y;
        xint = (int)x -1;
        yint = (int)y -1;
        //setPixel(y,x,color);
        Hline(yint++,xint,3,color);
        Hline(yint++,xint,3,color);
        Hline(yint,xint,3,color);
        angexec += 0.017453292;
        if (angexec >= 6.283185307)
            angexec = 0;
    };
}

//mappa punti cerchio
const char MappaCerchio[346*3] = {83,1,14,
    79,2,22,72,3,36,67,4,46,64,5,52,61,6,22,97,6,22,
    58,7,18,104,7,18,55,8,16,109,8,16,53,9,15,112,9,15,
    51,10,14,115,10,14,49,11,13,118,11,13,47,12,12,121,12,12,
    45,13,12,123,13,12,43,14,12,125,14,12,42,15,11,127,15,11,
    40,16,11,129,16,11,39,17,10,131,17,10,37,18,10,133,18,10,
    36,19,10,134,19,10,35,20,9,136,20,9,33,21,10,137,21,10,
    32,22,9,139,22,9,31,23,9,140,23,9,30,24,9,141,24,9,
    29,25,8,143,25,8,28,26,8,144,26,8,27,27,8,145,27,8,
    26,28,8,146,28,8,25,29,8,147,29,8,24,30,8,148,30,8,
    23,31,8,149,31,8,22,32,8,150,32,8,21,33,8,151,33,8,
    21,34,7,152,34,7,20,35,7,153,35,7,19,36,7,154,36,7,
    18,37,7,155,37,7,18,38,7,155,38,7,17,39,7,156,39,7,
    16,40,7,157,40,7,16,41,6,158,41,6,15,42,7,158,42,7,
    14,43,7,159,43,7,14,44,6,160,44,6,13,45,7,160,45,7,
    13,46,6,161,46,6,12,47,6,162,47,6,12,48,6,162,48,6,
    11,49,6,163,49,6,11,50,6,163,50,6,10,51,6,164,51,6,
    10,52,6,164,52,6,9,53,6,165,53,6,9,54,6,165,54,6,
    8,55,6,166,55,6,8,56,6,166,56,6,8,57,5,167,57,5,
    7,58,6,167,58,6,7,59,5,168,59,5,7,60,5,168,60,5,
    6,61,6,168,61,6,6,62,5,169,62,5,6,63,5,169,63,5,
    5,64,6,169,64,6,5,65,5,170,65,5,5,66,5,170,66,5,
    4,67,6,170,67,6,4,68,5,171,68,5,4,69,5,171,69,5,
    4,70,5,171,70,5,4,71,4,172,71,4,3,72,5,172,72,5,
    3,73,5,172,73,5,3,74,5,172,74,5,3,75,5,172,75,5,
    3,76,4,173,76,4,3,77,4,173,77,4,3,78,4,173,78,4,
    2,79,5,173,79,5,2,80,5,173,80,5,2,81,5,173,81,5,
    2,82,5,173,82,5,2,83,4,174,83,4,2,84,4,174,84,4,
    1,85,5,174,85,5,1,86,5,174,86,5,1,87,5,174,87,5,
    1,88,5,174,88,5,1,89,5,174,89,5,1,90,5,174,90,5,
    1,91,5,174,91,5,1,92,5,174,92,5,1,93,5,174,93,5,
    1,94,5,174,94,5,2,95,4,174,95,4,2,96,4,174,96,4,
    2,97,5,173,97,5,2,98,5,173,98,5,2,99,5,173,99,5,
    2,100,5,173,100,5,3,101,4,173,101,4,3,102,4,173,102,4,
    3,103,4,173,103,4,3,104,5,172,104,5,3,105,5,172,105,5,
    3,106,5,172,106,5,3,107,5,172,107,5,4,108,4,172,108,4,
    4,109,5,171,109,5,4,110,5,171,110,5,4,111,5,171,111,5,
    4,112,6,170,112,6,5,113,5,170,113,5,5,114,5,170,114,5,
    5,115,6,169,115,6,6,116,5,169,116,5,6,117,5,169,117,5,
    6,118,6,168,118,6,7,119,5,168,119,5,7,120,5,168,120,5,
    7,121,6,167,121,6,8,122,5,167,122,5,8,123,6,166,123,6,
    8,124,6,166,124,6,9,125,6,165,125,6,9,126,6,165,126,6,
    10,127,6,164,127,6,10,128,6,164,128,6,11,129,6,163,129,6,
    11,130,6,163,130,6,12,131,6,162,131,6,12,132,6,162,132,6,
    13,133,6,161,133,6,13,134,7,160,134,7,14,135,6,160,135,6,
    14,136,7,159,136,7,15,137,7,158,137,7,16,138,6,158,138,6,
    16,139,7,157,139,7,17,140,7,156,140,7,18,141,7,155,141,7,
    18,142,7,155,142,7,19,143,7,154,143,7,20,144,7,153,144,7,
    21,145,7,152,145,7,21,146,8,151,146,8,22,147,8,150,147,8,
    23,148,8,149,148,8,24,149,8,148,149,8,25,150,8,147,150,8,
    26,151,8,146,151,8,27,152,8,145,152,8,28,153,8,144,153,8,
    29,154,8,143,154,8,30,155,9,141,155,9,31,156,9,140,156,9,
    32,157,9,139,157,9,33,158,10,137,158,10,35,159,9,136,159,9,
    36,160,10,134,160,10,37,161,10,133,161,10,39,162,10,131,162,10,
    40,163,11,129,163,11,42,164,11,127,164,11,43,165,12,125,165,12,
    45,166,12,123,166,12,47,167,12,121,167,12,49,168,13,118,168,13,
    51,169,14,115,169,14,53,170,15,112,170,15,55,171,16,109,171,16,
    58,172,18,104,172,18,61,173,22,97,173,22,64,174,52,67,175,46,
    72,176,36,79,177,22,83,178,14 };

void NHLCD::DisegnaCerchio(void){
    int n,x,y,l;
    const char * ptr = MappaCerchio;
    
    //dobbiamo plottare 346 righe, di ciascuna abbiamo y,x,lunghezza dall'array che 
    //contiene 346 punti con 3 char ciascuno
    for (n=0;n<346;n++) {
        //usiamo 3 variabili perche' apparentemente scambia i termini *ptr++ se messi 
        //  nella stessa Hline
        x = 30+ *ptr++;
        y = 30+ *ptr++;
        l = *ptr++;
        Hline(y, x, l, nero);        
    };
}


const char MappaTelescopio[660*2] = {
0,10,1,10,2,10,3,9,3,10,3,11,4,9,4,10,
4,11,5,9,5,10,5,11,6,9,6,10,6,11,7,9,
7,10,7,11,8,9,8,10,8,11,9,8,9,9,9,10,
9,11,9,12,10,8,10,9,10,10,10,11,10,12,11,8,
11,9,11,10,11,11,11,12,12,8,12,9,12,10,12,11,
12,12,13,8,13,9,13,10,13,11,13,12,14,8,14,9,
14,10,14,11,14,12,15,7,15,8,15,9,15,10,15,11,
15,12,15,13,16,7,16,8,16,9,16,10,16,11,16,12,
16,13,17,7,17,8,17,9,17,10,17,11,17,12,17,13,
18,7,18,8,18,9,18,10,18,11,18,12,18,13,19,7,
19,8,19,9,19,10,19,11,19,12,19,13,20,7,20,8,
20,9,20,10,20,11,20,12,20,13,21,6,21,7,21,8,
21,9,21,10,21,11,21,12,21,13,21,14,22,6,22,7,
22,8,22,9,22,10,22,11,22,12,22,13,22,14,23,6,
23,7,23,8,23,9,23,10,23,11,23,12,23,13,23,14,
24,6,24,7,24,8,24,9,24,10,24,11,24,12,24,13,
24,14,25,6,25,7,25,8,25,9,25,10,25,11,25,12,
25,13,25,14,26,6,26,7,26,8,26,9,26,10,26,11,
26,12,26,13,26,14,27,5,27,6,27,7,27,8,27,9,
27,10,27,11,27,12,27,13,27,14,27,15,28,5,28,6,
28,7,28,8,28,9,28,10,28,11,28,12,28,13,28,14,
28,15,29,5,29,6,29,7,29,8,29,9,29,10,29,11,
29,12,29,13,29,14,29,15,30,5,30,6,30,7,30,8,
30,9,30,10,30,11,30,12,30,13,30,14,30,15,31,5,
31,6,31,7,31,8,31,9,31,10,31,11,31,12,31,13,
31,14,31,15,32,5,32,6,32,7,32,8,32,9,32,10,
32,11,32,12,32,13,32,14,32,15,33,4,33,5,33,6,
33,7,33,8,33,9,33,10,33,11,33,12,33,13,33,14,
33,15,33,16,34,4,34,5,34,6,34,7,34,8,34,9,
34,10,34,11,34,12,34,13,34,14,34,15,34,16,35,4,
35,5,35,6,35,7,35,8,35,9,35,10,35,11,35,12,
35,13,35,14,35,15,35,16,36,4,36,5,36,6,36,7,
36,8,36,9,36,10,36,11,36,12,36,13,36,14,36,15,
36,16,37,4,37,5,37,6,37,7,37,8,37,9,37,10,
37,11,37,12,37,13,37,14,37,15,37,16,38,4,38,5,
38,6,38,7,38,8,38,9,38,10,38,11,38,12,38,13,
38,14,38,15,38,16,39,3,39,4,39,5,39,6,39,7,
39,8,39,9,39,10,39,11,39,12,39,13,39,14,39,15,
39,16,39,17,40,3,40,4,40,5,40,6,40,7,40,8,
40,9,40,10,40,11,40,12,40,13,40,14,40,15,40,16,
40,17,41,3,41,4,41,5,41,6,41,7,41,8,41,9,
41,10,41,11,41,12,41,13,41,14,41,15,41,16,41,17,
42,3,42,4,42,5,42,6,42,7,42,8,42,9,42,10,
42,11,42,12,42,13,42,14,42,15,42,16,42,17,43,3,
43,4,43,5,43,6,43,7,43,8,43,9,43,10,43,11,
43,12,43,13,43,14,43,15,43,16,43,17,44,3,44,4,
44,5,44,6,44,7,44,8,44,9,44,10,44,11,44,12,
44,13,44,14,44,15,44,16,44,17,45,2,45,3,45,4,
45,5,45,6,45,7,45,8,45,9,45,10,45,11,45,12,
45,13,45,14,45,15,45,16,45,17,45,18,46,2,46,3,
46,4,46,5,46,6,46,7,46,8,46,9,46,10,46,11,
46,12,46,13,46,14,46,15,46,16,46,17,46,18,47,2,
47,3,47,4,47,5,47,6,47,7,47,8,47,9,47,10,
47,11,47,12,47,13,47,14,47,15,47,16,47,17,47,18,
48,2,48,3,48,4,48,5,48,6,48,7,48,8,48,9,
48,10,48,11,48,12,48,13,48,14,48,15,48,16,48,17,
48,18,49,2,49,3,49,4,49,5,49,6,49,7,49,8,
49,9,49,10,49,11,49,12,49,13,49,14,49,15,49,16,
49,17,49,18,50,2,50,3,50,4,50,5,50,6,50,7,
50,8,50,9,50,10,50,11,50,12,50,13,50,14,50,15,
50,16,50,17,50,18,51,1,51,2,51,3,51,4,51,5,
51,6,51,7,51,8,51,9,51,10,51,11,51,12,51,13,
51,14,51,15,51,16,51,17,51,18,51,19,52,1,52,2,
52,3,52,4,52,5,52,6,52,7,52,8,52,9,52,10,
52,11,52,12,52,13,52,14,52,15,52,16,52,17,52,18,
52,19,53,1,53,2,53,3,53,4,53,5,53,6,53,7,
53,8,53,9,53,10,53,11,53,12,53,13,53,14,53,15,
53,16,53,17,53,18,53,19,54,1,54,2,54,3,54,4,
54,5,54,6,54,7,54,8,54,9,54,10,54,11,54,12,
54,13,54,14,54,15,54,16,54,17,54,18,54,19,55,1,
55,2,55,3,55,4,55,5,55,6,55,7,55,8,55,9,
55,10,55,11,55,12,55,13,55,14,55,15,55,16,55,17,
55,18,55,19,56,1,56,2,56,3,56,4,56,5,56,6,
56,7,56,8,56,9,56,10,56,11,56,12,56,13,56,14,
56,15,56,16,56,17,56,18,56,19,57,0,57,1,57,2,
57,3,57,4,57,5,57,6,57,7,57,8,57,9,57,10,
57,11,57,12,57,13,57,14,57,15,57,16,57,17,57,18,
57,19,57,20,58,0,58,1,58,2,58,3,58,4,58,5,
58,6,58,7,58,8,58,9,58,10,58,11,58,12,58,13,
58,14,58,15,58,16,58,17,58,18,58,19,58,20,59,0,
59,1,59,2,59,3,59,4,59,5,59,6,59,7,59,8,
59,9,59,10,59,11,59,12,59,13,59,14,59,15,59,16,
59,17,59,18,59,19,59,20};

void NHLCD::DrawTelescope(int angle, int color) {
    float x1,y1,x2,y2,cosang,sinang,angexec;
    int n;
    const char * ptr = MappaTelescopio;

    //centro dell'immagine da ruotare
    #define x0 10
    #define y0 60
    /* formula
    x2 = cos(ang)*(x1-x0) - sin(ang)*(y1-y0) + x0
    y2 = sin(ang)*(x1-x0) - cos(ang)*(y1-y0) + y0
    dove x0y0 è il centro dell'immagine
    x1y1 coordinate originali dell'elemento
    x2y2 coordinate trasformate dopo la rotazione
    */

    //precalcola costanti
    angexec = (float)angle;
    angexec *= 0.017453292; // 2pi/360
    cosang = cos(angexec);
    sinang = sin(angexec);
    //disegna l'immagine pixel per pixel
    for(n=0; n<660; n++){
        y1 = *ptr++;
        x1 = *ptr++;
        y2 = sinang * (x1-x0);
        y2 += cosang * (y1-y0);
        y2 += y0;
        x2 = cosang * (x1-x0);
        x2 -= sinang * (y1-y0);
        x2 += x0;
        //aggiungiamo il centro schermo diminuito di metà immagine
        setPixel(y2+60 , x2+110, color);
        setPixel(y2+61 , x2+110, color);
    };
}
