/**
 * @file system_module.cpp
 * @brief Implementation of system mode management functionality
 *
 * This module provides functions for handling different system operation modes,
 * including ESP-NOW communication mode and OTA update mode. It manages
 * transitions between modes and updates the display accordingly.
 */

 #include "system_module.h"
 #include "common.h"
 #include "display_module.h"
 #include "emotes_module.h"
 #include "espnow_module.h"
 #include "gif_module.h"
 #include "menu_module.h"
 #include "serial_module.h"
 #include "wifi_module.h"
 
 //==============================================================================
 // GLOBAL VARIABLES
 //==============================================================================
 
 /** @brief Current system operation mode */
 SystemMode currentMode = SystemMode::ESP_MODE;
 
 //==============================================================================
 // MODE MANAGEMENT FUNCTIONS
 //==============================================================================
 
 /**
  * @brief Get the current system operation mode
  *
  * @return Current SystemMode value
  */
 SystemMode getCurrentMode() { return currentMode; }
 
 /**
  * @brief Check if the system is already in the target mode
  *
  * @param targetMode The mode to check against current mode
  * @return true if already in target mode, false otherwise
  */
 static bool isAlreadyInTargetMode(SystemMode targetMode) {
   return currentMode == targetMode;
 }
 
 /**
  * @brief Clean up resources from the current mode before transition
  *
  * Handles shutting down ESP-NOW or WiFi services as needed when
  * transitioning between modes.
  *
  * @return true if cleanup successful, false otherwise
  */
 static bool cleanupCurrentMode() {
   // Don't clean up if we're in or transitioning to crash mode
   if (currentMode == SystemMode::CRASH_MODE) {
     return true;
   }
   
   if (currentMode == SystemMode::ESP_MODE) {
     // When switching from ESP mode to WiFi mode
     if (getCurrentESPNowState() == ESPNowState::ON) {
       // ESP_LOGI log removed
       if (!toggleESPNow()) {
         ESP_LOGE(SYSTEM_LOG, "Failed to disable ESP-NOW communications");
         return false;
       }
     }
   } else {
     // When switching from WiFi mode to ESP mode
     // Use WiFi module's cleanup function instead of direct WiFi manipulation
     cleanupWiFiServices();
     cleanupSerial();
     ESP_LOGI(SYSTEM_LOG, "WiFi services cleaned up for mode transition");
   }
   return true;
 }
 
 /**
  * @brief Initialize the target mode after transition
  *
  * Sets up necessary services for the new mode.
  *
  * @param targetMode The mode to initialize
  * @return true if initialization successful, false otherwise
  */
 static bool initializeTargetMode(SystemMode targetMode) {
   bool success = false;
 
   // CRASH_MODE doesn't need initialization - it's a halt state
   if (targetMode == SystemMode::CRASH_MODE) {
     return true;
   }
 
   if (targetMode == SystemMode::ESP_MODE) {
     // Ensure we're in STA mode for ESP-NOW - but let WiFi module clean up first
     // Only set mode if we're not already in a compatible state
     success = prepareForESPMode();
     if (!success) {
       ESP_LOGE(SYSTEM_LOG, "Failed to prepare WiFi for ESP-NOW mode");
     }
   } else if (targetMode == SystemMode::UPDATE_MODE) {
     ESP_LOGI(SYSTEM_LOG, "DEBUG: Initializing UPDATE_MODE");
     // Initialize WiFi Manager (sets up AP mode)
     success = prepareForUpdateMode();
     if (!success) {
       ESP_LOGE(SYSTEM_LOG, "Failed to initialize WiFi Manager in AP mode");
     }
     ESP_LOGI(SYSTEM_LOG, "DEBUG: About to call initSerial()");
     initSerial();
 
     // Initialize serial interface for USB updates (non-critical for
     // UPDATE_MODE)
     // if (!initSerial()) {
     //   ESP_LOGW(SYSTEM_LOG, "Serial interface initialization failed - WiFi "
     //                        "updates still available");
     //   // Don't fail UPDATE_MODE if serial fails - WiFi updates still work
     // } else {
     //   ESP_LOGI(
     //       SYSTEM_LOG,
     //       "Serial interface initialized - both WiFi and USB updates available");
     // }
   }
   return success;
 }
 
 /**
  * @brief Update the display based on the new system mode
  *
  * @param newMode The mode to update display for
  */
 void updateDisplayForMode(SystemMode newMode) {
   // Important to stop GIF playback before newMode is detected
   // this prevents a timing issue for any GIF currently in progress
   stopGifPlayback();
   
   if (newMode == SystemMode::UPDATE_MODE) {
     clearDisplay();
     delay(200);
     displayStaticImage(UPDATER_STATIC, 128, 128);
   }
   // CRASH_MODE display is handled in main.cpp's enterCrashMode()
   // We don't update display here to avoid overwriting the crash screen
 }
 
 //==============================================================================
 // PUBLIC API FUNCTIONS
 //==============================================================================
 /**
  * @brief Initialize the system with all required modules
  *
  * Sets up all system modules including serial communication for debugging
  * and alternative firmware updates.
  *
  * @return true if initialization successful, false otherwise
  */
 bool initSystem() {
   ESP_LOGI(SYSTEM_LOG, "Initializing system modules...");
 
   // Initialize serial communication (non-critical)
   // Serial works independently of system mode for debugging/updates
   if (!initSerial()) {
     ESP_LOGW(SYSTEM_LOG, "Serial interface initialization failed - continuing "
                          "without serial support");
     // Non-critical failure, continue with other initialization
   } else {
     ESP_LOGI(SYSTEM_LOG, "Serial interface initialized successfully");
   }
 
   // Your other system initialization code can go here
   // For example, display, filesystem, etc.
 
   ESP_LOGI(SYSTEM_LOG, "System initialization complete");
   return true;
 }
 
 /**
  * @brief Main system update function - call this in your main loop
  *
  * Handles all system-level updates including serial communication,
  * mode management, and other system services.
  */
 void updateSystem() {
   // Handle serial commands and updates
   handleSerialCommands();
   
   // Check if serial update is in progress and handle appropriately
   if (isSerialUpdateActive()) {
     // During serial update, we may want to pause other activities
     // or provide visual feedback
     SerialUpdateState serialState = getSerialUpdateState();
 
     // Optional: Update display to show serial update status
     static SerialUpdateState lastSerialState = SerialUpdateState::IDLE;
     if (serialState != lastSerialState) {
       switch (serialState) {
       case SerialUpdateState::RECEIVING:
         ESP_LOGI(SYSTEM_LOG, "Serial firmware update in progress...");
         // Could update display here if desired
         break;
       case SerialUpdateState::PROCESSING:
         ESP_LOGI(SYSTEM_LOG, "Processing serial firmware update...");
         break;
       case SerialUpdateState::SUCCESS:
         ESP_LOGI(SYSTEM_LOG, "Serial firmware update completed successfully");
         break;
       case SerialUpdateState::ERROR:
         ESP_LOGW(SYSTEM_LOG, "Serial firmware update failed");
         break;
       default:
         break;
       }
       lastSerialState = serialState;
     }
   }
 
   // Continue with other system updates
   // Note: You might want to add other system-level updates here
   // that need to run regardless of the current mode
 }
 
 /**
  * @brief Transition the system to a new operation mode
  *
  * Handles the complete transition process: cleanup, initialization,
  * and display update.
  *
  * @param targetMode The mode to transition to
  * @return true if transition successful, false otherwise
  */
 bool transitionToMode(SystemMode targetMode) {
   // CRASH_MODE is a special case - always allow transition
   if (targetMode == SystemMode::CRASH_MODE) {
     currentMode = targetMode;
     ESP_LOGI(SYSTEM_LOG, "Transitioned to CRASH_MODE");
     return true;
   }
   
   // Skip if already in target mode
   if (isAlreadyInTargetMode(targetMode)) {
     return true;
   }
   
   // Don't allow transitions FROM crash mode (except during initial crash setup)
   if (currentMode == SystemMode::CRASH_MODE) {
     ESP_LOGW(SYSTEM_LOG, "Cannot transition from CRASH_MODE - reset required");
     return false;
   }
     
   // If serial update is active, don't allow mode transitions
   if (isSerialUpdateActive()) {
     ESP_LOGW(SYSTEM_LOG, "Cannot change modes during serial update");
     return false;
   }
 
   if (targetMode == SystemMode::UPDATE_MODE && !canTransitionModes()) {
     ESP_LOGW(SYSTEM_LOG, "System not ready for mode transition");
     return false;
   }
 
   menu_resetStates();
 
   if (!cleanupCurrentMode())
     return false;
 
   bool success = initializeTargetMode(targetMode);
 
   if (success) {
     // Update the global state
     currentMode = targetMode;
     updateDisplayForMode(targetMode);
   }
 
   return success;
 }
 
 /**
  * @brief Toggle between ESP-NOW and Update modes
  *
  * Switches from current mode to the alternative mode.
  */
 void toggleSystemMode() {
   // Prevent mode switching during serial updates
   if (isSerialUpdateActive()) {
     ESP_LOGW(SYSTEM_LOG, "Cannot toggle system mode during serial update");
     return;
   }
   
   // Prevent mode switching from crash mode
   if (currentMode == SystemMode::CRASH_MODE) {
     ESP_LOGW(SYSTEM_LOG, "Cannot toggle system mode from CRASH_MODE");
     return;
   }
 
   SystemMode targetMode = (currentMode == SystemMode::ESP_MODE)
                               ? SystemMode::UPDATE_MODE
                               : SystemMode::ESP_MODE;
   if (!transitionToMode(targetMode)) {
     ESP_LOGE("SystemMode", "Failed to toggle system mode");
     // Handle failure - could try to revert to previous state if needed
   }
 }
 
 /**
  * @brief Check if system can safely transition modes
  *
  * Verifies that no critical operations (like serial updates) are in progress
  * that would prevent safe mode transitions.
  *
  * @return true if mode transition is safe, false otherwise
  */
 bool canTransitionModes() { 
   // Can't transition if in crash mode
   if (currentMode == SystemMode::CRASH_MODE) {
     return false;
   }
   return !isSerialUpdateActive(); 
 }