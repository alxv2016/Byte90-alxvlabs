/**
 * @file main.cpp
 * @brief Main application entry point for BYTE-90 device firmware
 * 
 * This is a CLEANED UP version using the new modular menu system.
 * All old callback functions have been removed - the menu system
 * handles everything internally now!
 */

#include "adxl_module.h"
#include "animation_module.h"
#include "clock_module.h"
#include "clock_sync.h"
#include "common.h"
#include "display_module.h"
#include "effects_core.h"
#include "effects_tints.h"
#include "effects_retro.h"
#include "effects_matrix.h"
#include "effects_themes.h"

#include "emotes_module.h"
#include "espnow_module.h"
#include "flash_module.h"
#include "gif_module.h"
#include "haptics_module.h"
#include "haptics_effects.h"
#include "i2c_module.h"
#include "menu_module.h"
#include "motion_module.h"
#include "serial_module.h"
#include "soundsfx_module.h"
#include "speaker_module.h"

#include "states_module.h"
#include "preferences_module.h"
#include "wifi_module.h"


//==============================================================================
// GLOBAL VARIABLES
//==============================================================================

static bool systemInitialized = false;

//==============================================================================
// UTILITY FUNCTIONS (STATIC)
//==============================================================================

/**
 * @brief Handle device crash modes and debugging
 */
static void checkDeviceCrashModes() {
  ESP_LOGE("BYTE-90", "System in crash mode - checking hardware");
  
  bool i2cOk = initializeI2C();
  bool displayOk = initializeOLED();
  bool motionOk = initializeADXL345();
  bool rtcOk = initializeClock();
  bool hapticsOk = initializeHaptics(HAPTIC_ACTUATOR_ERM);
  bool flashOk = (initializeFS() == FSStatus::FS_SUCCESS);
  bool speakerOk = initializeSpeaker(false);
  
  String failures = "";
  if (!i2cOk) failures += "I2C/ ";
  if (!displayOk) failures += "Display/ ";
  if (!motionOk) failures += "Motion/ ";
  if (!rtcOk) failures += "RTC/ ";
  if (!hapticsOk) failures += "Haptics/ ";
  if (!flashOk) failures += "Flash/ ";
  if (!speakerOk) failures += "Speaker/ ";
  
  if (failures.length() > 0) {
    displayLoadingScreen("System in crash mode", failures.c_str(), "Hardware...[ERROR]", true);
    ESP_LOGE("BYTE-90", "Hardware failures: %s", failures.c_str());
  }
}


//==============================================================================
// INITIALIZATION FUNCTIONS
//==============================================================================

/**
 * @brief Initialize all hardware components
 * @return true if successful, false otherwise
 */
static bool initializeHardware() {

  if (!initializeI2C()) {
    ESP_LOGE("BYTE-90", "I2C initialization failed");
    return false;
  }

  if (!initializeOLED()) {
    ESP_LOGE("BYTE-90", "Display initialization failed");
    return false;
  }

  if (initializeSpeaker(true)) {
    ESP_LOGI("BYTE-90", "Speaker initialized successfully");
  } else {
    ESP_LOGW("BYTE-90", "Speaker initialization skipped (not supported)");
  }

  if (initializeHaptics(HAPTIC_ACTUATOR_ERM)) {
    ESP_LOGI("BYTE-90", "Haptics initialized successfully");
  } else {
    ESP_LOGW("BYTE-90", "Haptics initialization skipped (not supported)");
  }

  if (initializeClock()) {
    ESP_LOGI("BYTE-90", "Clock initialized successfully");
  } else {
    ESP_LOGW("BYTE-90", "Clock initialization skipped (not supported)");
  }

  if (!initializeADXL345()) {
    ESP_LOGE("BYTE-90", "Motion detection (ADXL345) initialization failed");
    return false;
  }

  if (initializeFS() != FSStatus::FS_SUCCESS) {
    ESP_LOGE("BYTE-90", "Flash filesystem initialization failed");
    return false;
  }

  return true;
}

/**
 * @brief Initialize all software components
 * @return true if successful, false otherwise
 */
static bool initializeSoftware() {
  menu_init();
  delay(500);
  if (!initializeGIFPlayer()) {
    ESP_LOGE("BYTE-90", "GIF player initialization failed");
    return false;
  }

  if (!initializeClockSync()) {
    ESP_LOGE("BYTE-90", "Clock sync initialization failed");
    return false;
  }

  sfxInit();

  initPreferencesManager();  // 1. Initialize preferences system first
  delay(500);
  initWiFiManager();         // 2. Then WiFi manager (needs preferences)
  delay(500);
  initSystemStateManager();  // 3. Finally state manager (calls enableWiFi)
  
  return true;
}

/**
 * @brief Show startup animation and message
 */
static void showSystemStartUp() {
  completeDisplaySetup();
  
  // Initialize new modular effects system BEFORE DOS animation
  effectsCore_init();
  effectsRetro_init();
  effectsMatrix_init();
  
  // Note: Individual effect states are already loaded from preferences
  // Now DOS animation can use theme colors
  displayDOSStartupAnimation();
  initializeAnimationModule();

  clearDisplay();
  displayStaticImage(STARTUP_STATIC, 128, 128, true);
  delay(300);
  playBootAnimation();
}

//==============================================================================
// ENTRY POINTS
//==============================================================================

void setup() {
  esp_log_level_set("*", ESP_LOG_VERBOSE);
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  setWifiDebug(false);
  setEspnowDebug(false);
  setStatesDebug(false);


  if (!initializeHardware()) {
    ESP_LOGE("BYTE-90", "Hardware initialization failed!");
    checkDeviceCrashModes();
    delay(1000);
    return;
  }

  if (!initializeSoftware()) {
    ESP_LOGE("BYTE-90", "Software initialization failed!");
    checkDeviceCrashModes();
    delay(1000);
    return;
  }

  systemInitialized = true;
  showSystemStartUp();

  ESP_LOGI("SYSTEM", "System initialization complete");
  printSystemStatus();
  
  ESP_LOGE("BYTE-90", "=== BOOT COMPLETE ===");
}

void loop() {
  if (!systemInitialized)
    return;

  // Always update menu system first
  menu_update();
  
  // Update audio system (always needed)
  audioLoop();
  
  // Handle WiFi preference changes (automatic mode transitions)
  updateSystemStateMachine();
  // Monitor serial update state changes and handle serial commands
  updateSerialState();
  
  // If menu is active, skip system mode operations
  if (menu_isActive()) {
    return;
  }
  
  // Handle different system modes (5-mode architecture)
  if (getCurrentState() == SystemState::UPDATE_MODE) {
    handleWebServer();
  } else if (getCurrentState() == SystemState::CLOCK_MODE) {
    // Clock mode - update the clock display
    updateClockDisplay();
    clockMaintenance();
    clockSyncMaintenance();
  } else if (getCurrentState() == SystemState::ESP_MODE) {
    // ESP-NOW pairing and communication mode
    handleCommunication();
    playEmotes();
    ADXLDataPolling();
  } else if (getCurrentState() == SystemState::WIFI_MODE) {
    playEmotes();
    ADXLDataPolling();
  } else {
    // IDLE mode - minimal system operation
    playEmotes();
    ADXLDataPolling();
  }
}
