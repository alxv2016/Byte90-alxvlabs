/**
 * @file serial_module.cpp - Part 1: Headers, Constants & Utilities
 * @brief Implementation of Web Serial API integration for firmware and
 * filesystem updates
 */

#include "serial_module.h"
#include "common.h"
#include "flash_module.h"
#include "ota_module.h"
#include "preferences_module.h"
#include "states_module.h"
#include "wifi_endpoints.h"
#include "wifi_module.h"
#include <esp_ota_ops.h>

//==============================================================================
// GLOBAL VARIABLES
//==============================================================================

static SerialUpdateState currentSerialState = SerialUpdateState::IDLE;
static String commandBuffer = "";
static UpdateProgress updateProgress = {0, 0, 0, ""};
static bool verboseLogging = false;
static size_t expectedFirmwareSize = 0;
static int currentUpdateCommand = U_FLASH;
static size_t total_written = 0;

//==============================================================================
// CONSTANTS & DEFINITIONS
//==============================================================================

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

//==============================================================================
// UTILITY FUNCTIONS (STATIC)
//==============================================================================

/**
 * @brief Decode base64 encoded string with validation
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

// formatBytes() moved to common.cpp

/**
 * @brief Get string representation of current serial update state
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
 * @brief Parse incoming serial command
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

/**
 * @brief Send JSON response over serial
 * @param jsonResponse Pre-formatted JSON response string
 * @param isError Whether this is an error response
 */
static void sendSerialResponse(const String &jsonResponse,
                               bool isError = false) {
  Serial.println((isError ? RESP_ERROR : RESP_OK) + jsonResponse);
}

/**
 * @brief Forward declaration of createSerialJsonResponse
 */
static String createSerialJsonResponse(bool success, const String &message,
                                       bool completed = false,
                                       int progress = 0);

/**
 * @brief Send progress update over serial
 * @param percentage Progress percentage
 * @param message Status message
 */
static void sendProgressUpdate(int percentage, const String &message) {
  String jsonResponse = createSerialJsonResponse(
      (currentSerialState != SerialUpdateState::ERROR), message,
      (currentSerialState == SerialUpdateState::SUCCESS), percentage);
  Serial.println(RESP_PROGRESS + jsonResponse);
}

/**
 * @file serial_module.cpp - Part 2: JSON Response Functions
 * @brief JSON response creation functions for different command types
 */

//==============================================================================
// JSON RESPONSE FUNCTIONS (STATIC)
//==============================================================================

/**
 * @brief Create a JSON response with current update status
 * @param success Whether the operation was successful
 * @param message Status message
 * @param completed Whether the update process has completed
 * @param progress Current progress percentage
 * @return JSON string with status information
 */
static String createSerialJsonResponse(bool success, const String &message,
                                       bool completed, int progress) {
  // Pre-calculate approximate size
  const char *stateStr = getSerialStateString();
  size_t estimatedSize =
      150 + message.length() + strlen(stateStr) + strlen(FIRMWARE_VERSION);

  String response;
  response.reserve(estimatedSize);

  response += "{\"success\":";
  response += success ? "true" : "false";
  response += ",\"state\":\"";
  response += stateStr;
  response += "\",\"progress\":";
  response += progress;
  response += ",\"received\":";
  response += updateProgress.receivedSize;
  response += ",\"total\":";
  response += updateProgress.totalSize;
  response += ",\"version\":\"";
  response += FIRMWARE_VERSION;
  response += "\",\"message\":\"";
  response += message;
  response += "\",\"completed\":";
  response += completed ? "true" : "false";
  response += "}";

  return response;
}

/**
 * @brief Create a JSON response with device information
 */
