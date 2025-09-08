#include "FileManager.h"

FileManager::FileManager() 
  : sdInitialized(false), sdCardPresent(false), totalSpace(0), freeSpace(0),
    filesWritten(0), filesRead(0), errorCount(0), loggingEnabled(false), cacheSize(0)
{
 /* memset(workBuffer, 0, sizeof(workBuffer));
  memset(pathBuffer, 0, sizeof(pathBuffer));
  strcpy(logFilename, "/logs/system.log");*/
  // REDUCIDO:
  uint8_t workBuffer[32];    // Era 128 → 32 (96 bytes ahorrados)
  char pathBuffer[24];       // Era 32 → 24 (8 bytes ahorrados)
  strcpy(logFilename, "sys.log"); // Sin path largo
  clearCache();
}

FileManager::~FileManager() {}

bool FileManager::initialize() {
  Serial.println(F("Inicializando sistema de archivos SD..."));
  
  SPI.begin();
  
  for (int retry = 0; retry < SD_RETRY_COUNT; retry++) {
    if (SD.begin(SD_CS)) {
      sdInitialized = true;
      sdCardPresent = true;
      break;
    }
    delay(100);
    Serial.print(F("Reintento SD: "));
    Serial.println(retry + 1);
  }
  
  if (!sdInitialized) {
    Serial.println(F("ERROR: No se pudo inicializar tarjeta SD"));
    return false;
  }
  
  if (!validateSDCard() || !initializeDirectories()) {
    Serial.println(F("ERROR: Validación de SD falló"));
    return false;
  }
  
  updateSpaceInfo();
  Serial.print(F("SD inicializada - Espacio total: "));
  Serial.print(totalSpace / 1024);
  Serial.println(F(" KB"));
  
  return true;
}

bool FileManager::validateSDCard() {
  const char testFile[] = "/test.tmp";
  const char testData[] = "MACKIE_TEST";
  
  if (!writeFile(testFile, testData, strlen(testData))) {
    logError("SD write test");
    return false;
  }
  
  char readData[32];
  size_t readSize;
  if (!readFile(testFile, readData, sizeof(readData), &readSize)) {
    logError("SD read test");
    return false;
  }
  
  deleteFile(testFile);
  
  if (readSize != strlen(testData) || strcmp(readData, testData) != 0) {
    logError("SD data integrity test");
    return false;
  }
  
  return true;
}

bool FileManager::initializeDirectories() {
  const char* dirs[] = {PRESET_DIRECTORY, LOG_DIRECTORY, TEMP_DIRECTORY};
  
  for (int i = 0; i < 3; i++) {
    if (!createDirectory(dirs[i])) {
      Serial.print(F("ADVERTENCIA: No se pudo crear directorio "));
      Serial.println(dirs[i]);
    }
  }
  
  return true;
}

bool FileManager::saveConfiguration(const AppConfig& config, const EncoderConfig encoders[NUM_ENCODERS][NUM_BANKS]) {
  Serial.println(F("Guardando configuración..."));
  
  if (fileExists(CONFIG_FILENAME)) {
    createBackup(CONFIG_FILENAME);
  }
  
  ConfigFileHeader header;
  header.dataSize = sizeof(AppConfig) + sizeof(EncoderConfig) * NUM_ENCODERS * NUM_BANKS;
  header.timestamp = millis() / 1000;
  
  File configFile = SD.open(CONFIG_FILENAME, FILE_WRITE);
  if (!configFile) {
    logError("open config for write");
    return false;
  }
  
  if (!writeFileHeader(configFile, header)) {
    configFile.close();
    logError("write config header");
    return false;
  }
  
  if (configFile.write((const uint8_t*)&config, sizeof(config)) != sizeof(config)) {
    configFile.close();
    logError("write app config");
    return false;
  }
  
  size_t encoderDataSize = sizeof(EncoderConfig) * NUM_ENCODERS * NUM_BANKS;
  if (configFile.write((const uint8_t*)encoders, encoderDataSize) != encoderDataSize) {
    configFile.close();
    logError("write encoder config");
    return false;
  }
  
  configFile.close();
  filesWritten++;
  
  if (!verifyFileIntegrity(CONFIG_FILENAME)) {
    Serial.println(F("ERROR: Verificación de integridad falló"));
    restoreFromBackup(CONFIG_FILENAME);
    return false;
  }
  
  logSuccess("save configuration");
  return true;
}

