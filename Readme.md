\# Controlador MIDI Mackie para Arduino Mega 2560



Controlador MIDI profesional con 16 encoders, pantalla TFT de 3.5" y compatibilidad completa con Studio One 7.



\## Características Principales



\### Hardware

\- \*\*Arduino Mega 2560\*\* (requerido por cantidad de pines e interrupciones)

\- \*\*4x MCP23017\*\* - Expansores I2C para 16 encoders + 16 switches + botones

\- \*\*Pantalla ST7796 3.5"\*\* - 480x320 píxeles con control táctil por encoder

\- \*\*16 Encoders rotativos\*\* con pulsador integrado

\- \*\*16 Switches\*\* - 8 Mute + 8 Solo

\- \*\*5 Botones transporte\*\* - Play, Stop, Record, Bank+, Bank-

\- \*\*1 Encoder navegación\*\* con botón para menús

\- \*\*Tarjeta SD\*\* para configuración y presets



\### Software

\- \*\*4 Bancos\*\* de 16 encoders (64 controles totales)

\- \*\*Sincronización bidireccional\*\* con Studio One 7

\- \*\*Sistema de colores\*\* con paleta Studio One 7 completa

\- \*\*VU meters\*\* con decaimiento realista

\- \*\*Sistema de menús\*\* completo navegable

\- \*\*Gestión de presets\*\* con backup automático

\- \*\*MTC sync\*\* (MIDI Time Code)

\- \*\*Modo salvapantallas\*\* configurable



\## Arquitectura del Código



\### Estructura Modular

```

MackieMIDIController/

├── MackieMIDIController.ino    # Archivo principal

├── Config.h                    # Configuración global

├── HardwareManager.h/.cpp      # Gestión MCP23017

├── DisplayManager.h/.cpp       # Control pantalla ST7796

├── MidiManager.h/.cpp          # Comunicación MIDI

├── EncoderManager.h/.cpp       # Lógica de encoders

├── MenuManager.h/.cpp          # Sistema de menús

└── FileManager.h/.cpp          # Gestión SD y archivos

```



\### Managers Implementados



\#### HardwareManager

\- Control de 4 MCP23017 con interrupciones optimizadas

\- Lectura de encoders con aceleración adaptativa

\- Polling eficiente para switches y botones

\- Sistema de diagnóstico y recuperación ante errores



\#### DisplayManager  

\- Control completo de ST7796 con orientación configurable

\- VU meters con suavizado y colores por nivel

\- Sistema de menús con scroll automático

\- Funciones de test y benchmark de rendimiento



\#### MidiManager

\- Comunicación MIDI completa con validación

\- Procesamiento SysEx para Studio One 7

\- MTC sync con reconstrucción de timestamp

\- Estadísticas y diagnóstico de tráfico MIDI



\#### EncoderManager

\- Gestión de 4 bancos x 16 encoders

\- Smoothing y aceleración configurable

\- Estados Mute/Solo con envío automático de CCs

\- Sistema de presets y configuración per-encoder



\#### MenuManager

\- Sistema de menús jerárquico completo

\- Edición de valores en tiempo real

\- Diálogos de confirmación y mensajes modales

\- Navegación intuitiva con encoder dedicado



\#### FileManager

\- Gestión robusta de tarjeta SD con recuperación ante errores

\- Sistema de presets con backup automático

\- Validación de integridad de archivos

\- Logging del sistema opcional



\## Instalación y Configuración



\### Librerías Requeridas

```cpp

// Instalar desde Library Manager de Arduino IDE

\#include <Adafruit\_ST7796.h>    // Pantalla TFT

\#include <Adafruit\_MCP23X17.h>  // Expansores I2C

\#include <MIDI.h>               // Comunicación MIDI

\#include <SD.h>                 // Tarjeta SD

\#include <SPI.h>                // Comunicación SPI

\#include <Wire.h>               // Comunicación I2C

```



\### Conexiones Hardware



\#### Pantalla ST7796

```

ST7796    Arduino Mega

CS   ->   Pin 53

DC   ->   Pin 49  

RST  ->   Pin 47

BL   ->   Pin 9 (PWM)

SCK  ->   Pin 52

MOSI ->   Pin 51

```



\#### MCP23017 (I2C)

```

MCP1 (0x20) - Encoders Volumen (0-7)

MCP2 (0x21) - Encoders Pan (8-15)  

MCP3 (0x22) - Switches Mute/Solo

MCP4 (0x23) - Botones + Encoder Nav



INT Pins:

MCP1 INTA -> Pin 2  (INT0)

MCP1 INTB -> Pin 3  (INT1)

MCP2 INTA -> Pin 18 (INT5)

MCP2 INTB -> Pin 19 (INT4)

```



\#### MIDI

```

MIDI Out -> Serial1 (Pin 18/19)

Baud Rate: 31250

```



\#### SD Card

```

CS -> Pin 45

Resto usar SPI estándar

```



\## Configuración por Defecto



\### Encoders

\- \*\*Encoders 1-8\*\*: Volume (CC 7-14, Canal 1)

\- \*\*Encoders 9-16\*\*: Pan (CC 10-17, Canal 1)

\- \*\*Switches 1-8\*\*: Mute (CC 23-30)

\- \*\*Switches 9-16\*\*: Solo (CC 31-38)



\### Pantalla

\- \*\*Resolución\*\*: 480x320

\- \*\*Orientación\*\*: 270° (landscape)

\- \*\*Brillo\*\*: 100%

\- \*\*Timeout\*\*: 5 minutos



\### MIDI

\- \*\*Canal base\*\*: 1

\- \*\*MTC\*\*: Habilitado

