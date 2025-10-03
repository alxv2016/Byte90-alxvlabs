/**
 * @file ota_module.cpp
 * @brief Implementation of Over-The-Air update functionality for firmware and
 * filesystem updates
 */

#include "ota_module.h"
#include "common.h"
#include "display_module.h"
#include "wifi_module.h"

//==============================================================================
// GLOBAL VARIABLES
//==============================================================================

OTAState otaState = OTAState::IDLE;
String otaMessage = "";
static int uploadTotal = 0;
static int fileSize = 0;
static String currentFilename = "";

//==============================================================================
// UTILITY FUNCTIONS (STATIC)
//==============================================================================

/**
 * @brief Get string representation of current OTA state
 * @return String representation of the current OTA state
 */
const char *getOTAStateString() {
  switch (otaState) {
  case OTAState::IDLE:
    return "IDLE";
  case OTAState::UPLOADING:
    return "UPLOADING";
  case OTAState::UPDATING:
    return "UPDATING";
  case OTAState::SUCCESS:
    return "SUCCESS";
  case OTAState::ERROR:
    return "ERROR";
  default:
    return "UNKNOWN";
  }
}

/**
 * @brief Create a JSON response with current update status
 * @param success Whether the operation was successful
 * @param completed Whether the update process has completed
 * @param progress Current progress percentage
 * @return JSON string with status information
 */
static String createJsonResponse(bool success, bool completed,
                                 const String &progress) {
  // Pre-calculate approximate size
  const char *stateStr = getOTAStateString();
  size_t estimatedSize = 200 + currentFilename.length() + otaMessage.length() +
                         progress.length() + strlen(stateStr) +
                         strlen(FIRMWARE_VERSION);

  String response;
  response.reserve(estimatedSize);

  response += "{\"success\":";
  response += success ? "true" : "false";
  response += ",\"state\":\"";
  response += stateStr;
  response += "\",\"filename\":\"";
  response += currentFilename;
  response += "\",\"progress\":";
  response += progress;
  response += ",\"total\":";
  response += fileSize;
  response += ",\"version\":\"";
  response += FIRMWARE_VERSION;
  response += "\",\"message\":\"";
  response += otaMessage;
  response += "\",\"completed\":";
  response += completed ? "true" : "false";
  response += "}";

  return response;
}

/**
 * @brief Check if current OTA state indicates an update is in progress
 * @param state OTA state to check
 * @return true if state is a valid update state
 */
static bool isValidUpdateState(OTAState state) {
  switch (state) {
  case OTAState::UPLOADING:
  case OTAState::UPDATING:
  case OTAState::SUCCESS:
    return true;
  default:
    return false;
  }
}

/**
 * @brief Initialize file upload for OTA update
 * @param upload HTTP upload object
 * @return true if initialization was successful
 */
static bool initializeUpload(HTTPUpload &upload) {
  const size_t MAX_FIRMWARE_SIZE = 1536 * 1024;
  const size_t MAX_SPIFFS_SIZE = 3 * 1024 * 1024;

  bool isFileSystem = upload.filename.indexOf(FILESYSTEM_BIN) >= 0;
  size_t maxSize = isFileSystem ? MAX_SPIFFS_SIZE : MAX_FIRMWARE_SIZE;

  if (upload.totalSize > maxSize) {
    otaState = OTAState::ERROR;
    otaMessage = "File too large: " + String(upload.totalSize) +
                 " bytes exceeds " + String(maxSize) + " byte limit";
    ESP_LOGE(OTA_LOG, "%s", otaMessage.c_str());
    return false;
  }

  otaState = OTAState::UPLOADING;
  otaMessage = "Your update is being uploaded to your device, please wait.";
  uploadTotal = 0;
  currentFilename = upload.filename;
  fileSize = upload.totalSize;

  int command = isFileSystem ? U_SPIFFS : U_FLASH;

  if (!Update.begin(UPDATE_SIZE_UNKNOWN, command)) {
    otaState = OTAState::ERROR;
    otaMessage = "Error: " + String(Update.errorString());
    ESP_LOGE(OTA_LOG, "%s", otaMessage.c_str());
    return false;
  }
  return true;
}

