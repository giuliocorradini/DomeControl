/* mbed Newhaven LCD Library, for the NHD-320240WG model
 * Copyright (c) 2011, Paul Evans
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "mbed.h" 

class NHLCD {
    public:
        /* Creates a Newhaven LCD interface using several DigitalOut pins and a 8-bit BusInOut
         *
         * @param PIN_RD lettura dal RA8835
         * @param PIN_WR scrittura al RA8835
         * @param PIN_A0 Register select signal (1 = data, 0 = command)
         * @param PIN_CS Active LOW chip select
         * @param PIN_Busy dal chip RA8835
         */
        NHLCD(PinName PIN_RD,PinName PIN_WR,PinName PIN_A0,PinName PIN_CS,PinName PIN_Busy);
        
        /* Initializes the LCD 
         */
        void Init();
        
        /* Outputs a command across the 8-bit bus to the LCD
         *
         * @param j hex-code of command
         */
        void comm_out(unsigned char j);
        
        /* Outputs data across the 8-bit bus to the LCD
         *
         * @param j data to send
         */
        void data_out(unsigned char j);
        
        /* Clears the entire screen of set pixels
         */
        void clearScreen();
        
        /* Writes text to the LCD (with a resolution of 40x30 characters)
         *
         * @param text a string of text to write on the screen
         * @param row the row of the first character
         * @param col the column of the first character
         */
        void text(char* text, char row, char col);

        //idem ma per schermo grafico
        void graph_text(char * textptr, unsigned int row, unsigned int col, char color);

        
        /* Sets an individual pixel on the LCD (with a resolution of 320x240 pixels)
         *
         * @param row the row of the pixel
         * @param col the column of the pixel
         * @param color 1 = on, 0 = off, 2=grigio
         */
        void setPixel(int row, int col, int color);
        
        /* draws a line on the LCD
         *
         * @param r1 the row of the first endpoint
         * @param c1 the column of the first endpoint
         * @param r2 the row of the second endpoint
         * @param c2 the column of the second endpoint
         * @param color 1 = on, 0 = off
         */
        void drawLine(int r1, int c1, int r2, int c2, int color);

        //disegna una linea verticale
        void Vline (unsigned int row, unsigned int col, char h, char color);
 
        //disegna una linea orizzontale
        void Hline (unsigned int row, unsigned int col, unsigned int w, char color);
        
        /* draws and fills a rectangle on the LCD
         *
         * @param row the row of the top-left pixel
         * @param col the column of the top-left pixel
         * @param width the width of the rectangle
         * @param height the height of the rectangle
         * @param color 1 = on, 0 = off
         */
        void fillRect(int row, int col, int width, int height, int color);

        void Rect( unsigned int row, unsigned int col, unsigned int w, unsigned int h, unsigned short color);

        void BtnDraw(int row1, int col1, int row2, int col2, char * text);
        
        void DrawCircle(int yCenter, int xCenter, int radius, int color);
        
        void DrawRoundRect(int y, int x, int width, int height, int radius, char color);

        void DrawCircleArc(int yCenter, int xCenter, int radius, int angleBegin, int angleEnd, int color);

        void DisegnaCerchio(void);
        void DrawTelescope(int angle, int color);
        void Busy(void);
        
        private:
        DigitalOut RD,WR,A0,CS;
        DigitalIn BUSY;   
        //BusInOut *LCD_PORT;
        unsigned char screenBuffer[240*40];   
        
        char VramRd(unsigned int Loc);
        void RamSelect(unsigned int Loc);
        //void Busy(void);

};

#define bianco 0
#define nero 1
#define grigio 2
#define nogrigio 3       //per cancellare il grigio che è in'un'altra area di memoria

#define LCD_XDOTS 320
#define LCD_YDOTS 240
#define GRAPHIC_AREA 40     //bytes in una riga
#define Schermo180          //ribalta lo schermo di 180° causa errore di montaggio
void delay(unsigned int n);
void delay1(unsigned int n);
void swap(int* a, int* b);
