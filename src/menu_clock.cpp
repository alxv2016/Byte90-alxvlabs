/**
 * @file menu_clock.cpp
 * @brief Clock and time settings menu implementation
 *
 * Provides clock and time settings functionality for the BYTE-90 device,
 * including clock mode management, timezone selection, and time synchronization.
 *
 * This module handles:
 * - Clock mode entry and exit with power management
 * - Timezone selection and configuration
 * - Time synchronization and preferences management
 * - Integration with clock module, clock sync, and preferences
 * - Debug logging and system integration
 * - Menu context management and item counting
 * - System state transitions and mode management
 * - WiFi mode coordination for time synchronization
 * - Haptic feedback management for power optimization
 */

#include "menu_clock.h"
#include "clock_module.h"
#include "clock_sync.h"
#include "preferences_module.h"
#include "menu_module.h"
#include "states_module.h"
#include "wifi_module.h"
#include "haptics_module.h"
#include <stdarg.h>

//==============================================================================
// DEBUG SYSTEM
//==============================================================================

// Debug control - set to true to enable debug logging
static bool menuClockDebugEnabled = false;

/**
 * @brief Centralized debug logging function for menu clock operations
 * @param format Printf-style format string
 * @param ... Variable arguments for format string
 */
static void menuClockDebug(const char* format, ...) {
  if (!menuClockDebugEnabled) {
    return;
  }
  
  va_list args;
  va_start(args, format);
  esp_log_writev(ESP_LOG_INFO, "MENU_CLOCK", format, args);
  va_end(args);
}

/**
 * @brief Enable or disable debug logging for menu clock operations
 * @param enabled true to enable debug logging, false to disable
 * 
 * @example
 * // Enable debug logging
 * setMenuClockDebug(true);
 * 
 * // Disable debug logging  
 * setMenuClockDebug(false);
 */
void setMenuClockDebug(bool enabled) {
  menuClockDebugEnabled = enabled;
  if (enabled) {
    ESP_LOGI("MENU_CLOCK", "Menu clock debug logging enabled");
  } else {
    ESP_LOGI("MENU_CLOCK", "Menu clock debug logging disabled");
  }
}

//==============================================================================
// CLOCK STATUS FLAGS
//==============================================================================

static bool wifiEnabled = false;

//==============================================================================
// CLOCK TOGGLE FUNCTIONS
//==============================================================================

 /**
  * @brief Toggle enhanced clock display mode with WiFi auto-connect and time sync
  * 
  * Enters the enhanced clock display mode, which provides a dedicated
  * clock interface with automatic WiFi connection and time synchronization.
  * This mode is optimized for power efficiency and time display.
  */
 static void enterClockMode() {
    menuClockDebug("Entering clock mode");
    
    // Exit menu first
    menu_exit();
    
    // Disable haptics to save power
    disableHaptics();
    
    // Transition to clock mode
    transitionToState(SystemState::CLOCK_MODE);
    if (getCurrentState() == SystemState::CLOCK_MODE) {
        menuClockDebug("Successfully entered clock mode");
    } else {
        ESP_LOGE("MENU_CLOCK", "Failed to transition to clock mode");
    }
}

 /**
  * @brief Handle button press when in clock mode
  * @return true if clock mode was exited, false if not in clock mode
  * 
  * Handles button press events when the device is in clock mode. This function
  * should be called from the button handler to provide single-click exit
  * functionality from clock mode.
  */
 bool handleClockModeButtonPress() {
    if (getCurrentState() != SystemState::CLOCK_MODE) {
        return false; // Not in clock mode
    }

    menuClockDebug("Button pressed in clock mode - exiting...");

    // Check if WiFi mode is enabled to determine where to transition
    if (getWiFiModeEnabled()) {
        menuClockDebug("WiFi mode enabled - transitioning to WIFI_MODE");
        transitionToState(SystemState::WIFI_MODE);
    } else {
        menuClockDebug("WiFi mode disabled - transitioning to IDLE_MODE");
        transitionToState(SystemState::IDLE_MODE);
    }
    sfxPlay("shutdown", 0, true);
    menuClockDebug("Successfully exited clock mode");
    return true;
}

