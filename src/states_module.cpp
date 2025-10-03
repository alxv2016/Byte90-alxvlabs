/**
 * @file states_module.cpp
 * @brief Implementation of system state management module
 *
 * Provides functionality for managing system states, state transitions,
 * and state machine operations for the BYTE-90 device.
 * 
 * This module handles:
 * - System state initialization and management
 * - State transitions with validation and cleanup
 * - State-specific entry and exit logic
 * - Startup state configuration and persistence
 * - State machine updates and monitoring
 * - Integration with WiFi, display, and other system modules
 */

#include "states_module.h"
#include "wifi_module.h"
#include "preferences_module.h"
#include "display_module.h"
#include "clock_sync.h"
#include "espnow_module.h"
#include <esp_log.h>
#include <stdarg.h>

//==============================================================================
// DEBUG SYSTEM
//==============================================================================

// Debug control - set to true to enable debug logging
static bool statesDebugEnabled = false;

/**
 * @brief Centralized debug logging function for states operations
 * @param format Printf-style format string
 * @param ... Variable arguments for format string
 */
static void statesDebug(const char* format, ...) {
  if (!statesDebugEnabled) {
    return;
  }
  
  va_list args;
  va_start(args, format);
  esp_log_writev(ESP_LOG_INFO, "STATE_MGR", format, args);
  va_end(args);
}

/**
 * @brief Enable or disable debug logging for states operations
 * @param enabled true to enable debug logging, false to disable
 * 
 * @example
 * // Enable debug logging
 * setStatesDebug(true);
 * 
 * // Disable debug logging  
 * setStatesDebug(false);
 */
void setStatesDebug(bool enabled) {
  statesDebugEnabled = enabled;
  if (enabled) {
    ESP_LOGI("STATE_MGR", "States debug logging enabled");
  } else {
    ESP_LOGI("STATE_MGR", "States debug logging disabled");
  }
}

//==============================================================================
// CONSTANTS & DEFINITIONS
//==============================================================================

static const char* TAG = "STATE_MGR";

//==============================================================================
// GLOBAL VARIABLES
//==============================================================================

// State management variables
static SystemState currentState = IDLE_MODE;
static SystemState startupState = IDLE_MODE;
static SystemState previousState = IDLE_MODE;

//==============================================================================
// PUBLIC API FUNCTIONS
//==============================================================================

/**
 * @brief Initializes the system state manager and sets up initial state
 * 
 * Loads startup state from preferences, validates state consistency with
 * WiFi mode settings, and initializes the system in the appropriate state.
 * Handles state correction if inconsistencies are detected.
 */
void initSystemStateManager() {
    statesDebug("Initializing State Manager...");
    
    // Load startup state from preferences
    startupState = (SystemState)loadStartupMode();

    bool wifiModeEnabled = getWiFiModeEnabled();
    if (wifiModeEnabled && startupState == IDLE_MODE) {
        ESP_LOGW(TAG, "WiFi mode enabled but startup state is IDLE - correcting to WIFI_MODE");
        startupState = WIFI_MODE;
        saveStartupMode((uint8_t)startupState);
    } else if (!wifiModeEnabled && startupState == WIFI_MODE) {
        ESP_LOGW(TAG, "WiFi mode disabled but startup state is WIFI - correcting to IDLE_MODE");
        startupState = IDLE_MODE;
        saveStartupMode((uint8_t)startupState);
    }
    
    // Validate startup state (only IDLE_MODE and WIFI_MODE are valid)
    if (!isValidStartupState(startupState)) {
        ESP_LOGW(TAG, "Invalid startup state %d, defaulting to IDLE_MODE", startupState);
        startupState = IDLE_MODE;
        saveStartupMode((uint8_t)startupState);
    }
    
    // Set initial state
    currentState = startupState;
    previousState = startupState;
    
    statesDebug("Starting in %s", getStateString(currentState));
    
    // Initialize the starting state
    enterState(currentState);
}

/**
 * @brief Updates the system state machine - should be called in main loop
 * 
 * Handles state transitions, timeouts, and state-specific logic.
 * Monitors current state and performs automatic transitions when needed.
 */
void updateSystemStateMachine() {
    unsigned long currentTime = millis();
    
    switch (currentState) {
        case IDLE_MODE:
            // No automatic transitions from IDLE_MODE
            break;
            
        case WIFI_MODE:
            // No automatic transitions from WIFI_MODE
            // (24-hour clock sync is handled in main.cpp)
            break;

       case ESP_MODE:
            // No automatic transitions from ESP_MODE
            // ESP-NOW communication is persistent until user changes state
            break;
            
        case UPDATE_MODE:
            // No automatic transitions from UPDATE_MODE
            // Update mode is persistent until user changes state
            break;
            
        case CLOCK_MODE:
             // No automatic transitions from CLOCK_MODE
            // TODO: Add logic to detect NTP sync completion and return early
            break;
    }
}

