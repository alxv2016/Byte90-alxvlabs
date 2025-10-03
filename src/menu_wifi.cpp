/**
 * @file menu_wifi.cpp
 * @brief WiFi and network settings menu implementation
 *
 * Provides WiFi and network settings menu functionality for the BYTE-90 device,
 * including WiFi management, update mode, and network status monitoring.
 * 
 * This module handles:
 * - WiFi menu items and navigation
 * - WiFi toggle and management functions
 * - Update mode entry and exit handling
 * - WiFi restart and forget functionality
 * - Network status submenu and monitoring
 * - Debug logging and menu context management
 * - Integration with WiFi, OTA, preferences, and states modules
 * - Button press handling for update mode exit
 */

 #include "menu_wifi.h"
 #include "ota_module.h"
 #include "preferences_module.h"
 #include "serial_module.h"
 #include "wifi_common.h"
 #include "haptics_module.h"
 #include "gif_module.h"
 #include "menu_module.h"
 #include "espnow_module.h"
 #include "states_module.h"
 #include "wifi_module.h"
 #include "soundsfx_module.h"
 #include <stdarg.h>

//==============================================================================
// DEBUG SYSTEM
//==============================================================================

// Debug control - set to true to enable debug logging
static bool menuWifiDebugEnabled = false;

/**
 * @brief Centralized debug logging function for menu WiFi operations
 * @param format Printf-style format string
 * @param ... Variable arguments for format string
 */
static void menuWifiDebug(const char* format, ...) {
  if (!menuWifiDebugEnabled) {
    return;
  }
  
  va_list args;
  va_start(args, format);
  esp_log_writev(ESP_LOG_INFO, "MENU_WIFI", format, args);
  va_end(args);
}

/**
 * @brief Enable or disable debug logging for menu WiFi operations
 * @param enabled true to enable debug logging, false to disable
 * 
 * @example
 * // Enable debug logging
 * setMenuWifiDebug(true);
 * 
 * // Disable debug logging  
 * setMenuWifiDebug(false);
 */
void setMenuWifiDebug(bool enabled) {
  menuWifiDebugEnabled = enabled;
  if (enabled) {
    ESP_LOGI("MENU_WIFI", "Menu WiFi debug logging enabled");
  } else {
    ESP_LOGI("MENU_WIFI", "Menu WiFi debug logging disabled");
  }
}

//==============================================================================
// WIFI STATUS FLAGS
//==============================================================================

static bool wifiEnabled = false;

//==============================================================================
// WIFI TOGGLE FUNCTIONS
//==============================================================================
 
/**
 * @brief Toggle WiFi mode on/off
 * @param enabled true to enable WiFi mode, false to disable
 * 
 * Toggles WiFi mode on or off, updates preferences, and transitions to the
 * appropriate system state. Exits the menu after the operation.
 */
static void toggleWifi(bool enabled) {
  if (enabled) {
      menuWifiDebug("Enabling WiFi - transitioning to WIFI_MODE");
      setWiFiModeEnabled(true);  
      transitionToState(SystemState::WIFI_MODE);
  } else {
      menuWifiDebug("Disabling WiFi - transitioning to IDLE_MODE");
      setWiFiModeEnabled(false);  
      transitionToState(SystemState::IDLE_MODE);
  }

  menu_exit();
}
 
/**
 * @brief Enter update mode
 * 
 * Prepares the device for update mode by initializing serial interface,
 * disabling haptics for power conservation, and transitioning to update state.
 * Exits the menu and sets up the device for firmware updates.
 */
 static void enterUpdateModeMenu() {
   menuWifiDebug("Starting update mode preparation");

   menu_exit();
   // Initialize serial interface (required for update mode)
   bool serialSuccess = initSerial();
   if (serialSuccess) {
     menuWifiDebug("Serial interface initialized successfully");
   } else {
     ESP_LOGE("MENU_WIFI", "Failed to initialize serial interface");
   }
   // Disable haptics to save power during update mode
   disableHaptics();
   // Transition to update mode using system module
   transitionToState(SystemState::UPDATE_MODE);
   if (getCurrentState() == SystemState::UPDATE_MODE) {
     menuWifiDebug("Successfully entered update mode");
   } else {
     ESP_LOGE("MENU_WIFI", "Failed to transition to update mode");
   }
 
 }
 
