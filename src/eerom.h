/*
 *  eerom.h
 *  EEROM locations and read/write facilities
 *
 *  DomeControl for STM32 Nucleo
 *  (C) 2021 - Giulio Corradini, Osservatorio di Cavezzo
 */

#pragma once

//elenco inizio locazioni di memoria EEROM
#define EeromTouchLoc   0       //16 locazioni per calibrazione touchscreen
#define EeromParkLoc    16      //2 locazioni per quota parcheggio
#define EeromStopRampPulsesLoc  0x20
#define I2cMemAddr      0xA0    //indirizzo eerom su bus I2C

//  Read a sequence of bytes from EEROM at given address
bool eerom_load(char *dst, int n, int addr);

//  Write a sequence of bytes to EEROM
bool eerom_store(char *src, int n, int addr);

template<typename T>
class EeromObject {
    public:
        int address;
        EeromObject(int);
        operator T();
        operator bool();
        EeromObject<T>& operator=(T);

    private:
        bool i2c_status;
        T cached_object;
};