static String createDeviceInfoResponse(bool success, const String &message) {
  size_t flashSize = ESP.getFlashChipSize();
  size_t sketchSize = ESP.getSketchSize();
  size_t freeHeap = ESP.getFreeHeap();
  StorageInfo fsInfo = getDetailedFlashStats();

  // Get partition info
  const esp_partition_t *running = esp_ota_get_running_partition();
  const esp_partition_t *update_partition =
      esp_ota_get_next_update_partition(NULL);

  // Pre-format variable strings
  String flashSizeStr = formatBytes(flashSize);
  String freeHeapStr = formatBytes(freeHeap);
  String freeSpaceStr = String(fsInfo.freeSpaceMB, 2) + "MB";
  String chipModelStr = ESP.getChipModel();
  String chipRevisionStr = String(ESP.getChipRevision());
  String currentModeStr = getCurrentState() == SystemState::UPDATE_MODE
                              ? "Update Mode"
                              : "Standby Mode";

  // Calculate estimated size
  size_t estimatedSize = 300 + message.length() + strlen(FIRMWARE_VERSION) +
                         flashSizeStr.length() + freeHeapStr.length() +
                         freeSpaceStr.length() + chipModelStr.length() +
                         chipRevisionStr.length() + currentModeStr.length();

  if (running)
    estimatedSize += strlen(running->label) + 30;
  if (update_partition)
    estimatedSize += strlen(update_partition->label) + 60;

  String response;
  response.reserve(estimatedSize);

  response += "{\"success\":";
  response += success ? "true" : "false";
  response += ",\"message\":\"";
  response += message;
  response += "\",\"firmware_version\":\"";
  response += FIRMWARE_VERSION;
  response += "\",\"mcu\":\"";
  response += chipModelStr;
  response += "\",\"chip_revision\":\"";
  response += chipRevisionStr;
  response += "\",\"flash_size\":\"";
  response += flashSizeStr;
  response += "\",\"flash_available\":\"";
  response += freeSpaceStr;
  response += "\",\"free_heap\":\"";
  response += freeHeapStr;
  response += "\",\"current_mode\":\"";
  response += currentModeStr;
  response += "\"";

  if (running) {
    response += ",\"running_partition\":\"";
    response += running->label;
    response += "\"";
  }

  if (update_partition) {
    response += ",\"update_partition\":\"";
    response += update_partition->label;
    response += "\",\"update_partition_size\":\"";
    response += formatBytes(update_partition->size);
    response += "\"";
  }

  response += "}";
  return response;
}

/**
 * @brief Create a JSON response with status information
 */
static String createStatusResponse(bool success, const String &message) {
  // Pre-format variable strings
  const char *serialStateStr = getSerialStateString();
  String systemModeStr = getCurrentState() == SystemState::UPDATE_MODE
                             ? "Update Mode"
                             : "Standby Mode";

  // Calculate estimated size
  size_t estimatedSize =
      200 + message.length() + strlen(serialStateStr) + systemModeStr.length();

  String response;
  response.reserve(estimatedSize);

  response += "{\"success\":";
  response += success ? "true" : "false";
  response += ",\"message\":\"";
  response += message;
  response += "\",\"state\":\"";
  response += serialStateStr;
  response += "\",\"system_mode\":\"";
  response += systemModeStr;
  response += "\",\"wifi_connected\":";
  response += isWifiNetworkConnected() ? "true" : "false";
  response += "\",\"update_active\":";
  response +=
      (currentSerialState != SerialUpdateState::IDLE) ? "true" : "false";
  response += ",\"progress\":";
  response += updateProgress.percentage;
  response += ",\"received\":";
  response += updateProgress.receivedSize;
  response += ",\"total\":";
  response += updateProgress.totalSize;
  response += "}";

  return response;
}

/**
 * @brief Create a JSON response for WiFi saved credentials
 */
static String createWiFiCredentialsResponse(bool success, const String &message,
                                            const String &ssid = "",
                                            bool hasCredentials = false) {
  // Calculate estimated size
  size_t estimatedSize = 120 + message.length() + ssid.length();

  String response;
  response.reserve(estimatedSize);

  response += "{\"success\":";
  response += success ? "true" : "false";
  response += ",\"message\":\"";
  response += message;
  response += "\",\"ssid\":\"";
  response += ssid;
  response += "\",\"has_credentials\":";
  response += hasCredentials ? "true" : "false";
  response += "\"}";

  return response;
}

