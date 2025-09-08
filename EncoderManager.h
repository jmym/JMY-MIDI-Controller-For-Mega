#ifndef ENCODER_MANAGER_H
#define ENCODER_MANAGER_H

#include "Config.h"
#include <Arduino.h>

class EncoderManager {
private:
    EncoderConfig encoderBanks[NUM_ENCODERS][NUM_BANKS];
    bool encoderAccelerationEnabled;
    uint8_t currentBank;
    
public:
    EncoderManager();
    ~EncoderManager();
    
    bool initialize(AppConfig* config);
    void setCurrentBank(uint8_t bank);
    
    // Configuration access
    EncoderConfig getEncoderConfig(uint8_t index, uint8_t bank);
    EncoderConfig& getEncoderConfigMutable(uint8_t index, uint8_t bank);
    EncoderConfig (*getEncoderBanks())[NUM_BANKS];
    
    // Processing
    void processEncoderChange(uint8_t encoderIndex, int8_t change, uint8_t bank);
    void processSwitchPress(uint8_t switchIndex, uint8_t bank);
    
    // DAW synchronization
    void syncFromDAW(uint8_t track, uint8_t bank, uint8_t value, uint16_t color);
    void setEncoderDAWValue(uint8_t track, uint8_t bank, uint8_t value);
    uint8_t getEncoderDAWValue(uint8_t track, uint8_t bank);
    void syncNameFromDAW(uint8_t track, uint8_t bank, const char* name);
    
    // Configuration management
    void resetEncoderConfig(uint8_t index, uint8_t bank);
    void resetAllBanks();
    
    // Acceleration
    bool getEncoderAcceleration() const;
    void setEncoderAcceleration(bool enabled);
};

#endif // ENCODER_MANAGER_H