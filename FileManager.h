#ifndef FILE_MANAGER_H
#define FILE_MANAGER_H

#include "Config.h"
#include <SD.h>
#include <SPI.h>

// Configuración del sistema de archivos
#define CONFIG_FILENAME        "config.cfg"
#define ENCODERS_FILENAME      "encoders.dat"
#define BACKUP_EXTENSION       ".bak"
#define PRESET_DIRECTORY       "/presets"
#define LOG_DIRECTORY          "/logs"
#define TEMP_DIRECTORY         "/temp"

#define MAX_FILENAME_LENGTH    16//32
#define MAX_PRESET_NAME        16
#define CONFIG_VERSION         1
#define MAX_BACKUP_FILES       5
#define SD_RETRY_COUNT         3

// Estructura de header para archivos de configuración
struct ConfigFileHeader {
  uint32_t magic;           // Magic number para validación
  uint16_t version;         // Versión del formato
  uint16_t checksum;        // Checksum de los datos
  uint32_t dataSize;        // Tamaño de los datos
  uint32_t timestamp;       // Timestamp de creación
  char description[32];     // Descripción del archivo
  
  ConfigFileHeader() : magic(0x4D434B52), version(CONFIG_VERSION), 
                      checksum(0), dataSize(0), timestamp(0) {
    strcpy(description, "Mackie MIDI Config");
  }
};

// Información de archivo
struct FileInfo {
  char name[MAX_FILENAME_LENGTH];
  uint32_t size;
  uint32_t timestamp;
  bool isDirectory;
  
  FileInfo() : size(0), timestamp(0), isDirectory(false) {
    name[0] = '\0';
  }
};

class FileManager {
private:
  bool sdInitialized;
  bool sdCardPresent;
  uint32_t totalSpace;
  uint32_t freeSpace;
  uint32_t filesWritten;
  uint32_t filesRead;
  uint32_t errorCount;
  
  // Buffers para operaciones
  uint8_t workBuffer[64];//512
  char pathBuffer[32];//64
  
  // Métodos privados de utilidad
  bool initializeDirectories();
  bool validateSDCard();
  uint16_t calculateChecksum(const void* data, size_t size);
  bool writeFileHeader(File& file, const ConfigFileHeader& header);
  bool readFileHeader(File& file, ConfigFileHeader& header);
  bool verifyFileIntegrity(const char* filename);
  
  // Gestión de backups
  void createBackup(const char* filename);
  void cleanupOldBackups(const char* baseFilename);
  bool restoreFromBackup(const char* filename);
  
  // Operaciones de archivo seguras
  bool safeWrite(const char* filename, const void* data, size_t size);
  bool safeRead(const char* filename, void* data, size_t size);
  
  // Logging interno
  void logError(const char* operation, const char* filename = nullptr);
  void logSuccess(const char* operation, const char* filename = nullptr);

public:
  FileManager();
  ~FileManager();
  
  // Inicialización
  bool initialize();
  bool checkSDHealth();
  void reinitializeSD();
  
  // Operaciones de configuración principal
  bool saveConfiguration(const AppConfig& config, 
                        const EncoderConfig encoders[NUM_ENCODERS][NUM_BANKS]);
  bool loadConfiguration(AppConfig& config, 
                        EncoderConfig encoders[NUM_ENCODERS][NUM_BANKS]);
  bool resetConfiguration();
  
  // Gestión de presets
  bool savePreset(const char* presetName, 
                 const EncoderConfig encoders[NUM_ENCODERS][NUM_BANKS]);
  bool loadPreset(const char* presetName, 
                 EncoderConfig encoders[NUM_ENCODERS][NUM_BANKS]);
  bool deletePreset(const char* presetName);
  bool renamePreset(const char* oldName, const char* newName);
  
  // Listado de archivos
  uint8_t listPresets(char presetNames[][MAX_PRESET_NAME], uint8_t maxPresets);
  uint8_t listDirectory(const char* path, FileInfo* files, uint8_t maxFiles);
  bool fileExists(const char* filename);
  uint32_t getFileSize(const char* filename);
  
  // Operaciones de archivo generales
  bool writeFile(const char* filename, const void* data, size_t size);
  bool readFile(const char* filename, void* data, size_t maxSize, size_t* actualSize = nullptr);
  bool appendToFile(const char* filename, const void* data, size_t size);
  bool deleteFile(const char* filename);
  bool copyFile(const char* source, const char* dest);
  
  // Gestión de directorios
  bool createDirectory(const char* path);
  bool deleteDirectory(const char* path);
  bool directoryExists(const char* path);
  
  // Información del sistema de archivos
  uint32_t getTotalSpace() const { return totalSpace; }
  uint32_t getFreeSpace() const { return freeSpace; }
  uint32_t getUsedSpace() const { return totalSpace - freeSpace; }
  uint8_t getUsagePercent() const { 
    return totalSpace > 0 ? (100 * getUsedSpace()) / totalSpace : 0; 
  }
  
  // Estado del sistema
  bool isSDPresent() const { return sdCardPresent; }
  bool isInitialized() const { return sdInitialized; }
  uint32_t getFilesWritten() const { return filesWritten; }
  uint32_t getFilesRead() const { return filesRead; }
  uint32_t getErrorCount() const { return errorCount; }
  
  // Mantenimiento
  void updateSpaceInfo();
  void defragmentFiles();
  bool repairFileSystem();
  void clearTempFiles();
  
  // Logging y diagnóstico
  bool enableLogging(bool enable);
  bool writeLogEntry(const char* message);
  void printSystemInfo() const;
  void printDirectoryTree(const char* path = "/") const;
  
  // Importación/Exportación
  bool exportConfiguration(const char* filename);
  bool importConfiguration(const char* filename);
  bool exportPresets(const char* filename);
  bool importPresets(const char* filename);
  
  // Utilidades de validación
  bool isValidFilename(const char* filename) const;
  bool isValidPresetName(const char* name) const;
  void sanitizeFilename(char* filename) const;
  
  // Recovery y reparación
  bool recoverConfiguration();
  bool verifyAllFiles();
  uint8_t scanForCorruption();
  
private:
  // Variables de logging
  bool loggingEnabled;
  char logFilename[MAX_FILENAME_LENGTH];
  
  // Cache de información de archivos
  struct FileCache {
    char filename[12];//MAX_FILENAME_LENGTH
    uint32_t size;
    uint32_t timestamp;
    uint16_t checksum;
  } cache[2]; //era 10
  uint8_t cacheSize;
  
  // Métodos de cache
  void addToCache(const char* filename, uint32_t size, uint32_t timestamp, uint16_t checksum);
  bool findInCache(const char* filename, FileCache& entry) const;
  void clearCache();
  
  // Utilidades internas
  void generateTimestamp(char* buffer, size_t bufferSize) const;
  bool createPath(const char* fullPath);
  void printFileInfo(const FileInfo& info) const;
};

// Funciones globales de utilidad para el sistema de archivos
namespace FileUtils {
  // Conversión de tipos de archivo
  const char* getFileExtension(const char* filename);
  void removeExtension(char* filename);
  void addExtension(char* filename, const char* extension);
  
  // Formateo de tamaños
  void formatFileSize(uint32_t bytes, char* buffer, size_t bufferSize);
  void formatTimestamp(uint32_t timestamp, char* buffer, size_t bufferSize);
  
  // Validación
  bool isHiddenFile(const char* filename);
  bool isSystemFile(const char* filename);
  bool isConfigFile(const char* filename);
}

#endif // FILE_MANAGER_H