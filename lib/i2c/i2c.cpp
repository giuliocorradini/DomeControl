#include "i2c.h"

//I2C per eerom
I2C i2c(PB_9, PB_8);
char i2cmembuff[9];             //buffer di scambio dati per i2c