/**
 * @brief Create a serial-friendly string containing all preferences for JSON
 * responses
 * @return String containing all preference values formatted for serial output
 */
static String createPreferencesString(void) {
  String result = "=== USER PREFERENCES ===\n";

  // Audio preferences using preferences module
  result +=
      "Audio: enabled=" + String(getAudioEnabled() ? "true" : "false") + "\n";

  // System preferences using preferences module and states module
  bool wifiEnabled = (getCurrentState() == SystemState::WIFI_MODE);
  char ssid[32] = {0};
  char password[64] = {0};
  bool wifiSaved = loadWiFiCredentials(ssid, password);

  result += "System: wifi=" + String(wifiEnabled ? "true" : "false");
  result += ", wifiSaved=" + String(wifiSaved ? "true" : "false");
  result += ", wifiAutoConnect=" + String("true"); // Default to true for now
  if (wifiSaved) {
    result += ", ssid=" + String(ssid) + ", password=***HIDDEN***";
  }
  result += "\n";

  // Visual effects preferences using preferences module
  result += "Effects: glitch=" + String(getGlitchEnabled() ? "true" : "false");
  result += ", scanlines=" + String(getScanlinesEnabled() ? "true" : "false");
  result += ", dithering=" + String(getDitheringEnabled() ? "true" : "false");
  result += ", chromatic=" + String(getChromaticEnabled() ? "true" : "false");
  result += ", dotMatrix=" + String(getDotMatrixEnabled() ? "true" : "false");
  result += ", pixelate=" + String(getPixelateEnabled() ? "true" : "false");
  result += ", tint=" + String(getTintEnabled() ? "true" : "false");
  result += ", tintColor=0x" + String(getTintColor(), HEX);
  result += ", tintIntensity=" + String(getTintIntensity(), 2) + "\n";

  // Haptic preferences using preferences module
  result +=
      "User: haptic=" + String(getHapticEnabled() ? "true" : "false") + "\n";

  return result;
}

/**
 * @file serial_module.cpp - Part 3: Device Info & Status Commands
 * @brief Handlers for device information and status commands
 */

//==============================================================================
// DEVICE INFO & STATUS COMMAND HANDLERS (STATIC)
//==============================================================================

/**
 * @brief Handle GET_INFO command
 */
static void handleGetInfo() {
  String jsonResponse = createDeviceInfoResponse(true, "Device information");
  sendSerialResponse(jsonResponse);
}

/**
 * @brief Handle GET_STATUS command
 */
static void handleGetStatus() {
  String jsonResponse = createStatusResponse(true, "Device status");
  sendSerialResponse(jsonResponse);
}

/**
 * @brief Handle RESTART command
 */
static void handleRestart() {
  String jsonResponse = createSerialJsonResponse(true, "Restarting device...");
  sendSerialResponse(jsonResponse);
  delay(1000);
  ESP.restart();
}

/**
 * @brief Handle GET_LOGS command
 */
static void handleGetLogs() {
  String jsonResponse = createSerialJsonResponse(
      true,
      "Verbose logging " + String(verboseLogging ? "enabled" : "disabled"));
  sendSerialResponse(jsonResponse);
}

/**
 * @brief Handle VERBOSE command
 * @param cmd Command with verbose setting data
 */
static void handleVerbose(const SerialCommand &cmd) {
  verboseLogging = (cmd.data == "1" || cmd.data.equalsIgnoreCase("true"));
  String jsonResponse = createSerialJsonResponse(
      true,
      "Verbose logging " + String(verboseLogging ? "enabled" : "disabled"));
  sendSerialResponse(jsonResponse);
}

/**
 * @file serial_module.cpp - Part 4: WiFi Command Handlers
 * @brief Handlers for WiFi configuration and management commands
 */

