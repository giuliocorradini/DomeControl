#include "mbed.h"

//I2C
extern I2C i2c;
extern char i2cmembuff[9];      //buffer di scambio dati per i2c
#define I2cMemAddr 0xA0         //indirizzo eerom su bus I2C
#define i2cNoEnd    true
#define i2cEnd      false