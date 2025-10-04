/**
 * @file main.cpp
 * @brief Main application entry point
 *
 * This is the primary entry point for the BYTE-90 device firmware. It handles
 * initialization of hardware and software components, and manages the main
 * application loop for different system modes.
 */

 #include "adxl_module.h"
 #include "animation_module.h"
 #include "common.h"
 #include "display_module.h"
 #include "effects_module.h"
 #include "emotes_module.h"
 #include "espnow_module.h"
 #include "flash_module.h"
 #include "gif_module.h"
 #include "menu_module.h"
 #include "motion_module.h"
 #include "serial_module.h"
 #include "system_module.h"
 #include "wifi_module.h"
 
 //==============================================================================
 // GLOBAL VARIABLES
 //==============================================================================
 
 /** @brief Flag indicating if system initialization was successful */
 static bool systemInitialized = false;
 
 //==============================================================================
 // MENU CALLBACK FUNCTIONS
 //==============================================================================
 
 /**
  * @brief Callback for effect changes from menu
  * @param newEffect The newly selected effect
  */
 void onMenuEffectChanged(EffectType newEffect) {
   ESP_LOGI("BYTE-90", "Menu: Effect changed to %s",
            menu_getEffectName(newEffect).c_str());
 
   // Effect is already applied by menu_module through menu_applyEffect()
   // Additional custom logic can be added here if needed
 }
 
 /**
  * @brief Callback for ESP-NOW toggle from menu
  * @param enabled True to enable ESP-NOW, false to disable
  */
 void onMenuESPNowToggled(bool enabled) {
   ESP_LOGI("BYTE-90", "Menu: ESP-NOW toggle requested - %s",
            enabled ? "ON" : "OFF");
 
   // Use existing system function
   if (enabled != menu_getESPNowStatus()) {
     toggleESPNow();
   }
 }
 
 /**
  * @brief Callback for update mode toggle from menu
  * @param enabled True to enable update mode, false to disable
  */
 void onMenuUpdateModeToggled(bool enabled) {
   ESP_LOGI("BYTE-90", "Menu: Update mode toggle requested - %s",
            enabled ? "ON" : "OFF");
 
   SystemMode targetMode =
       enabled ? SystemMode::UPDATE_MODE : SystemMode::ESP_MODE;
   if (getCurrentMode() != targetMode) {
     transitionToMode(targetMode);
   }
 }
 
 /**
  * @brief Callback for deep sleep request from menu
  */
 void onMenuDeepSleepRequested() {
   ESP_LOGI("BYTE-90", "Menu: Deep sleep requested");
 
   // Use existing motion module function
   handleDeepSleep();
 }
 
 //==============================================================================
 // FUNCTION DECLARATIONS
 //==============================================================================
 
 bool initializeHardware();
 bool initializeSoftware();
 void showSystemStartUp();
 void enterCrashMode();
 
 //==============================================================================
 // INITIALIZATION FUNCTIONS
 //==============================================================================
 
 /**
  * @brief Enter crash mode - display crash screen and halt normal operation
  * 
  * This function determines the device type, displays the appropriate crash image,
  * transitions to CRASH_MODE, and initializes serial for debugging.
  */
 void enterCrashMode() {
   // Determine device mode at compile time
   #if DEVICE_MODE == MAC_MODE
     deviceMode = "MAC_MODE";
   #elif DEVICE_MODE == PC_MODE
     deviceMode = "PC_MODE";
   #else
     deviceMode = "BYTE_MODE";
   #endif
   
   ESP_LOGE("BYTE-90", "SYSTEM CRASH - Entering crash mode for %s", deviceMode);
   
   // Display appropriate crash screen based on device mode
   #if DEVICE_MODE == MAC_MODE
     displayStaticImage(CRASH_STATIC, 128, 128);
   #elif DEVICE_MODE == PC_MODE
     displayStaticImage(CRASH_STATIC, 128, 128);
   #else
     displayStaticImage(BYTE_CRASH_STATIC, 128, 128);
   #endif
 
   // Set system mode to CRASH_MODE
   transitionToMode(SystemMode::CRASH_MODE);
 
   // Initialize serial for debugging in crash mode
   if (!initSystem()) {
     ESP_LOGE("BYTE-90", "Failed to initialize serial in crash mode");
   }
   
   ESP_LOGE("BYTE-90", "System halted in crash mode. Serial debugging available. Reset required.");
 }
 
 /**
  * @brief Initialize hardware components
  *
  * Sets up the display, accelerometer, filesystem, and button inputs.
  *
  * @return true if all hardware initialization was successful
  */
 bool initializeHardware() {
   if (!initializeOLED()) {
     ESP_LOGE("BYTE-90", "Failed to initialize Display Module");
     return false;
   }
 
   if (!initializeADXL345()) {
     ESP_LOGE("BYTE-90", "Failed to initialize ADXL Module");
     return false;
   }
 
   if (initializeFS() != FSStatus::FS_SUCCESS) {
     ESP_LOGE(FLASH_LOG, "Filesystem initialization failed!");
     return false;
   }
 
   menu_init();
 
   return true;
 }
 
 /**
  * @brief Initialize software components
  *
  * Sets up the GIF player, ESP-NOW communication, and other software modules.
  *
  * @return true if all software initialization was successful
  */
 bool initializeSoftware() {
   if (!initializeGIFPlayer()) {
     ESP_LOGE("BYTE-90", "GIF player initialization failed");
     return false;
   }
 
   if (!initializeESPNOW()) {
     ESP_LOGW("BYTE-90", "ESP-NOW initialization failed");
   }
 
   menu_setEffectChangeCallback(onMenuEffectChanged);
   menu_setESPNowToggleCallback(onMenuESPNowToggled);
   menu_setUpdateModeToggleCallback(onMenuUpdateModeToggled);
   menu_setDeepSleepCallback(onMenuDeepSleepRequested);
 
   return true;
 }
 
 /**
  * @brief Show startup animation and message
  *
  * Displays the boot logo and animation sequence.
  */
 void showSystemStartUp() {
   displayDOSStartupAnimation();
   // displayBootMessage("ALXV LABS");
   initializeAnimationModule();
   initializeEffectsModule();
   initializeEffectCycling();
 
   clearDisplay();
   playBootAnimation();
 }
 
 //==============================================================================
 // ARDUINO ENTRY POINTS
 //==============================================================================
 void setup() {
   esp_log_level_set("*", ESP_LOG_VERBOSE);
   Serial.begin(115120);
   pinMode(LED_BUILTIN, OUTPUT);
   digitalWrite(LED_BUILTIN, LOW);
 
   if (!initializeHardware() || !initializeSoftware()) {
     enterCrashMode();
     return;  // Important: stop setup() here
   }
   
   systemInitialized = true;
   showSystemStartUp();
 }
 
 void loop() {
   // Handle CRASH_MODE specially - only allow serial debugging
   if (getCurrentMode() == SystemMode::CRASH_MODE) {
     updateSystem(); // This handles serial commands for debugging
   }
   
   // Normal operation only if system initialized successfully
   if (!systemInitialized) {
     return;
   }
 
   menu_update();
   
   // Mode-specific operations
   if (getCurrentMode() == SystemMode::UPDATE_MODE) {
     updateSystem();
     handleWiFiManager();
     if (!menu_isActive()) {
       ADXLDataPolling();
     }
   } else if (getCurrentMode() == SystemMode::ESP_MODE) {
     handleCommunication();
     if (!menu_isActive()) {
       playEmotes(); // This calls ADXLDataPlling() which calls menu_update()
     }
   }
 }