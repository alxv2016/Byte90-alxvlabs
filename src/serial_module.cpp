/**
 * @file serial_module.cpp
 * @brief Implementation of Web Serial API integration functionality
 *
 * This module provides functions for handling firmware and filesystem updates
 * via USB serial connection using Web Serial API. Works alongside the existing
 * WiFi-based OTA system to provide an alternative update method with enhanced
 * data integrity and size validation.
 */

#include "serial_module.h"
#include "common.h"
#include "flash_module.h"
#include "ota_module.h"
#include "system_module.h"
#include "wifi_module.h"
#include <esp_ota_ops.h>

//==============================================================================
// GLOBAL VARIABLES
//==============================================================================

/** @brief Current serial update state */
static SerialUpdateState currentSerialState = SerialUpdateState::IDLE;
/** @brief Command buffer for incoming serial data */
static String commandBuffer = "";
/** @brief Current update progress tracking */
static UpdateProgress updateProgress = {0, 0, 0, ""};
/** @brief Whether verbose logging is enabled */
static bool verboseLogging = false;
/** @brief Expected firmware size for current update */
static size_t expectedFirmwareSize = 0;
/** @brief Update type (firmware or filesystem) */
static int currentUpdateCommand = U_FLASH;
/** @brief Total bytes written for size validation */
static size_t total_written = 0;

//==============================================================================
// BASE64 DECODING
//==============================================================================

/** @brief Base64 decode table for optimized decoding */
static const uint8_t base64_decode_table[256] = {
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 62,  255,
    255, 255, 63,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  255, 255,
    255, 254, 255, 255, 255, 0,   1,   2,   3,   4,   5,   6,   7,   8,   9,
    10,  11,  12,  13,  14,  15,  16,  17,  18,  19,  20,  21,  22,  23,  24,
    25,  255, 255, 255, 255, 255, 255, 26,  27,  28,  29,  30,  31,  32,  33,
    34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,  48,
    49,  50,  51,  255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255};

/**
 * @brief Decode base64 encoded string with validation
 *
 * @param input Base64 encoded string
 * @param output Buffer to store decoded data
 * @param maxOutputSize Maximum size of output buffer
 * @return Number of bytes decoded, or 0 on error
 */
static size_t simpleBase64Decode(const String &input, uint8_t *output,
                                 size_t maxOutputSize) {
  if (input.length() % 4 != 0) {
    return 0;
  }

  // Basic character validation
  for (size_t i = 0; i < input.length(); i++) {
    char c = input[i];
    if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
          (c >= '0' && c <= '9') || c == '+' || c == '/' || c == '=')) {
      return 0;
    }
  }

  size_t outputLen = 0;
  size_t inputLen = input.length();

  for (size_t i = 0; i < inputLen && outputLen < maxOutputSize - 3; i += 4) {
    uint8_t a = base64_decode_table[(uint8_t)input[i]];
    uint8_t b = base64_decode_table[(uint8_t)input[i + 1]];
    uint8_t c = base64_decode_table[(uint8_t)input[i + 2]];
    uint8_t d = base64_decode_table[(uint8_t)input[i + 3]];

    if (a == 255 || b == 255) {
      return 0;
    }

    output[outputLen++] = (a << 2) | (b >> 4);

    if (c != 254 && outputLen < maxOutputSize) {
      output[outputLen++] = (b << 4) | (c >> 2);
    }

    if (d != 254 && outputLen < maxOutputSize) {
      output[outputLen++] = (c << 6) | d;
    }
  }

  return outputLen;
}

//==============================================================================
// UTILITY FUNCTIONS
//==============================================================================

/**
 * @brief Convert bytes to human-readable format
 *
 * @param bytes Number of bytes to convert
 * @return String with human-readable size (e.g., "16MB", "234KB", "1.5GB")
 */
static String formatBytes(size_t bytes) {
  if (bytes >= 1024 * 1024 * 1024) {
    float gb = (float)bytes / (1024 * 1024 * 1024);
    return String(gb, 1) + "GB";
  } else if (bytes >= 1024 * 1024) {
    float mb = (float)bytes / (1024 * 1024);
    return String(mb, 1) + "MB";
  } else if (bytes >= 1024) {
    float kb = (float)bytes / 1024;
    return String(kb, 1) + "KB";
  } else {
    return String(bytes) + "B";
  }
}