//==============================================================================
// WIFI COMMAND HANDLERS (STATIC)
//==============================================================================

/**
 * @brief Handle WIFI_SCAN command
 */
static void handleWiFiScan() {
  if (isSerialUpdateActive()) {
    String jsonResponse = createSerialJsonResponse(
        false, "Cannot scan WiFi during firmware update");
    sendSerialResponse(jsonResponse, true);
    return;
  }

  String jsonResponse = scanWiFiNetworks();
  sendSerialResponse(jsonResponse);
}

/**
 * @brief Handle WIFI_STATUS command
 */
static void handleWiFiStatus() {
  String jsonResponse = getWiFiStatusJson();
  sendSerialResponse(jsonResponse);
}

/**
 * @brief Handle WIFI_CONNECT command
 * @param cmd Command with SSID and password data (format: "ssid,password")
 */
/**
 * @brief Handle WIFI_CONNECT command
 * @param cmd Command with SSID and password data (format: "ssid,password")
 */
static void handleWiFiConnect(const SerialCommand &cmd) {
  ESP_LOGE(SERIAL_LOG, "=== SERIAL WIFI CONNECT ===");
  ESP_LOGE(SERIAL_LOG, "Raw command data: '%s'", cmd.data.c_str());
  
  if (isSerialUpdateActive()) {
    String jsonResponse = createSerialJsonResponse(false, "Cannot connect to WiFi during firmware update");
    sendSerialResponse(jsonResponse, true);
    return;
  }

  int commaPos = cmd.data.indexOf(',');
  if (commaPos == -1) {
    ESP_LOGE(SERIAL_LOG, "Invalid command format - no comma found");
    String jsonResponse = createSerialJsonResponse(false, "Invalid WIFI_CONNECT format. Expected: ssid,password");
    sendSerialResponse(jsonResponse, true);
    return;
  }

  String ssid = cmd.data.substring(0, commaPos);
  String password = cmd.data.substring(commaPos + 1);
  
  ssid.trim();
  password.trim();
  
  ESP_LOGE(SERIAL_LOG, "Parsed SSID: '%s', Password: '%s' (length: %d)", 
           ssid.c_str(), password.c_str(), password.length());

  if (ssid.isEmpty()) {
    ESP_LOGE(SERIAL_LOG, "SSID is empty after parsing");
    String jsonResponse = createSerialJsonResponse(false, "SSID cannot be empty");
    sendSerialResponse(jsonResponse, true);
    return;
  }

  // Check if already connected to this network
  if (isWifiNetworkConnected() && WiFi.SSID().equals(ssid)) {
    ESP_LOGE(SERIAL_LOG, "Already connected to requested network");
    
    // Save credentials since we're connected to this network
    if (saveWiFiCredentials(ssid.c_str(), password.c_str())) {
      ESP_LOGE(SERIAL_LOG, "Credentials saved successfully");
    }
    
    String jsonResponse = createSerialJsonResponse(true, "Already connected to " + ssid);
    sendSerialResponse(jsonResponse);
    return;
  }

  // Disconnect from any current network first
  if (isWifiNetworkConnected()) {
    ESP_LOGE(SERIAL_LOG, "Disconnecting from current network first");
    disconnectFromWiFi();
    delay(1000); // Give time for disconnection
  }

  // Start connection attempt
  ESP_LOGE(SERIAL_LOG, "Attempting WiFi connection with provided credentials...");
  connectToWiFi(ssid.c_str(), password.c_str());

  // Wait for connection result with timeout
  unsigned long startTime = millis();
  const unsigned long TIMEOUT_MS = 20000; // 20 seconds timeout
  const unsigned long CHECK_INTERVAL = 1000; // Check every second
  
  while (millis() - startTime < TIMEOUT_MS) {
    delay(CHECK_INTERVAL);
    
    if (isWifiNetworkConnected()) {
      ESP_LOGE(SERIAL_LOG, "Connection successful");
      
      // Save credentials on successful connection
      if (saveWiFiCredentials(ssid.c_str(), password.c_str())) {
        ESP_LOGE(SERIAL_LOG, "Credentials saved successfully");
      } else {
        ESP_LOGW(SERIAL_LOG, "Failed to save credentials, but connection successful");
      }
      
      String jsonResponse = createSerialJsonResponse(true, "Successfully connected to " + ssid);
      sendSerialResponse(jsonResponse);
      return;
    }
    
    ESP_LOGE(SERIAL_LOG, "Still waiting for connection... (%lu ms elapsed)", millis() - startTime);
  }
  
  // Timeout reached without successful connection
  ESP_LOGE(SERIAL_LOG, "Connection timeout reached after %lu ms", TIMEOUT_MS);
  String jsonResponse = createSerialJsonResponse(false, "Connection failed: timeout or incorrect password");
  sendSerialResponse(jsonResponse, true);
}

