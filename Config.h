#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <stdint.h>

// ==================== VERSIÓN Y INFORMACIÓN ====================
#define FIRMWARE_VERSION_MAJOR  1
#define FIRMWARE_VERSION_MINOR  0
#define FIRMWARE_VERSION_PATCH  0
#define DEVICE_NAME            "Mackie MIDI Controller"
#define MANUFACTURER_ID         0x7B  // Studio One compatible

// ==================== CONFIGURACIÓN DE HARDWARE ====================
// Pines de la pantalla ST7796
#define TFT_CS          53
#define TFT_DC          49  
#define TFT_RST         47
#define TFT_BL          9   // PWM para backlight
#define SD_CS           45

// Pines de interrupción MCP23017
#define INT_MCP1_A      2   // INT0 - Encoders Volumen INTA
#define INT_MCP1_B      3   // INT1 - Encoders Volumen INTB  
#define INT_MCP2_A      18  // INT5 - Encoders Pan INTA
#define INT_MCP2_B      19  // INT4 - Encoders Pan INTB

// Direcciones I2C de los MCP23017
#define MCP_ENCODERS_VOL_ADDR   0x20  // MCP1 - Encoders Volumen
#define MCP_ENCODERS_PAN_ADDR   0x21  // MCP2 - Encoders Pan
#define MCP_SWITCHES_ADDR       0x22  // MCP3 - Switches Mute/Solo
#define MCP_BUTTONS_ENC_ADDR    0x23  // MCP4 - Botones + Nav Encoder

// Pines del encoder de navegación (en MCP4)
#define ENC_NAV_A_PIN   8
#define ENC_NAV_B_PIN   9
#define ENC_NAV_SW_PIN  10

// ==================== CONFIGURACIÓN DEL SISTEMA ====================
#define NUM_ENCODERS    16
#define NUM_BANKS       4
#define NUM_SWITCHES    16  // 8 Mute + 8 Solo
#define NUM_BUTTONS     5   // Transport buttons

// Intervalos de tiempo (ms)
#define POLL_INTERVAL_MS        5    // Polling switches/botones
#define DISPLAY_UPDATE_INTERVAL 50   // Actualización pantalla (20 FPS)
#define NAV_ENCODER_INTERVAL    2    // Encoder navegación (500 Hz)
#define ENCODER_DEBOUNCE_MS     1    // Debounce encoders
#define BUTTON_DEBOUNCE_MS      50   // Debounce botones
#define SCREENSAVER_CHECK_MS    1000 // Verificar salvapantallas

// ==================== CONFIGURACIÓN MIDI ====================
#define MIDI_CHANNEL_DEFAULT    1
#define MIDI_BAUD_RATE         31250
#define MTC_FRAME_RATE         30    // 30 FPS para MTC
#define ENCODER_ACCELERATION   true

// Comandos MIDI
#define MIDI_PLAY              0xFA
#define MIDI_STOP              0xFC  
#define MIDI_RECORD            0xFB
#define MIDI_JOG_FORWARD       60    // CC60
#define MIDI_JOG_BACKWARD      61    // CC61

// ==================== CONFIGURACIÓN DE PANTALLA ====================
#define TFT_WIDTH              480
#define TFT_HEIGHT             320
#define DEFAULT_BRIGHTNESS     100
#define MIN_BRIGHTNESS         0
#define MAX_BRIGHTNESS         100

// Reducir buffers enormes
#define SYSEX_BUFFER_SIZE 32    // Era 256
#define MAX_PRESET_NAME 10      // Era 16
#define MAX_FILENAME_LENGTH 16  // Era 32

// ==================== COLORES STUDIO ONE 7 (RGB565) ====================
#define NUM_COLORS             24

// ==================== COLOR DEFINITIONS ====================
#ifndef COLOR_BLACK
#define COLOR_BLACK     0x0000
#define COLOR_WHITE     0xFFFF
#define COLOR_RED       0xF800
#define COLOR_GREEN     0x07E0
#define COLOR_BLUE      0x001F
#define COLOR_CYAN      0x07FF
#define COLOR_MAGENTA   0xF81F
#define COLOR_YELLOW    0xFFE0
#define COLOR_ORANGE    0xFC00
#define COLOR_LIGHT_GRAY 0xC618
#define COLOR_DARK_GRAY  0x7BEF
#endif

