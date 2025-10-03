/**
 * @file espnow_module.cpp
 * @brief Implementation of ESP-NOW wireless communication functionality
 *
 * Provides functions for device-to-device communication using ESP-NOW, including
 * device discovery, pairing, message exchange, and state management.
 */

#include "espnow_module.h"
#include "common.h"
#include "emotes_module.h"
#include "motion_module.h"
#include "wifi_common.h"
#include "wifi_module.h"
#include "preferences_module.h"
#include "wifi_endpoints.h"
#include "states_module.h"
#include "soundsfx_communication.h"
#include "soundsfx_module.h"
#include <stdarg.h>

//==============================================================================
// GLOBAL VARIABLES
//==============================================================================

const int MAX_FAILURES = 4;
const int MAX_BROADCAST_ATTEMPTS = 6;
static const char *currentAnimationPath = nullptr;
static ConversationType currentConversationType = ConversationType::HELLO;

static const ConversationConfig CONVERSATIONS[] = {
    {ConversationType::HELLO, COMS_HELLO_EMOTE},
    {ConversationType::QUESTION_01, COMS_TALK_01_EMOTE},
    {ConversationType::QUESTION_02, COMS_TALK_02_EMOTE},
    {ConversationType::QUESTION_03, COMS_TALK_03_EMOTE},
    {ConversationType::AGREE, COMS_AGREED_EMOTE},
    {ConversationType::DISAGREE, COMS_DISAGREE_EMOTE},
    {ConversationType::YELL, COMS_YELL_EMOTE},
    {ConversationType::LAUGH, COMS_LAUGH_EMOTE},
    {ConversationType::WINK, COMS_WINK_EMOTE},
    {ConversationType::ZONE, COMS_ZONED_EMOTE},
    {ConversationType::SHOCK, COMS_SHOCK_EMOTE}};

// Add this mapping structure after your existing ConversationConfig
struct ConversationSoundMapping {
  ConversationType type;
  const char* soundType;
  int delayMs;
};

// Define sound mappings for each conversation type
static const ConversationSoundMapping CONVERSATION_SOUND_TYPES[] = {
  {ConversationType::HELLO, "talking", 100},
  {ConversationType::QUESTION_01, "alttalking", 100},
  {ConversationType::QUESTION_02, "talking", 100},
  {ConversationType::QUESTION_03, "alttalking", 100},
  {ConversationType::AGREE, "talking", 100},
  {ConversationType::DISAGREE, "angry", 60},
  {ConversationType::YELL, "angry", 60},
  {ConversationType::LAUGH, "laughing", 60},
  {ConversationType::WINK, "talking", 90},
  {ConversationType::ZONE, "talking", 100},
  {ConversationType::SHOCK, "shock", 80}
};

uint8_t peerMac[6] = {0};
bool espNowStateChanged = false;

ComStatus currentStatus = ComStatus::DISCOVERY;
ComState currentComState = ComState::NONE;
DeviceRole currentRole = DeviceRole::UNKNOWN;
ESPNowState currentESPNowState = ESPNowState::OFF;

int consecutiveFailures = 0;
int broadcastAttempts = 0;
unsigned long lastBroadcastTime = 0;
unsigned long lastStatusTime = 0;
unsigned long lastMessageTime = 0;
unsigned long lastToggleTime = 0;

//==============================================================================
// UTILITY FUNCTIONS (STATIC)
//==============================================================================

// Debug control - set to true to enable debug logging
static bool espnowDebugEnabled = false;

/**
 * @brief Centralized debug logging function for ESP-NOW module operations
 * @param format Printf-style format string
 * @param ... Variable arguments for format string
 */
static void espnowDebug(const char* format, ...) {
  if (!espnowDebugEnabled) {
    return;
  }
  
  va_list args;
  va_start(args, format);
  esp_log_writev(ESP_LOG_INFO, ESPNOW_LOG, format, args);
  va_end(args);
}

/**
 * @brief Enable or disable debug logging for ESP-NOW module operations
 * @param enabled true to enable debug logging, false to disable
 * 
 * @example
 * // Enable debug logging
 * setEspnowDebug(true);
 * 
 * // Disable debug logging  
 * setEspnowDebug(false);
 */
void setEspnowDebug(bool enabled) {
  espnowDebugEnabled = enabled;
  if (enabled) {
    ESP_LOGI(ESPNOW_LOG, "ESP-NOW module debug logging enabled");
  } else {
    ESP_LOGI(ESPNOW_LOG, "ESP-NOW module debug logging disabled");
  }
}