/**
 * @brief Transitions the system to a new state
 * @param newState The target state to transition to
 * 
 * Handles state transitions with proper cleanup and initialization.
 * Manages state history and validates transitions.
 */
void transitionToState(SystemState newState) {
    if (newState == currentState) {
        ESP_LOGD(TAG, "Already in %s - no transition needed", getStateString(newState));
        return;
    }
    
    statesDebug("Transitioning from %s to %s", 
             getStateString(currentState), getStateString(newState));
    
    // Store previous state (but not if transitioning from temporary modes)
    if (currentState != UPDATE_MODE && currentState != CLOCK_MODE) {
        previousState = currentState;
    }
    
    // Exit current state
    exitState(currentState);
    
    // Update state
    SystemState oldState = currentState;
    currentState = newState;
    
    // Enter new state
    enterState(newState);
    updateDisplayForMode(newState);
    
     // Update startup preference for persistent states
     if (isValidStartupState(newState)) {
        if (oldState != UPDATE_MODE && oldState != CLOCK_MODE) {
            startupState = newState;
            saveStartupMode((uint8_t)startupState);
            
            // NEW: Also sync WiFi mode preference
            if (newState == WIFI_MODE) {
                setWiFiModeEnabled(true);
            } else if (newState == IDLE_MODE) {
                setWiFiModeEnabled(false);
            }
            
            statesDebug("Startup mode updated to %s", getStateString(startupState));
        }
    }
    
    // Save last known good state
    saveLastKnownGoodState((uint8_t)newState);
}

/**
 * @brief Handles cleanup and exit logic for a specific state
 * @param state The state to exit from
 * 
 * Performs state-specific cleanup operations when exiting a state.
 * Handles WiFi operations, ESP-NOW shutdown, and other cleanup tasks.
 */
void exitState(SystemState state) {
    ESP_LOGD(TAG, "Exiting %s", getStateString(state));
    
    switch (state) {
        case IDLE_MODE:
            // Nothing to clean up
            break;
            
        case WIFI_MODE:
            // Keep WiFi connected for now - will be handled by entering state
            break;

        case ESP_MODE:
            // Stop ESP-NOW (placeholder - actual implementation in ESP-NOW module)
            statesDebug("Stopping ESP-NOW communication");
            shutdownCommunication();
            break;
            
        case UPDATE_MODE:
            // Stop AP mode
            statesDebug("Stopping WiFi AP");
            stopWiFiAP(false);
             if (isWifiNetworkConnected()) {
                    ESP_LOGI("STATE_MGR", "WiFi connected during UPDATE_MODE exit - enabling WiFi mode");
                    setWiFiModeEnabled(true);
                }
            break;
            
        case CLOCK_MODE:
            // Disconnect WiFi if it was only for clock sync
            if (!getWiFiModeEnabled()) {
                disconnectFromWiFi();
                statesDebug("WiFi disconnected - clock mode active");
            }
            break;
    }
}

/**
 * @brief Handles initialization and entry logic for a specific state
 * @param state The state to enter
 * 
 * Performs state-specific initialization operations when entering a state.
 * Handles WiFi configuration, ESP-NOW setup, and other state-specific tasks.
 */
void enterState(SystemState state) {
    ESP_LOGD(TAG, "Entering %s", getStateString(state));
    
    switch (state) {
        case IDLE_MODE:
            // Disable WiFi completely for power savings
            disableWiFi();
            statesDebug("WiFi disabled - power saving mode active");
            break;
            
        case WIFI_MODE:
            enableWiFi();
            statesDebug("WiFi enabled - waiting for station to be ready");
            break;

        case ESP_MODE:
            // Enable WiFi for ESP-NOW but don't connect to any network
            enableESPNowWiFi();
            statesDebug("WiFi enabled for ESP-NOW communication");
            break;
            
        case UPDATE_MODE:
            startWiFiAP();
            statesDebug("WiFi AP started for updates");
            break;
            
        case CLOCK_MODE:
            // Enable WiFi and connect for NTP sync
            enableWiFi();
            syncAndDisplayTime();
            statesDebug("WiFi enabled for clock synchronization");
            break;
    }
}

/**
 * @brief Gets the current system state
 * @return Current SystemState value
 * 
 * Returns the currently active system state.
 */
SystemState getCurrentState() {
    return currentState;
}