/**
 * @brief Get string representation of current system mode
 *
 * @return String representation of the current system mode
 */
static String getSystemModeString() {
  SystemMode mode = getCurrentMode();
  
  switch (mode) {
    case SystemMode::ESP_MODE:
      return "ESP Mode";
    case SystemMode::UPDATE_MODE:
      return "Update Mode";
    case SystemMode::CRASH_MODE:
      return "Crash Mode";
    default:
      return "Unknown Mode";
  }
}

/**
 * @brief Get string representation of current serial update state
 *
 * @return String representation of the current serial update state
 */
static const char *getSerialStateString() {
  switch (currentSerialState) {
  case SerialUpdateState::IDLE:
    return "IDLE";
  case SerialUpdateState::RECEIVING:
    return "RECEIVING";
  case SerialUpdateState::PROCESSING:
    return "PROCESSING";
  case SerialUpdateState::SUCCESS:
    return "SUCCESS";
  case SerialUpdateState::ERROR:
    return "ERROR";
  default:
    return "UNKNOWN";
  }
}

/**
 * @brief Create a JSON response with current update status
 *
 * @param success Whether the operation was successful
 * @param message Status message
 * @param completed Whether the update process has completed
 * @param progress Current progress percentage
 * @return JSON string with status information
 */
static String createSerialJsonResponse(bool success, String message,
                                       bool completed = false,
                                       int progress = 0) {
  return "{\"success\":" + String(success ? "true" : "false") +
         ",\"state\":\"" + String(getSerialStateString()) + "\"" +
         ",\"progress\":" + String(progress) +
         ",\"received\":" + String(updateProgress.receivedSize) +
         ",\"total\":" + String(updateProgress.totalSize) + ",\"version\":\"" +
         String(FIRMWARE_VERSION) + "\"" + ",\"message\":\"" + message + "\"" +
         ",\"completed\":" + String(completed ? "true" : "false") + "}";
}

/**
 * @brief Create a JSON response with device information (human-readable sizes)
 *
 * @param success Whether the operation was successful
 * @param message Status message
 * @return JSON string with device information
 */
static String createDeviceInfoResponse(bool success, String message) {
  size_t flashSize = ESP.getFlashChipSize();
  size_t sketchSize = ESP.getSketchSize();
  size_t flashAvailable = flashSize - sketchSize;
  size_t freeHeap = ESP.getFreeHeap();
  StorageInfo fsInfo = updateFlashStats();

  String response =
      "{\"success\":" + String(success ? "true" : "false") + ",\"message\":\"" +
      message + "\"" + ",\"firmware_version\":\"" + String(FIRMWARE_VERSION) + "\"" + 
      ",\"mcu\":\"" + String(ESP.getChipModel()) + "\"" +
      ",\"chip_revision\":\"" + String(ESP.getChipRevision()) + "\"" +
      ",\"flash_size\":\"" + formatBytes(flashSize) + "\"" +
      ",\"flash_available\":\"" + String(fsInfo.freeSpaceMB, 2) + "MB\"" +
      ",\"free_heap\":\"" + formatBytes(freeHeap) + "\"" +
      ",\"current_mode\":\"" + getSystemModeString() + "\"";

  // Add partition information
  const esp_partition_t *running = esp_ota_get_running_partition();
  const esp_partition_t *update_partition =
      esp_ota_get_next_update_partition(NULL);

  if (running) {
    response += ",\"running_partition\":\"" + String(running->label) + "\"";
  }
  if (update_partition) {
    response +=
        ",\"update_partition\":\"" + String(update_partition->label) + "\"";
    response += ",\"update_partition_size\":\"" +
                formatBytes(update_partition->size) + "\"";
  }

  response += "}";
  return response;
}

/**
 * @brief Create a JSON response with status information
 *
 * @param success Whether the operation was successful
 * @param message Status message
 * @return JSON string with status information
 */