bool FileManager::loadConfiguration(AppConfig& config, EncoderConfig encoders[NUM_ENCODERS][NUM_BANKS]) {
  Serial.println(F("Cargando configuración..."));
  
  if (!fileExists(CONFIG_FILENAME)) {
    Serial.println(F("Archivo de configuración no existe"));
    return false;
  }
  
  if (!verifyFileIntegrity(CONFIG_FILENAME)) {
    Serial.println(F("Archivo corrupto, intentando restaurar desde backup"));
    if (!restoreFromBackup(CONFIG_FILENAME)) {
      return false;
    }
  }
  
  File configFile = SD.open(CONFIG_FILENAME, FILE_READ);
  if (!configFile) {
    logError("open config for read");
    return false;
  }
  
  ConfigFileHeader header;
  if (!readFileHeader(configFile, header)) {
    configFile.close();
    logError("read config header");
    return false;
  }
  
  if (header.version != CONFIG_VERSION) {
    Serial.println(F("Versión de configuración incompatible"));
    configFile.close();
    return false;
  }
  
  if (configFile.read((uint8_t*)&config, sizeof(config)) != sizeof(config)) {
    configFile.close();
    logError("read app config");
    return false;
  }
  
  size_t encoderDataSize = sizeof(EncoderConfig) * NUM_ENCODERS * NUM_BANKS;
  if (configFile.read((uint8_t*)encoders, encoderDataSize) != encoderDataSize) {
    configFile.close();
    logError("read encoder config");
    return false;
  }
  
  configFile.close();
  filesRead++;
  logSuccess("load configuration");
  return true;
}

bool FileManager::resetConfiguration() {
  Serial.println(F("Reseteando configuración..."));
  
  if (fileExists(CONFIG_FILENAME)) {
    createBackup(CONFIG_FILENAME);
  }
  
  bool success = deleteFile(CONFIG_FILENAME);
  
  if (success) {
    Serial.println(F("Configuración reseteada"));
    logSuccess("reset configuration");
  } else {
    logError("reset configuration");
  }
  
  return success;
}

bool FileManager::savePreset(const char* presetName, const EncoderConfig encoders[NUM_ENCODERS][NUM_BANKS]) {
  if (!isValidPresetName(presetName)) {
    logError("invalid preset name");
    return false;
  }
  
  snprintf(pathBuffer, sizeof(pathBuffer), "%s/%s.pre", PRESET_DIRECTORY, presetName);
  
  ConfigFileHeader header;
  header.dataSize = sizeof(EncoderConfig) * NUM_ENCODERS * NUM_BANKS;
  header.timestamp = millis() / 1000;
  snprintf(header.description, sizeof(header.description), "Preset: %s", presetName);
  
  File presetFile = SD.open(pathBuffer, FILE_WRITE);
  if (!presetFile) {
    logError("create preset file", pathBuffer);
    return false;
  }
  
  if (!writeFileHeader(presetFile, header)) {
    presetFile.close();
    logError("write preset header");
    return false;
  }
  
  size_t dataSize = sizeof(EncoderConfig) * NUM_ENCODERS * NUM_BANKS;
  if (presetFile.write((const uint8_t*)encoders, dataSize) != dataSize) {
    presetFile.close();
    logError("write preset data");
    return false;
  }
  
  presetFile.close();
  filesWritten++;
  
  Serial.print(F("Preset '"));
  Serial.print(presetName);
  Serial.println(F("' guardado"));
  
  return true;
}