// Paleta de colores Studio One 7 en formato RGB565
static const uint16_t STUDIO_ONE_COLORS[NUM_COLORS] PROGMEM = {
  0xF800, // Rojo brillante
  0xFD20, // Naranja  
  0xFFE0, // Amarillo
  0x87E0, // Verde lima
  0x07E0, // Verde
  0x07FF, // Cian
  0x051F, // Azul claro
  0x001F, // Azul
  0x401F, // Azul violeta
  0x801F, // Púrpura
  0xF81F, // Magenta
  0xF810, // Rojo oscuro
  0xFC00, // Naranja oscuro
  0xFD20, // Amarillo oscuro
  0xFFE0, // Verde lima oscuro
  0x87E0, // Verde oscuro
  0x07E0, // Cian oscuro
  0x07FF, // Azul claro oscuro
  0x051F, // Azul oscuro
  0x001F, // Violeta oscuro
  0x401F, // Púrpura oscuro
  0x801F, // Magenta oscuro
  0xF81F, // Gris claro
  0xF810  // Gris oscuro
};

// ==================== ENUMERACIONES ====================
enum ControlType {
  CT_CC = 0,
  CT_NOTE = 1,
  CT_PITCH = 2
};

enum ButtonIndex {
  BTN_PLAY = 0,
  BTN_STOP = 1,  
  BTN_REC = 2,
  BTN_BANK_UP = 3,
  BTN_BANK_DOWN = 4
};

enum DisplayOrientation {
  ORIENT_0 = 0,
  ORIENT_90 = 1,
  ORIENT_180 = 2,
  ORIENT_270 = 3
};

enum MenuValueType {
  MENU_ACTION = 0,
  MENU_INTEGER = 1,
  MENU_BOOLEAN = 2,
  MENU_OPTION = 3
};

enum class MenuType {
    MAIN_MENU,
    ENCODER_SETTINGS,
    BANK_MANAGEMENT,
    SYSTEM_SETTINGS,
    MIDI_SETTINGS,
    DISPLAY_SETTINGS,
    PRESET_MANAGEMENT,
    NONE
};

// ==================== ESTRUCTURAS DE DATOS ====================
struct EncoderConfig {
  uint8_t channel : 4;           // Canal MIDI (1-16)
  uint8_t control : 7;        // Número de CC/Note (0-127)
  //uint8_t cc;               // Número CC o Note
  uint8_t controlType : 2;   // Tipo de control (CC/Note/Pitch)
  bool isPan : 1;               // true si es encoder de pan
  int8_t value;            // Valor actual local (0-127)
  int8_t dawValue;         // Valor recibido del DAW (0-127)
  int8_t minValue;         // Valor mínimo (0-127)
  int8_t maxValue;         // Valor máximo (0-127)
  bool isMute : 1;              // Estado mute
  bool isSolo : 1;              // Estado solo
  uint16_t trackColor;      // Color del track
 // uint16_t panColor;        // Color del pan (puede ser diferente)
  char trackName[5];        // Nombre del track (8 chars + null terminator)
  
  // Constructor por defecto
  EncoderConfig() {
  channel = 1;
  control = 7;           // Reemplaza 'cc = 7'
  controlType = CT_CC;
  isPan = false;
  value = 64;
  dawValue = 64;
  minValue = 0;
  maxValue = 127;
  isMute = false;
  isSolo = false;
  trackColor = 0xFFFF;
  strncpy(trackName, "Trk", 5); // Nombre corto
  trackName[5] = '\0';
    //panColor = 0xFFFF;
    //strcpy(trackName, "Track");
  }
};

struct AppConfig {
  uint8_t brightness;                  // Brillo pantalla (0-100)
  uint32_t screensaverTimeout;        // Timeout en ms (0=deshabilitado)
  uint8_t currentBank;                // Banco actual (0-3)
  uint8_t mtcOffset;                  // Offset MTC en frames
  bool encoderAcceleration;           // Aceleración de encoders
  uint8_t midiChannel;                // Canal MIDI base (1-16)
  DisplayOrientation orientation;      // Orientación pantalla
  bool autoSave;                      // Auto-guardar configuración
  uint8_t encoderSensitivity;         // Sensibilidad encoders (1-10)
  uint16_t vuMeterDecay;              // Decaimiento VU meters (ms)
  // Constructor por defecto
  AppConfig() {
    brightness = DEFAULT_BRIGHTNESS;
    screensaverTimeout = 300000;  // 5 minutos
    currentBank = 0;
    mtcOffset = 0;
    encoderAcceleration = true;
    midiChannel = MIDI_CHANNEL_DEFAULT;
    orientation = ORIENT_270;
    autoSave = true;
    encoderSensitivity = 5;
    vuMeterDecay = 1000;
  }
};

struct MtcData {
  uint8_t hours;
  uint8_t minutes;
  uint8_t seconds;
  uint8_t frames;
  bool isRunning;
  
  MtcData() : hours(0), minutes(0), seconds(0), frames(0), isRunning(false) {}
};

struct TransportState {
  bool isPlaying;
  bool isRecording;
  bool isPaused;
  
  TransportState() : isPlaying(false), isRecording(false), isPaused(false) {}
};