static String createStatusResponse(bool success, String message) {
  return "{\"success\":" + String(success ? "true" : "false") +
         ",\"message\":\"" + message + "\"" + ",\"state\":\"" +
         String(getSerialStateString()) + "\"" + ",\"system_mode\":\"" +
         getSystemModeString() +
         "\"" + ",\"wifi_connected\":" +
         String((WiFi.status() == WL_CONNECTED) ? "true" : "false") +
         ",\"update_active\":" +
         String((currentSerialState != SerialUpdateState::IDLE) ? "true"
                                                                : "false") +
         ",\"progress\":" + String(updateProgress.percentage) +
         ",\"received\":" + String(updateProgress.receivedSize) +
         ",\"total\":" + String(updateProgress.totalSize) + "}";
}

/**
 * @brief Send JSON response over serial
 *
 * @param jsonResponse Pre-formatted JSON response string
 * @param isError Whether this is an error response
 */
static void sendSerialResponse(const String &jsonResponse,
                               bool isError = false) {
  Serial.println((isError ? RESP_ERROR : RESP_OK) + jsonResponse);
}

/**
 * @brief Send progress update over serial
 *
 * @param percentage Progress percentage (0-100)
 * @param message Status message
 */
static void sendProgressUpdate(int percentage, const String &message) {
  String jsonResponse = createSerialJsonResponse(
      (currentSerialState != SerialUpdateState::ERROR), message,
      (currentSerialState == SerialUpdateState::SUCCESS), percentage);
  Serial.println(RESP_PROGRESS + jsonResponse);
}

/**
 * @brief Parse incoming serial command
 *
 * @param line Raw command line
 * @return Parsed SerialCommand structure
 */
static SerialCommand parseCommand(const String &line) {
  SerialCommand cmd;
  int colonPos = line.indexOf(':');

  if (colonPos != -1) {
    cmd.command = line.substring(0, colonPos);
    cmd.data = line.substring(colonPos + 1);
    cmd.command.trim();
    cmd.data.trim();
  } else {
    cmd.command = line;
    cmd.command.trim();
  }

  return cmd;
}

//==============================================================================
// COMMAND HANDLERS
//==============================================================================

/**
 * @brief Handle GET_INFO command
 *
 * Retrieves and sends device information including firmware version,
 * chip details, and partition information.
 */
static void handleGetInfo() {
  String jsonResponse = createDeviceInfoResponse(true, "Device information");
  sendSerialResponse(jsonResponse);
}

/**
 * @brief Handle GET_STATUS command
 *
 * Retrieves and sends current device status including system mode,
 * WiFi connection, and update progress.
 */
static void handleGetStatus() {
  String jsonResponse = createStatusResponse(true, "Device status");
  sendSerialResponse(jsonResponse);
}

/**
 * @brief Initialize file upload for serial update
 *
 * @param cmd Command with firmware size and type parameters
 * @return true if initialization was successful
 */
static bool initializeSerialUpdate(const SerialCommand &cmd) {
  // Parse parameters: size,type (e.g., "1048576,firmware" or
  // "3087360,filesystem")
  int commaPos = cmd.data.indexOf(',');
  if (commaPos == -1) {
    String jsonResponse = createSerialJsonResponse(
        false, "Invalid START_UPDATE format. Expected: size,type");
    sendSerialResponse(jsonResponse, true);
    return false;
  }

  String sizeStr = cmd.data.substring(0, commaPos);
  String typeStr = cmd.data.substring(commaPos + 1);

  expectedFirmwareSize = sizeStr.toInt();

  // Determine update type and size limits
  size_t maxAllowedSize;
  if (typeStr == "filesystem") {
    currentUpdateCommand = U_SPIFFS;
    maxAllowedSize = 3 * 1024 * 1024; // 3MB for SPIFFS partition
  } else if (typeStr == "firmware") {
    currentUpdateCommand = U_FLASH;
    maxAllowedSize = 1536 * 1024; // 1.5MB for OTA partitions
  } else {
    String jsonResponse = createSerialJsonResponse(
        false, "Invalid update type. Expected: firmware or filesystem");
    sendSerialResponse(jsonResponse, true);
    return false;
  }

  // Validate size against type-specific limits
  if (expectedFirmwareSize == 0) {
    String jsonResponse = createSerialJsonResponse(false, "Invalid file size");
    sendSerialResponse(jsonResponse, true);
    return false;
  }

  if (expectedFirmwareSize < 1024 || expectedFirmwareSize > maxAllowedSize) {
    String jsonResponse = createSerialJsonResponse(
        false, "File size out of range (1KB - " + formatBytes(maxAllowedSize) +
                   " for " + typeStr + ")");
    sendSerialResponse(jsonResponse, true);
    return false;
  }

  // Validate partition space (keep existing partition check)
  const esp_partition_t *update_partition =
      (currentUpdateCommand == U_SPIFFS)
          ? esp_partition_find_first(ESP_PARTITION_TYPE_DATA,
                                     ESP_PARTITION_SUBTYPE_DATA_SPIFFS, NULL)
          : esp_ota_get_next_update_partition(NULL);

  if (!update_partition || update_partition->size < expectedFirmwareSize) {
    String jsonResponse = createSerialJsonResponse(
        false, "File too large for available partition");
    sendSerialResponse(jsonResponse, true);
    return false;
  }

  // Initialize update
  if (!Update.begin(expectedFirmwareSize, currentUpdateCommand)) {
    String jsonResponse = createSerialJsonResponse(
        false, "Failed to initialize update: " + String(Update.errorString()));
    sendSerialResponse(jsonResponse, true);
    return false;
  }

  return true;
}

