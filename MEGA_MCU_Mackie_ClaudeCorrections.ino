/*
 * Controlador MIDI Mackie para Arduino Mega
 * Controlador completo con 16 encoders y pantalla ST7796
 * Versión modular optimizada con correcciones
 * 
 * Hardware:
 * - Arduino Mega 2560
 * - 4x MCP23017 I2C Expanders
 * - Pantalla ST7796 3.5" 320x480
 * - 16 Encoders rotativos
 * - 16 Switches (Mute/Solo)
 * - 5 Botones transporte
 * - 1 Encoder navegación con botón
 * - Tarjeta SD
 * 
 * Autor: Generado por Claude
 * Fecha: 2025
 */

#include "Config.h"
#include "EncoderManager.h"  // ¡Añadir esta línea!
#include "HardwareManager.h"
#include "DisplayManager.h"
#include "Midi_Controller.h"
#include "MenuManager.h"
#include "FileManager.h"

// Instancias globales de los managers
HardwareManager hardware;
DisplayManager display;
Midi_Controller midi_controller;
MenuManager menu;
EncoderManager encoders;
FileManager fileSystem;

// Variables globales del sistema
SystemState systemState;
AppConfig appConfig;
volatile InterruptFlags interruptFlags;

// Declaraciones de funciones auxiliares
void processInterrupts();
void processNavigationEncoder();
void updateDisplay(unsigned long currentTime);
void updateScreensaver(unsigned long currentTime);
void performDiagnostics();
void changeBankSafely(int8_t direction);

#ifdef __arm__
// should use uinstd.h to define sbrk but Due causes a conflict
extern "C" char* sbrk(int incr);
#else  // __ARM__
extern char *__brkval;
#endif  // __arm__

int freeMemory() {
  char top;
#ifdef __arm__
  return &top - reinterpret_cast<char*>(sbrk(0));
#elif defined(CORE_TEENSY) || (ARDUINO > 103 && ARDUINO != 151)
  return &top - __brkval;
#else  // __arm__
  return __brkval ? &top - __brkval : &top - __malloc_heap_start;
#endif  // __arm__
}

void setup() {
  Serial.begin(115200);
  Serial.println(F("Inicializando Controlador MIDI Mackie..."));
  
  // Inicializar sistema por orden de dependencias
  if (!fileSystem.initialize()) {
    Serial.println(F("ERROR: Falló inicialización SD"));
  }
  
  fileSystem.loadConfiguration(appConfig, encoders.getEncoderBanks());
  
  if (!hardware.initialize()) {
    Serial.println(F("ERROR CRÍTICO: Falló inicialización hardware"));
    while(1) { delay(1000); }
  }
  
  if (!display.initialize(appConfig.orientation, appConfig.brightness)) {
    Serial.println(F("ERROR: Falló inicialización pantalla"));
  }
  
  midi_controller.initialize(appConfig.midiChannel);
  menu.initialize(&appConfig, &systemState);
  encoders.initialize(&appConfig);
  
  // Configurar interrupciones
  hardware.setupInterrupts();
  
  // Estado inicial del sistema
  systemState.lastActivityTime = millis();
  systemState.screensaverActive = false;
  systemState.inMenu = false;
  systemState.currentBank = appConfig.currentBank;
  
  Serial.println(F("Sistema inicializado correctamente"));
  Serial.print(F("Memoria libre: "));
  Serial.println(freeMemory());
}

void loop() {
  unsigned long currentTime = millis();
  
  // Procesar interrupciones de encoders
  processInterrupts();
  
  // Polling de switches y botones (optimizado)
  if (currentTime - systemState.lastPollTime >= POLL_INTERVAL_MS) {
    hardware.pollSwitchesAndButtons();
    systemState.lastPollTime = currentTime;
  }
  
  // Procesar encoder de navegación
  processNavigationEncoder();
  
  // Procesar MIDI entrante
  midi_controller.processMidiInput();
  
  // Actualizar pantalla (con control de framrate)
  if (currentTime - systemState.lastDisplayUpdate >= DISPLAY_UPDATE_INTERVAL) {
    updateDisplay(currentTime);
    systemState.lastDisplayUpdate = currentTime;
  }
  
  // Gestión de salvapantallas
  updateScreensaver(currentTime);
  
  // Watchdog y diagnósticos cada segundo
  if (currentTime - systemState.lastDiagnostic >= 1000) {
    performDiagnostics();
    systemState.lastDiagnostic = currentTime;
  }
  
  // Pequeño delay para estabilidad
  delay(1);
}

void processInterrupts() {
  // Procesar interrupciones de encoders con debounce
  if (interruptFlags.mcp1A) {
    hardware.processMCP1AEncoders();
    interruptFlags.mcp1A = false;
  }
  
  if (interruptFlags.mcp1B) {
    hardware.processMCP1BEncoders();  
    interruptFlags.mcp1B = false;
  }
  
  if (interruptFlags.mcp2A) {
    hardware.processMCP2AEncoders();
    interruptFlags.mcp2A = false;
  }
  
  if (interruptFlags.mcp2B) {
    hardware.processMCP2BEncoders();
    interruptFlags.mcp2B = false;
  }
}

