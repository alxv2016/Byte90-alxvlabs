/**
 * @file espnow_module.h
 * @brief Header for ESP-NOW wireless communication functionality
 *
 * Provides functions and types for device-to-device communication using ESP-NOW,
 * including device discovery, pairing, message exchange, and state management.
 */

#ifndef ESPNOW_MODULE_H
#define ESPNOW_MODULE_H

#include "common.h"
#include <esp_now.h>
#include <WiFi.h>

//==============================================================================
// FORWARD DECLARATIONS
//==============================================================================

class MotionState;

//==============================================================================
// CONSTANTS & DEFINITIONS
//==============================================================================

static const char* ESPNOW_LOG = "::ESPNOW_MODULE::";

#define APP_SIGNATURE 0xCAFE2025

//==============================================================================
// TYPE DEFINITIONS
//==============================================================================

enum class ESPNowState {
    ON,
    OFF
};

enum class DeviceRole {
    UNKNOWN,
    INITIATOR,
    RESPONDER
};

enum class ComState {
    NONE,
    WAITING,
    PROCESSING
};

enum class ComStatus {
    DISCOVERY,
    PAIRED
};

enum class ConversationType {
    HELLO,
    QUESTION_01,
    QUESTION_02,
    QUESTION_03,
    AGREE,
    DISAGREE,
    YELL,
    LAUGH,
    WINK,
    ZONE,
    SHOCK
};

struct ConversationConfig {
    ConversationType type;
    const char* gifPath;
};

struct Message {
    uint32_t signature;
    uint8_t mac[6];
    char text[32];
    ConversationType type;
};

struct ComsInterval {
    static const unsigned long STATUS_INTERVAL = 6000;
    static const unsigned long MESSAGE_INTERVAL = 4000;
    static const unsigned long DISCOVERY_INTERVAL = 1000;
    static const unsigned long TOGGLE_DEBOUNCE = 5000;
};

//==============================================================================
// PUBLIC API FUNCTIONS
//==============================================================================

/**
 * @brief Initialize ESP-NOW communication
 * @return true if initialization successful
 */
bool initializeESPNOW();

/**
 * @brief Start discovery mode to find other devices
 * @return true if discovery mode started successfully
 */
bool startDiscovery();

/**
 * @brief Check if device is paired with another device
 * @return true if paired and ESP-NOW is active
 */
bool isPaired();

/**
 * @brief Restart ESP-NOW communication
 * @return true if restart successful
 */
bool restartCommunication();

/**
 * @brief Shut down ESP-NOW communication
 */
void shutdownCommunication();

/**
 * @brief Main communication handling function
 */
void handleCommunication();

/**
 * @brief Force disconnect from current peer
 */
void forceDisconnect();

/**
 * @brief Reset the animation path and communication state
 */
void resetAnimationPath();

/**
 * @brief Get the currently active animation path
 * @return Current animation path or nullptr if none
 */
const char* getCurrentAnimationPath();

/**
 * @brief Get current ESP-NOW state
 * @return Current ESP-NOW state (ON/OFF)
 */
ESPNowState getCurrentESPNowState();

/**
 * @brief Get current communication state
 * @return Current communication state (PROCESSING/WAITING/NONE)
 */
ComState getCurrentComState();

/**
 * @brief Check if ESP-NOW state was changed
 * @return true if ESP-NOW state was changed
 */
bool espNowStateChangedState();

/**
 * @brief Reset ESP-NOW state changed flag
 */
void resetEspNowStateChanged();

/**
 * @brief Enable or disable debug logging for ESP-NOW module operations
 * @param enabled true to enable debug logging, false to disable
 */
void setEspnowDebug(bool enabled);

/**
 * @brief Get sound type for a specific conversation type
 * @param type Conversation type to find sound for
 * @return Sound type string or nullptr if not found
 */
const char* getCurrentConversationSoundType(ConversationType type);

/**
 * @brief Get sound delay for a specific conversation type
 * @param type Conversation type to find delay for
 * @return Delay in milliseconds
 */
int getCurrentConversationSoundDelay(ConversationType type);

/**
 * @brief Get current conversation type from last received message
 * @return Current conversation type
 */
ConversationType getCurrentConversationType();

#endif /* ESPNOW_MODULE_H */