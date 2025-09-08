#ifndef STRINGS_H
#define STRINGS_H

#include <Arduino.h>
#include <avr/pgmspace.h>

namespace Strings {
const char str_volume[] PROGMEM = "Volumen";
const char str_pan[] PROGMEM = "Pan";
const char str_mute[] PROGMEM = "Mute";
const char str_solo[] PROGMEM = "Solo";
const char str_track[] PROGMEM = "Track";
const char str_bank[] PROGMEM = "Banco";
const char str_time[] PROGMEM = "Tiempo";

const char str_encoders[] PROGMEM = "Configurar Encoders";
const char str_display[] PROGMEM = "Configurar Pantalla";
const char str_midi[] PROGMEM = "Configurar MIDI";
const char str_global[] PROGMEM = "Ajustes Globales";
const char str_system_test[] PROGMEM = "Test Sistema";
const char str_exit[] PROGMEM = "Salir";

const char str_loading[] PROGMEM = "Cargando...";
const char str_error[] PROGMEM = "Error";
const char str_success[] PROGMEM = "Exito";
const char str_warning[] PROGMEM = "Advertencia";

const char* const interface_strings[] PROGMEM = {
    str_loading, str_error, str_success, str_warning,
    str_volume, str_pan, str_mute, str_solo, str_track, str_bank, str_time,
    str_encoders, str_display, str_midi, str_global, str_system_test, str_exit
};
enum StringIndex {
    STR_LOADING_IDX,
    STR_ERROR_IDX,
    STR_SUCCESS_IDX,
    STR_WARNING_IDX,
    STR_VOLUME_IDX,
    STR_PAN_IDX,
    STR_MUTE_IDX,
    STR_SOLO_IDX,
    STR_TRACK_IDX,
    STR_BANK_IDX,
    STR_TIME_IDX,
    STR_ENCODERS_IDX,
    STR_DISPLAY_IDX,
    STR_MIDI_IDX,
    STR_GLOBAL_IDX,
    STR_SYSTEM_TEST_IDX,
    STR_EXIT_IDX
};
/*enum StringIndex {
    STR_LOADING, STR_ERROR, STR_SUCCESS, STR_WARNING,
    STR_VOLUME, STR_PAN, STR_MUTE, STR_SOLO, STR_TRACK, STR_BANK, STR_TIME,
    STR_ENCODERS, STR_DISPLAY, STR_MIDI, STR_GLOBAL, STR_SYSTEM_TEST, STR_EXIT
};*/
}

class StringUtils {
public:
    static void readString(uint8_t index, char* buffer, size_t bufferSize) {
        if (index >= sizeof(Strings::interface_strings)/sizeof(Strings::interface_strings[0])) {
            buffer[0] = '\0';
            return;
        }
        const char* address = (const char*)pgm_read_ptr(&Strings::interface_strings[index]);
        strncpy_P(buffer, address, bufferSize);
        buffer[bufferSize - 1] = '\0';
    }

    static void readString(const char* progmemStr, char* buffer, size_t bufferSize) {
        strncpy_P(buffer, progmemStr, bufferSize);
        buffer[bufferSize - 1] = '\0';
    }

    static void printString(uint8_t index) {
        char buffer[50];
        readString(index, buffer, sizeof(buffer));
        Serial.print(buffer);
    }

    static void printString(const char* progmemStr) {
        char buffer[50];
        readString(progmemStr, buffer, sizeof(buffer));
        Serial.print(buffer);
    }
};

#endif