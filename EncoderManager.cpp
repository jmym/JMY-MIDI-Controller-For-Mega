#include "EncoderManager.h"
#include "Midi_Controller.h"

extern Midi_Controller midi_controller;

EncoderManager::EncoderManager() 
    : encoderAccelerationEnabled(true), currentBank(0) 
{
    // Initialize encoder banks with default values
    for (int bank = 0; bank < NUM_BANKS; bank++) {
        for (int enc = 0; enc < NUM_ENCODERS; enc++) {
            encoderBanks[bank][enc] = EncoderConfig();
        }
    }
}

EncoderManager::~EncoderManager() {}

bool EncoderManager::initialize(AppConfig* config) {
    Serial.println(F("Inicializando gestor de encoders..."));
    encoderAccelerationEnabled = config->encoderAcceleration;
    currentBank = config->currentBank;
    return true;
}

void EncoderManager::setCurrentBank(uint8_t bank) {
    currentBank = bank;
}

EncoderConfig EncoderManager::getEncoderConfig(uint8_t index, uint8_t bank) {
    if (index < NUM_ENCODERS && bank < NUM_BANKS) {
        return encoderBanks[bank][index];
    }
    return EncoderConfig();
}

EncoderConfig& EncoderManager::getEncoderConfigMutable(uint8_t index, uint8_t bank) {
    static EncoderConfig dummy;
    if (index < NUM_ENCODERS && bank < NUM_BANKS) {
        return encoderBanks[bank][index];
    }
    return dummy;
}

EncoderConfig (*EncoderManager::getEncoderBanks())[NUM_BANKS] {
    return encoderBanks;
}

void EncoderManager::processEncoderChange(uint8_t encoderIndex, int8_t change, uint8_t bank) {
    if (encoderIndex >= NUM_ENCODERS || bank >= NUM_BANKS) return;
    
    EncoderConfig& config = encoderBanks[bank][encoderIndex];
    config.value = constrain(config.value + change, config.minValue, config.maxValue);
    
    // Send MIDI message based on control type
    switch (config.controlType) {
        case CT_CC:
            midi_controller.sendControlChange(config.channel, config.control, config.value);
            break;
        case CT_NOTE:
            if (change > 0) {
                midi_controller.sendNoteOn(config.channel, config.control, config.value);
            } else {
                midi_controller.sendNoteOff(config.channel, config.control, 0);
            }
            break;
        case CT_PITCH:
            midi_controller.sendPitchBend(config.channel, map(config.value, 0, 127, -8192, 8191));
            break;
    }
}

void EncoderManager::processSwitchPress(uint8_t switchIndex, uint8_t bank) {
    // Handle mute/solo switches
    if (switchIndex < 8) {
        // Mute switches
        uint8_t track = switchIndex;
        if (track < NUM_ENCODERS && bank < NUM_BANKS) {
            encoderBanks[bank][track].isMute = !encoderBanks[bank][track].isMute;
            // Send MIDI mute command (CC 120-127 are typically used for mutes)
            midi_controller.sendControlChange(encoderBanks[bank][track].channel, 120 + track, encoderBanks[bank][track].isMute ? 127 : 0);
        }
    } else if (switchIndex < 16) {
        // Solo switches
        uint8_t track = switchIndex - 8;
        if (track < NUM_ENCODERS && bank < NUM_BANKS) {
            encoderBanks[bank][track].isSolo = !encoderBanks[bank][track].isSolo;
            // Send MIDI solo command
            midi_controller.sendControlChange(encoderBanks[bank][track].channel, 110 + track, encoderBanks[bank][track].isSolo ? 127 : 0);
        }
    }
}

void EncoderManager::syncFromDAW(uint8_t track, uint8_t bank, uint8_t value, uint16_t color) {
    if (track < NUM_ENCODERS && bank < NUM_BANKS) {
        encoderBanks[bank][track].dawValue = value;
        encoderBanks[bank][track].trackColor = color;
        //encoderBanks[bank][track].panColor = color;
    }
}

void EncoderManager::setEncoderDAWValue(uint8_t track, uint8_t bank, uint8_t value) {
    if (track < NUM_ENCODERS && bank < NUM_BANKS) {
        encoderBanks[bank][track].dawValue = value;
    }
}

uint8_t EncoderManager::getEncoderDAWValue(uint8_t track, uint8_t bank) {
    if (track < NUM_ENCODERS && bank < NUM_BANKS) {
        return encoderBanks[bank][track].dawValue;
    }
    return 0;
}

void EncoderManager::syncNameFromDAW(uint8_t track, uint8_t bank, const char* name) {
     if (track < NUM_ENCODERS && bank < NUM_BANKS && name) {
    // Copiar máximo 5 caracteres + null terminator
        strncpy(encoderBanks[bank][track].trackName, name, 5);
        encoderBanks[bank][track].trackName[5] = '\0';
    }
}

void EncoderManager::resetEncoderConfig(uint8_t index, uint8_t bank) {
    if (index < NUM_ENCODERS && bank < NUM_BANKS) {
        encoderBanks[bank][index] = EncoderConfig();
    }
}

void EncoderManager::resetAllBanks() {
    for (int bank = 0; bank < NUM_BANKS; bank++) {
        for (int enc = 0; enc < NUM_ENCODERS; enc++) {
            encoderBanks[bank][enc] = EncoderConfig();
        }
    }
}

bool EncoderManager::getEncoderAcceleration() const {
    return encoderAccelerationEnabled;
}

void EncoderManager::setEncoderAcceleration(bool enabled) {
    encoderAccelerationEnabled = enabled;
}