void processNavigationEncoder() {
  static unsigned long lastNavUpdate = 0;
  if (millis() - lastNavUpdate >= NAV_ENCODER_INTERVAL) {
    hardware.readNavigationEncoder();
    lastNavUpdate = millis();
  }
}

void updateDisplay(unsigned long currentTime) {
  if (systemState.screensaverActive) {
    display.drawScreensaver(midi_controller.getMtcData(), systemState.currentBank);
  } else if (systemState.inMenu) {
    menu.draw(display);
  } else {
    // Pantalla principal con meters y información
    display.drawMainScreen(
      encoders.getEncoderBanks()[systemState.currentBank],
      midi_controller.getMtcData(),
      systemState.currentBank,
      midi_controller.getTransportState()
    );
  }
}

void updateScreensaver(unsigned long currentTime) {
  if (appConfig.screensaverTimeout > 0) {
    if (!systemState.screensaverActive && 
        (currentTime - systemState.lastActivityTime) > appConfig.screensaverTimeout) {
      systemState.screensaverActive = true;
      display.setBrightness(0);
      Serial.println(F("Salvapantallas activado"));
    }
  }
}

void performDiagnostics() {
  // Verificar memoria disponible
  int freeRam = freeMemory();
  if (freeRam < 1000) {
    Serial.print(F("ADVERTENCIA: Poca memoria libre: "));
    Serial.println(freeRam);
  }
  
  // Verificar estado de MCPs
  if (!hardware.checkMCPHealth()) {
    Serial.println(F("ADVERTENCIA: Problema detectado en MCPs"));
  }
  
  // Verificar SD card
  if (!fileSystem.checkSDHealth()) {
    Serial.println(F("ADVERTENCIA: Problema con tarjeta SD"));
  }
}

// Callback para eventos del sistema
void onEncoderChange(uint8_t encoderIndex, int8_t change) {
  encoders.processEncoderChange(encoderIndex, change, systemState.currentBank);
  resetActivity();
}

void onSwitchPress(uint8_t switchIndex) {
  encoders.processSwitchPress(switchIndex, systemState.currentBank);
  resetActivity();
}

void onButtonPress(uint8_t buttonIndex) {
  switch (buttonIndex) {
    case BTN_PLAY: midi_controller.sendTransportCommand(MIDI_PLAY); break;
    case BTN_STOP: midi_controller.sendTransportCommand(MIDI_STOP); break;
    case BTN_REC: midi_controller.sendTransportCommand(MIDI_RECORD); break;
    case BTN_BANK_UP: changeBankSafely(1); break;
    case BTN_BANK_DOWN: changeBankSafely(-1); break;
  }
  resetActivity();
}

void onNavigationEncoderChange(int8_t change) {
  if (systemState.inMenu) {
    menu.navigate(change);
  } else {
    midi_controller.sendJogWheel(change);
  }
  resetActivity();
}

void onNavigationButtonPress() {
  if (systemState.inMenu) {
    menu.select();
  } else {
    systemState.inMenu = true;
    menu.enter();
  }
  resetActivity();
}

void onMenuExit() {
  systemState.inMenu = false;
  // Guardar configuración si hubo cambios
  fileSystem.saveConfiguration(appConfig, encoders.getEncoderBanks());
}

void resetActivity() {
  systemState.lastActivityTime = millis();
  if (systemState.screensaverActive) {
    systemState.screensaverActive = false;
    display.setBrightness(appConfig.brightness);
  }
}

void changeBankSafely(int8_t direction) {
  uint8_t newBank = (systemState.currentBank + direction + NUM_BANKS) % NUM_BANKS;
  if (newBank != systemState.currentBank) {
    systemState.currentBank = newBank;
    appConfig.currentBank = newBank;
    encoders.setCurrentBank(newBank);
    Serial.print(F("Banco cambiado a: "));
    Serial.println(newBank + 1);
  }
}

// ISRs - mantener lo más simple posible
void handleMCP1AInterrupt() { interruptFlags.mcp1A = true; }
void handleMCP1BInterrupt() { interruptFlags.mcp1B = true; }
void handleMCP2AInterrupt() { interruptFlags.mcp2A = true; }  
void handleMCP2BInterrupt() { interruptFlags.mcp2B = true; }

// Funciones de sincronización con DAW
void syncEncoderColorFromDAW(uint8_t track, uint8_t bank, uint16_t color) {
  encoders.syncFromDAW(track, bank, encoders.getEncoderDAWValue(track, bank), color);
}

void syncEncoderValueFromDAW(uint8_t track, uint8_t bank, uint8_t value) {
  encoders.setEncoderDAWValue(track, bank, value);
}

void updateVUMeterLevel(uint8_t track, uint8_t level) {
  display.setVULevel(track, level);
}

void syncEncoderNameFromDAW(uint8_t track, uint8_t bank, const char* name) {
  encoders.syncNameFromDAW(track, bank, name);
}