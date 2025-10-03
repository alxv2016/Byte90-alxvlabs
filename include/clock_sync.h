/**
 * @file clock_sync.h
 * @brief Time synchronization and timezone management for clock system
 *
 * Provides internet time synchronization, timezone management, and intelligent
 * time source selection. Works with clock_module.h for complete clock functionality.
 */

#ifndef CLOCK_SYNC_H
#define CLOCK_SYNC_H

#include "clock_module.h"  // For rtc_time_t and core clock functions
#include "common.h"
#include <Arduino.h>

//==============================================================================
// CONSTANTS & DEFINITIONS
//==============================================================================

static const char *CLOCK_SYNC_LOG = "::CLOCK_SYNC::";

//==============================================================================
// TYPE DEFINITIONS
//==============================================================================

typedef struct {
    const char* name;           // Display name
    const char* tzString;       // POSIX timezone string
    const char* description;    // Human readable description
} timezone_info_t;

typedef enum {
    SYNC_SOURCE_UNKNOWN,
    SYNC_SOURCE_BUILD_TIME,
    SYNC_SOURCE_RTC_EXISTING,
    SYNC_SOURCE_INTERNET_WIFI,
    SYNC_SOURCE_INTERNET_TEMP
} time_sync_source_t;

typedef struct {
    time_sync_source_t source;
    bool lastSyncFromInternet;
    unsigned long lastSyncTime;
    char sourceDescription[32];
} time_sync_status_t;

//==============================================================================
// INITIALIZATION & SHUTDOWN
//==============================================================================

/**
 * @brief Initialize the time synchronization system
 * @return true if initialization successful, false otherwise
 */
bool initializeClockSync(void);

/**
 * @brief Periodic maintenance for time sync system
 */
void clockSyncMaintenance(void);

//==============================================================================
// TIMEZONE MANAGEMENT FUNCTIONS
//==============================================================================

/**
 * @brief Initialize timezone support
 * @return true if timezone initialized successfully
 */
bool initializeTimezone(void);

/**
 * @brief Set timezone string (stores for later use in direct GMT offset method)
 * @param tzString POSIX timezone string (e.g., "EST5EDT,M3.2.0,M11.1.0")
 * @return true if timezone string stored successfully
 */
bool setTimezone(const char* tzString);

/**
 * @brief Set timezone by name from predefined list
 * @param timezoneName Name of timezone (e.g., "US_Eastern", "UK", "Japan")
 * @return true if timezone found and set successfully
 */
bool setTimezoneByName(const char* timezoneName);

/**
 * @brief Get current timezone string
 * @return Current timezone string
 */
const char* getCurrentTimezone(void);

/**
 * @brief Load timezone from preferences using existing preferences system
 * @return true if timezone loaded successfully
 */
bool getTimezoneFromPreferences(void);

/**
 * @brief Save timezone to preferences using existing preferences system
 * @param tzString Timezone string to save
 * @return true if saved successfully
 */
bool saveTimezoneToPreferences(const char* tzString);

//==============================================================================
// TIME SYNCHRONIZATION FUNCTIONS
//==============================================================================

/**
 * @brief Attempt to sync time from internet via WiFi (smart mode-aware)
 * @return true if time sync successful, false otherwise
 */
bool syncTimeFromInternet(void);

/**
 * @brief Synchronize time and update clock display
 * Combines time synchronization with display updates for seamless operation
 */
void syncAndDisplayTime();

/**
 * @brief Set initial time with intelligent source selection
 * @param forceInternetSync Force internet sync even if RTC time seems valid
 * @return true if time was set from best available source
 */
bool syncInitialTime(bool forceInternetSync = false);

/**
 * @brief Check if periodic time sync is due
 * @return true if sync is due, false otherwise
 */
bool isPeriodicSyncDue(void);

//==============================================================================
// NETWORK TIME FUNCTIONS (INTERNAL/ADVANCED)
//==============================================================================

/**
 * @brief Configure NTP client with current timezone settings
 * @return true if NTP configuration successful
 */
bool configureNTPWithTimezone(void);

/**
 * @brief Get system time from NTP (requires active internet connection)
 * @param timeStruct Pointer to store the time
 * @return true if valid time retrieved
 */
bool getSystemTime(rtc_time_t *timeStruct);

/**
 * @brief Enable or disable debug logging for clock sync operations
 * @param enabled true to enable debug logging, false to disable
 */
void setClockSyncDebug(bool enabled);


#endif /* CLOCK_SYNC_H */