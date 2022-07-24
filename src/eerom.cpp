#include "eerom.h"
#include "i2c.h"
#include "mbed.h"

bool eerom_load(char *dst, int n, int addr) {
    bool isThereAProblem = false;

    isThereAProblem |= i2c.write(I2cMemAddr, reinterpret_cast<char *>(&addr), 1, i2cNoEnd);
    isThereAProblem |= i2c.read(I2cMemAddr, dst, n, i2cEnd);

    return !isThereAProblem;
}

bool eerom_store(char *src, int n, int addr) {
    bool isThereAProblem = false;

    isThereAProblem |= i2c.write(I2cMemAddr, reinterpret_cast<char *>(&addr), 1, i2cNoEnd);
    isThereAProblem |= i2c.write(I2cMemAddr, src, n, i2cEnd);

    return !isThereAProblem;
}

template<typename T>
EeromObject<T>::EeromObject(int address) {
    this->address = address;
    i2c_status = true;
}

template<typename T>
EeromObject<T>::operator T() {
    i2c_status = eerom_load(reinterpret_cast<char *>(&cached_object), sizeof(T), address);
    
    if(i2c_status)
        return cached_object;

    return 0;
}

template<typename T>
EeromObject<T>& EeromObject<T>::operator =(T obj) {
    i2c_status = eerom_store(reinterpret_cast<char *>(&obj), sizeof(T), address);
    return this;
}

template<typename T>
EeromObject<T>::operator bool() {
    return i2c_status;
}