bool FileManager::loadPreset(const char* presetName, EncoderConfig encoders[NUM_ENCODERS][NUM_BANKS]) {
  if (!isValidPresetName(presetName)) {
    return false;
  }
  
  snprintf(pathBuffer, sizeof(pathBuffer), "%s/%s.pre", PRESET_DIRECTORY, presetName);
  
  if (!fileExists(pathBuffer)) {
    Serial.print(F("Preset '"));
    Serial.print(presetName);
    Serial.println(F("' no existe"));
    return false;
  }
  
  File presetFile = SD.open(pathBuffer, FILE_READ);
  if (!presetFile) {
    logError("open preset file", pathBuffer);
    return false;
  }
  
  ConfigFileHeader header;
  if (!readFileHeader(presetFile, header)) {
    presetFile.close();
    logError("read preset header");
    return false;
  }
  
  size_t dataSize = sizeof(EncoderConfig) * NUM_ENCODERS * NUM_BANKS;
  if (presetFile.read((uint8_t*)encoders, dataSize) != dataSize) {
    presetFile.close();
    logError("read preset data");
    return false;
  }
  
  presetFile.close();
  filesRead++;
  
  Serial.print(F("Preset '"));
  Serial.print(presetName);
  Serial.println(F("' cargado"));
  
  return true;
}

bool FileManager::deletePreset(const char* presetName) {
  if (!isValidPresetName(presetName)) {
    return false;
  }
  
  snprintf(pathBuffer, sizeof(pathBuffer), "%s/%s.pre", PRESET_DIRECTORY, presetName);
  
  if (!fileExists(pathBuffer)) {
    Serial.print(F("Preset '"));
    Serial.print(presetName);
    Serial.println(F("' no existe"));
    return false;
  }
  
  if (deleteFile(pathBuffer)) {
    Serial.print(F("Preset '"));
    Serial.print(presetName);
    Serial.println(F("' eliminado"));
    return true;
  }
  
  logError("delete preset", pathBuffer);
  return false;
}

bool FileManager::renamePreset(const char* oldName, const char* newName) {
  if (!isValidPresetName(oldName) || !isValidPresetName(newName)) {
    return false;
  }
  
  char oldPath[64], newPath[64];
  snprintf(oldPath, sizeof(oldPath), "%s/%s.pre", PRESET_DIRECTORY, oldName);
  snprintf(newPath, sizeof(newPath), "%s/%s.pre", PRESET_DIRECTORY, newName);
  
  if (!fileExists(oldPath)) {
    Serial.print(F("Preset origen '"));
    Serial.print(oldName);
    Serial.println(F("' no existe"));
    return false;
  }
  
  if (fileExists(newPath)) {
    Serial.print(F("Preset destino '"));
    Serial.print(newName);
    Serial.println(F("' ya existe"));
    return false;
  }
  
  if (copyFile(oldPath, newPath) && deleteFile(oldPath)) {
    Serial.print(F("Preset renombrado"));
    return true;
  }
  
  logError("rename preset");
  return false;
}

uint8_t FileManager::listPresets(char presetNames[][MAX_PRESET_NAME], uint8_t maxPresets) {
  File dir = SD.open(PRESET_DIRECTORY);
  if (!dir || !dir.isDirectory()) {
    return 0;
  }
  
  uint8_t count = 0;
  File entry = dir.openNextFile();
  
  while (entry && count < maxPresets) {
    if (!entry.isDirectory()) {
      const char* name = entry.name();
      const char* ext = strrchr(name, '.');
      
      if (ext && strcmp(ext, ".pre") == 0) {
        size_t nameLen = ext - name;
        nameLen = min(nameLen, (size_t)(MAX_PRESET_NAME - 1));
        strncpy(presetNames[count], name, nameLen);
        presetNames[count][nameLen] = '\0';
        count++;
      }
    }
    entry.close();
    entry = dir.openNextFile();
  }
  
  dir.close();
  return count;
}