/**
 * @brief Handle WIFI_DISCONNECT command
 */
static void handleWiFiDisconnect() {
  if (isSerialUpdateActive()) {
    String jsonResponse = createSerialJsonResponse(
        false, "Cannot disconnect WiFi during firmware update");
    sendSerialResponse(jsonResponse, true);
    return;
  }

  String currentSSID = WiFi.SSID();
  bool wasConnected = isWifiNetworkConnected();

  // Disconnect from WiFi
  disconnectFromWiFi();

  // Check if disconnection was successful
  bool isNowDisconnected = !isWifiNetworkConnected();

  if (isNowDisconnected) {
    String message = wasConnected ? "Disconnected from " + currentSSID
                                  : "WiFi was already disconnected";
    String jsonResponse = createSerialJsonResponse(true, message);
    sendSerialResponse(jsonResponse);
  } else {
    String jsonResponse =
        createSerialJsonResponse(false, "Failed to disconnect from WiFi");
    sendSerialResponse(jsonResponse, true);
  }
}

/**
 * @brief Handle WIFI_GET_SAVED command
 */
static void handleWiFiGetSaved() {
  char ssid[32] = {0};
  char password[64] = {0};
  bool hasCredentials = loadWiFiCredentials(ssid, password);

  if (hasCredentials) {
    String message = "Saved credentials found for network: " + String(ssid);
    String jsonResponse =
        createWiFiCredentialsResponse(true, message, String(ssid), true);
    sendSerialResponse(jsonResponse);
  } else {
    String jsonResponse = createWiFiCredentialsResponse(
        true, "No saved WiFi credentials found", "", false);
    sendSerialResponse(jsonResponse);
  }
}

/**
 * @brief Handle WIFI_FORGET command
 */
static void handleWiFiForget() {
  if (isSerialUpdateActive()) {
    String jsonResponse = createSerialJsonResponse(
        false, "Cannot clear WiFi credentials during firmware update");
    sendSerialResponse(jsonResponse, true);
    return;
  }

  char ssid[32] = {0};
  char password[64] = {0};
  bool hadCredentials = loadWiFiCredentials(ssid, password);

  // Use preferences module to clear WiFi credentials
  clearWiFiCredentials();

  String message = hadCredentials
                       ? "Cleared saved credentials for " + String(ssid)
                       : "No credentials were saved";
  String jsonResponse = createWiFiCredentialsResponse(true, message, "", false);
  sendSerialResponse(jsonResponse);
}

/**
 * @file serial_module.cpp - Part 5: Firmware Update Commands
 * @brief Handlers for firmware and filesystem update commands
 */

//==============================================================================
// FIRMWARE UPDATE COMMAND HANDLERS (STATIC)
//==============================================================================

/**
 * @brief Initialize file upload for serial update
 * @param cmd Command with firmware size and type parameters
 * @return true if initialization was successful
 */