\- \*\*Aceleración encoders\*\*: Habilitada



\## Uso del Sistema



\### Operación Básica

1\. \*\*Encoders\*\*: Giro para cambiar valores, colores indican estado

2\. \*\*Switches\*\*: Press para Mute/Solo, LED visual en pantalla

3\. \*\*Botones transporte\*\*: Play/Stop/Record/Bank+/Bank-

4\. \*\*Encoder navegación\*\*: Press para entrar a menú, giro para navegar



\### Sistema de Menús

```

Menú Principal

├── Configurar Encoders

│   ├── Seleccionar Encoder

│   ├── Modo Vol/Pan

│   ├── Tipo Control (CC/Note/Pitch)

│   ├── Canal MIDI

│   └── Ajustar Rango

├── Configurar Pantalla  

│   ├── Brillo

│   ├── Timeout

│   └── Orientación

├── Configurar MIDI

│   ├── Canal MIDI

│   ├── Aceleración Encoders

│   └── Offset MTC

└── Ajustes Globales

&nbsp;   ├── Guardar/Cargar Config

&nbsp;   ├── Presets

&nbsp;   └── Reset Sistema

```



\### Gestión de Bancos

\- \*\*4 bancos\*\* disponibles (A, B, C, D)

\- \*\*Cambio\*\*: Botones Bank+ / Bank-

\- \*\*Cada banco\*\*: Configuración independiente de todos los encoders

\- \*\*Presets\*\*: Guardar/cargar configuraciones completas



\## Integración Studio One 7



\### SysEx Messages

El controlador envía y recibe mensajes SysEx específicos:

```cpp

// Header Studio One

0xF0 0x00 0x21 0x7B \[command] \[track] \[bank] \[data...] 0xF7



// Comandos:

0x01 - Color Update

0x02 - Value Update  

0x03 - Name Update

0x04 - VU Update

0x05 - Transport

```



\### Sincronización Automática

\- \*\*Valores\*\*: El DAW envía valores actuales al controlador

\- \*\*Colores\*\*: Colores de track se sincronizan automáticamente

\- \*\*VU Meters\*\*: Niveles de audio en tiempo real

\- \*\*Transport\*\*: Estado Play/Stop/Record sincronizado



\## Personalización Avanzada



\### Modificar Configuración

Editar `Config.h` para cambiar:

\- Pines de conexión

\- Direcciones I2C

\- Colores de la paleta

\- Timeouts y intervalos

\- Número de encoders/bancos



\### Agregar Funcionalidad

La arquitectura modular permite:

\- Nuevos tipos de control MIDI

\- Pantallas diferentes

\- Más encoders/botones

\- Protocolos MIDI adicionales



\## Solución de Problemas



\### Problemas Comunes



\#### Pantalla no funciona

1\. Verificar conexiones SPI

2\. Comprobar voltaje (3.3V/5V según modelo)

3\. Test con `display.runDisplayTest()`



\#### MCP23017 no responde  

1\. Verificar direcciones I2C (0x20-0x23)

2\. Comprobar conexiones SDA/SCL

3\. Usar `hardware.checkMCPHealth()`



\#### MIDI no envía

1\. Verificar baudrate 31250

2\. Comprobar conexión Serial1

3\. Test con `midi.sendTestSequence()`



\#### SD no lee

1\. Formatear FAT32

2\. Verificar conexión SPI

3\. Usar `fileSystem.checkSDHealth()`



\### Diagnósticos del Sistema

Usar Serial Monitor (115200 baud) para:

```cpp

// Ver estadísticas

hardware.printDiagnostics();

midi.printMidiStatistics();

fileSystem.printSystemInfo();

```



\## Performance y Optimizaciones



\### Uso de Memoria

\- \*\*SRAM\*\*: ~6KB de ~8KB disponibles

\- \*\*Flash\*\*: ~180KB de ~248KB disponibles

\- \*\*Optimizaciones\*\*: PROGMEM para constantes, buffers optimizados



\### Velocidad de Respuesta

\- \*\*Encoders\*\*: <1ms latencia con interrupciones

\- \*\*Pantalla\*\*: 20 FPS (50ms refresh)

\- \*\*MIDI\*\*: Procesamiento en tiempo real

\- \*\*SD\*\*: Escritura asíncrona cuando es posible



\### Consideraciones de Consumo

\- \*\*Modo normal\*\*: ~200mA @ 5V

\- \*\*Salvapantallas\*\*: ~150mA @ 5V

\- \*\*Standby\*\*: ~100mA @ 5V



\## Expansión Future



\### Características Planeadas

\- \[ ] Control táctil directo en pantalla

\- \[ ] WiFi/Bluetooth para control remoto

\- \[ ] Más bancos de memoria (8-16)

\- \[ ] Macros programables

\- \[ ] Múltiples páginas de VU meters

\- \[ ] Integración con otros DAWs



\### Hardware Adicional Soportado

\- Faders motorizados (con driver adicional)

\- Más botones/encoders (hasta límites del Mega)

\- Pantalla más grande (compatible con bibliotecas GFX)

\- LED strips para indicación visual



\## Licencia y Créditos



Este proyecto es de código abierto. Desarrollado para la comunidad de controladores MIDI DIY.



\### Agradecimientos

\- Comunidad Arduino

\- Adafruit por las excelentes bibliotecas

\- PreSonus por la documentación de Studio One

\- Mackie por el protocolo de referencia



---

\*\*Versión\*\*: 1.0.0  

\*\*Última actualización\*\*: Septiembre 2025  

\*\*Compatibilidad\*\*: Arduino IDE 1.8.19+, Studio One 5.0+

