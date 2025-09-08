#ifndef MIDI_CONTROLLER_H
#define MIDI_CONTROLLER_H

#include "Config.h"
#include <MIDI.h>
#include <Arduino.h>

// Definiciones específicas MIDI
#define SYSEX_BUFFER_SIZE     128
#define MTC_QUARTER_FRAME     0xF1
#define MTC_FRAMES_PER_SEC    30
#define STUDIO_ONE_DEVICE_ID  0x7B

// Tipos de mensajes SysEx Studio One
#define SYSEX_COLOR_UPDATE    0x01
#define SYSEX_VALUE_UPDATE    0x02
#define SYSEX_NAME_UPDATE     0x03
#define SYSEX_VU_UPDATE       0x04
#define SYSEX_TRANSPORT       0x05

class Midi_Controller {
private:
  // Estados MIDI
  MtcData currentMtc;
  TransportState currentTransport;
  uint8_t currentMidiChannel;
  bool mtcSync;
  
  // Buffer para SysEx
  uint8_t sysExBuffer[32];//SYSEX_BUFFER_SIZE
  uint16_t sysExIndex;
  bool receivingSysEx;
  
  // Variables MTC
  uint8_t mtcQuarterFrame;
  uint32_t lastMtcTime;
  bool mtcTimebaseValid;
  
  // Estadísticas y diagnóstico
  uint32_t midiMessagesReceived;
  uint32_t midiMessagesSent;
  uint32_t sysExMessagesProcessed;
  uint32_t mtcFramesReceived;
  uint32_t errorCount;
  
  // Callbacks privados de MIDI
  static void handleNoteOn(uint8_t channel, uint8_t note, uint8_t velocity);
  static void handleNoteOff(uint8_t channel, uint8_t note, uint8_t velocity);
  static void handleControlChange(uint8_t channel, uint8_t number, uint8_t value);
  static void handlePitchBend(uint8_t channel, int bend);
  static void handleSystemExclusive(uint8_t* data, unsigned size);
  static void handleTimeCodeQuarterFrame(uint8_t data);
  static void handleClock();
  static void handleStart();
  static void handleStop();
  static void handleContinue();
  
  // Procesamiento interno
  void processSysExMessage(const uint8_t* data, uint16_t length);
  void processStudioOneMessage(const uint8_t* data, uint16_t length);
  void processColorUpdate(uint8_t track, uint8_t bank, const uint8_t* colorData);
  void processValueUpdate(uint8_t track, uint8_t bank, uint8_t value);
  void processVUUpdate(uint8_t track, uint8_t level);
  void processNameUpdate(uint8_t track, uint8_t bank, const char* name);
  
  // Procesamiento MTC
  void updateMtcFromQuarterFrame(uint8_t data);
  void reconstructMtcTime();
  bool validateMtcData();
  
  // Utilidades
  uint16_t rgb24ToRgb565(uint32_t rgb24);
  uint32_t rgb565ToRgb24(uint16_t rgb565);
  void logMidiError(const char* error);

public:
  Midi_Controller();
  ~Midi_Controller();
  
  // Inicialización
  bool initialize(uint8_t midiChannel = MIDI_CHANNEL_DEFAULT);
  void setMidiChannel(uint8_t channel);  // AÑADIDA ESTA LÍNEA
  uint8_t getMidiChannel() const { return currentMidiChannel; }
  
  // Procesamiento principal
  void processMidiInput();
  
  // Envío de mensajes MIDI
  void sendControlChange(uint8_t channel, uint8_t cc, uint8_t value);
  void sendNoteOn(uint8_t channel, uint8_t note, uint8_t velocity);
  void sendNoteOff(uint8_t channel, uint8_t note, uint8_t velocity);
  void sendPitchBend(uint8_t channel, int16_t value);
  void sendTransportCommand(uint8_t command);
  void sendJogWheel(int8_t direction);
  void sendAllNotesOff(uint8_t channel);
  
  // Envío de SysEx personalizado
  void sendStudioOneColorRequest(uint8_t track, uint8_t bank);
  void sendStudioOneValueRequest(uint8_t track, uint8_t bank);
  void sendCustomSysEx(const uint8_t* data, uint16_t length);
  
  // Getters de estado
  const MtcData& getMtcData() const { return currentMtc; }
  const TransportState& getTransportState() const { return currentTransport; }
  bool isMtcSynced() const { return mtcSync && mtcTimebaseValid; }
  
  // Control MTC
  void enableMtcSync(bool enable) { mtcSync = enable; }
  bool isMtcSyncEnabled() const { return mtcSync; }
  void resetMtcTimebase();
  
  // Diagnóstico y estadísticas
  void printMidiStatistics() const;
  void resetStatistics();
  uint32_t getMessagesReceived() const { return midiMessagesReceived; }
  uint32_t getMessagesSent() const { return midiMessagesSent; }
  uint32_t getErrorCount() const { return errorCount; }
  
  // Test y calibración
  bool testMidiConnection();
  void sendTestSequence();
  
  // Configuración avanzada
  void setMidiThru(bool enable);
  void setSysExAutoResponse(bool enable);

private:
  // Referencia estática para callbacks
  static Midi_Controller* instance;
  
  // Configuración interna
  bool midiThruEnabled;
  bool sysExAutoResponse;
  unsigned long lastActivityTime;
  
  // Validación de datos
  bool isValidMidiChannel(uint8_t channel) const;
  bool isValidControlNumber(uint8_t cc) const;
  bool isValidNoteNumber(uint8_t note) const;
};

// Instancia global MIDI
extern midi::MidiInterface<midi::SerialMIDI<HardwareSerial>> MIDI;

#endif // MIDI_CONTROLLER_H