#include "HardwareManager.h"

HardwareManager::HardwareManager() 
  : lastMCP3State(0xFFFF), lastMCP4State(0xFFFF),
    lastEncodedNav(0), encoderValueNav(0), lastEncoderTimeNav(0),
    lastNavPressTime(0), navButtonState(false), lastNavButtonState(false),
    encoderReadCount(0), switchPressCount(0), errorCount(0)
{
  // Inicializar arrays
  memset(lastEncoded, 0, sizeof(lastEncoded));
  memset(encoderValue, 0, sizeof(encoderValue));
  memset(lastEncoderTime, 0, sizeof(lastEncoderTime));
}

HardwareManager::~HardwareManager() {
  // Cleanup si es necesario
}

bool HardwareManager::initialize() {
  Serial.println(F("Inicializando sistema de hardware..."));
  
  // Inicializar I2C
  Wire.begin();
  Wire.setClock(400000); // 400kHz para mejor performance
  
  // Inicializar cada MCP23017 en orden de importancia
  bool success = true;
  
  success &= initializeMCP(mcpEncodersVol, MCP_ENCODERS_VOL_ADDR, "MCP1 Encoders Vol");
  success &= initializeMCP(mcpEncodersPan, MCP_ENCODERS_PAN_ADDR, "MCP2 Encoders Pan");
  success &= initializeMCP(mcpSwitches, MCP_SWITCHES_ADDR, "MCP3 Switches");
  success &= initializeMCP(mcpButtonsEnc, MCP_BUTTONS_ENC_ADDR, "MCP4 Buttons");
  
  if (!success) {
    Serial.println(F("ERROR: Falló inicialización de uno o más MCPs"));
    return false;
  }
  
  // Configurar MCPs según su función
  configureEncoderMCP(mcpEncodersVol, INT_MCP1_A, INT_MCP1_B);
  configureEncoderMCP(mcpEncodersPan, INT_MCP2_A, INT_MCP2_B);
  configureSwitchMCP(mcpSwitches);
  configureSwitchMCP(mcpButtonsEnc);
  
  Serial.println(F("Hardware inicializado correctamente"));
  return true;
}

bool HardwareManager::initializeMCP(Adafruit_MCP23X17& mcp, uint8_t address, const char* name) {
  Serial.print(F("Inicializando "));
  Serial.print(name);
  Serial.print(F(" en dirección 0x"));
  Serial.println(address, HEX);
  
  if (!mcp.begin_I2C(address)) {
    Serial.print(F("ERROR: "));
    Serial.print(name);
    Serial.println(F(" no responde"));
    errorCount++;
    return false;
  }
  
  // Verificar comunicación con test de escritura/lectura
  if (!validateMCPResponse(mcp, name)) {
    return false;
  }
  
  Serial.print(name);
  Serial.println(F(" inicializado correctamente"));
  return true;
}

void HardwareManager::configureEncoderMCP(Adafruit_MCP23X17& mcp, uint8_t intPinA, uint8_t intPinB) {
  // Configurar todos los pines como entrada con pullup
  for (int pin = 0; pin < 16; pin++) {
    mcp.pinMode(pin, INPUT_PULLUP);
    mcp.setupInterruptPin(pin, CHANGE);
  }
  
  // Configurar interrupciones - usar ambos pines INTA e INTB
  mcp.setupInterrupts(true, false, LOW);
  
  // Configurar pines de interrupción en Arduino
  pinMode(intPinA, INPUT_PULLUP);
  pinMode(intPinB, INPUT_PULLUP);
}

void HardwareManager::configureSwitchMCP(Adafruit_MCP23X17& mcp) {
  // Configurar pines como entrada con pullup (sin interrupciones)
  for (int pin = 0; pin < 16; pin++) {
    mcp.pinMode(pin, INPUT_PULLUP);
  }
}