/**
 * @brief Handle START_UPDATE command
 *
 * Initializes firmware update process with size and type validation.
 *
 * @param cmd Command with firmware size and type parameters
 */
static void handleStartUpdate(const SerialCommand &cmd) {
  // If already in progress, abort and reset
  if (currentSerialState != SerialUpdateState::IDLE) {
    if (Update.isRunning()) {
      Update.abort();
    }
    currentSerialState = SerialUpdateState::IDLE;
    updateProgress = {0, 0, 0, ""};
  }

  if (!initializeSerialUpdate(cmd)) {
    return;
  }

  // Reset progress tracking
  updateProgress.totalSize = expectedFirmwareSize;
  updateProgress.receivedSize = 0;
  updateProgress.percentage = 0;
  updateProgress.message = "Update started";
  total_written = 0;

  currentSerialState = SerialUpdateState::RECEIVING;

  String jsonResponse = createSerialJsonResponse(
      true, "Update initialized. Ready to receive data.");
  sendSerialResponse(jsonResponse);
  sendProgressUpdate(0, "Ready to receive firmware data");
}

/**
 * @brief Handle the writing of uploaded data during serial update
 *
 * @param cmd Command containing base64-encoded firmware chunk
 * @return Current progress percentage
 */
static int handleChunkWrite(const SerialCommand &cmd) {
  static uint8_t decodedBuffer[2048];
  size_t decodedSize =
      simpleBase64Decode(cmd.data, decodedBuffer, sizeof(decodedBuffer));

  if (decodedSize == 0) {
    Serial.println("ERROR:{\"success\":false,\"message\":\"Decode failed\"}");
    return -1;
  }

  // Size overflow protection
  if (total_written + decodedSize > expectedFirmwareSize) {
    currentSerialState = SerialUpdateState::ERROR;
    Update.abort();
    Serial.println(
        "ERROR:{\"success\":false,\"message\":\"Data exceeds expected size\"}");
    return -1;
  }

  size_t written = Update.write(decodedBuffer, decodedSize);
  if (written != decodedSize) {
    currentSerialState = SerialUpdateState::ERROR;
    Update.abort();
    Serial.println(
        "ERROR:{\"success\":false,\"message\":\"Flash write failed\"}");
    return -1;
  }

  total_written += written;
  updateProgress.receivedSize += written;
  updateProgress.percentage =
      (updateProgress.receivedSize * 100) / updateProgress.totalSize;

  return updateProgress.percentage;
}

/**
 * @brief Handle SEND_CHUNK command
 *
 * Processes incoming firmware data chunks with size validation.
 *
 * @param cmd Command containing base64-encoded firmware chunk
 */
