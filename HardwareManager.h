#ifndef HARDWARE_MANAGER_H
#define HARDWARE_MANAGER_H

#include "Config.h"
#include <Wire.h>
#include <Adafruit_MCP23X17.h>

class HardwareManager {
private:
  // Instancias de los MCP23017
  Adafruit_MCP23X17 mcpEncodersVol;    // MCP1 - Encoders Volumen
  Adafruit_MCP23X17 mcpEncodersPan;    // MCP2 - Encoders Pan
  Adafruit_MCP23X17 mcpSwitches;       // MCP3 - Switches Mute/Solo  
  Adafruit_MCP23X17 mcpButtonsEnc;     // MCP4 - Botones + Nav Encoder
  
  // Estados para detección de cambios
  uint16_t lastMCP3State;
  uint16_t lastMCP4State;
  
  // Variables para encoders normales
  int8_t lastEncoded[NUM_ENCODERS];
  int32_t encoderValue[NUM_ENCODERS];
  unsigned long lastEncoderTime[NUM_ENCODERS];
  
  // Variables para encoder de navegación  
  int8_t lastEncodedNav;
  int32_t encoderValueNav;
  unsigned long lastEncoderTimeNav;
  unsigned long lastNavPressTime;
  bool navButtonState;
  bool lastNavButtonState;
  
  // Contadores de diagnóstico
  uint32_t encoderReadCount;
  uint32_t switchPressCount;
  uint32_t errorCount;
  
  // Métodos privados de inicialización
  bool initializeMCP(Adafruit_MCP23X17& mcp, uint8_t address, const char* name);
  void configureEncoderMCP(Adafruit_MCP23X17& mcp, uint8_t intPinA, uint8_t intPinB);
  void configureSwitchMCP(Adafruit_MCP23X17& mcp);
  
  // Procesamiento de encoders
  void processEncoder(int mcpIndex, int encoderIndex, Adafruit_MCP23X17& mcp);
  int8_t calculateEncoderChange(int8_t encoded, int8_t lastEncoded);
  uint8_t calculateAcceleration(unsigned long timeDiff);
  
  // Validación y diagnóstico
  bool validateMCPResponse(Adafruit_MCP23X17& mcp, const char* name);
  void handleMCPError(const char* mcpName, const char* operation);

public:
  HardwareManager();
  ~HardwareManager();
  
  // Inicialización del sistema
  bool initialize();
  void setupInterrupts();
  
  // Procesamiento de encoders (llamado desde ISRs)
  void processMCP1AEncoders();
  void processMCP1BEncoders();  
  void processMCP2AEncoders();
  void processMCP2BEncoders();
  
  // Polling de switches y botones
  void pollSwitchesAndButtons();
  
  // Encoder de navegación
  void readNavigationEncoder();
  
  // Diagnóstico y mantenimiento
  bool checkMCPHealth();
  void resetMCPs();
  void clearAllInterrupts();
  
  // Getters para diagnóstico
  uint32_t getEncoderReadCount() const { return encoderReadCount; }
  uint32_t getSwitchPressCount() const { return switchPressCount; }
  uint32_t getErrorCount() const { return errorCount; }
  
  // Test y calibración
  bool testAllMCPs();
  void calibrateEncoders();
  
  // Métodos de utilidad
  bool isInitialized() const;
  void printDiagnostics() const;
};

#endif // HARDWARE_MANAGER_H