/**
 * @brief Handle button press when in update mode to exit
 * @return true if button press was handled (update mode was exited), false otherwise
 * 
 * Handles button press events when the device is in update mode. Exits update mode
 * and transitions to the appropriate state based on WiFi mode settings. Returns true
 * if the button press was handled (device was in update mode), false otherwise.
 */
 bool handleUpdateModeButtonPress() {
   if (getCurrentState() != SystemState::UPDATE_MODE) {
     return false; // Not in update mode
   }
 
   menuWifiDebug("Button pressed in update mode - exiting...");
   notifyUpdateModeExit();
  // Check if WiFi mode is enabled to determine where to transition
  if (isWifiNetworkConnected()) {
    menuWifiDebug("WiFi connected - transitioning to WIFI_MODE");
    setWiFiModeEnabled(true);  // Update preference to match reality
    transitionToState(SystemState::WIFI_MODE);
  } else if (getWiFiModeEnabled()) {
    menuWifiDebug("WiFi mode enabled but not connected - transitioning to WIFI_MODE");
    transitionToState(SystemState::WIFI_MODE);
  } else {
    menuWifiDebug("WiFi not connected and mode disabled - transitioning to IDLE_MODE");
    transitionToState(SystemState::IDLE_MODE);
  }
  
  menuWifi_init(); 
  sfxPlay("shutdown", 0, true);
  menuWifiDebug("Successfully exited update mode");
  return true; // Button press was handled
 }
 
/**
 * @brief Restart WiFi connection
 * 
 * Restarts the WiFi connection by disconnecting and reconnecting using
 * stored credentials. Only performs restart if WiFi is currently connected.
 */
static void restartWifi() {
  if (isWifiNetworkConnected()) {
      menuWifiDebug("Restarting WiFi connection");
      
      disconnectFromWiFi();  // Direct API call
      delay(500);
      connectToWiFiFromPreferences();     // Direct API call
      
  } else {
      menuWifiDebug("WiFi not connected - nothing to restart");
  }
  
}
 
/**
 * @brief Forget WiFi credentials and disconnect
 * 
 * Disconnects from the current WiFi network and clears all stored
 * WiFi credentials from persistent storage. Used to reset network settings.
 */
static void forgetWifi() {
  menuWifiDebug("Forgetting WiFi credentials");
  
  // Disconnect from current network
  disconnectFromWiFi();
  // Clear stored credentials using new PreferencesManager
  clearWiFiCredentials();
}
 
 //==============================================================================
 // STATUS SUBMENU DEFINITION
 //==============================================================================
 
 /**
  * @brief WiFi status submenu items with read-only information
  */
 static MenuItem wifiStatusMenu[] = {
     DEFINE_MENU_INFO(status, MENU_LABEL_WIFI_STATUS,
                      getCurrentWiFiStatus),
     DEFINE_MENU_INFO(network, MENU_LABEL_WIFI,
                      getCurrentWiFiNetwork),
     DEFINE_MENU_INFO(signal, MENU_LABEL_WIFI_SIGNAL,
                      getCurrentWiFiSignalStrength),
     DEFINE_MENU_INFO(ip, MENU_LABEL_WIFI_IP, getCurrentWiFiIP),
     DEFINE_MENU_BACK()};
 
 static const int wifiStatusMenuCount = MENU_ITEM_COUNT(wifiStatusMenu);
 
 //==============================================================================
 // MENU DEFINITION
 //==============================================================================
 
 /**
  * @brief WiFi menu items
  */
 static MenuItem wifiMenu[] = {
     DEFINE_MENU_TOGGLE(wifi_toggle, MENU_LABEL_WIFI, toggleWifi, &wifiEnabled),
     DEFINE_MENU_SUBMENU(wifi_status, MENU_LABEL_WIFI_STATUS, wifiStatusMenu,
                         wifiStatusMenuCount),
     DEFINE_MENU_ACTION(update_mode, MENU_LABEL_UPDATE_MODE, enterUpdateModeMenu),
     DEFINE_MENU_ACTION(restart_wifi, MENU_LABEL_RESTART_WIFI, restartWifi),
     DEFINE_MENU_ACTION(forget_wifi, MENU_LABEL_FORGET_WIFI, forgetWifi),
     DEFINE_MENU_BACK()};
 
 static const int wifiMenuCount = MENU_ITEM_COUNT(wifiMenu);
 