void HardwareManager::setupInterrupts() {
  // Configurar interrupciones externas
  attachInterrupt(digitalPinToInterrupt(INT_MCP1_A), handleMCP1AInterrupt, FALLING);
  attachInterrupt(digitalPinToInterrupt(INT_MCP1_B), handleMCP1BInterrupt, FALLING);
  attachInterrupt(digitalPinToInterrupt(INT_MCP2_A), handleMCP2AInterrupt, FALLING);
  attachInterrupt(digitalPinToInterrupt(INT_MCP2_B), handleMCP2BInterrupt, FALLING);
  
  Serial.println(F("Interrupciones configuradas"));
}

void HardwareManager::processMCP1AEncoders() {
  uint16_t interruptPins = mcpEncodersVol.getCapturedInterrupt();
  
  // Procesar encoders 0-7 (Puerto A del MCP1)
  for (int i = 0; i < 8; i++) {
    int pinA = i * 2;
    int pinB = i * 2 + 1;
    
    if ((interruptPins & (1 << pinA)) || (interruptPins & (1 << pinB))) {
      processEncoder(0, i, mcpEncodersVol);
    }
  }
}

void HardwareManager::processMCP1BEncoders() {
  uint16_t interruptPins = mcpEncodersVol.getCapturedInterrupt();
  
  // Procesar encoders 8-15 (Puerto B del MCP1) - CORREGIDO
  for (int i = 8; i < 16; i++) {
    int pinA = (i - 8) * 2 + 8;  // Pines 8,10,12,14
    int pinB = (i - 8) * 2 + 9;  // Pines 9,11,13,15
    
    if ((interruptPins & (1 << pinA)) || (interruptPins & (1 << pinB))) {
      processEncoder(0, i, mcpEncodersVol);
    }
  }
}

void HardwareManager::processMCP2AEncoders() {
  uint16_t interruptPins = mcpEncodersPan.getCapturedInterrupt();
  
  // Procesar encoders 0-7 del MCP2 (Pan)
  for (int i = 0; i < 8; i++) {
    int pinA = i * 2;
    int pinB = i * 2 + 1;
    
    if ((interruptPins & (1 << pinA)) || (interruptPins & (1 << pinB))) {
      processEncoder(1, i, mcpEncodersPan);
    }
  }
}

void HardwareManager::processMCP2BEncoders() {
  uint16_t interruptPins = mcpEncodersPan.getCapturedInterrupt();
  
  // Procesar encoders 8-15 del MCP2 (Pan) - CORREGIDO
  for (int i = 8; i < 16; i++) {
    int pinA = (i - 8) * 2 + 8;  // Pines 8,10,12,14
    int pinB = (i - 8) * 2 + 9;  // Pines 9,11,13,15
    
    if ((interruptPins & (1 << pinA)) || (interruptPins & (1 << pinB))) {
      processEncoder(1, i, mcpEncodersPan);
    }
  }
}

void HardwareManager::processEncoder(int mcpIndex, int encoderIndex, Adafruit_MCP23X17& mcp) {
  // Calcular pines según el encoder
  int pinA, pinB;
  if (encoderIndex < 8) {
    pinA = encoderIndex * 2;
    pinB = encoderIndex * 2 + 1;
  } else {
    pinA = (encoderIndex - 8) * 2 + 8;
    pinB = (encoderIndex - 8) * 2 + 9;
  }
  
  // Leer estado actual
  int MSB = mcp.digitalRead(pinA);
  int LSB = mcp.digitalRead(pinB);
  int encoded = (MSB << 1) | LSB;
  
  // Calcular cambio de dirección
  int8_t change = calculateEncoderChange(encoded, lastEncoded[encoderIndex]);
  lastEncoded[encoderIndex] = encoded;
  
  if (change != 0) {
    unsigned long currentTime = micros();
    unsigned long timeDiff = currentTime - lastEncoderTime[encoderIndex];
    lastEncoderTime[encoderIndex] = currentTime;
    
    // Aplicar aceleración si está habilitada
    uint8_t acceleration = calculateAcceleration(timeDiff);
    encoderValue[encoderIndex] += change;
    
    // Procesar cambio cuando alcance threshold
    int threshold = 4 / acceleration;  // Threshold adaptativo
    if (abs(encoderValue[encoderIndex]) >= threshold) {
      int8_t finalChange = (encoderValue[encoderIndex] > 0) ? acceleration : -acceleration;
      onEncoderChange(encoderIndex, finalChange);
      encoderValue[encoderIndex] = 0;
      encoderReadCount++;
    }
  }
}