/**
 * @brief ESP-NOW send callback function
 * @param mac MAC address of the recipient
 * @param status Send status (success/failure)
 */
static void Send_data_cb(const uint8_t *mac, esp_now_send_status_t status);

/**
 * @brief ESP-NOW receive callback function
 * @param mac MAC address of the sender
 * @param data Pointer to received data
 * @param len Length of received data
 */
static void Receive_data_cb(const uint8_t *mac, const uint8_t *data, int len);

/**
 * @brief Handle device pairing process
 * @param mac MAC address of device to pair with
 */
static void handlePairing(const uint8_t *mac);

/**
 * @brief Handle connection loss and reset communication
 */
static void handleConnectionLost(void);

/**
 * @brief Send a data message to paired device
 * @param text Message text content
 * @param type Conversation type for animation selection
 * @return true if message was sent successfully
 */
static bool sendDataMessage(const char *text, ConversationType type);

/**
 * @brief Send a discovery broadcast message
 * @return true if message was sent successfully
 */
static bool sendDiscoveryMessage(void);

/**
 * @brief Get animation path for a specific conversation type
 * @param type Conversation type to find animation for
 * @return Path to the animation file or nullptr if not found
 */
const char *getAnimationPath(ConversationType type) {
  for (const auto &conv : CONVERSATIONS) {
    if (conv.type == type)
      return conv.gifPath;
  }
  return nullptr;
}

const char* getCurrentConversationSoundType(ConversationType type) {
  for (const auto &mapping : CONVERSATION_SOUND_TYPES) {
    if (mapping.type == type) {
      return mapping.soundType;
    }
  }
  return "talking"; // Default fallback
}

int getCurrentConversationSoundDelay(ConversationType type) {
  for (const auto &mapping : CONVERSATION_SOUND_TYPES) {
    if (mapping.type == type) {
      return mapping.delayMs;
    }
  }
  return 300; // Default delay
}

ConversationType getCurrentConversationType() {
  return currentConversationType;
}

/**
 * @brief Format MAC address as string
 * @param mac MAC address bytes
 * @param output String buffer to store formatted address
 */
static void formatMacAddress(const uint8_t *mac, char *output) {
  snprintf(output, 18, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2],
           mac[3], mac[4], mac[5]);
}

/**
 * @brief Set up a peer for ESP-NOW communication
 * @param mac MAC address of the peer to set up
 * @return true if peer was set up successfully
 */
static bool setupPeer(const uint8_t *mac) {
  // Remove peer if it already exists (clean slate)
  if (esp_now_is_peer_exist(mac)) {
    esp_now_del_peer(mac);
  }

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, mac, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  esp_err_t result = esp_now_add_peer(&peerInfo);
  if (result != ESP_OK) {
    ESP_LOGE(ESPNOW_LOG, "Failed to add peer: %s", esp_err_to_name(result));
    return false;
  }
  
  return true;
}

/**
 * @brief Callback function to receive data from ESP-NOW protocol
 */
static void Receive_data_cb(const uint8_t *mac, const uint8_t *data, int len) {
  // Log EVERY received message, regardless of validity
  char macStr[18];
  formatMacAddress(mac, macStr);
  ESP_LOGI(ESPNOW_LOG, "📨 RAW RECEIVE: %d bytes from %s", len, macStr);
  
  if (len != sizeof(Message)) {
    ESP_LOGE(ESPNOW_LOG, "Invalid message size: received %d bytes, expected %d",
             len, sizeof(Message));
    return;
  }

  Message *msg = (Message *)data;
  ESP_LOGI(ESPNOW_LOG, "📨 MSG: sig=0x%X, text='%s', type=%d from %s", 
           msg->signature, msg->text, (int)msg->type, macStr);
           
  if (msg->signature != APP_SIGNATURE) {
    ESP_LOGE(ESPNOW_LOG, "App signature mismatch: received 0x%X, expected 0x%X",
             msg->signature, APP_SIGNATURE);
    return;
  }

  if (currentStatus == ComStatus::DISCOVERY) {
    ESP_LOGI(ESPNOW_LOG, "🔗 Processing pairing from %s", macStr);
    handlePairing(mac);
    
    // Only process the message if pairing was successful
    if (currentStatus != ComStatus::PAIRED) {
      espnowDebug("Pairing failed - ignoring message");
      return;
    }
    
    // Log successful pairing with role assignment
    espnowDebug("Pairing successful - Role: %s", 
                currentRole == DeviceRole::INITIATOR ? "INITIATOR" : "RESPONDER");
  }

  currentAnimationPath = getAnimationPath(msg->type);
  currentConversationType = msg->type;
  
  if (msg->type == ConversationType::SHOCK ||
    msg->type == ConversationType::ZONE) {
      currentComState = ComState::PROCESSING;
    } else if (currentAnimationPath != nullptr) {
      currentComState = ComState::PROCESSING;
    }

}

