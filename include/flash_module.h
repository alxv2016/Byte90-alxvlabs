/**
 * @file flash_module.h
 * @brief Header for flash memory and filesystem operations
 *
 * Provides functions for initializing and interacting with the LittleFS filesystem,
 * including operations for checking file existence and tracking storage statistics.
 */

#ifndef FLASH_MODULE_H
#define FLASH_MODULE_H

#include "common.h"
#include <LittleFS.h>

//==============================================================================
// CONSTANTS & DEFINITIONS
//==============================================================================

static const char* FLASH_LOG = "::FLASH_MODULE::";

//==============================================================================
// TYPE DEFINITIONS
//==============================================================================

enum class FSStatus {
  FS_SUCCESS,
  FS_MOUNT_FAILED,
  FS_FORMAT_FAILED,
  FS_FILE_MISSING
};

struct StorageInfo {
 int gifCount;
 float usedSpaceMB;
 float totalSpaceMB;
 float freeSpaceMB;
};

//==============================================================================
// PUBLIC API FUNCTIONS
//==============================================================================

/**
 * @brief Initialize the filesystem (optimized for fast boot)
 * @param formatOnFail If true, format the filesystem if mounting fails
 * @return FSStatus indicating success or specific failure type
 */
FSStatus initializeFS(bool formatOnFail = false);

/**
 * @brief Check if filesystem is initialized
 * @return true if filesystem is initialized, false otherwise
 */
bool getFSStatus();

/**
 * @brief Check if a file exists in the filesystem
 * @param path Path to the file to check
 * @return true if file exists, false otherwise or if filesystem not initialized
 */
bool fileExists(const char* path);
  
/**
 * @brief Update filesystem statistics with optional GIF counting
 * @param includeGifCount If true, count GIF files (slow), if false skip counting
 * @return StorageInfo with complete or basic filesystem statistics
 */
StorageInfo getDetailedFlashStats(bool includeGifCount = true);

/**
 * @brief Get total filesystem capacity
 * @return Total space in bytes
 */
size_t getTotalSpace();

/**
 * @brief Get used filesystem space
 * @return Used space in bytes
 */
size_t getUsedSpace();

/**
 * @brief Get free filesystem space
 * @return Free space in bytes
 */
size_t getFreeSpace();

/**
 * @brief Validate filesystem contents (call after boot if needed)
 * @return true if all required files/directories exist
 */
bool validateFilesystemContents();

#endif /* FLASH_MODULE_H */