/**
 * @brief Select North America Eastern timezone
 * 
 * Sets the system timezone to North America Eastern (EST/EDT) and saves
 * the timezone preference to persistent storage. This timezone includes
 * automatic daylight saving time transitions.
 * 
 * @note The POSIX timezone string "EST5EDT,M3.2.0,M11.1.0" represents:
 *       - EST5EDT: Eastern Standard Time (UTC-5), Eastern Daylight Time (UTC-4)
 *       - M3.2.0: DST starts on 2nd Sunday of March at 2:00 AM
 *       - M11.1.0: DST ends on 1st Sunday of November at 2:00 AM
 */
static void selectNorthAmericaEastern() {
     setTimezoneByName("North_America_Eastern");
     saveTimezoneToPreferences("EST5EDT,M3.2.0,M11.1.0");
     menuClockDebug("Selected: North America Eastern");
 }

static void selectNorthAmericaCentral() {
    setTimezoneByName("North_America_Central");
    saveTimezoneToPreferences("CST6CDT,M3.2.0,M11.1.0");
    menuClockDebug("Selected: North America Central");
}

static void selectNorthAmericaMountain() {
    setTimezoneByName("North_America_Mountain");
    saveTimezoneToPreferences("MST7MDT,M3.2.0,M11.1.0");
    menuClockDebug("Selected: North America Mountain");
}

static void selectNorthAmericaPacific() {
    setTimezoneByName("North_America_Pacific");
    saveTimezoneToPreferences("PST8PDT,M3.2.0,M11.1.0");
    menuClockDebug("Selected: North America Pacific");
}

/**
 * @brief Select UTC timezone
 * 
 * Sets the system timezone to UTC (Coordinated Universal Time) and saves
 * the timezone preference to persistent storage. UTC is the primary
 * time standard and does not observe daylight saving time.
 */
static void selectUTC() {
     setTimezoneByName("UTC");
     saveTimezoneToPreferences("UTC0");
     menuClockDebug("Selected: UTC");
 }

static void selectUK() {
    setTimezoneByName("UK");
    saveTimezoneToPreferences("GMT0BST,M3.5.0,M10.5.0");
    menuClockDebug("Selected: UK");
}

static void selectCentralEurope() {
    setTimezoneByName("Central_Europe");
    saveTimezoneToPreferences("CET-1CEST,M3.5.0,M10.5.0");
    menuClockDebug("Selected: Central Europe");
}

static void selectJapan() {
    setTimezoneByName("Japan");
    saveTimezoneToPreferences("JST-9");
    menuClockDebug("Selected: Japan");
}

static void selectChina() {
    setTimezoneByName("China");
    saveTimezoneToPreferences("CST-8");
    menuClockDebug("Selected: China");
}

static void selectAustraliaEastern() {
    setTimezoneByName("Australia_Eastern");
    saveTimezoneToPreferences("AEST-10AEDT,M10.1.0,M4.1.0");
    menuClockDebug("Selected: Australia Eastern");
}

/**
 * @brief Timezone selection submenu
 */