/**
 * @brief Callback function to send data from ESP-NOW protocol
 */
static void Send_data_cb(const uint8_t *mac, esp_now_send_status_t status) {
  char macStr[18];
  formatMacAddress(mac, macStr);
  if (status == ESP_NOW_SEND_SUCCESS) {
    consecutiveFailures = 0;
  } else {
    ESP_LOGE(ESPNOW_LOG, "::Delivery failed to:: %s", macStr);
    if (++consecutiveFailures >= MAX_FAILURES) {
      handleConnectionLost();
    }
  }
}

/**
 * @brief Handle lost connection to paired device
 */
static void handleConnectionLost() {
  currentStatus = ComStatus::DISCOVERY;
  currentRole = DeviceRole::UNKNOWN;
  ESP_LOGW(ESPNOW_LOG, "Connection reset - returning to discovery mode");

  if (isPaired()) {
    esp_now_del_peer(peerMac);
  }

  memset(peerMac, 0, 6);

  resetAnimationPath();
  consecutiveFailures = 0;
  broadcastAttempts = 0;
  lastBroadcastTime = 0;
  lastMessageTime = 0;

  if (!startDiscovery()) {
    ESP_LOGE(ESPNOW_LOG, "Failed to return to discovery mode");
    return;
  }
}

/**
 * @brief Handle pairing with another device
 * @param mac MAC address of device to pair with
 */
static void handlePairing(const uint8_t *mac) {
  char macStr[18];
  formatMacAddress(mac, macStr);
  
  // Check if we're already paired with this exact device
  if (currentStatus == ComStatus::PAIRED && memcmp(peerMac, mac, 6) == 0) {
    espnowDebug("Already paired with %s - ignoring duplicate pairing request", macStr);
    return; // Don't send ACK messages during pairing
  }
  
  // If we're already paired with a different device, disconnect first
  if (currentStatus == ComStatus::PAIRED) {
    espnowDebug("Already paired with different device - disconnecting to pair with %s", macStr);
    esp_now_del_peer(peerMac);
    memset(peerMac, 0, 6);
    currentStatus = ComStatus::DISCOVERY;
    currentRole = DeviceRole::UNKNOWN;
  }
  
  espnowDebug("Starting pairing process with %s", macStr);
  
  // Store the new peer MAC
  memcpy(peerMac, mac, 6);
  
  // Set up the peer for bidirectional communication
  if (!setupPeer(mac)) {
    ESP_LOGE(ESPNOW_LOG, "Failed to add peer %s during pairing", macStr);
    memset(peerMac, 0, 6); // Clear failed peer
    return;
  }
  
  // Determine device role based on MAC comparison
  uint8_t myMac[6];
  WiFi.macAddress(myMac);
  currentRole = (memcmp(myMac, mac, 6) > 0) ? DeviceRole::INITIATOR : DeviceRole::RESPONDER;
  
  // Set pairing status only after everything succeeds
  currentStatus = ComStatus::PAIRED;
  
  // Reset conversation timing to prevent immediate message sending
  lastMessageTime = millis();
  
  espnowDebug("Pairing completed with %s (Role: %s)", 
              macStr, currentRole == DeviceRole::INITIATOR ? "INITIATOR" : "RESPONDER");
}

/**
 * @brief Send a discovery broadcast message
 * @return true if message was sent successfully
 */