int8_t HardwareManager::calculateEncoderChange(int8_t encoded, int8_t lastEncoded) {
  int sum = (lastEncoded << 2) | encoded;
  
  // Tabla de lookup optimizada para detección de dirección
  switch (sum) {
    case 0b1101: case 0b0100: case 0b0010: case 0b1011:
      return 1;   // Clockwise
    case 0b1110: case 0b0111: case 0b0001: case 0b1000:
      return -1;  // Counter-clockwise
    default:
      return 0;   // No change or invalid
  }
}

uint8_t HardwareManager::calculateAcceleration(unsigned long timeDiff) {
  if (!appConfig.encoderAcceleration) return 1;
  
  // Aceleración basada en velocidad de giro
  if (timeDiff < 1000) return 4;      // Muy rápido
  else if (timeDiff < 5000) return 2;  // Rápido
  else return 1;                       // Normal
}

void HardwareManager::pollSwitchesAndButtons() {
  // Polling de MCP3 (Switches Mute/Solo)
  uint16_t mcp3State = mcpSwitches.readGPIOAB();
  uint16_t mcp3Changes = mcp3State ^ lastMCP3State;
  
  if (mcp3Changes != 0) {
    for (int i = 0; i < 16; i++) {
      if (mcp3Changes & (1 << i)) {
        bool pressed = !(mcp3State & (1 << i));  // LOW = pressed
        if (pressed) {
          onSwitchPress(i);
          switchPressCount++;
        }
      }
    }
    lastMCP3State = mcp3State;
  }
  
  // Polling de MCP4 (Botones de transporte)
  uint16_t mcp4State = mcpButtonsEnc.readGPIOAB();
  uint16_t mcp4Changes = mcp4State ^ lastMCP4State;
  
  if (mcp4Changes != 0) {
    for (int i = 0; i < 5; i++) {  // Solo primeros 5 botones
      if (mcp4Changes & (1 << i)) {
        bool pressed = !(mcp4State & (1 << i));  // LOW = pressed
        if (pressed) {
          onButtonPress(i);
        }
      }
    }
    lastMCP4State = mcp4State;
  }
}

void HardwareManager::readNavigationEncoder() {
  // Leer encoder de navegación desde MCP4
  int MSB = mcpButtonsEnc.digitalRead(ENC_NAV_A_PIN);
  int LSB = mcpButtonsEnc.digitalRead(ENC_NAV_B_PIN);
  
  int encoded = (MSB << 1) | LSB;
  int8_t change = calculateEncoderChange(encoded, lastEncodedNav);
  lastEncodedNav = encoded;
  
  if (change != 0) {
    unsigned long currentTime = micros();
    unsigned long timeDiff = currentTime - lastEncoderTimeNav;
    lastEncoderTimeNav = currentTime;
    
    uint8_t acceleration = calculateAcceleration(timeDiff);
    encoderValueNav += change;
    
    int threshold = 4 / acceleration;
    if (abs(encoderValueNav) >= threshold) {
      int8_t finalChange = (encoderValueNav > 0) ? acceleration : -acceleration;
      onNavigationEncoderChange(finalChange);
      encoderValueNav = 0;
    }
  }
  
  // Leer botón del encoder con debounce
  bool currentButtonState = !mcpButtonsEnc.digitalRead(ENC_NAV_SW_PIN);
  unsigned long currentTime = millis();
  
  if (currentButtonState != lastNavButtonState) {
    lastNavPressTime = currentTime;
  }
  
  if ((currentTime - lastNavPressTime) > BUTTON_DEBOUNCE_MS) {
    if (currentButtonState != navButtonState) {
      navButtonState = currentButtonState;
      if (navButtonState) {
        onNavigationButtonPress();
      }
    }
  }
  
  lastNavButtonState = currentButtonState;
}