struct SystemState {
  unsigned long lastActivityTime;
  unsigned long lastPollTime;
  unsigned long lastDisplayUpdate;
  unsigned long lastDiagnostic;
  bool screensaverActive;
  bool inMenu;
  uint8_t currentBank;
  uint16_t freeMemory;
  bool sdCardPresent;
  bool displayNeedsUpdate;
  bool menuActive;
  
  SystemState() {
    lastActivityTime = 0;
    lastPollTime = 0;
    lastDisplayUpdate = 0;
    lastDiagnostic = 0;
    screensaverActive = false;
    inMenu = false;
    currentBank = 0;
    freeMemory = 0;
    sdCardPresent = false;
    bool displayNeedsUpdate = false;
    bool menuActive = false;
    
  }
};

struct InterruptFlags {
  volatile bool mcp1A;
  volatile bool mcp1B;
  volatile bool mcp2A;
  volatile bool mcp2B;
  
  InterruptFlags() : mcp1A(false), mcp1B(false), mcp2A(false), mcp2B(false) {}
};
// Strings de interfaz en PROGMEM
const char str_loading[] PROGMEM = "Cargando...";
const char str_error[] PROGMEM = "Error";
const char str_success[] PROGMEM = "Exito";
const char str_warning[] PROGMEM = "Advertencia";
const char str_menu_main[] PROGMEM = "Menu Principal";
const char str_menu_encoders[] PROGMEM = "Encoders";
const char str_menu_display[] PROGMEM = "Pantalla";
const char str_menu_midi[] PROGMEM = "MIDI";
const char str_menu_global[] PROGMEM = "Global";

// Array de punteros para fácil acceso
const char* const interface_strings[] PROGMEM = {
    str_loading, str_error, str_success, str_warning,
    str_menu_main, str_menu_encoders, str_menu_display,
    str_menu_midi, str_menu_global
};

// Macros útiles
/*#define STR_LOADING 0
#define STR_ERROR 1
#define STR_SUCCESS 2
#define STR_WARNING 3
#define STR_MENU_MAIN 4
#define STR_MENU_ENCODERS 5
#define STR_MENU_DISPLAY 6
#define STR_MENU_MIDI 7
#define STR_MENU_GLOBAL 8*/
// Usa enum en su lugar:
enum ConfigStringIndex {
    CFG_STR_LOADING,
    CFG_STR_ERROR,
    CFG_STR_SUCCESS,
    CFG_STR_WARNING,
    CFG_STR_MENU_MAIN,
    CFG_STR_MENU_ENCODERS,
    CFG_STR_MENU_DISPLAY,
    CFG_STR_MENU_MIDI,
    CFG_STR_MENU_GLOBAL
};
// ==================== UTILIDADES ====================
// Macro para obtener color de la paleta
#define GET_STUDIO_COLOR(index) pgm_read_word(&STUDIO_ONE_COLORS[index % NUM_COLORS])

// Macro para conversión RGB to RGB565
#define RGB_TO_565(r, g, b) (((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3))

// Macro para constrain con verificación de tipo
template<typename T>
inline T constrainValue(T value, T minVal, T maxVal) {
  return (value < minVal) ? minVal : (value > maxVal) ? maxVal : value;
}

// Macro para mapeo seguro
template<typename T>
inline T mapValue(T value, T fromLow, T fromHigh, T toLow, T toHigh) {
  return toLow + (value - fromLow) * (toHigh - toLow) / (fromHigh - fromLow);
}

// ==================== VALIDACIÓN DE CONFIGURACIÓN ====================
#if NUM_ENCODERS > 16
#error "Máximo 16 encoders soportados"
#endif

#if NUM_BANKS > 8  
#error "Máximo 8 bancos soportados"
#endif

#if TFT_WIDTH != 480 || TFT_HEIGHT != 320
#error "Pantalla debe ser 480x320 para ST7796"
#endif

// ==================== DECLARACIONES EXTERNAS ====================
// Variables globales declaradas en el archivo principal
extern SystemState systemState;
extern AppConfig appConfig;
extern volatile InterruptFlags interruptFlags;

// Callbacks del sistema (implementados en el archivo principal)
extern void onEncoderChange(uint8_t encoderIndex, int8_t change);
extern void onSwitchPress(uint8_t switchIndex);
extern void onButtonPress(uint8_t buttonIndex);
extern void onNavigationEncoderChange(int8_t change);
extern void onNavigationButtonPress();
extern void onMenuExit();
extern void resetActivity();

// ISRs (implementados en el archivo principal)
extern void handleMCP1AInterrupt();
extern void handleMCP1BInterrupt(); 
extern void handleMCP2AInterrupt();
extern void handleMCP2BInterrupt();

#endif // CONFIG_H