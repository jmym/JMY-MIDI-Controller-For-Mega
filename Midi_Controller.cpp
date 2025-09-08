#include "Midi_Controller.h"

// Inicializar la instancia estática
Midi_Controller* Midi_Controller::instance = nullptr;

// Instancia MIDI global
MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI);

Midi_Controller::Midi_Controller()
  : currentMidiChannel(MIDI_CHANNEL_DEFAULT), mtcSync(true),
    sysExIndex(0), receivingSysEx(false), mtcQuarterFrame(0),
    lastMtcTime(0), mtcTimebaseValid(false),
    midiMessagesReceived(0), midiMessagesSent(0),
    sysExMessagesProcessed(0), mtcFramesReceived(0), errorCount(0),
    midiThruEnabled(false), sysExAutoResponse(true), lastActivityTime(0)
{
  instance = this;
}

Midi_Controller::~Midi_Controller() {
  instance = nullptr;
}

// AÑADE ESTA FUNCIÓN:
void Midi_Controller::setMidiChannel(uint8_t channel) {
  if (channel >= 1 && channel <= 16) {
    currentMidiChannel = channel;
    Serial.print(F("Canal MIDI cambiado a: "));
    Serial.println(channel);
  }
}

bool Midi_Controller::initialize(uint8_t midiChannel) {
  Serial.println(F("Inicializando controlador MIDI..."));
  
  currentMidiChannel = midiChannel;
  
  // Configurar MIDI
  MIDI.begin(MIDI_CHANNEL_OMNI);
  MIDI.turnThruOff();
  
  // Configurar callbacks
  MIDI.setHandleNoteOn(handleNoteOn);
  MIDI.setHandleNoteOff(handleNoteOff);
  MIDI.setHandleControlChange(handleControlChange);
  MIDI.setHandlePitchBend(handlePitchBend);
  MIDI.setHandleSystemExclusive(handleSystemExclusive);
  MIDI.setHandleTimeCodeQuarterFrame(handleTimeCodeQuarterFrame);
  MIDI.setHandleClock(handleClock);
  MIDI.setHandleStart(handleStart);
  MIDI.setHandleStop(handleStop);
  MIDI.setHandleContinue(handleContinue);
  
  Serial.println(F("Controlador MIDI inicializado"));
  return true;
}

// ... resto del código existente de Midi_Controller.cpp

void Midi_Controller::processMidiInput() {
  MIDI.read();
}

void Midi_Controller::sendControlChange(uint8_t channel, uint8_t cc, uint8_t value) {
  if (!isValidMidiChannel(channel) || !isValidControlNumber(cc)) return;
  
  MIDI.sendControlChange(cc, value, channel);
  midiMessagesSent++;
  lastActivityTime = millis();
}

void Midi_Controller::sendNoteOn(uint8_t channel, uint8_t note, uint8_t velocity) {
  if (!isValidMidiChannel(channel) || !isValidNoteNumber(note)) return;
  
  MIDI.sendNoteOn(note, velocity, channel);
  midiMessagesSent++;
  lastActivityTime = millis();
}

void Midi_Controller::sendNoteOff(uint8_t channel, uint8_t note, uint8_t velocity) {
  if (!isValidMidiChannel(channel) || !isValidNoteNumber(note)) return;
  
  MIDI.sendNoteOff(note, velocity, channel);
  midiMessagesSent++;
  lastActivityTime = millis();
}

void Midi_Controller::sendPitchBend(uint8_t channel, int16_t value) {
  if (!isValidMidiChannel(channel)) return;
  
  MIDI.sendPitchBend(value, channel);
  midiMessagesSent++;
  lastActivityTime = millis();
}

void Midi_Controller::sendTransportCommand(uint8_t command) {
  MIDI.sendRealTime(command);
  midiMessagesSent++;
  lastActivityTime = millis();
}

void Midi_Controller::sendJogWheel(int8_t direction) {
  uint8_t cc = (direction > 0) ? MIDI_JOG_FORWARD : MIDI_JOG_BACKWARD;
  sendControlChange(currentMidiChannel, cc, abs(direction));
}