static bool initializeSerialUpdate(const SerialCommand &cmd) {
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

  size_t maxAllowedSize;
  if (typeStr == "filesystem") {
    currentUpdateCommand = U_SPIFFS;
    maxAllowedSize = 3 * 1024 * 1024;
  } else if (typeStr == "firmware") {
    currentUpdateCommand = U_FLASH;
    maxAllowedSize = 1536 * 1024;
  } else {
    String jsonResponse = createSerialJsonResponse(
        false, "Invalid update type. Expected: firmware or filesystem");
    sendSerialResponse(jsonResponse, true);
    return false;
  }

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
 * @param cmd Command with firmware size and type parameters
 */
static void handleStartUpdate(const SerialCommand &cmd) {
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
    return;
  }

  Serial.println("OK:{\"success\":true}");

  static int lastPercent = -1;
  if (updateProgress.percentage >= lastPercent + 10) {
    lastPercent = updateProgress.percentage;
    sendProgressUpdate(updateProgress.percentage, "Uploading firmware...");
  }
}

/**
 * @brief Finalize a serial update after upload is complete
 * @return true if update was successfully finalized
 */
static bool finalizeSerialUpdate() {
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
 * @brief Handle GET_PREFERENCES command - shows all current preferences
 */
static void handleGetPreferences() {
  String prefsExport = createPreferencesString();

  // Convert the multi-line preferences export to a JSON-safe format
  prefsExport.replace("\n", "\\n");
  prefsExport.replace("\"", "\\\"");

  String jsonResponse = "{\"success\":true,\"message\":\"Current "
                        "preferences\",\"preferences\":\"" +
                        prefsExport + "\"}";
  sendSerialResponse(jsonResponse);
}

/**
 * @brief Handle RESET_PREFERENCES command - resets all preferences to factory
 * defaults
 */
static void handleResetPreferences() {
  if (isSerialUpdateActive()) {
    String jsonResponse = createSerialJsonResponse(
        false, "Cannot reset preferences during firmware update");
    sendSerialResponse(jsonResponse, true);
    return;
  }

  // Use preferences module to clear all preferences
  clearAllPreferences();
  String jsonResponse = createSerialJsonResponse(
      true, "All preferences reset to factory defaults");
  sendSerialResponse(jsonResponse);
}

//==============================================================================
// MAIN COMMAND PROCESSING (STATIC)
//==============================================================================

/**
 * @brief Process a complete command line
 * @param commandBuffer Complete command string to process
 */
static void processCommand(const String &commandBuffer) {
  SerialCommand cmd = parseCommand(commandBuffer);

  // Device Information Commands
  if (cmd.command == CMD_GET_INFO) {
    handleGetInfo();
  } else if (cmd.command == CMD_GET_STATUS) {
    handleGetStatus();
  } else if (cmd.command == CMD_RESTART) {
    handleRestart();
  } else if (cmd.command == CMD_GET_LOGS) {
    handleGetLogs();
  } else if (cmd.command == CMD_GET_PREFERENCES) {
    handleGetPreferences();
  } else if (cmd.command == CMD_RESET_PREFERENCES) {
    handleResetPreferences();
  }

  // Firmware Update Commands
  else if (cmd.command == CMD_START_UPDATE) {
    handleStartUpdate(cmd);
  } else if (cmd.command == CMD_SEND_CHUNK) {
    handleSendChunk(cmd);
  } else if (cmd.command == CMD_FINISH_UPDATE) {
    handleFinishUpdate();
  } else if (cmd.command == CMD_ABORT_UPDATE) {
    handleAbortUpdate();
  }

  // WiFi Configuration Commands
  else if (cmd.command == CMD_WIFI_SCAN) {
    handleWiFiScan();
  } else if (cmd.command == CMD_WIFI_STATUS) {
    handleWiFiStatus();
  } else if (cmd.command == CMD_WIFI_CONNECT) {
    handleWiFiConnect(cmd);
  } else if (cmd.command == CMD_WIFI_DISCONNECT) {
    handleWiFiDisconnect();
  } else if (cmd.command == CMD_WIFI_GET_SAVED) {
    handleWiFiGetSaved();
  } else if (cmd.command == CMD_WIFI_FORGET) {
    handleWiFiForget();
  }

  // Utility Commands
  else if (cmd.command == "VERBOSE") {
    handleVerbose(cmd);
  }

  // Unknown Command
  else {
    String jsonResponse =
        createSerialJsonResponse(false, "Unknown command: " + cmd.command);
    sendSerialResponse(jsonResponse, true);
  }
}

//==============================================================================
// PUBLIC API FUNCTIONS
//==============================================================================

bool initSerial() {
  Serial.setRxBufferSize(8192);
  Serial.setTxBufferSize(4096);
  Serial.begin(SERIAL_BAUD_RATE);

  esp_log_level_set("*", ESP_LOG_NONE);

  unsigned long startTime = millis();
  while (!Serial && (millis() - startTime < 2000)) {
    delay(10);
  }

  currentSerialState = SerialUpdateState::IDLE;
  commandBuffer = "";
  verboseLogging = false;

  String jsonResponse = createSerialJsonResponse(
      true, "BYTE-90 Serial Interface Ready - WiFi commands available");
  sendSerialResponse(jsonResponse);

  return true;
}

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

      if (commandBuffer.length() > SERIAL_COMMAND_BUFFER_SIZE) {
        commandBuffer = "";
        String jsonResponse =
            createSerialJsonResponse(false, "Command too long");
        sendSerialResponse(jsonResponse, true);
      }
    }
  }
}