uint8_t FileManager::listDirectory(const char* path, FileInfo* files, uint8_t maxFiles) {
  File dir = SD.open(path);
  if (!dir || !dir.isDirectory()) {
    return 0;
  }
  
  uint8_t count = 0;
  File entry = dir.openNextFile();
  
  while (entry && count < maxFiles) {
    FileInfo& info = files[count];
    
    strncpy(info.name, entry.name(), MAX_FILENAME_LENGTH - 1);
    info.name[MAX_FILENAME_LENGTH - 1] = '\0';
    info.size = entry.size();
    info.timestamp = millis() / 1000;
    info.isDirectory = entry.isDirectory();
    
    count++;
    entry.close();
    entry = dir.openNextFile();
  }
  
  dir.close();
  return count;
}

bool FileManager::fileExists(const char* filename) {
  return SD.exists(filename);
}

uint32_t FileManager::getFileSize(const char* filename) {
  File file = SD.open(filename, FILE_READ);
  if (!file) return 0;
  
  uint32_t size = file.size();
  file.close();
  return size;
}

bool FileManager::writeFile(const char* filename, const void* data, size_t size) {
  File file = SD.open(filename, FILE_WRITE);
  if (!file) {
    errorCount++;
    return false;
  }
  
  bool success = (file.write((const uint8_t*)data, size) == size);
  file.close();
  
  if (success) {
    filesWritten++;
  } else {
    errorCount++;
  }
  
  return success;
}

bool FileManager::readFile(const char* filename, void* data, size_t maxSize, size_t* actualSize) {
  File file = SD.open(filename, FILE_READ);
  if (!file) {
    errorCount++;
    return false;
  }
  
  size_t fileSize = file.size();
  size_t readSize = min(fileSize, maxSize);
  
  bool success = (file.read((uint8_t*)data, readSize) == readSize);
  file.close();
  
  if (actualSize) {
    *actualSize = success ? readSize : 0;
  }
  
  if (success) {
    filesRead++;
  } else {
    errorCount++;
  }
  
  return success;
}

bool FileManager::appendToFile(const char* filename, const void* data, size_t size) {
  File file = SD.open(filename, FILE_WRITE);
  if (!file) {
    errorCount++;
    return false;
  }
  
  file.seek(file.size());
  
  bool success = (file.write((const uint8_t*)data, size) == size);
  file.close();
  
  if (success) {
    filesWritten++;
  } else {
    errorCount++;
  }
  
  return success;
}

bool FileManager::deleteFile(const char* filename) {
  bool success = SD.remove(filename);
  if (!success) {
    errorCount++;
  }
  return success;
}

bool FileManager::copyFile(const char* source, const char* dest) {
  File sourceFile = SD.open(source, FILE_READ);
  if (!sourceFile) return false;
  
  File destFile = SD.open(dest, FILE_WRITE);
  if (!destFile) {
    sourceFile.close();
    return false;
  }
  
  while (sourceFile.available()) {
    size_t bytesToRead = min((size_t)sourceFile.available(), sizeof(workBuffer));
    size_t bytesRead = sourceFile.read(workBuffer, bytesToRead);
    if (destFile.write(workBuffer, bytesRead) != bytesRead) {
      sourceFile.close();
      destFile.close();
      return false;
    }
  }
  
  sourceFile.close();
  destFile.close();
  return true;
}

bool FileManager::createDirectory(const char* path) {
  if (directoryExists(path)) return true;
  
  char dummyFile[64];
  snprintf(dummyFile, sizeof(dummyFile), "%s/.dummy", path);
  
  File file = SD.open(dummyFile, FILE_WRITE);
  if (file) {
    file.close();
    SD.remove(dummyFile);
    return true;
  }
  
  return false;
}

bool FileManager::deleteDirectory(const char* path) {
  File dir = SD.open(path);
  if (!dir || !dir.isDirectory()) {
    return false;
  }
  
  File entry = dir.openNextFile();
  while (entry) {
    char fullPath[32];//64
    snprintf(fullPath, sizeof(fullPath), "%s/%s", path, entry.name());
    
    if (entry.isDirectory()) {
      deleteDirectory(fullPath);
    } else {
      deleteFile(fullPath);
    }
    
    entry.close();
    entry = dir.openNextFile();
  }
  
  dir.close();
  return SD.remove(path);
}