void Midi_Controller::sendAllNotesOff(uint8_t channel) {
  if (!isValidMidiChannel(channel)) return;
  
  MIDI.sendControlChange(123, 0, channel); // All Notes Off
  midiMessagesSent++;
  lastActivityTime = millis();
}

void Midi_Controller::sendStudioOneColorRequest(uint8_t track, uint8_t bank) {
  uint8_t sysexData[] = {
    0xF0, 0x00, 0x21, 0x7B,  // Header Studio One
    0x01,                     // Color Request
    track, bank,              // Track and bank
    0xF7                      // End of SysEx
  };
  sendCustomSysEx(sysexData, sizeof(sysexData));
}

void Midi_Controller::sendStudioOneValueRequest(uint8_t track, uint8_t bank) {
  uint8_t sysexData[] = {
    0xF0, 0x00, 0x21, 0x7B,  // Header Studio One
    0x02,                     // Value Request
    track, bank,              // Track and bank
    0xF7                      // End of SysEx
  };
  sendCustomSysEx(sysexData, sizeof(sysexData));
}

void Midi_Controller::sendCustomSysEx(const uint8_t* data, uint16_t length) {
  if (!data || length == 0) return;
  
  MIDI.sendSysEx(length, data, true);
  midiMessagesSent++;
  lastActivityTime = millis();
}

// Callbacks estáticos
void Midi_Controller::handleNoteOn(uint8_t channel, uint8_t note, uint8_t velocity) {
  if (instance) {
    instance->midiMessagesReceived++;
    instance->lastActivityTime = millis();
    // Procesar Note On según sea necesario
  }
}

void Midi_Controller::handleNoteOff(uint8_t channel, uint8_t note, uint8_t velocity) {
  if (instance) {
    instance->midiMessagesReceived++;
    instance->lastActivityTime = millis();
    // Procesar Note Off según sea necesario
  }
}

void Midi_Controller::handleControlChange(uint8_t channel, uint8_t number, uint8_t value) {
  if (instance) {
    instance->midiMessagesReceived++;
    instance->lastActivityTime = millis();
    // Procesar Control Change según sea necesario
  }
}

void Midi_Controller::handlePitchBend(uint8_t channel, int bend) {
  if (instance) {
    instance->midiMessagesReceived++;
    instance->lastActivityTime = millis();
    // Procesar Pitch Bend según sea necesario
  }
}

void Midi_Controller::handleSystemExclusive(uint8_t* data, unsigned size) {
  if (instance) {
    instance->midiMessagesReceived++;
    instance->sysExMessagesProcessed++;
    instance->lastActivityTime = millis();
    instance->processSysExMessage(data, size);
  }
}

void Midi_Controller::handleTimeCodeQuarterFrame(uint8_t data) {
  if (instance) {
    instance->midiMessagesReceived++;
    instance->mtcFramesReceived++;
    instance->lastActivityTime = millis();
    instance->updateMtcFromQuarterFrame(data);
  }
}

void Midi_Controller::handleClock() {
  if (instance) {
    instance->midiMessagesReceived++;
    instance->lastActivityTime = millis();
  }
}

void Midi_Controller::handleStart() {
  if (instance) {
    instance->midiMessagesReceived++;
    instance->currentTransport.isPlaying = true;
    instance->currentTransport.isPaused = false;
    instance->lastActivityTime = millis();
  }
}

void Midi_Controller::handleStop() {
  if (instance) {
    instance->midiMessagesReceived++;
    instance->currentTransport.isPlaying = false;
    instance->currentTransport.isPaused = false;
    instance->lastActivityTime = millis();
  }
}

void Midi_Controller::handleContinue() {
  if (instance) {
    instance->midiMessagesReceived++;
    instance->currentTransport.isPlaying = true;
    instance->currentTransport.isPaused = false;
    instance->lastActivityTime = millis();
  }
}

void Midi_Controller::processSysExMessage(const uint8_t* data, uint16_t length) {
  // Verificar header de Studio One
  if (length >= 5 && data[0] == 0xF0 && data[1] == 0x00 && 
      data[2] == 0x21 && data[3] == 0x7B) {
    processStudioOneMessage(data, length);
  }
}