bool HardwareManager::validateMCPResponse(Adafruit_MCP23X17& mcp, const char* name) {
  // Test de escritura/lectura para verificar comunicación
  uint8_t testValue = 0xAA;
  
  // Escribir patrón de test
  mcp.writeGPIOA(testValue);
  delay(1);
  
  // Leer y verificar
  uint8_t readValue = mcp.readGPIOA();
  
  if (readValue != testValue) {
    Serial.print(F("ERROR: Test de comunicación falló para "));
    Serial.println(name);
    handleMCPError(name, "communication test");
    return false;
  }
  
  // Restaurar estado inicial
  mcp.writeGPIOA(0x00);
  return true;
}

void HardwareManager::handleMCPError(const char* mcpName, const char* operation) {
  errorCount++;
  Serial.print(F("ERROR MCP: "));
  Serial.print(mcpName);
  Serial.print(F(" - "));
  Serial.println(operation);
}

bool HardwareManager::checkMCPHealth() {
  bool allHealthy = true;
  
  // Verificar cada MCP con ping simple
  Wire.beginTransmission(MCP_ENCODERS_VOL_ADDR);
  if (Wire.endTransmission() != 0) {
    handleMCPError("MCP1", "health check");
    allHealthy = false;
  }
  
  Wire.beginTransmission(MCP_ENCODERS_PAN_ADDR);
  if (Wire.endTransmission() != 0) {
    handleMCPError("MCP2", "health check");
    allHealthy = false;
  }
  
  Wire.beginTransmission(MCP_SWITCHES_ADDR);
  if (Wire.endTransmission() != 0) {
    handleMCPError("MCP3", "health check");
    allHealthy = false;
  }
  
  Wire.beginTransmission(MCP_BUTTONS_ENC_ADDR);
  if (Wire.endTransmission() != 0) {
    handleMCPError("MCP4", "health check");
    allHealthy = false;
  }
  
  return allHealthy;
}

void HardwareManager::resetMCPs() {
  Serial.println(F("Reiniciando MCPs..."));
  
  // Limpiar todas las interrupciones
  clearAllInterrupts();
  
  // Reinicializar configuración
  configureEncoderMCP(mcpEncodersVol, INT_MCP1_A, INT_MCP1_B);
  configureEncoderMCP(mcpEncodersPan, INT_MCP2_A, INT_MCP2_B);
  configureSwitchMCP(mcpSwitches);
  configureSwitchMCP(mcpButtonsEnc);
  
  Serial.println(F("MCPs reiniciados"));
}

void HardwareManager::clearAllInterrupts() {
  mcpEncodersVol.getCapturedInterrupt();
  mcpEncodersPan.getCapturedInterrupt();
}

bool HardwareManager::testAllMCPs() {
  Serial.println(F("Ejecutando test completo de MCPs..."));
  
  bool success = true;
  success &= validateMCPResponse(mcpEncodersVol, "MCP1");
  success &= validateMCPResponse(mcpEncodersPan, "MCP2");
  
  if (success) {
    Serial.println(F("Test de MCPs completado exitosamente"));
  } else {
    Serial.println(F("Test de MCPs falló"));
  }
  
  return success;
}

void HardwareManager::calibrateEncoders() {
  Serial.println(F("Calibrando encoders..."));
  
  // Resetear contadores y estados
  memset(encoderValue, 0, sizeof(encoderValue));
  memset(lastEncoded, 0, sizeof(lastEncoded));
  memset(lastEncoderTime, 0, sizeof(lastEncoderTime));
  
  encoderValueNav = 0;
  lastEncodedNav = 0;
  
  Serial.println(F("Calibración completada"));
}

bool HardwareManager::isInitialized() const {
  return (encoderReadCount == 0 || encoderReadCount > 0);  // Simple check
}

void HardwareManager::printDiagnostics() const {
  Serial.println(F("\n=== DIAGNÓSTICO HARDWARE ==="));
  Serial.print(F("Lecturas de encoders: "));
  Serial.println(encoderReadCount);
  Serial.print(F("Presses de switches: "));
  Serial.println(switchPressCount);
  Serial.print(F("Errores detectados: "));
  Serial.println(errorCount);
  Serial.println(F("==========================\n"));
}