static bool sendDiscoveryMessage() {
  if (getCurrentESPNowState() == ESPNowState::OFF ||
      currentStatus != ComStatus::DISCOVERY) {
    return false;
  }

  unsigned long currentTime = millis();
  unsigned long previousMessageTime = lastMessageTime;

  if (currentTime - lastMessageTime < ComsInterval::DISCOVERY_INTERVAL)
    return false;

  if (currentTime - lastBroadcastTime < ComsInterval::DISCOVERY_INTERVAL) {
    return false;
  }

  lastMessageTime = currentTime;
  lastBroadcastTime = currentTime;
  broadcastAttempts++;

  espnowDebug("📡 Sending discovery broadcast #%d/%d: 'SEARCHING PEERS'", 
           broadcastAttempts, MAX_BROADCAST_ATTEMPTS);

  Message msg = {};
  msg.signature = APP_SIGNATURE;
  WiFi.macAddress(msg.mac);
  strcpy(msg.text, "SEARCHING PEERS");
  msg.type = ConversationType::HELLO;

  uint8_t broadcastAddr[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

  if (broadcastAttempts >= MAX_BROADCAST_ATTEMPTS) {
    ESP_LOGW(ESPNOW_LOG, "Max discovery attempts reached, resetting discovery cycle");
    handleConnectionLost();
    broadcastAttempts = 0;
  }

  esp_err_t result = esp_now_send(broadcastAddr, (uint8_t *)&msg, sizeof(Message));
  if (result == ESP_OK) {
    ESP_LOGV(ESPNOW_LOG, "Discovery message sent successfully");
    return true;
  } else {
    ESP_LOGW(ESPNOW_LOG, "Failed to send discovery message: %s", esp_err_to_name(result));
    return false;
  }
}

/**
 * @brief Send a data message to the paired device
 * @param text Message text content
 * @param type Conversation type for animation selection
 * @return true if message was sent successfully
 */
static bool sendDataMessage(const char *text,
                            ConversationType type = ConversationType::HELLO) {
  if (!isPaired())
    return false;

  unsigned long currentTime = millis();
  unsigned long previousMessageTime = lastMessageTime;

  if (currentTime - lastMessageTime < ComsInterval::MESSAGE_INTERVAL)
    return false;

  lastMessageTime = currentTime;

  Message msg = {};
  msg.signature = APP_SIGNATURE;
  WiFi.macAddress(msg.mac);
  strncpy(msg.text, text, sizeof(msg.text) - 1);
  msg.text[sizeof(msg.text) - 1] = '\0';
  msg.type = type;
  
  currentAnimationPath = getAnimationPath(msg.type);
  currentComState = ComState::PROCESSING;

  return esp_now_send(peerMac, (uint8_t *)&msg, sizeof(Message)) == ESP_OK;
}

/**
 * @brief Handle sequential conversation between paired devices
 */
static void handleSequentialConversation() {
  static unsigned long lastConversationTime = 0;
  static int sequenceIndex = 0;
  static bool orientationTriggered = false;

  if (!isPaired() || currentRole == DeviceRole::UNKNOWN ||
      millis() - lastConversationTime < ComsInterval::MESSAGE_INTERVAL) {
    return;
  }

  if (motionOriented()) {
    if (!orientationTriggered) {
      if (sendDataMessage("ORIENTATION_CHANGE", ConversationType::SHOCK)) {
        orientationTriggered = true;
        lastConversationTime = millis();
      }
    } else {
      if (sendDataMessage("ORIENTATION_ZONED", ConversationType::ZONE)) {
        orientationTriggered = false;
        lastConversationTime = millis();
      }
    }
    return;
  }

  orientationTriggered = false;

  if (millis() - lastConversationTime > ComsInterval::MESSAGE_INTERVAL) {
    const size_t sequenceLength =
        sizeof(CONVERSATIONS) / sizeof(CONVERSATIONS[0]);

    bool activeSender =
        (currentRole == DeviceRole::INITIATOR && sequenceIndex % 2 == 0) ||
        (currentRole == DeviceRole::RESPONDER && sequenceIndex % 2 == 1);

    currentComState = ComState::WAITING;

    if (activeSender) {
      delay(random(100, 500));
      if (sendDataMessage("CONVERSE", CONVERSATIONS[sequenceIndex].type)) {
        lastConversationTime = millis();
      }
    }

    sequenceIndex = (sequenceIndex + 1) % sequenceLength;
  }
}

//==============================================================================
// PUBLIC API FUNCTIONS
//==============================================================================

const char *getCurrentAnimationPath() { 
  return currentAnimationPath; 
}

void resetAnimationPath() {
  currentAnimationPath = nullptr;
  currentComState = ComState::NONE;
}

bool initializeESPNOW() {
  espnowDebug("Initializing ESP-NOW protocol...");

  if (esp_now_init() != ESP_OK) {
    ESP_LOGE(ESPNOW_LOG, "ESP-NOW init failed!");
    return false;
  }

  espnowDebug("Registering ESP-NOW callbacks...");
  
  esp_err_t send_result = esp_now_register_send_cb(Send_data_cb);
  esp_err_t recv_result = esp_now_register_recv_cb(Receive_data_cb);
  
  if (send_result != ESP_OK) {
    ESP_LOGE(ESPNOW_LOG, "Failed to register send callback: %s", esp_err_to_name(send_result));
    return false;
  }
  
  if (recv_result != ESP_OK) {
    ESP_LOGE(ESPNOW_LOG, "Failed to register receive callback: %s", esp_err_to_name(recv_result));
    return false;
  }
  
  espnowDebug("ESP-NOW callbacks registered successfully");
  espnowDebug("ESP-NOW initialization completed successfully");
  return true;
}

bool isPaired() {
  ESPNowState espnowState = getCurrentESPNowState();
  return espnowState == ESPNowState::ON && currentStatus == ComStatus::PAIRED;
}

bool startDiscovery() {
  if (getCurrentESPNowState() == ESPNowState::OFF) {
    espnowDebug("Cannot start discovery - ESP-NOW is OFF");
    return false;
  }

  // Remove existing broadcast peer if it exists
  uint8_t broadcastAddr[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  if (esp_now_is_peer_exist(broadcastAddr)) {
    esp_now_del_peer(broadcastAddr);
  }

  espnowDebug("Setting up discovery mode with broadcast address...");
  
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, broadcastAddr, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    ESP_LOGE(ESPNOW_LOG, "ESPNOW Discovery setup failed!");
    return false;
  }

  espnowDebug("Discovery mode setup completed successfully");
  return true;
}

void handleCommunication() {
  static unsigned long lastAttempt = 0;
  static bool firstDiscoveryLogShown = false;
  
  if (getCurrentESPNowState() == ESPNowState::OFF)
    return;

  if (millis() - lastAttempt > ComsInterval::STATUS_INTERVAL) {
    if (currentStatus == ComStatus::DISCOVERY) {
      if (!firstDiscoveryLogShown) {
        espnowDebug("🔍 ESP-NOW discovery active - looking for peers...");
        firstDiscoveryLogShown = true;
      }
      sendDiscoveryMessage();
    } else if (currentStatus == ComStatus::PAIRED) {
      firstDiscoveryLogShown = false; // Reset for next discovery cycle
      handleSequentialConversation();
    }
    lastAttempt = millis();
  }
}

void forceDisconnect() {
  if (isPaired() || currentStatus == ComStatus::DISCOVERY) {
    handleConnectionLost();
  }
}

void shutdownCommunication() {
  espnowDebug("Shutting down ESP-NOW communication...");
  forceDisconnect();
  esp_now_deinit();
  espnowDebug("ESP-NOW communication shutdown completed");
  if (getCurrentState() == SystemState::ESP_MODE) {
    WiFi.disconnect(true);
  }
  currentESPNowState = ESPNowState::OFF;
  currentStatus = ComStatus::DISCOVERY;
  espNowStateChanged = true;
  ESP_LOGW(ESPNOW_LOG, "ESPNOW is turned off");
}

bool restartCommunication() {
   if (isWifiNetworkConnected()) {
    ESP_LOGW(ESPNOW_LOG, "Cannot start ESP-NOW while in WiFi mode");
    return false;
  }

  if (currentESPNowState == ESPNowState::OFF) {
    espnowDebug("Initializing ESP-NOW for communication...");
    initializeESPNOW();
  }

  currentESPNowState = ESPNowState::ON;
  currentStatus = ComStatus::DISCOVERY;

  espnowDebug("ESP-NOW communication restarted - entering DISCOVERY mode");
  espnowDebug("Broadcasting discovery messages every %lu ms", ComsInterval::DISCOVERY_INTERVAL);

  if (!startDiscovery()) {
    currentESPNowState = ESPNowState::OFF;
    shutdownCommunication();
    ESP_LOGE(ESPNOW_LOG, "Failed to restart discovery mode!");
    return false;
  }

  espnowDebug("ESP-NOW discovery mode started successfully");
  espNowStateChanged = true;
  return true;
}

ESPNowState getCurrentESPNowState() { 
  return currentESPNowState; 
}

bool espNowStateChangedState() { 
  return espNowStateChanged; 
}

ComState getCurrentComState() { 
  return currentComState; 
}

void resetEspNowStateChanged() { 
  espNowStateChanged = false; 
}

//==============================================================================
// ESP-NOW INITIALIZATION FUNCTIONS
//==============================================================================