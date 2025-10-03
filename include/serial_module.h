/**
 * @file serial_module.h
 * @brief Web Serial API integration for firmware and filesystem updates via USB
 */

#ifndef SERIAL_MODULE_H
#define SERIAL_MODULE_H

#include "common.h"
#include <Update.h>
#include <esp_ota_ops.h>

//==============================================================================
// CONSTANTS & DEFINITIONS
//==============================================================================

static const char* SERIAL_LOG = "::SERIAL_MODULE::";

#define SERIAL_BAUD_RATE 921600
#define SERIAL_COMMAND_BUFFER_SIZE 4096

#define CMD_GET_INFO "GET_INFO"
#define CMD_GET_STATUS "GET_STATUS"
#define CMD_START_UPDATE "START_UPDATE"
#define CMD_SEND_CHUNK "SEND_CHUNK"
#define CMD_FINISH_UPDATE "FINISH_UPDATE"
#define CMD_ABORT_UPDATE "ABORT_UPDATE"
#define CMD_RESTART "RESTART"
#define CMD_GET_LOGS "GET_LOGS"
#define CMD_GET_PREFERENCES "GET_PREFERENCES"
#define CMD_RESET_PREFERENCES "RESET_PREFERENCES"

// WiFi Configuration Commands
#define CMD_WIFI_SCAN "WIFI_SCAN"
#define CMD_WIFI_STATUS "WIFI_STATUS"
#define CMD_WIFI_CONNECT "WIFI_CONNECT"
#define CMD_WIFI_DISCONNECT "WIFI_DISCONNECT"
#define CMD_WIFI_GET_SAVED "WIFI_GET_SAVED"
#define CMD_WIFI_FORGET "WIFI_FORGET"

#define RESP_OK "OK:"
#define RESP_ERROR "ERROR:"
#define RESP_PROGRESS "PROGRESS:"

//==============================================================================
// TYPE DEFINITIONS
//==============================================================================

enum class SerialUpdateState {
  IDLE,
  RECEIVING,
  PROCESSING,
  SUCCESS,
  ERROR
};

struct SerialCommand {
  String command;
  String data;
};

struct UpdateProgress {
  size_t totalSize;
  size_t receivedSize;
  int percentage;
  String message;
};

//==============================================================================
// PUBLIC API FUNCTIONS
//==============================================================================

/**
 * @brief Initialize the serial system
 * @return true if initialization successful, false otherwise
 */
bool initSerial();

/**
 * @brief Handle incoming serial commands
 */
void handleSerialCommands();

/**
 * @brief Check if serial update is in progress
 * @return true if serial update is active, false if idle
 */
bool isSerialUpdateActive();

/**
 * @brief Get current serial update state
 * @return Current SerialUpdateState enumeration value
 */
SerialUpdateState getSerialUpdateState();

/**
 * @brief Abort current serial update
 */
void abortSerialUpdate();

/**
 * @brief Monitor serial update state changes and handle serial commands
 * Should be called regularly in main loop
 */
void updateSerialState();

/**
 * @brief Enable/disable verbose logging over serial
 * @param enabled True to enable verbose logging, false to disable
 */
void setSerialVerbose(bool enabled);

/**
 * @brief Clean shutdown of serial interface when leaving UPDATE_MODE
 */
void cleanupSerial();

/**
 * @brief Notify connected clients that update mode is being exited
 */
void notifyUpdateModeExit();

#endif /* SERIAL_MODULE_H */