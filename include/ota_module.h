/**
 * @file ota_module.h
 * @brief Over-The-Air update functionality for firmware and filesystem updates over WiFi
 */

#ifndef OTA_MODULE_H
#define OTA_MODULE_H

#include "common.h"
#include "flash_module.h"
#include <WebServer.h>
#include <Update.h>
#include <esp_ota_ops.h>

//==============================================================================
// CONSTANTS & DEFINITIONS
//==============================================================================

#ifndef FIRMWARE_VERSION
#define FIRMWARE_VERSION "1.0.0"
#endif

#define FIRMWARE_BIN "byte90.bin"
#define FILESYSTEM_BIN "byte90animations.bin"

static const char* OTA_LOG = "::OTA_MODULE::";

//==============================================================================
// TYPE DEFINITIONS
//==============================================================================

enum class OTAState {
  IDLE,
  UPLOADING,
  UPDATING,
  SUCCESS,
  ERROR,
  UNKNOWN
};

//==============================================================================
// EXTERNAL VARIABLES
//==============================================================================

extern WebServer webServer;
extern OTAState otaState;
extern String otaMessage;

//==============================================================================
// PUBLIC API FUNCTIONS
//==============================================================================

/**
 * @brief Initialize the OTA system
 * @return true if initialization was successful
 */
bool initOTA();

/**
 * @brief Set up HTTP endpoints for OTA updates
 */
void setupOTAEndpoints();

/**
 * @brief Handle the file upload process for OTA updates
 */
void handleFileUpload();

#endif /* OTA_MODULE_H */