/**
 * @file states_module.h
 * @brief Header for system state management module
 *
 * Provides functionality for managing system states, state transitions,
 * and state machine operations for the BYTE-90 device.
 */

#ifndef STATES_MODULE_H
#define STATES_MODULE_H

#include <Arduino.h>

//==============================================================================
// CONSTANTS & DEFINITIONS
//==============================================================================

static const char *STATES_LOG = "::STATES_MODULE::";

/**
 * @brief System state enumeration defining all possible device operating modes
 */
enum SystemState {
    IDLE_MODE,      ///< WiFi off, power saving mode
    WIFI_MODE,      ///< Normal WiFi operation mode
    ESP_MODE,       ///< ESP-NOW pairing mode
    UPDATE_MODE,    ///< Access Point mode for firmware updates
    CLOCK_MODE      ///< Temporary time synchronization mode
};

//==============================================================================
// PUBLIC API FUNCTIONS
//==============================================================================

/**
 * @brief Initializes the system state manager and sets up initial state
 */
void initSystemStateManager();

/**
 * @brief Updates the system state machine - should be called in main loop
 * Handles state transitions, timeouts, and state-specific logic
 */
void updateSystemStateMachine();

/**
 * @brief Transitions the system to a new state
 * @param newState The target state to transition to
 */
void transitionToState(SystemState newState);

/**
 * @brief Handles cleanup and exit logic for a specific state
 * @param state The state to exit from
 */
void exitState(SystemState state);

/**
 * @brief Handles initialization and entry logic for a specific state
 * @param state The state to enter
 */
void enterState(SystemState state);

/**
 * @brief Gets the current system state
 * @return Current SystemState value
 */
SystemState getCurrentState();

/**
 * @brief Gets the startup state that should be used on system boot
 * @return Startup SystemState value
 */
SystemState getStartupState();

/**
 * @brief Sets the startup state for system boot
 * @param state The SystemState to use on startup
 */
void setStartupState(SystemState state);

/**
 * @brief Gets a human-readable string representation of a state
 * @param state The SystemState to convert to string
 * @return String representation of the state
 */
const char* getStateString(SystemState state);

/**
 * @brief Validates if a state is a valid startup state
 * @param state The SystemState to validate
 * @return true if state is valid for startup, false otherwise
 */
bool isValidStartupState(SystemState state);

/**
 * @brief Prints current system status information to serial output
 */
void printSystemStatus();

/**
 * @brief Enable or disable debug logging for states operations
 * @param enabled true to enable debug logging, false to disable
 */
void setStatesDebug(bool enabled);

#endif /* STATES_MODULE_H */