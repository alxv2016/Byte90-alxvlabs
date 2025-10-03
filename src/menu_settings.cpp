/**
 * @file menu_settings.cpp
 * @brief System settings menu implementation
 *
 * Provides system settings menu functionality for the BYTE-90 device,
 * including audio, haptic feedback, and effects management.
 *
 * This module handles:
 * - Audio system toggle and management
 * - Haptic feedback control and configuration
 * - Effects clearing and reset functionality
 * - Dynamic menu building based on hardware support
 * - Settings persistence using preferences module
 * - Debug logging and system integration
 * - Menu context management and item counting
 * - Integration with speaker, haptics, and effects modules
 */

 #include "menu_settings.h"
 #include "display_module.h"
 #include "effects_core.h"
 #include "haptics_effects.h"
 #include "haptics_module.h"
 #include "preferences_module.h"
 #include "speaker_module.h"
 #include "common.h"
 #include <stdarg.h>
 
//==============================================================================
// DEBUG SYSTEM
//==============================================================================

// Debug control - set to true to enable debug logging
static bool menuSettingsDebugEnabled = false;

/**
 * @brief Centralized debug logging function for menu settings operations
 * @param format Printf-style format string
 * @param ... Variable arguments for format string
 */
static void menuSettingsDebug(const char* format, ...) {
  if (!menuSettingsDebugEnabled) {
    return;
  }
  
  va_list args;
  va_start(args, format);
  esp_log_writev(ESP_LOG_INFO, "MENU", format, args);
  va_end(args);
}

/**
 * @brief Enable or disable debug logging for menu settings operations
 * @param enabled true to enable debug logging, false to disable
 * 
 * @example
 * // Enable debug logging
 * setMenuSettingsDebug(true);
 * 
 * // Disable debug logging  
 * setMenuSettingsDebug(false);
 */
void setMenuSettingsDebug(bool enabled) {
  menuSettingsDebugEnabled = enabled;
  if (enabled) {
    ESP_LOGI("MENU", "Menu settings debug logging enabled");
  } else {
    ESP_LOGI("MENU", "Menu settings debug logging disabled");
  }
}

//==============================================================================
// SETTINGS STATUS FLAGS
//==============================================================================

static bool audioEnabled = false;
 static bool hapticEnabled = true;
 
 //==============================================================================
 // SETTINGS TOGGLE FUNCTIONS
 //==============================================================================
 
 /**
  * @brief Toggle audio functionality
  * @param enabled true to enable audio, false to disable
  * 
  * Enables or disables the audio system based on the provided parameter.
  * When enabling audio, initializes the speaker system. When disabling,
  * the audio system is turned off (though complete disable functionality
  * may not be available in all configurations).
  */
 static void toggleAudio(bool enabled) {
     audioEnabled = enabled;
 
     if (enabled) {
         // Enable audio system
         initializeSpeaker(false); // Don't check preferences, force enable
     } else {
         // Disable audio system
         // Audio disable functionality may not be available
     }
 
     // Save audio preference using preferences module
     setAudioEnabled(enabled);
 }
 
 /**
  * @brief Toggle haptic feedback
  * @param enabled true to enable haptic feedback, false to disable
  * 
  * Enables or disables haptic feedback functionality. When enabling haptics,
  * the haptic system is initialized and a confirmation click is provided
  * to verify the system is working properly. When disabling, the haptic
  * system is turned off.
  */
 static void toggleHaptic(bool enabled) {
     hapticEnabled = enabled;
 
     if (enabled) {
         enableHaptics();
 
         // Give a feedback click to confirm it's working
         if (isHapticsReady()) {
             quickClick(2); // Medium intensity click
         } else {
             ESP_LOGW("MENU_SETTINGS",
                      "Haptic feedback enabled but hardware not ready");
         }
     } else {
         disableHaptics();
     }
 
     // Save haptic preference using preferences module
     setHapticEnabled(enabled);
 }
 
 /**
  * @brief Clear all effects and themes
  * 
  * Disables all visual effects and clears user preferences related to
  * themes and effects. This function provides a way to reset the device
  * to a clean state without any applied visual effects.
  */
 static void clearEffects() {
     // Disable all effects using new modular system
     effectsCore_disableAllEffects();
 
     // Clear effect preferences using preferences module
     clearUserPreferences();
 }
 
 //==============================================================================
 // CONSERVATIVE DYNAMIC MENU APPROACH
 //==============================================================================
 
 // Pre-defined menu items (safe initialization)
 static MenuItem audioItem = DEFINE_MENU_TOGGLE(audio, MENU_LABEL_AUDIO, toggleAudio, &audioEnabled);
 static MenuItem hapticItem = DEFINE_MENU_TOGGLE(haptic, MENU_LABEL_HAPTIC, toggleHaptic, &hapticEnabled);
 static MenuItem clearEffectsItem = DEFINE_MENU_ACTION(clear_effects, MENU_LABEL_CLEAR_EFFECTS, clearEffects);
 static MenuItem backItem = DEFINE_MENU_BACK();
 
 // Dynamic menu array (pointers to pre-defined items)
 static MenuItem* dynamicSettingsMenu[10];
 static int dynamicSettingsCount = 0;
 
 // Actual menu items for MenuContext
 static MenuItem actualSettingsItems[10];
 
 //==============================================================================
 // SAFE MENU BUILDING
 //==============================================================================
 
 /**
  * @brief Build settings menu using safe pointer assignments
  * 
  * Dynamically builds the settings menu based on hardware support and
  * current system state. The menu is constructed using a conservative
  * approach with pre-defined menu items and safe pointer assignments.
  */
 static void buildSettingsMenu() {
     dynamicSettingsCount = 0;
     
     // Conditionally add audio and haptic toggles for SERIES_2
     if (checkHardwareSupport()) {
         menuSettingsDebug("Adding audio and haptic controls for SERIES_2 hardware");
         dynamicSettingsMenu[dynamicSettingsCount++] = &audioItem;
         dynamicSettingsMenu[dynamicSettingsCount++] = &hapticItem;
     }
     
     // Always include clear effects action and back button
     dynamicSettingsMenu[dynamicSettingsCount++] = &clearEffectsItem;
     dynamicSettingsMenu[dynamicSettingsCount++] = &backItem;
     
     // Copy to actual items array
     for (int i = 0; i < dynamicSettingsCount; i++) {
         actualSettingsItems[i] = *dynamicSettingsMenu[i];
     }
     
     menuSettingsDebug("Built settings menu with %d items", dynamicSettingsCount);
 }
 
 //==============================================================================
 // PRIVATE FUNCTIONS
 //==============================================================================
 
 /**
  * @brief Update audio status based on current state
  * 
  * Updates the internal audio enabled flag by reading the current
  * audio setting from the preferences module. This ensures the
  * internal state matches the persisted preference.
  */
 static void updateAudioStatus() {
     // Check current audio state from preferences module
     audioEnabled = getAudioEnabled();
 }
 
 /**
  * @brief Update haptic status based on current state
  * 
  * Updates the internal haptic enabled flag by reading the current
  * haptic setting from the preferences module. This ensures the
  * internal state matches the persisted preference.
  */
 static void updateHapticStatus() {
     // Check current haptic setting from preferences module
     hapticEnabled = getHapticEnabled();
 }
 
 //==============================================================================
 // PUBLIC API IMPLEMENTATION
 //==============================================================================
 