static MenuItem timezoneMenu[] = {
    DEFINE_MENU_ACTION(tz_na_eastern, MENU_LABEL_TZ_EASTERN, selectNorthAmericaEastern),
    DEFINE_MENU_ACTION(tz_na_central, MENU_LABEL_TZ_CENTRAL, selectNorthAmericaCentral),
    DEFINE_MENU_ACTION(tz_na_mountain, MENU_LABEL_TZ_MOUNTAIN, selectNorthAmericaMountain),
    DEFINE_MENU_ACTION(tz_na_pacific, MENU_LABEL_TZ_PACIFIC, selectNorthAmericaPacific),
    DEFINE_MENU_ACTION(tz_utc, MENU_LABEL_TZ_UTC, selectUTC),
    DEFINE_MENU_ACTION(tz_uk, MENU_LABEL_TZ_UK, selectUK),
    DEFINE_MENU_ACTION(tz_central_europe, MENU_LABEL_TZ_EUROPE, selectCentralEurope),
    DEFINE_MENU_ACTION(tz_japan, MENU_LABEL_TZ_JAPAN, selectJapan),
    DEFINE_MENU_ACTION(tz_china, MENU_LABEL_TZ_CHINA, selectChina),
    DEFINE_MENU_ACTION(tz_australia, MENU_LABEL_TZ_AUSTRALIA, selectAustraliaEastern),
    DEFINE_MENU_BACK()
};

static const int timezoneMenuCount = MENU_ITEM_COUNT(timezoneMenu);

//==============================================================================
// TIMEZONE SUBMENU API
//==============================================================================

/**
 * @brief Get the timezone submenu items
 * @return Pointer to timezone menu array
 * 
 * The returned pointer is valid for the lifetime of the program
 * and should not be modified directly.
 */
MenuItem* menuTimezone_getItems() {
     return timezoneMenu;
 }

/**
 * @brief Get the number of timezone submenu items
 * @return Number of timezone menu items
 * 
 * The count is constant and determined at compile time based on
 * the timezone menu array definition.
 */
int menuTimezone_getItemCount() {
     return timezoneMenuCount;
 }

/**
 * @brief Get the timezone submenu context
 * @param context Pointer to context to populate
 * 
 * The context is populated with current menu state and should
 * be used immediately for menu display operations.
 */
void menuTimezone_getContext(MenuContext* context) {
     if (context) {
         context->items = timezoneMenu;
         context->itemCount = timezoneMenuCount;
         context->selectedIndex = 0;
         context->title = MENU_LABEL_TIMEZONE;
     }
 }

//==============================================================================
// MENU DEFINITION
//==============================================================================

/**
 * @brief Clock menu items
 */
static MenuItem clockMenu[] = {
    DEFINE_MENU_ACTION(clock_mode, MENU_LABEL_CLOCK_MODE, enterClockMode),
    DEFINE_MENU_SUBMENU(timezone, MENU_LABEL_TIMEZONE, timezoneMenu, timezoneMenuCount),
    DEFINE_MENU_BACK()
};

static const int clockMenuCount = MENU_ITEM_COUNT(clockMenu);

//==============================================================================
// PUBLIC API IMPLEMENTATION
//==============================================================================

/**
 * @brief Initialize clock menu module
 * 
 * Initializes the clock menu module by loading current system state
 * from the preferences module. This function should be called during
 * system initialization to set up the clock menu with the current
 * system preferences.
 */
void menuClock_init() {
     // Update status flags from preferences
     wifiEnabled = getWiFiModeEnabled();
       // Load WiFi enabled state from preferences
   menuClockDebug("WiFi mode loaded from preferences: %s", 
     wifiEnabled ? "enabled" : "disabled");
 }

/**
 * @brief Get the clock menu items
 * @return Pointer to clock menu array
 * 
 * The returned pointer is valid for the lifetime of the program
 * and should not be modified directly.
 */
MenuItem* menuClock_getItems() {
     return clockMenu;
 }

/**
 * @brief Get the number of clock items
 * @return Number of clock menu items
 * 
 * The count is constant and determined at compile time based on
 * the clock menu array definition.
 */
int menuClock_getItemCount() {
     return clockMenuCount;
 }

/**
 * @brief Get the clock menu context
 * @param context Pointer to context to populate
 * The context is populated with current menu state and should
 * be used immediately for menu display operations.
 */
void menuClock_getContext(MenuContext* context) {
     if (context) {
         context->items = clockMenu;
         context->itemCount = clockMenuCount;
         context->selectedIndex = 0;
         context->title = MENU_LABEL_CLOCK;
     }
 }