static void handleSendChunk(const SerialCommand &cmd) {
  if (currentSerialState != SerialUpdateState::RECEIVING) {
    Serial.println("ERROR:{\"success\":false,\"message\":\"Not receiving\"}");
    return;
  }

  if (cmd.data.length() == 0) {
    Serial.println("ERROR:{\"success\":false,\"message\":\"Empty chunk\"}");
    return;
  }

  int progress = handleChunkWrite(cmd);
  if (progress < 0) {
    return; // Error already handled in handleChunkWrite
  }

  Serial.println("OK:{\"success\":true}");

  // Progress updates only every 10%
  static int lastPercent = -1;
  if (updateProgress.percentage >= lastPercent + 10) {
    lastPercent = updateProgress.percentage;
    sendProgressUpdate(updateProgress.percentage, "Uploading firmware...");
  }
}

/**
 * @brief Finalize a serial update after upload is complete
 *
 * @return true if update was successfully finalized
 */
static bool finalizeSerialUpdate() {
  // Verify expected size was received
  if (total_written != expectedFirmwareSize) {
    currentSerialState = SerialUpdateState::ERROR;
    Update.abort();
    String jsonResponse = createSerialJsonResponse(
        false, "Size mismatch - Expected: " + String(expectedFirmwareSize) +
                   ", Received: " + String(total_written));
    sendSerialResponse(jsonResponse, true);
    ESP_LOGE(SERIAL_LOG, "Size mismatch - Expected: %d, Received: %d",
             expectedFirmwareSize, total_written);
    return false;
  }

  if (Update.end(true)) {
    currentSerialState = SerialUpdateState::SUCCESS;
    updateProgress.message = "Update completed successfully";
    return true;
  } else {
    currentSerialState = SerialUpdateState::ERROR;
    String jsonResponse = createSerialJsonResponse(
        false, "Update failed: " + String(Update.errorString()));
    sendSerialResponse(jsonResponse, true);
    Update.abort();
    return false;
  }
}

/**
 * @brief Handle FINISH_UPDATE command
 *
 * Finalizes the update process after verifying all data was received.
 */
static void handleFinishUpdate() {
  if (currentSerialState != SerialUpdateState::RECEIVING) {
    String jsonResponse =
        createSerialJsonResponse(false, "Not in receiving state");
    sendSerialResponse(jsonResponse, true);
    return;
  }

  currentSerialState = SerialUpdateState::PROCESSING;
  sendProgressUpdate(100, "Finalizing update...");

  if (finalizeSerialUpdate()) {
    String jsonResponse = createSerialJsonResponse(
        true, "Update completed successfully. Device will restart.", true, 100);
    sendSerialResponse(jsonResponse);
    delay(1000);
    ESP.restart();
  }
}

/**
 * @brief Handle ABORT_UPDATE command
 *
 * Safely cancels any ongoing update process and resets state.
 */
static void handleAbortUpdate() {
  if (currentSerialState == SerialUpdateState::IDLE) {
    String jsonResponse =
        createSerialJsonResponse(true, "No update in progress");
    sendSerialResponse(jsonResponse);
    return;
  }

  Update.abort();
  currentSerialState = SerialUpdateState::IDLE;
  updateProgress = {0, 0, 0, "Update aborted"};

  String jsonResponse = createSerialJsonResponse(true, "Update aborted");
  sendSerialResponse(jsonResponse);
}

/**
 * @brief Handle RESTART command
 *
 * Restarts the device after sending confirmation response.
 */
static void handleRestart() {
  String jsonResponse = createSerialJsonResponse(true, "Restarting device...");
  sendSerialResponse(jsonResponse);
  delay(1000);
  ESP.restart();
}

/**
 * @brief Handle GET_LOGS command
 *
 * Returns the current verbose logging status.
 */
static void handleGetLogs() {
  String jsonResponse = createSerialJsonResponse(
      true,
      "Verbose logging " + String(verboseLogging ? "enabled" : "disabled"));
  sendSerialResponse(jsonResponse);
}

//==============================================================================
// SERIAL COMMAND PROCESSING
//==============================================================================

/**
 * @brief Process a complete command line
 *
 * @param commandBuffer Complete command string to process
 */
