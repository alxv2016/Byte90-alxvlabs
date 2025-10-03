/**
 * @file flash_module.cpp
 * @brief Implementation of flash memory and filesystem operations
 *
 * Provides functions for initializing and interacting with the LittleFS filesystem,
 * including operations for checking file existence and tracking storage statistics.
 */

#include "flash_module.h"
#include "common.h"

//==============================================================================
// GLOBAL VARIABLES
//==============================================================================

bool FSInitialized = false;
static size_t totalBytes = 0;
static size_t usedBytes = 0;

//==============================================================================
// UTILITY FUNCTIONS (STATIC)
//==============================================================================

/**
 * @brief Check only critical files needed for boot
 * @return true if critical files exist, false otherwise
 */
bool checkCriticalFiles() {
  if (!LittleFS.exists("/gifs")) {
    ESP_LOGE(FLASH_LOG, "Critical directory /gifs not found");
    return false;
  }
  
  const char *criticalFiles[] = {
    "/gifs/rest.gif"
  };
  
  for (const char *file : criticalFiles) {
    if (!LittleFS.exists(file)) {
      ESP_LOGE(FLASH_LOG, "Critical file %s not found", file);
      return false;
    }
  }
  
  return true;
}

/**
 * @brief Complete file validation
 * @return true if all required files/directories exist
 */
bool checkFileStatus() {
  const char *requiredDirs[] = {"/gifs"};
  bool dirMissing = false;
  
  for (const char *dir : requiredDirs) {
    if (!LittleFS.exists(dir) || !LittleFS.open(dir).isDirectory()) {
      ESP_LOGW(FLASH_LOG, "Warning: Required directory %s not found", dir);
      dirMissing = true;
    }
  }

  const char *requiredFiles[] = {"/index.html", "/styles.css", "/script.js"};
  bool fileMissing = false;
  
  for (const char *file : requiredFiles) {
    if (!fileExists(file)) {
      ESP_LOGW(FLASH_LOG, "Warning: Required file %s not found", file);
      fileMissing = true;
    }
  }

  const char *essentialGifs[] = {
    "/gifs/startup.gif", 
    "/gifs/rest.gif"
  };
  
  bool gifMissing = false;
  for (const char *gif : essentialGifs) {
    if (!fileExists(gif)) {
      ESP_LOGW(FLASH_LOG, "Warning: Essential GIF %s not found", gif);
      gifMissing = true;
    }
  }
 
  if (dirMissing || fileMissing || gifMissing) {
    ESP_LOGW(FLASH_LOG, "Please ensure you have uploaded the complete data folder");
    return false;
  }
  return true;
}

//==============================================================================
// PUBLIC API FUNCTIONS
//==============================================================================

/**
 * @brief Initialize the filesystem
 * @param formatOnFail If true, format the filesystem if mounting fails
 * @return FSStatus indicating success or specific failure type
 */
FSStatus initializeFS(bool formatOnFail) {
  if (FSInitialized) {
    return FSStatus::FS_SUCCESS;
  }
   
  if (!LittleFS.begin(false)) {
    ESP_LOGE(FLASH_LOG, "Failed to mount LittleFS");
    if (formatOnFail) {
      ESP_LOGW(FLASH_LOG, "Formatting filesystem...");
      if (!LittleFS.begin(true)) {
        ESP_LOGE(FLASH_LOG, "Failed to format and mount LittleFS");
        return FSStatus::FS_FORMAT_FAILED;
      }
    } else {
      return FSStatus::FS_MOUNT_FAILED;
    }
  }
 
  FSInitialized = true;
  
  if (!checkCriticalFiles()) {
    ESP_LOGE(FLASH_LOG, "Critical files missing from filesystem");
    return FSStatus::FS_FILE_MISSING;
  }
  
  totalBytes = LittleFS.totalBytes();
  usedBytes = LittleFS.usedBytes();
  
  return FSStatus::FS_SUCCESS;
}

/**
 * @brief Update filesystem statistics with optional GIF counting
 * @param includeGifCount If true, count GIF files (slow), if false skip counting
 * @return StorageInfo structure
 */
StorageInfo getDetailedFlashStats(bool includeGifCount) {
  const float KB = 1024.0;
  const float MB = KB * 1024.0;
  StorageInfo info = {0, 0, 0, 0};

  if (!FSInitialized) {
    ESP_LOGW(FLASH_LOG, "Cannot update stats: filesystem not initialized");
    return info;
  }

  info.totalSpaceMB = LittleFS.totalBytes() / MB;
  info.usedSpaceMB = LittleFS.usedBytes() / MB;
  info.freeSpaceMB = info.totalSpaceMB - info.usedSpaceMB;
  
  totalBytes = LittleFS.totalBytes();
  usedBytes = LittleFS.usedBytes();

  if (includeGifCount) {
    File root = LittleFS.open("/gifs");
    if (root && root.isDirectory()) {
      File file = root.openNextFile();
      while (file) {
        if (strstr(file.name(), ".gif")) {
          info.gifCount++;
        }
        file = root.openNextFile();
      }
      root.close();
    }
  } else {
    info.gifCount = -1;
  }

  float percentUsed = (info.totalSpaceMB > 0) ? (info.usedSpaceMB * 100.0f / info.totalSpaceMB) : 0;

  if (includeGifCount && info.gifCount >= 0) {
    ESP_LOGW(FLASH_LOG, "Storage Stats: %.2f%% used (%.2f/%.2f MB), %.2f MB free, %d GIFs", 
             percentUsed, info.usedSpaceMB, info.totalSpaceMB, info.freeSpaceMB, info.gifCount);
  }

  return info;
}

/**
 * @brief Validate filesystem contents
 * @return true if all required files/directories exist
 */
bool validateFilesystemContents() {
  if (!FSInitialized) {
    ESP_LOGW(FLASH_LOG, "Cannot validate: filesystem not initialized");
    return false;
  }
  
  return checkFileStatus();
}

/**
 * @brief Check if filesystem is initialized
 * @return true if filesystem is initialized, false otherwise
 */
bool getFSStatus() {
  return FSInitialized;
}

/**
 * @brief Check if a file exists in the filesystem
 * @param path Path to the file to check
 * @return true if file exists, false otherwise or if filesystem not initialized
 */
bool fileExists(const char* path) {
  if (!FSInitialized) {
    return false;
  }
  return LittleFS.exists(path);
}

/**
 * @brief Get total filesystem capacity
 * @return Total space in bytes
 */
size_t getTotalSpace() {
  return totalBytes;
}

/**
 * @brief Get used filesystem space
 * @return Used space in bytes
 */
size_t getUsedSpace() {
  return usedBytes;
}

/**
 * @brief Get free filesystem space
 * @return Free space in bytes
 */
size_t getFreeSpace() {
  return totalBytes - usedBytes;
}