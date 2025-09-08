#ifndef PROGMEM_UTILS_H
#define PROGMEM_UTILS_H

#include <Arduino.h>
#include <avr/pgmspace.h>

class ProgmemUtils {
public:
    // Leer string de PROGMEM a buffer
    static void readString(uint8_t index, char* buffer, size_t bufferSize) {
        const char* address = (const char*)pgm_read_ptr(&interface_strings[index]);
        strncpy_P(buffer, address, bufferSize);
        buffer[bufferSize - 1] = '\0';
    }

    // Leer string directo de dirección
    static void readString(const char* progmemStr, char* buffer, size_t bufferSize) {
        strncpy_P(buffer, progmemStr, bufferSize);
        buffer[bufferSize - 1] = '\0';
    }

    // Imprimir string directamente desde PROGMEM
    static void printString(uint8_t index) {
        const char* address = (const char*)pgm_read_ptr(&interface_strings[index]);
        printString(address);
    }

    static void printString(const char* progmemStr) {
        char buffer[50];
        strncpy_P(buffer, progmemStr, sizeof(buffer));
        Serial.print(buffer);
    }

    // Obtener longitud de string en PROGMEM
    static size_t getStringLength(const char* progmemStr) {
        return strlen_P(progmemStr);
    }
};

#endif