/**
 * @brief Initialize settings module
 * 
 * Initializes the settings menu module by updating status flags from the current
 * system state and building the dynamic settings menu. This function should be
 * called during system initialization to set up the settings menu with the
 * current audio and haptic preferences.
 */
void menuSettings_init() {
     // Update status flags from current system state
     menuSettings_updateStatus();
     
     // Build the menu
     buildSettingsMenu();
     
     menuSettingsDebug("Settings module initialized");
 }
 
/**
 * @brief Update settings status flags
 * 
 * Updates the internal status flags for audio and haptic settings by reading
 * the current values from the preferences module. This function also rebuilds
 * the menu to ensure it reflects the current system state.
 */
void menuSettings_updateStatus() {
     updateAudioStatus();
     updateHapticStatus();
     
     // Rebuild menu to ensure it's current
     buildSettingsMenu();
 }
 
/**
 * @brief Get the settings menu items
 * @return Pointer to settings menu array
 * 
 * Returns a pointer to the array of menu items for the settings menu.
 * The menu is dynamically built based on hardware support and current
 * system state before returning the items.
 * 
 */
MenuItem* menuSettings_getItems() {
     // Ensure menu is built and current
     buildSettingsMenu();
     return actualSettingsItems;
 }
 
/**
 * @brief Get the number of setting items
 * @return Number of setting menu items
 * 
 * Returns the current number of items in the settings menu. The count
 * is dynamically determined based on hardware support and system state.
 * 
 */
int menuSettings_getItemCount() {
     // Ensure menu is built
     buildSettingsMenu();
     return dynamicSettingsCount;
 }
 
/**
 * @brief Get the settings menu context
 * @param context Pointer to context to populate
 * 
 * Populates the provided MenuContext structure with the current settings
 * menu information. The context includes all necessary data for menu
 * display and navigation.
 */
void menuSettings_getContext(MenuContext* context) {
     if (context) {
         // Ensure menu is built and current
         buildSettingsMenu();
         
         context->items = actualSettingsItems;
         context->itemCount = dynamicSettingsCount;
         context->selectedIndex = 0;
         context->title = MENU_LABEL_SETTINGS;
         
         ESP_LOGD("SETTINGS", "Settings context: %d items, title=%s", 
                  dynamicSettingsCount, MENU_LABEL_SETTINGS);
     }
 }