void Midi_Controller::processStudioOneMessage(const uint8_t* data, uint16_t length) {
  if (length < 6) return; // Mínimo: header + command + track + bank
  
  uint8_t command = data[4];
  uint8_t track = data[5];
  uint8_t bank = (length > 6) ? data[6] : 0;
  
  switch (command) {
    case SYSEX_COLOR_UPDATE:
      if (length >= 10) { // header + command + track + bank + 3 bytes color + end
        processColorUpdate(track, bank, data + 7);
      }
      break;
    case SYSEX_VALUE_UPDATE:
      if (length >= 8) { // header + command + track + bank + value + end
        processValueUpdate(track, bank, data[7]);
      }
      break;
    case SYSEX_VU_UPDATE:
      if (length >= 8) { // header + command + track + level + end
        processVUUpdate(track, data[6]);
      }
      break;
    case SYSEX_NAME_UPDATE:
      if (length > 8) { // header + command + track + bank + name + end
        processNameUpdate(track, bank, (const char*)(data + 7));
      }
      break;
    case SYSEX_TRANSPORT:
      if (length >= 7) { // header + command + state + end
        uint8_t state = data[5];
        currentTransport.isPlaying = (state & 0x01);
        currentTransport.isRecording = (state & 0x02);
        currentTransport.isPaused = (state & 0x04);
      }
      break;
  }
}

void Midi_Controller::processColorUpdate(uint8_t track, uint8_t bank, const uint8_t* colorData) {
  // Convertir RGB24 a RGB565
  uint32_t rgb24 = (colorData[0] << 16) | (colorData[1] << 8) | colorData[2];
  uint16_t rgb565 = rgb24ToRgb565(rgb24);
  
  // Actualizar encoder correspondiente
  extern void syncEncoderColorFromDAW(uint8_t track, uint8_t bank, uint16_t color);
  syncEncoderColorFromDAW(track, bank, rgb565);
}

void Midi_Controller::processValueUpdate(uint8_t track, uint8_t bank, uint8_t value) {
  // Actualizar valor del encoder
  extern void syncEncoderValueFromDAW(uint8_t track, uint8_t bank, uint8_t value);
  syncEncoderValueFromDAW(track, bank, value);
}

void Midi_Controller::processVUUpdate(uint8_t track, uint8_t level) {
  // Actualizar VU meter
  extern void updateVUMeterLevel(uint8_t track, uint8_t level);
  updateVUMeterLevel(track, level);
}

void Midi_Controller::processNameUpdate(uint8_t track, uint8_t bank, const char* name) {
  // Actualizar nombre del track
  extern void syncEncoderNameFromDAW(uint8_t track, uint8_t bank, const char* name);
  syncEncoderNameFromDAW(track, bank, name);
}

void Midi_Controller::updateMtcFromQuarterFrame(uint8_t data) {
  uint8_t pieceType = (data >> 4) & 0x07;
  uint8_t pieceValue = data & 0x0F;
  
  switch (pieceType) {
    case 0: currentMtc.frames = (currentMtc.frames & 0xF0) | pieceValue; break;
    case 1: currentMtc.frames = (currentMtc.frames & 0x0F) | (pieceValue << 4); break;
    case 2: currentMtc.seconds = (currentMtc.seconds & 0xF0) | pieceValue; break;
    case 3: currentMtc.seconds = (currentMtc.seconds & 0x0F) | (pieceValue << 4); break;
    case 4: currentMtc.minutes = (currentMtc.minutes & 0xF0) | pieceValue; break;
    case 5: currentMtc.minutes = (currentMtc.minutes & 0x0F) | (pieceValue << 4); break;
    case 6: currentMtc.hours = (currentMtc.hours & 0xF0) | pieceValue; break;
    case 7: 
      currentMtc.hours = (currentMtc.hours & 0x0F) | (pieceValue << 4);
      currentMtc.isRunning = (pieceValue & 0x06) != 0; // Bit 1-2: running state
      break;
  }
  
  mtcQuarterFrame = pieceType;
  lastMtcTime = millis();
  
  // Reconstruir tiempo completo cada frame completo
  if (pieceType == 7) {
    reconstructMtcTime();
  }
}