void cleanupSerial() {
  if (currentSerialState != SerialUpdateState::IDLE) {
    abortSerialUpdate();
  }

  currentSerialState = SerialUpdateState::IDLE;
  commandBuffer = "";
  verboseLogging = false;
  updateProgress = {0, 0, 0, ""};
}

bool isSerialUpdateActive() {
  return currentSerialState != SerialUpdateState::IDLE;
}

SerialUpdateState getSerialUpdateState() { return currentSerialState; }

void abortSerialUpdate() { handleAbortUpdate(); }

void setSerialVerbose(bool enabled) { verboseLogging = enabled; }

void notifyUpdateModeExit() {
  // Only send notification if we're actually in a state where clients might be
  // connected
  if (getCurrentState() == SystemState::UPDATE_MODE) {
    String jsonResponse = createSerialJsonResponse(
        true, "Update mode exiting - device will disconnect", true, 100);
    Serial.println("NOTIFY:" + jsonResponse);
    Serial.flush(); // Ensure message is sent immediately
    delay(200);     // Brief delay to allow transmission to complete

    ESP_LOGI(SERIAL_LOG,
             "Sent update mode exit notification to connected clients");
  }
}

/**
 * @brief Monitor serial update state changes and handle serial commands
 * Should be called regularly in main loop
 */
void updateSerialState() {
  // Only handle serial commands during UPDATE_MODE
  if (getCurrentState() == SystemState::UPDATE_MODE) {
    handleSerialCommands();
  }

  // Monitor serial update state changes
  if (isSerialUpdateActive()) {
    SerialUpdateState serialState = getSerialUpdateState();

    static SerialUpdateState lastSerialState = SerialUpdateState::IDLE;
    if (serialState != lastSerialState) {
      switch (serialState) {
      case SerialUpdateState::RECEIVING:
        ESP_LOGE(SERIAL_LOG, "Serial firmware update in progress...");
        break;
      case SerialUpdateState::PROCESSING:
        ESP_LOGE(SERIAL_LOG, "Processing serial firmware update...");
        break;
      case SerialUpdateState::SUCCESS:
        ESP_LOGE(SERIAL_LOG, "Serial firmware update completed successfully");
        break;
      case SerialUpdateState::ERROR:
        ESP_LOGW(SERIAL_LOG, "Serial firmware update failed");
        break;
      default:
        break;
      }
      lastSerialState = serialState;
    }
  }
}