static void processCommand(const String &commandBuffer) {
  SerialCommand cmd = parseCommand(commandBuffer);

  if (cmd.command == CMD_GET_INFO) {
    handleGetInfo();
  } else if (cmd.command == CMD_GET_STATUS) {
    handleGetStatus();
  } else if (cmd.command == CMD_START_UPDATE) {
    handleStartUpdate(cmd);
  } else if (cmd.command == CMD_SEND_CHUNK) {
    handleSendChunk(cmd);
  } else if (cmd.command == CMD_FINISH_UPDATE) {
    handleFinishUpdate();
  } else if (cmd.command == CMD_ABORT_UPDATE) {
    handleAbortUpdate();
  } else if (cmd.command == CMD_RESTART) {
    handleRestart();
  } else if (cmd.command == CMD_GET_LOGS) {
    handleGetLogs();
  } else if (cmd.command == "VERBOSE") {
    verboseLogging = (cmd.data == "1" || cmd.data.equalsIgnoreCase("true"));
    String jsonResponse = createSerialJsonResponse(
        true,
        "Verbose logging " + String(verboseLogging ? "enabled" : "disabled"));
    sendSerialResponse(jsonResponse);
  } else {
    String jsonResponse =
        createSerialJsonResponse(false, "Unknown command: " + cmd.command);
    sendSerialResponse(jsonResponse, true);
  }
}

//==============================================================================
// PUBLIC API IMPLEMENTATION
//==============================================================================

/**
 * @brief Initialize the serial system
 *
 * Sets up serial communication and initializes state variables.
 *
 * @return true if initialization was successful
 */
bool initSerial() {
  Serial.setRxBufferSize(8192); // Increased from 4096
  Serial.setTxBufferSize(4096); // Increased from 2048
  Serial.begin(SERIAL_BAUD_RATE);

  esp_log_level_set("*", ESP_LOG_NONE);

  // Wait for serial connection (optional, mainly for debugging)
  unsigned long startTime = millis();
  while (!Serial && (millis() - startTime < 2000)) {
    delay(10);
  }

  currentSerialState = SerialUpdateState::IDLE;
  commandBuffer = "";
  verboseLogging = false;

  ESP_LOGI(SERIAL_LOG, "Serial interface initialized at %d baud",
           SERIAL_BAUD_RATE);

  String jsonResponse =
      createSerialJsonResponse(true, "BYTE-90 Serial Interface Ready");
  sendSerialResponse(jsonResponse);

  return true;
}

/**
 * @brief Handle incoming serial commands
 *
 * Processes incoming serial data and executes commands.
 * Should be called regularly in main loop.
 */
void handleSerialCommands() {
  int processed = 0;
  while (Serial.available() && processed < 128) {
    char c = Serial.read();
    processed++;

    if (c == '\n' || c == '\r') {
      if (commandBuffer.length() > 0) {
        processCommand(commandBuffer);
        commandBuffer = "";
      }
    } else if (c != '\r') {
      commandBuffer += c;

      // Buffer overflow protection
      if (commandBuffer.length() > SERIAL_COMMAND_BUFFER_SIZE) {
        commandBuffer = "";
        String jsonResponse =
            createSerialJsonResponse(false, "Command too long");
        sendSerialResponse(jsonResponse, true);
      }
    }
  }
}

/**
 * @brief Clean shutdown of serial interface when leaving UPDATE_MODE
 *
 * Gracefully stops serial command processing and resets state.
 * This function should be called by system module during mode transitions.
 */
void cleanupSerial() {
  if (currentSerialState != SerialUpdateState::IDLE) {
    ESP_LOGI(SERIAL_LOG,
             "Aborting active serial update during mode transition");
    abortSerialUpdate();
  }

  currentSerialState = SerialUpdateState::IDLE;
  commandBuffer = "";
  verboseLogging = false;
  updateProgress = {0, 0, 0, ""};

  ESP_LOGI(SERIAL_LOG, "Serial interface cleaned up for mode transition");
}

bool isSerialUpdateActive() {
  return currentSerialState != SerialUpdateState::IDLE;
}

SerialUpdateState getSerialUpdateState() { return currentSerialState; }

void abortSerialUpdate() { handleAbortUpdate(); }

void setSerialVerbose(bool enabled) { verboseLogging = enabled; }