bool FileManager::directoryExists(const char* path) {
  File dir = SD.open(path);
  bool exists = dir && dir.isDirectory();
  if (dir) dir.close();
  return exists;
}

bool FileManager::writeFileHeader(File& file, const ConfigFileHeader& header) {
  ConfigFileHeader tempHeader = header;
  tempHeader.checksum = calculateChecksum(&tempHeader.dataSize, sizeof(tempHeader.dataSize));
  
  return (file.write((const uint8_t*)&tempHeader, sizeof(tempHeader)) == sizeof(tempHeader));
}

bool FileManager::readFileHeader(File& file, ConfigFileHeader& header) {
  if (file.read((uint8_t*)&header, sizeof(header)) != sizeof(header)) {
    return false;
  }
  
  if (header.magic != 0x4D434B52) {
    return false;
  }
  
  return true;
}

uint16_t FileManager::calculateChecksum(const void* data, size_t size) {
  const uint8_t* bytes = (const uint8_t*)data;
  uint16_t checksum = 0;
  
  for (size_t i = 0; i < size; i++) {
    checksum = (checksum << 1) ^ bytes[i];
  }
  
  return checksum;
}

bool FileManager::verifyFileIntegrity(const char* filename) {
  File file = SD.open(filename, FILE_READ);
  if (!file) return false;
  
  ConfigFileHeader header;
  if (!readFileHeader(file, header)) {
    file.close();
    return false;
  }
  
  uint32_t expectedSize = sizeof(ConfigFileHeader) + header.dataSize;
  if (file.size() != expectedSize) {
    file.close();
    return false;
  }
  
  file.close();
  return true;
}

void FileManager::createBackup(const char* filename) {
  char backupName[24];//64
  snprintf(backupName, sizeof(backupName), "%s%s", filename, BACKUP_EXTENSION);
  
  if (fileExists(filename)) {
    copyFile(filename, backupName);
  }
}

bool FileManager::restoreFromBackup(const char* filename) {
  char backupName[64];
  snprintf(backupName, sizeof(backupName), "%s%s", filename, BACKUP_EXTENSION);
  
  if (fileExists(backupName)) {
    return copyFile(backupName, filename);
  }
  
  return false;
}

void FileManager::updateSpaceInfo() {
  totalSpace = 1024 * 1024;
  
  uint32_t used = 0;
  File root = SD.open("/");
  if (root) {
    File entry = root.openNextFile();
    while (entry) {
      used += entry.size();
      entry.close();
      entry = root.openNextFile();
    }
    root.close();
  }
  
  freeSpace = totalSpace > used ? totalSpace - used : 0;
}

void FileManager::clearTempFiles() {
  Serial.println(F("Limpiando archivos temporales..."));
  
  File tempDir = SD.open(TEMP_DIRECTORY);
  if (!tempDir || !tempDir.isDirectory()) {
    return;
  }
  
  uint8_t filesDeleted = 0;
  File entry = tempDir.openNextFile();
  
  while (entry) {
    if (!entry.isDirectory()) {
      char fullPath[64];
      snprintf(fullPath, sizeof(fullPath), "%s/%s", TEMP_DIRECTORY, entry.name());
      
      if (deleteFile(fullPath)) {
        filesDeleted++;
      }
    }
    
    entry.close();
    entry = tempDir.openNextFile();
  }
  
  tempDir.close();
  
  Serial.print(F("Archivos temporales eliminados: "));
  Serial.println(filesDeleted);
}

void FileManager::defragmentFiles() {
  Serial.println(F("Desfragmentación no soportada"));
}

bool FileManager::repairFileSystem() {
  Serial.println(F("Reparando sistema de archivos..."));
  
  bool repaired = false;
  
  if (!directoryExists(PRESET_DIRECTORY)) {
    createDirectory(PRESET_DIRECTORY);
    repaired = true;
  }
  
  if (!directoryExists(LOG_DIRECTORY)) {
    createDirectory(LOG_DIRECTORY);
    repaired = true;
  }
  
  if (!directoryExists(TEMP_DIRECTORY)) {
    createDirectory(TEMP_DIRECTORY);
    repaired = true;
  }
  
  if (repaired) {
    Serial.println(F("Estructura reparada"));
  }
  
  return repaired;
}