/**
 * @brief Handle the writing of uploaded data during OTA update
 * @param upload HTTP upload object
 * @return Current progress percentage
 */
static int handleUploadWrite(HTTPUpload &upload) {
  int progress = 0;
  if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
    Update.abort();
    otaState = OTAState::ERROR;
    otaMessage = "Error: Flash write failed";
    return progress;
  }

  otaState = OTAState::UPLOADING;
  uploadTotal = Update.size();
  fileSize = Update.size();

  progress = (Update.progress() * 100) / Update.size();
  otaMessage = "Progress: " + String(progress) + "%";
  ESP_LOGW(OTA_LOG, "Progress: %d%% (Written: %d, Total: %d)\n", progress,
           Update.progress(), uploadTotal);

  return progress;
}

/**
 * @brief Finalize an OTA update after upload is complete
 * @param upload HTTP upload object
 * @return true if update was successfully finalized
 */
static bool finalizeUpload(HTTPUpload &upload) {
  otaState = OTAState::UPDATING;
  otaMessage = "BYTE-90 is updates are being applied.";

  if (Update.end(true)) {
    otaState = OTAState::SUCCESS;
    otaMessage = "Update successful! Device will restart in a moment.";
    return true;
  } else {
    otaState = OTAState::ERROR;
    otaMessage = "Error: " + String(Update.errorString());
    return false;
  }
}

/**
 * @brief Handle completion of the update process
 */
static void handleUpdateComplete() {
  String jsonResponse;
  if (otaState == OTAState::ERROR || Update.hasError()) {
    otaState = OTAState::ERROR;
    jsonResponse = createJsonResponse(false, false, "0");
  } else {
    otaState = OTAState::SUCCESS;
    jsonResponse = createJsonResponse(true, true, "100");
  }

  webServer.send(200, "application/json", jsonResponse);

  if (otaState == OTAState::SUCCESS) {
    webServer.handleClient();
    delay(1000);
    stopWiFiAP(true);
  }
}

//==============================================================================
// PUBLIC API FUNCTIONS
//==============================================================================

bool initOTA() {
  const esp_partition_t *running = esp_ota_get_running_partition();
  const esp_partition_t *update_partition =
      esp_ota_get_next_update_partition(NULL);

  setupOTAEndpoints();
  
  if (!running || !update_partition) {
    ESP_LOGE(OTA_LOG, "Failed to get OTA partitions");
    return false;
  }
  return true;
}

void handleFileUpload() {
  HTTPUpload &upload = webServer.upload();

  switch (upload.status) {
  case UPLOAD_FILE_START:
    if (!upload.filename.endsWith(".bin")) {
      otaState = OTAState::ERROR;
      otaMessage =
          "Invalid file type, please choose the correct firmware files.";
      return;
    }

    if (upload.filename.indexOf(FIRMWARE_BIN) < 0 &&
        upload.filename.indexOf(FILESYSTEM_BIN) < 0) {
      otaState = OTAState::ERROR;
      otaMessage = "Invalid firmware, the file must be " FIRMWARE_BIN
                   " or " FILESYSTEM_BIN ".";
      return;
    }
    if (!initializeUpload(upload)) {
      return;
    }
    break;
  case UPLOAD_FILE_WRITE:
    if (otaState != OTAState::ERROR) {
      handleUploadWrite(upload);
    }
    break;

  case UPLOAD_FILE_END:
    if (otaState != OTAState::ERROR) {
      finalizeUpload(upload);
    }
    break;

  case UPLOAD_FILE_ABORTED:
    otaState = OTAState::ERROR;
    otaMessage = "Device has timed out, upload aborted.";
    Update.abort();
    break;
  }
}

void setupOTAEndpoints() {
  webServer.on("/update", HTTP_POST, handleUpdateComplete, handleFileUpload);
  webServer.on("/update/status", HTTP_GET, []() {
    int progress = 0;
    if (Update.size() > 0) {
      progress = (Update.progress() * 100) / Update.size();
    }

    String jsonResponse =
        createJsonResponse(isValidUpdateState(otaState),
                           otaState == OTAState::SUCCESS, String(progress));
    webServer.send(200, "application/json", jsonResponse);
  });
}