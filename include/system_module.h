/**
 * @file system_module.h
 * @brief Header for system mode management functionality
 * 
 * This module provides functions for handling different system operation modes,
 * including ESP-NOW communication mode and OTA update mode. It manages
 * transitions between modes and updates the display accordingly.
 */

 #ifndef SYSTEM_MODULE_H
 #define SYSTEM_MODULE_H
 
 //==============================================================================
 // CONSTANTS & DEFINITIONS
 //==============================================================================
 
 /** @brief Log tag for System module messages */
 static const char* SYSTEM_LOG = "::SYSTEM_MODULE::";
 
 //==============================================================================
 // TYPE DEFINITIONS
 //==============================================================================
 
 /**
  * @brief System operation modes
  */
 enum class SystemMode {
   ESP_MODE,     /**< Default mode - ESP-NOW communication */
   UPDATE_MODE,   /**< WiFi configuration and OTA update mode */
   CRASH_MODE    /**< Crash mode - display crash image */
 };
 
 //==============================================================================
 // PUBLIC API FUNCTIONS
 //==============================================================================
 
 /**
  * @brief Get the current system operation mode
  * 
  * @return Current SystemMode value
  */
 SystemMode getCurrentMode();

 /**
  * @brief Initialize the system with all required modules
  * 
  * Sets up all system modules including serial communication for debugging
  * and alternative firmware updates.
  * 
  * @return true if initialization successful, false otherwise
  */
 bool initSystem();

 /**
  * @brief Main system update function - call this in your main loop
  * 
  * Handles all system-level updates including serial communication,
  * mode management, and other system services.
  */
 void updateSystem();
 
 /**
  * @brief Transition the system to a new operation mode
  * 
  * Handles the complete transition process: cleanup, initialization,
  * and display update.
  * 
  * @param targetMode The mode to transition to
  * @return true if transition successful, false otherwise
  */
 bool transitionToMode(SystemMode targetMode);
 
 /**
  * @brief Toggle between ESP-NOW and Update modes
  * 
  * Switches from current mode to the alternative mode.
  */
 void toggleSystemMode();
 
 /**
  * @brief Update the display based on the new system mode
  * 
  * @param newMode The mode to update display for
  */
 void updateDisplayForMode(SystemMode newMode);

 /**
  * @brief Check if system can safely transition modes
  * 
  * Verifies that no critical operations (like serial updates) are in progress
  * that would prevent safe mode transitions.
  * 
  * @return true if mode transition is safe, false otherwise
  */
 bool canTransitionModes();
 
 #endif /* SYSTEM_MODULE_H */