bool FileManager::checkSDHealth() {
  if (!sdInitialized) return false;
  return validateSDCard();
}

void FileManager::reinitializeSD() {
  Serial.println(F("Reinicializando SD..."));
  
  sdInitialized = false;
  sdCardPresent = false;
  
  delay(100);
  
  if (initialize()) {
    Serial.println(F("SD reinicializada"));
  } else {
    Serial.println(F("Error reinicialización"));
  }
}

bool FileManager::enableLogging(bool enable) {
  loggingEnabled = enable;
  
  if (enable && !fileExists(logFilename)) {
    const char* header = "=== LOG ===\n";
    writeFile(logFilename, header, strlen(header));
  }
  
  return true;
}

bool FileManager::writeLogEntry(const char* message) {
  if (!loggingEnabled || !message) {
    return false;
  }
  
  char logEntry[48];//128
  unsigned long timestamp = millis() / 1000;
  
  snprintf(logEntry, sizeof(logEntry), "[%lu] %s\n", timestamp, message);
  
  return appendToFile(logFilename, logEntry, strlen(logEntry));
}

bool FileManager::exportConfiguration(const char* filename) {
  return copyFile(CONFIG_FILENAME, filename);
}

bool FileManager::importConfiguration(const char* filename) {
  if (!fileExists(filename)) {
    return false;
  }
  
  createBackup(CONFIG_FILENAME);
  return copyFile(filename, CONFIG_FILENAME);
}

bool FileManager::exportPresets(const char* filename) {
  return false; // No implementado
}

bool FileManager::importPresets(const char* filename) {
  return false; // No implementado
}

bool FileManager::recoverConfiguration() {
  Serial.println(F("Recuperando configuración..."));
  
  if (restoreFromBackup(CONFIG_FILENAME)) {
    Serial.println(F("Recuperada desde backup"));
    return true;
  }
  
  Serial.println(F("Creando por defecto"));
  resetConfiguration();
  return false;
}

bool FileManager::verifyAllFiles() {
  Serial.println(F("Verificando archivos..."));
  
  bool allValid = true;
  uint8_t filesChecked = 0;
  uint8_t filesCorrupted = 0;
  
  if (fileExists(CONFIG_FILENAME)) {
    filesChecked++;
    if (!verifyFileIntegrity(CONFIG_FILENAME)) {
      Serial.println(F("Config corrupto"));
      allValid = false;
      filesCorrupted++;
    }
  }
  
  char presetNames[10][MAX_PRESET_NAME];
  uint8_t presetCount = listPresets(presetNames, 10);
  
  for (uint8_t i = 0; i < presetCount; i++) {
    snprintf(pathBuffer, sizeof(pathBuffer), "%s/%s.pre", PRESET_DIRECTORY, presetNames[i]);
    filesChecked++;
    
    if (!verifyFileIntegrity(pathBuffer)) {
      Serial.print(F("Preset corrupto: "));
      Serial.println(presetNames[i]);
      allValid = false;
      filesCorrupted++;
    }
  }
  
  Serial.print(F("Verificados: "));
  Serial.print(filesChecked);
  Serial.print(F(", Corruptos: "));
  Serial.println(filesCorrupted);
  
  return allValid;
}

uint8_t FileManager::scanForCorruption() {
  uint8_t corruptedFiles = 0;
  
  if (fileExists(CONFIG_FILENAME)) {
    if (!verifyFileIntegrity(CONFIG_FILENAME)) {
      corruptedFiles++;
    }
  }
  
  return corruptedFiles;
}