/**
 * @brief Gets the startup state that should be used on system boot
 * @return Startup SystemState value
 * 
 * Returns the state that the system should start in on boot.
 */
SystemState getStartupState() {
    return startupState;
}

/**
 * @brief Sets the startup state for system boot
 * @param state The SystemState to use on startup
 * 
 * Sets and persists the startup state for future system boots.
 * Only allows valid startup states (IDLE_MODE and WIFI_MODE).
 */
void setStartupState(SystemState state) {
    if (isValidStartupState(state)) {
        startupState = state;
        saveStartupMode((uint8_t)state);
        statesDebug("Startup state set to %s", getStateString(state));
    } else {
        ESP_LOGW(TAG, "Invalid startup state %s - only IDLE_MODE and WIFI_MODE allowed", 
                 getStateString(state));
    }
}

/**
 * @brief Gets a human-readable string representation of a state
 * @param state The SystemState to convert to string
 * @return String representation of the state
 * 
 * Converts SystemState enum values to descriptive strings for logging
 * and debugging purposes.
 */
const char* getStateString(SystemState state) {
    switch (state) {
        case IDLE_MODE: return "IDLE_MODE";
        case WIFI_MODE: return "WIFI_MODE";
        case ESP_MODE: return "ESP_MODE";
        case UPDATE_MODE: return "UPDATE_MODE";
        case CLOCK_MODE: return "CLOCK_MODE";
        default: return "UNKNOWN_STATE";
    }
}

/**
 * @brief Validates if a state is a valid startup state
 * @param state The SystemState to validate
 * @return true if state is valid for startup, false otherwise
 * 
 * Only IDLE_MODE and WIFI_MODE are valid startup states.
 * Other states are temporary and cannot be used for system startup.
 */
bool isValidStartupState(SystemState state) {
    return (state == IDLE_MODE || state == WIFI_MODE);
}


/**
 * @brief Prints current system status information to serial output
 * 
 * Displays comprehensive system status including current state,
 * startup state, and other relevant system information.
 */
void printSystemStatus() {
    String stateNames[] = {"IDLE_MODE", "WIFI_MODE", "UPDATE_MODE", "CLOCK_MODE", "ESP_MODE"};
    SystemState currentState = getCurrentState();
    SystemState startupMode = getStartupState();
    
    Serial.println("\n=== System Status ===");
    Serial.printf("Current State: %s\n", stateNames[currentState].c_str());
    Serial.printf("Startup Mode: %s\n", stateNames[startupMode].c_str());
    
    if (currentState == ESP_MODE) {
        Serial.println("WiFi Status: ESP-NOW Mode");
        Serial.printf("MAC Address: %s\n", WiFi.macAddress().c_str());
        Serial.println("ESP-NOW Channel: 1"); // This could be made dynamic later
    } else if (currentState != IDLE_MODE) {
        Serial.printf("WiFi Status: %s\n", isWifiNetworkConnected() ? "Connected" : "Disconnected");
        
        if (isWifiNetworkConnected()) {
            Serial.printf("IP Address: %s\n", WiFi.localIP().toString().c_str());
            Serial.printf("RSSI: %d dBm\n", WiFi.RSSI());
        }
        
        if (currentState == UPDATE_MODE) {
            Serial.printf("AP SSID: %s\n", WiFi.softAPSSID().c_str());
            Serial.printf("AP IP: %s\n", WiFi.softAPIP().toString().c_str());
        }
    }
    
    // Load and display stored credentials
    char storedSSID[32], storedPassword[64];
    if (loadWiFiCredentials(storedSSID, storedPassword)) {
        Serial.printf("Stored SSID: %s\n", storedSSID);
        
        // Show first and last letter with asterisks matching password length
        if (strlen(storedPassword) > 0) {
            if (strlen(storedPassword) == 1) {
                Serial.printf("Stored Password: %c\n", storedPassword[0]);
            } else {
                // Create asterisk string matching password length minus 2 (first and last char)
                String asterisks = "";
                for (int i = 0; i < strlen(storedPassword) - 2; i++) {
                    asterisks += "*";
                }
                Serial.printf("Stored Password: %c%s%c\n", 
                             storedPassword[0], 
                             asterisks.c_str(),
                             storedPassword[strlen(storedPassword) - 1]);
            }
        } else {
            Serial.println("Stored Password: None");
        }
    } else {
        Serial.println("Stored SSID: None");
        Serial.println("Stored Password: None");
    }
    
    Serial.printf("Free Heap: %d bytes\n", ESP.getFreeHeap());
    Serial.printf("Uptime: %lu seconds\n", millis() / 1000);
    Serial.println("====================\n");
}