//==============================================================================
// PUBLIC API FUNCTIONS
//==============================================================================
 
/**
 * @brief Initialize WiFi menu module
 * 
 * Initializes the WiFi menu module by loading the WiFi enabled state from
 * preferences and setting up the menu system for WiFi management.
 */
 void menuWifi_init() {
  wifiEnabled = getWiFiModeEnabled();
  // Load WiFi enabled state from preferences
  menuWifiDebug("WiFi mode loaded from preferences: %s", 
     wifiEnabled ? "enabled" : "disabled");
 }
 
/**
 * @brief Get the WiFi menu items
 * @return Pointer to WiFi menu array
 * 
 * Returns a pointer to the WiFi menu items array containing all available
 * WiFi management options including toggle, update mode, restart, and forget.
 */
 MenuItem *menuWifi_getItems() { return wifiMenu; }

/**
 * @brief Get the number of WiFi items
 * @return Number of WiFi menu items
 * 
 * Returns the total number of WiFi menu items available for selection.
 * Used by the menu system to determine menu size and navigation bounds.
 */
 int menuWifi_getItemCount() { return wifiMenuCount; }
 
/**
 * @brief Get the WiFi menu context
 * @param context Pointer to context to populate
 * 
 * Populates the provided MenuContext structure with WiFi menu information
 * including items array, item count, selected index, and menu title. Used by
 * the menu system to initialize WiFi management menus.
 */
 void menuWifi_getContext(MenuContext *context) {
   if (context) {
     context->items = wifiMenu;
     context->itemCount = wifiMenuCount;
     context->selectedIndex = 0;
     context->title = MENU_LABEL_WIFI;
   }
 }
 
 //==============================================================================
 // STATUS SUBMENU API
 //==============================================================================
 
/**
 * @brief Get the WiFi status submenu items
 * @return Pointer to WiFi status submenu array
 * 
 * Returns a pointer to the WiFi status submenu items array containing
 * network status information and navigation options.
 */
 MenuItem *menuWifiStatus_getItems() { return wifiStatusMenu; }

/**
 * @brief Get the number of WiFi status items
 * @return Number of WiFi status menu items
 * 
 * Returns the total number of WiFi status menu items available.
 * Used by the menu system to determine submenu size and navigation bounds.
 */
 int menuWifiStatus_getItemCount() { return wifiStatusMenuCount; }

/**
 * @brief Get the WiFi status submenu context
 * @param context Pointer to context to populate
 * 
 * Populates the provided MenuContext structure with WiFi status submenu information
 * including items array, item count, selected index, and menu title. Sets the
 * selected index to the "GO BACK" option since other items are read-only.
 */
 void menuWifiStatus_getContext(MenuContext *context) {
   if (context) {
     context->items = wifiStatusMenu;
     context->itemCount = wifiStatusMenuCount;
     context->selectedIndex =
         4; // Select "GO BACK" (last item) since others are read-only
     context->title = MENU_LABEL_WIFI_STATUS;
   }
 }