void FileManager::printDirectoryTree(const char* path) const {
  File dir = SD.open(path);
  if (!dir || !dir.isDirectory()) {
    Serial.print(F("Error abriendo: "));
    Serial.println(path);
    return;
  }
  
  Serial.print(F("Directorio "));
  Serial.println(path);
  Serial.println(F("------------------"));
  
  File entry = dir.openNextFile();
  while (entry) {
    Serial.print(entry.isDirectory() ? F("[DIR] ") : F("[FILE] "));
    Serial.print(entry.name());
    
    if (!entry.isDirectory()) {
      Serial.print(F(" ("));
      Serial.print(entry.size());
      Serial.print(F(" bytes)"));
    }
    
    Serial.println();
    entry.close();
    entry = dir.openNextFile();
  }
  
  dir.close();
  Serial.println(F("------------------"));
}

void FileManager::printSystemInfo() const {
  Serial.println(F("\n=== INFO SD ==="));
  Serial.print(F("Inicializada: "));
  Serial.println(sdInitialized ? F("Si") : F("No"));
  Serial.print(F("Presente: "));
  Serial.println(sdCardPresent ? F("Si") : F("No"));
  Serial.print(F("Total: "));
  Serial.print(totalSpace / 1024);
  Serial.println(F(" KB"));
  Serial.print(F("Libre: "));
  Serial.print(freeSpace / 1024);
  Serial.println(F(" KB"));
  Serial.print(F("Uso: "));
  Serial.print(getUsagePercent());
  Serial.println(F("%"));
  Serial.print(F("Escritos: "));
  Serial.println(filesWritten);
  Serial.print(F("Leidos: "));
  Serial.println(filesRead);
  Serial.print(F("Errores: "));
  Serial.println(errorCount);
  Serial.println(F("==============="));
}

bool FileManager::isValidFilename(const char* filename) const {
  if (!filename || strlen(filename) == 0 || strlen(filename) > MAX_FILENAME_LENGTH) {
    return false;
  }
  
  const char* invalid = "<>:\"|?*";
  return (strpbrk(filename, invalid) == nullptr);
}

bool FileManager::isValidPresetName(const char* name) const {
  if (!name || strlen(name) == 0 || strlen(name) >= MAX_PRESET_NAME) {
    return false;
  }
  
  for (const char* p = name; *p; p++) {
    if (!isalnum(*p) && *p != '_' && *p != '-' && *p != ' ') {
      return false;
    }
  }
  
  return true;
}

void FileManager::sanitizeFilename(char* filename) const {
  if (!filename) return;
  
  const char* invalid = "<>:\"|?*";
  
  for (char* p = filename; *p; p++) {
    if (strchr(invalid, *p) != nullptr) {
      *p = '_';
    }
  }
}

void FileManager::logError(const char* operation, const char* filename) {
  errorCount++;
  Serial.print(F("ERROR: "));
  Serial.print(operation);
  if (filename) {
    Serial.print(F(" ("));
    Serial.print(filename);
    Serial.print(F(")"));
  }
  Serial.println();
  
  if (loggingEnabled) {
    char logMsg[64];
    snprintf(logMsg, sizeof(logMsg), "ERROR: %s", operation);
    writeLogEntry(logMsg);
  }
}

void FileManager::logSuccess(const char* operation, const char* filename) {
  Serial.print(F("OK: "));
  Serial.print(operation);
  if (filename) {
    Serial.print(F(" ("));
    Serial.print(filename);
    Serial.print(F(")"));
  }
  Serial.println();
  
  if (loggingEnabled) {
    char logMsg[64];
    snprintf(logMsg, sizeof(logMsg), "OK: %s", operation);
    writeLogEntry(logMsg);
  }
}

void FileManager::addToCache(const char* filename, uint32_t size, uint32_t timestamp, uint16_t checksum) {
  if (cacheSize >= 10) {
    for (int i = 0; i < 9; i++) {
      cache[i] = cache[i + 1];
    }
    cacheSize = 9;
  }
  
  FileCache& entry = cache[cacheSize];
  strncpy(entry.filename, filename, MAX_FILENAME_LENGTH - 1);
  entry.filename[MAX_FILENAME_LENGTH - 1] = '\0';
  entry.size = size;
  entry.timestamp = timestamp;
  entry.checksum = checksum;
  
  cacheSize++;
}