void Midi_Controller::reconstructMtcTime() {
  // Validar datos MTC
  if (validateMtcData()) {
    mtcTimebaseValid = true;
  } else {
    mtcTimebaseValid = false;
    errorCount++;
  }
}

bool Midi_Controller::validateMtcData() {
  return (currentMtc.frames < 30 && 
          currentMtc.seconds < 60 && 
          currentMtc.minutes < 60 && 
          currentMtc.hours < 24);
}

uint16_t Midi_Controller::rgb24ToRgb565(uint32_t rgb24) {
  uint8_t r = (rgb24 >> 16) & 0xFF;
  uint8_t g = (rgb24 >> 8) & 0xFF;
  uint8_t b = rgb24 & 0xFF;
  
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

uint32_t Midi_Controller::rgb565ToRgb24(uint16_t rgb565) {
  uint8_t r = (rgb565 >> 8) & 0xF8;
  uint8_t g = (rgb565 >> 3) & 0xFC;
  uint8_t b = (rgb565 << 3) & 0xF8;
  
  return (r << 16) | (g << 8) | b;
}

void Midi_Controller::resetMtcTimebase() {
  memset(&currentMtc, 0, sizeof(currentMtc));
  mtcQuarterFrame = 0;
  mtcTimebaseValid = false;
}

void Midi_Controller::printMidiStatistics() const {
  Serial.println(F("\n=== ESTADÍSTICAS MIDI ==="));
  Serial.print(F("Mensajes recibidos: "));
  Serial.println(midiMessagesReceived);
  Serial.print(F("Mensajes enviados: "));
  Serial.println(midiMessagesSent);
  Serial.print(F("Mensajes SysEx: "));
  Serial.println(sysExMessagesProcessed);
  Serial.print(F("Frames MTC: "));
  Serial.println(mtcFramesReceived);
  Serial.print(F("Errores: "));
  Serial.println(errorCount);
  Serial.println(F("========================\n"));
}

void Midi_Controller::resetStatistics() {
  midiMessagesReceived = 0;
  midiMessagesSent = 0;
  sysExMessagesProcessed = 0;
  mtcFramesReceived = 0;
  errorCount = 0;
}

bool Midi_Controller::testMidiConnection() {
  Serial.println(F("Probando conexión MIDI..."));
  
  // Enviar mensaje de test
  sendControlChange(currentMidiChannel, 7, 64);
  delay(100);
  sendControlChange(currentMidiChannel, 7, 127);
  delay(100);
  sendControlChange(currentMidiChannel, 7, 64);
  
  Serial.println(F("Test MIDI completado"));
  return true;
}

void Midi_Controller::sendTestSequence() {
  // Secuencia de test más completa
  for (int i = 0; i < 8; i++) {
    sendControlChange(currentMidiChannel, 7 + i, 0);
    delay(50);
    sendControlChange(currentMidiChannel, 7 + i, 127);
    delay(50);
    sendControlChange(currentMidiChannel, 7 + i, 64);
    delay(50);
  }
}

void Midi_Controller::setMidiThru(bool enable) {
  midiThruEnabled = enable;
  if (enable) {
    MIDI.turnThruOn();
  } else {
    MIDI.turnThruOff();
  }
}

void Midi_Controller::setSysExAutoResponse(bool enable) {
  sysExAutoResponse = enable;
}

bool Midi_Controller::isValidMidiChannel(uint8_t channel) const {
  return (channel >= 1 && channel <= 16);
}

bool Midi_Controller::isValidControlNumber(uint8_t cc) const {
  return (cc <= 127);
}

bool Midi_Controller::isValidNoteNumber(uint8_t note) const {
  return (note <= 127);
}

void Midi_Controller::logMidiError(const char* error) {
  errorCount++;
  Serial.print(F("ERROR MIDI: "));
  Serial.println(error);
}