bool FileManager::findInCache(const char* filename, FileCache& entry) const {
  for (uint8_t i = 0; i < cacheSize; i++) {
    if (strcmp(cache[i].filename, filename) == 0) {
      entry = cache[i];
      return true;
    }
  }
  return false;
}

void FileManager::clearCache() {
  cacheSize = 0;
  memset(cache, 0, sizeof(cache));
}

void FileManager::generateTimestamp(char* buffer, size_t bufferSize) const {
  unsigned long timestamp = millis() / 1000;
  snprintf(buffer, bufferSize, "%lu", timestamp);
}

bool FileManager::createPath(const char* fullPath) {
  const char* lastSlash = strrchr(fullPath, '/');
  if (!lastSlash) return true;
  
  char dirPath[32];//64
  size_t dirLen = lastSlash - fullPath;
  strncpy(dirPath, fullPath, dirLen);
  dirPath[dirLen] = '\0';
  
  return createDirectory(dirPath);
}

void FileManager::printFileInfo(const FileInfo& info) const {
  Serial.print(info.isDirectory ? F("[DIR] ") : F("[FILE] "));
  Serial.print(info.name);
  
  if (!info.isDirectory) {
    Serial.print(F(" ("));
    Serial.print(info.size);
    Serial.print(F(" bytes)"));
  }
  
  Serial.println();
}

void FileManager::cleanupOldBackups(const char* baseFilename) {
  char backupName[32];//64
  snprintf(backupName, sizeof(backupName), "%s%s", baseFilename, BACKUP_EXTENSION);
  
  if (fileExists(backupName)) {
    uint32_t currentTime = millis() / 1000;
    if (currentTime > 604800) { // 7 días
      deleteFile(backupName);
    }
  }
}

// ========== NAMESPACE FILEUTILS ==========
namespace FileUtils {
  const char* getFileExtension(const char* filename) {
    const char* dot = strrchr(filename, '.');
    return dot ? dot + 1 : "";
  }
  
  void removeExtension(char* filename) {
    char* dot = strrchr(filename, '.');
    if (dot) *dot = '\0';
  }
  
  void addExtension(char* filename, const char* extension) {
    if (extension && extension[0] != '.') {
      strcat(filename, ".");
    }
    strcat(filename, extension);
  }
  
  void formatFileSize(uint32_t bytes, char* buffer, size_t bufferSize) {
    if (bytes < 1024) {
      snprintf(buffer, bufferSize, "%lu B", bytes);
    } else if (bytes < 1024 * 1024) {
      snprintf(buffer, bufferSize, "%.1f KB", bytes / 1024.0f);
    } else {
      snprintf(buffer, bufferSize, "%.1f MB", bytes / (1024.0f * 1024.0f));
    }
  }
  
  void formatTimestamp(uint32_t timestamp, char* buffer, size_t bufferSize) {
    uint32_t seconds = timestamp % 60;
    uint32_t minutes = (timestamp / 60) % 60;
    uint32_t hours = (timestamp / 3600) % 24;
    uint32_t days = timestamp / 86400;
    
    if (days > 0) {
      snprintf(buffer, bufferSize, "%lud %02lu:%02lu:%02lu", days, hours, minutes, seconds);
    } else {
      snprintf(buffer, bufferSize, "%02lu:%02lu:%02lu", hours, minutes, seconds);
    }
  }
  
  bool isHiddenFile(const char* filename) {
    return filename && filename[0] == '.';
  }
  
  bool isSystemFile(const char* filename) {
    if (!filename) return false;
    
    const char* systemFiles[] = {"System", "SYSTEM~1", "WPSettings.dat", "IndexerVolumeGuid"};
    
    for (uint8_t i = 0; i < 4; i++) {
      if (strstr(filename, systemFiles[i]) != nullptr) {
        return true;
      }
    }
    
    return false;
  }
  
  bool isConfigFile(const char* filename) {
    if (!filename) return false;
    
    const char* ext = getFileExtension(filename);
    return (strcmp(ext, "cfg") == 0 || strcmp(ext, "pre") == 0 || strcmp(ext, "bak") == 0);
  }
}