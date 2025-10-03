/**
 * @file clock_sync.cpp
 * @brief Time synchronization and timezone management implementation
 *
 * Provides internet time synchronization, timezone management, and intelligent
 * time source selection for the clock system.
 */

#include "clock_sync.h"
#include "preferences_module.h"
#include "wifi_module.h"
#include <sys/time.h>
#include <time.h>
#include <stdarg.h>

//==============================================================================
// GLOBAL VARIABLES
//==============================================================================

// Time synchronization tracking
static bool clockSyncInitialized = false;
static bool lastSyncFromInternet = false;
static unsigned long lastInternetSyncTime = 0;
static const unsigned long INTERNET_SYNC_INTERVAL =
    12 * 60 * 60 * 1000; // 12 hours (balanced between accuracy and stability)
static time_sync_status_t syncStatus;

// Timezone management
static char currentTimezoneString[64] =
    "EST5EDT,M3.2.0,M11.1.0"; // Default to North America Eastern

// Timezone definitions
static const timezone_info_t AVAILABLE_TIMEZONES[] = {
    // North America Timezones - CORRECTED FORMAT
    {"North_America_Eastern", "EST5EDT,M3.2.0,M11.1.0",
     "Eastern Time (EST/EDT) UTC-5/-4"},
    {"North_America_Central", "CST6CDT,M3.2.0,M11.1.0",
     "Central Time (CST/CDT) UTC-6/-5"},
    {"North_America_Mountain", "MST7MDT,M3.2.0,M11.1.0",
     "Mountain Time (MST/MDT) UTC-7/-6"},
    {"North_America_Pacific", "PST8PDT,M3.2.0,M11.1.0",
     "Pacific Time (PST/PDT) UTC-8/-7"},
    {"North_America_Alaska", "AKST9AKDT,M3.2.0,M11.1.0",
     "Alaska Time (AKST/AKDT) UTC-9/-8"},
    {"North_America_Hawaii", "HST10", "Hawaii Time (HST) UTC-10"},

    // Alternative simple format for better ESP32 compatibility (corrected)
    {"North_America_Eastern_Simple", "EST5", "Eastern Time (Simple) UTC-5"},
    {"North_America_Central_Simple", "CST6", "Central Time (Simple) UTC-6"},
    {"North_America_Mountain_Simple", "MST7", "Mountain Time (Simple) UTC-7"},
    {"North_America_Pacific_Simple", "PST8", "Pacific Time (Simple) UTC-8"},

    // Major World Timezones
    {"UTC", "UTC0", "Coordinated Universal Time UTC+0"},
    {"UK", "GMT0BST,M3.5.0,M10.5.0", "United Kingdom (GMT/BST) UTC+0/+1"},
    {"Central_Europe", "CET-1CEST,M3.5.0,M10.5.0",
     "Central Europe (CET/CEST) UTC+1/+2"},
    {"Eastern_Europe", "EET-2EEST,M3.5.0,M10.5.0",
     "Eastern Europe (EET/EEST) UTC+2/+3"},
    {"Japan", "JST-9", "Japan Standard Time (JST) UTC+9"},
    {"China", "CST-8", "China Standard Time (CST) UTC+8"},
    {"Australia_Eastern", "AEST-10AEDT,M10.1.0,M4.1.0",
     "Australia Eastern (AEST/AEDT) UTC+10/+11"},
    {"Australia_Central", "ACST-9:30ACDT,M10.1.0,M4.1.0",
     "Australia Central (ACST/ACDT) UTC+9:30/+10:30"},
    {"Australia_Western", "AWST-8", "Australia Western (AWST) UTC+8"},
};

static const size_t NUM_TIMEZONES =
    sizeof(AVAILABLE_TIMEZONES) / sizeof(AVAILABLE_TIMEZONES[0]);

//==============================================================================
// UTILITY FUNCTIONS (STATIC)
//==============================================================================

// Debug control - set to true to enable debug logging
static bool clockSyncDebugEnabled = false;

/**
 * @brief Centralized debug logging function for clock sync operations
 * @param format Printf-style format string
 * @param ... Variable arguments for format string
 */
static void clockSyncDebug(const char* format, ...) {
  if (!clockSyncDebugEnabled) {
    return;
  }
  
  va_list args;
  va_start(args, format);
  esp_log_writev(ESP_LOG_INFO, CLOCK_SYNC_LOG, format, args);
  va_end(args);
}

/**
 * @brief Enable or disable debug logging for clock sync operations
 * @param enabled true to enable debug logging, false to disable
 * 
 * @example
 * // Enable debug logging
 * setClockSyncDebug(true);
 * 
 * // Disable debug logging  
 * setClockSyncDebug(false);
 */
void setClockSyncDebug(bool enabled) {
  clockSyncDebugEnabled = enabled;
  if (enabled) {
    ESP_LOGI(CLOCK_SYNC_LOG, "Clock sync debug logging enabled");
  } else {
    ESP_LOGI(CLOCK_SYNC_LOG, "Clock sync debug logging disabled");
  }
}

/**
 * @brief Update sync status structure
 * @param source Source of the time sync
 * @param success Whether sync was successful
 */
static void updateSyncStatus(time_sync_source_t source, bool success) {
  syncStatus.source = source;

  if (success && (source == SYNC_SOURCE_INTERNET_WIFI ||
                  source == SYNC_SOURCE_INTERNET_TEMP)) {
    syncStatus.lastSyncFromInternet = true;
    syncStatus.lastSyncTime = millis();
    lastSyncFromInternet = true;
    lastInternetSyncTime = millis();
  }

  // Update description based on source
  switch (source) {
  case SYNC_SOURCE_BUILD_TIME:
    strcpy(syncStatus.sourceDescription, "Build Time (Fallback)");
    break;
  case SYNC_SOURCE_RTC_EXISTING:
    strcpy(syncStatus.sourceDescription, "RTC (Existing)");
    break;
  case SYNC_SOURCE_INTERNET_WIFI:
    strcpy(syncStatus.sourceDescription, "Internet (WiFi Mode)");
    break;
  case SYNC_SOURCE_INTERNET_TEMP:
    strcpy(syncStatus.sourceDescription, "Internet (Temporary WiFi)");
    break;
  default:
    strcpy(syncStatus.sourceDescription, "Unknown");
    break;
  }
}

//==============================================================================
// INITIALIZATION & SHUTDOWN
//==============================================================================

/**
 * @brief Initialize the time synchronization system
 * @return true if initialization successful, false otherwise
 */
bool initializeClockSync(void) {
  if (clockSyncInitialized) {
    return true;
  }

  // Initialize sync status
  syncStatus.source = SYNC_SOURCE_UNKNOWN;
  syncStatus.lastSyncFromInternet = false;
  syncStatus.lastSyncTime = 0;
  strcpy(syncStatus.sourceDescription, "Unknown");

  // Initialize timezone support
  initializeTimezone();

  // Set initial time with intelligent source selection
  if (!syncInitialTime(false)) {
    ESP_LOGW(CLOCK_SYNC_LOG, "Failed to set initial time");
    // Don't return false - system can still work with build time
  }

  clockSyncInitialized = true;
  return true;
}

/**
 * @brief Periodic maintenance for time sync system
 */
void clockSyncMaintenance(void) {
  if (!clockSyncInitialized) {
    return;
  }

  // Check for periodic internet time sync (non-blocking)
  if (isPeriodicSyncDue()) {
    syncTimeFromInternet();
  }

  // Additional check: if we haven't synced from internet in over 8 hours and we
  // have connectivity, force sync
  if (isWifiNetworkConnected() && lastSyncFromInternet) {
    unsigned long timeSinceLastSync = millis() - lastInternetSyncTime;
    const unsigned long MAX_DRIFT_INTERVAL = 8 * 60 * 60 * 1000; // 8 hours

    if (timeSinceLastSync > MAX_DRIFT_INTERVAL) {
      ESP_LOGW(CLOCK_SYNC_LOG,
               "RTC may have drifted - forcing sync after %lu hours",
               timeSinceLastSync / (60 * 60 * 1000));
      syncTimeFromInternet();
    }
  }
}

//==============================================================================
// TIMEZONE MANAGEMENT FUNCTIONS
//==============================================================================

/**
 * @brief Initialize timezone support
 * @return true if timezone initialized successfully
 */
bool initializeTimezone(void) {
  // Load timezone from preferences, or use North America Eastern as default
  if (!getTimezoneFromPreferences()) {
    ESP_LOGW(CLOCK_SYNC_LOG, "Using default North America Eastern timezone");
    setTimezoneByName("North_America_Eastern");
  }

  return true;
}

/**
 * @brief Set timezone string
 * @param tzString POSIX timezone string (e.g., "EST5EDT,M3.2.0,M11.1.0")
 * @return true if timezone string stored successfully
 */
bool setTimezone(const char *tzString) {
  if (!tzString) {
    ESP_LOGE(CLOCK_SYNC_LOG, "Invalid timezone string");
    return false;
  }

  clockSyncDebug("Setting timezone to: %s", tzString);

  // Store current timezone string for use in configureNTPWithTimezone()
  strncpy(currentTimezoneString, tzString, sizeof(currentTimezoneString) - 1);
  currentTimezoneString[sizeof(currentTimezoneString) - 1] = '\0';
  // Save to preferences
  saveTimezoneToPreferences(tzString);

  return true;
}

/**
 * @brief Set timezone by name from predefined list
 * @param timezoneName Name of timezone (e.g., "US_Eastern", "UK", "Japan")
 * @return true if timezone found and set successfully
 */
bool setTimezoneByName(const char *timezoneName) {
  if (!timezoneName) {
    ESP_LOGE(CLOCK_SYNC_LOG, "Invalid timezone name");
    return false;
  }

  // Search for timezone in our predefined list
  for (size_t i = 0; i < NUM_TIMEZONES; i++) {
    if (strcmp(AVAILABLE_TIMEZONES[i].name, timezoneName) == 0) {
      return setTimezone(AVAILABLE_TIMEZONES[i].tzString);
    }
  }

  ESP_LOGE(CLOCK_SYNC_LOG, "Timezone '%s' not found", timezoneName);
  return false;
}

/**
 * @brief Get current timezone string
 * @return Current timezone string
 */
const char *getCurrentTimezone(void) { return currentTimezoneString; }

/**
 * @brief Load timezone from preferences using preferences module
 * @return true if timezone loaded successfully
 */
bool getTimezoneFromPreferences(void) {
  char savedTz[64] = {0};
  
  if (loadTimezone(savedTz)) {
    clockSyncDebug("Loading saved timezone: %s", savedTz);
    return setTimezone(savedTz);
  }

  ESP_LOGW(CLOCK_SYNC_LOG,
           "No saved timezone found, using North America Eastern");
  return setTimezone("EST5EDT,M3.2.0,M11.1.0");
}

/**
 * @brief Save timezone to preferences using preferences module
 * @param tzString Timezone string to save
 * @return true if saved successfully
 */
bool saveTimezoneToPreferences(const char *tzString) {
  if (!tzString)
    return false;

  if (saveTimezone(tzString)) {
    clockSyncDebug("Timezone saved to preferences: %s", tzString);
    return true;
  }

  ESP_LOGE(CLOCK_SYNC_LOG, "Failed to save timezone to preferences");
  return false;
}

//==============================================================================
// TIME SYNCHRONIZATION FUNCTIONS
//==============================================================================

/**
 * @brief Parse timezone string and extract GMT offset and DST information
 * @param timezoneString The timezone string to parse
 * @param gmtOffsetHours Output parameter for GMT offset in hours
 * @param hasDST Output parameter indicating if timezone has DST
 * @return true if timezone was successfully parsed
 */
static bool parseTimezoneString(const char* timezoneString, int* gmtOffsetHours, bool* hasDST) {
  if (!timezoneString || !gmtOffsetHours || !hasDST) {
    return false;
  }

  *gmtOffsetHours = 0;
  *hasDST = false;

  // Parse common timezone formats
  if (strcmp(timezoneString, "EST5EDT,M3.2.0,M11.1.0") == 0) {
    *gmtOffsetHours = -5;
    *hasDST = true; // US Eastern
  } else if (strcmp(timezoneString, "CST6CDT,M3.2.0,M11.1.0") == 0) {
    *gmtOffsetHours = -6;
    *hasDST = true; // US Central
  } else if (strcmp(timezoneString, "MST7MDT,M3.2.0,M11.1.0") == 0) {
    *gmtOffsetHours = -7;
    *hasDST = true; // US Mountain
  } else if (strcmp(timezoneString, "PST8PDT,M3.2.0,M11.1.0") == 0) {
    *gmtOffsetHours = -8;
    *hasDST = true; // US Pacific
  } else if (strcmp(timezoneString, "AKST9AKDT,M3.2.0,M11.1.0") == 0) {
    *gmtOffsetHours = -9;
    *hasDST = true; // Alaska
  } else if (strcmp(timezoneString, "HST10") == 0) {
    *gmtOffsetHours = -10;
    *hasDST = false; // Hawaii
  } else if (strcmp(timezoneString, "UTC0") == 0) {
    *gmtOffsetHours = 0;
    *hasDST = false; // UTC
  } else if (strcmp(timezoneString, "GMT0BST,M3.5.0,M10.5.0") == 0) {
    *gmtOffsetHours = 0;
    *hasDST = true; // UK
  } else if (strcmp(timezoneString, "CET-1CEST,M3.5.0,M10.5.0") == 0) {
    *gmtOffsetHours = 1;
    *hasDST = true; // Central Europe
  } else if (strcmp(timezoneString, "EET-2EEST,M3.5.0,M10.5.0") == 0) {
    *gmtOffsetHours = 2;
    *hasDST = true; // Eastern Europe
  } else if (strcmp(timezoneString, "JST-9") == 0) {
    *gmtOffsetHours = 9;
    *hasDST = false; // Japan
  } else if (strcmp(timezoneString, "CST-8") == 0) {
    *gmtOffsetHours = 8;
    *hasDST = false; // China
  } else if (strcmp(timezoneString, "AEST-10AEDT,M10.1.0,M4.1.0") == 0) {
    *gmtOffsetHours = 10;
    *hasDST = true; // Australia Eastern
  } else if (strcmp(timezoneString, "ACST-9:30ACDT,M10.1.0,M4.1.0") == 0) {
    // Australia Central has +9:30 offset - use environment variable for fractional hours
    ESP_LOGW(CLOCK_SYNC_LOG, "Fractional hour timezone, using environment variable method: %s", timezoneString);
    setenv("TZ", timezoneString, 1);
    tzset();
    delay(100);
    configTime(0, 0, "time.google.com");
    return true;
  } else if (strcmp(timezoneString, "AWST-8") == 0) {
    *gmtOffsetHours = 8;
    *hasDST = false; // Australia Western
  } else if (strcmp(timezoneString, "EST5") == 0) {
    *gmtOffsetHours = -5;
    *hasDST = false; // Eastern Simple
  } else if (strcmp(timezoneString, "CST6") == 0) {
    *gmtOffsetHours = -6;
    *hasDST = false; // Central Simple
  } else if (strcmp(timezoneString, "MST7") == 0) {
    *gmtOffsetHours = -7;
    *hasDST = false; // Mountain Simple
  } else if (strcmp(timezoneString, "PST8") == 0) {
    *gmtOffsetHours = -8;
    *hasDST = false; // Pacific Simple
  } else {
    // Fallback: try environment variable method for unknown timezones
    ESP_LOGW(CLOCK_SYNC_LOG, "Unknown timezone format, trying environment variable method: %s", timezoneString);
    setenv("TZ", timezoneString, 1);
    tzset();
    delay(100);
    configTime(0, 0, "time.google.com");
    return true;
  }

  return true;
}

/**
 * @brief Calculate DST offset for the current time
 * @param timezoneString The timezone string
 * @param baseOffsetHours The base GMT offset in hours
 * @return The adjusted offset including DST if applicable
 */
static int calculateDSTOffset(const char* timezoneString, int baseOffsetHours) {
  if (!timezoneString) {
    return baseOffsetHours;
  }

  time_t tempTime = time(nullptr);
  if (tempTime <= 1000000000) {
    return baseOffsetHours; // Invalid time, return base offset
  }

  struct tm *timeInfo = gmtime(&tempTime);
  int month = timeInfo->tm_mon + 1;
  bool isDST = false;

  // US DST rules (March to November)
  if (strstr(timezoneString, "EDT") || strstr(timezoneString, "CDT") || 
      strstr(timezoneString, "MDT") || strstr(timezoneString, "PDT")) {
    isDST = (month > 3 && month < 11) ||
            (month == 3 && timeInfo->tm_mday > 14) ||
            (month == 11 && timeInfo->tm_mday < 7);
  }
  // EU DST rules (March to October)
  else if (strstr(timezoneString, "BST")) {
    isDST = (month > 3 && month < 10) ||
            (month == 3 && timeInfo->tm_mday > 25) ||
            (month == 10 && timeInfo->tm_mday < 25);
  }

  if (isDST) {
    clockSyncDebug("DST active, using UTC%+d", baseOffsetHours + 1);
    return baseOffsetHours + 1; // Add 1 hour for DST
  } else {
    clockSyncDebug("Standard time, using UTC%+d", baseOffsetHours);
    return baseOffsetHours;
  }
}

/**
 * @brief Perform NTP synchronization with timeout
 * @param gmtOffsetSeconds The GMT offset in seconds
 * @return true if synchronization successful
 */
static bool performNTPSync(int gmtOffsetSeconds) {
  clockSyncDebug("Using direct GMT offset: UTC%+d for timezone: %s",
           gmtOffsetSeconds / 3600, currentTimezoneString);
  configTime(gmtOffsetSeconds, 0, "time.google.com");

  // Wait up to 15 seconds for time sync (increased for better reliability)
  int attempts = 0;
  while (attempts < 30) {
    time_t now = time(nullptr);
    if (now > 1000000000) { // Valid timestamp (after year 2001)
      clockSyncDebug("NTP time sync successful with timezone: %s", currentTimezoneString);
      return true;
    }
    delay(500);
    attempts++;
  }

  ESP_LOGW(CLOCK_SYNC_LOG, "NTP time sync timeout");
  return false;
}

/**
 * @brief Verify timezone offset and log time information
 * @param timezoneString The timezone string to verify
 * @return true if verification passed
 */
static bool verifyTimezoneOffset(const char* timezoneString) {
  time_t now = time(nullptr);
  if (now <= 1000000000) {
    return false;
  }

  // Log both UTC and local time for verification
  struct tm *utc_tm = gmtime(&now);
  struct tm *local_tm = localtime(&now);

  clockSyncDebug("UTC time: %04d-%02d-%02d %02d:%02d:%02d",
           utc_tm->tm_year + 1900, utc_tm->tm_mon + 1, utc_tm->tm_mday,
           utc_tm->tm_hour, utc_tm->tm_min, utc_tm->tm_sec);

  clockSyncDebug("Local time: %04d-%02d-%02d %02d:%02d:%02d",
           local_tm->tm_year + 1900, local_tm->tm_mon + 1,
           local_tm->tm_mday, local_tm->tm_hour, local_tm->tm_min, local_tm->tm_sec);

  // Calculate timezone offset
  int hourDiff = local_tm->tm_hour - utc_tm->tm_hour;
  if (hourDiff < -12) hourDiff += 24;
  if (hourDiff > 12) hourDiff -= 24;
  
  clockSyncDebug("Timezone offset verification: %+d hours from UTC", hourDiff);

  // Special verification for EST timezone
  if (strcmp(timezoneString, "EST5EDT,M3.2.0,M11.1.0") == 0) {
    time_t rawTime = time(nullptr);
    struct tm *timeInfo = localtime(&rawTime);
    bool isDST = timeInfo->tm_isdst > 0;

    clockSyncDebug("EST timezone check - DST active: %s, expected offset: %s",
             isDST ? "YES" : "NO", isDST ? "UTC-4" : "UTC-5");

    int expectedOffset = isDST ? -4 : -5;
    if (hourDiff != expectedOffset) {
      ESP_LOGW(CLOCK_SYNC_LOG, "EST timezone offset verification failed: expected %+d, got %+d",
               expectedOffset, hourDiff);
      return false;
    } else {
      clockSyncDebug("EST timezone verified successfully with %+d hour offset (%s)",
               hourDiff, isDST ? "EDT" : "EST");
    }
  }

  return true;
}

/**
 * @brief Configure NTP client with current timezone settings
 * @return true if NTP configuration successful
 */
bool configureNTPWithTimezone(void) {
  clockSyncDebug("Configuring NTP with current timezone: %s", currentTimezoneString);

  // Parse timezone string to extract GMT offset and DST information
  int gmtOffsetHours = 0;
  bool hasDST = false;
  
  if (!parseTimezoneString(currentTimezoneString, &gmtOffsetHours, &hasDST)) {
    ESP_LOGE(CLOCK_SYNC_LOG, "Failed to parse timezone string: %s", currentTimezoneString);
    return false;
  }

  // Handle special cases that were handled in parseTimezoneString
  if (strcmp(currentTimezoneString, "ACST-9:30ACDT,M10.1.0,M4.1.0") == 0) {
    // Already handled in parseTimezoneString - return success
    return true;
  }

  // Set the timezone before NTP sync
  setenv("TZ", currentTimezoneString, 1);
  tzset();
  clockSyncDebug("Timezone set to: %s", currentTimezoneString);

  // Calculate DST offset if applicable
  int finalOffsetHours = hasDST ? calculateDSTOffset(currentTimezoneString, gmtOffsetHours) : gmtOffsetHours;

  // Perform NTP synchronization
  int gmtOffsetSeconds = finalOffsetHours * 3600;
  if (!performNTPSync(gmtOffsetSeconds)) {
    return false;
  }

  // Verify timezone offset and log time information
  if (!verifyTimezoneOffset(currentTimezoneString)) {
    ESP_LOGW(CLOCK_SYNC_LOG, "Timezone verification failed, but NTP sync was successful");
    // Don't return false here as the sync was successful, just verification failed
  }

  return true;
}

/**
 * @brief Get system time from NTP
 * @param timeStruct Pointer to store the time
 * @return true if valid time retrieved
 */
bool getSystemTime(rtc_time_t *timeStruct) {
  if (!timeStruct)
    return false;

  time_t now = time(nullptr);
  if (now < 1000000000)
    return false; // Invalid timestamp

  struct tm *timeinfo = localtime(&now);
  if (!timeinfo)
    return false;

  timeStruct->year = timeinfo->tm_year + 1900;
  timeStruct->month = timeinfo->tm_mon + 1;
  timeStruct->day = timeinfo->tm_mday;
  timeStruct->hour = timeinfo->tm_hour;
  timeStruct->minute = timeinfo->tm_min;
  timeStruct->second = timeinfo->tm_sec;
  timeStruct->dayOfWeek = timeinfo->tm_wday;
  timeStruct->unixtime = now;

  return true;
}

/**
 * @brief Attempt to sync time from internet via WiFi
 * @return true if time sync successful, false otherwise
 */
bool syncTimeFromInternet(void) {
  if (!isClockReady()) {
    ESP_LOGW(CLOCK_SYNC_LOG, "RTC not ready for internet sync");
    return false;
  }

  bool wifiConnected = isWifiNetworkConnected();
  ESP_LOGE(CLOCK_SYNC_LOG, "DEBUG: isWifiNetworkConnected() = %s", wifiConnected ? "TRUE" : "FALSE");
  ESP_LOGE(CLOCK_SYNC_LOG, "DEBUG: WiFi.status() = %d", WiFi.status());

  // Check if we're already in WiFi mode
  if (isWifiNetworkConnected()) {
    clockSyncDebug("Syncing time from internet (WiFi mode active)");

    // Direct NTP sync since we're already connected
    if (configureNTPWithTimezone()) {
      rtc_time_t ntpTime;
      if (getSystemTime(&ntpTime)) {
        if (setClockTimeFromStruct(&ntpTime)) {
          updateSyncStatus(SYNC_SOURCE_INTERNET_WIFI, true);
          clockSyncDebug("RTC synchronized from internet");
          logCurrentTime("Updated RTC time:");
          return true;
        }
      }
    }
    ESP_LOGW(CLOCK_SYNC_LOG, "Failed to sync time via WiFi mode");
    updateSyncStatus(SYNC_SOURCE_INTERNET_WIFI, false);
    return false;
  }

  // Not connected to WiFi - attempt connection if credentials available
  clockSyncDebug("Attempting WiFi connection for time sync");
  
  // Try to connect to WiFi using stored preferences
  if (connectToWiFiFromPreferences()) {
    // Wait a moment for connection to stabilize
    delay(2000);
    
    // Check if we're now connected
    if (isWifiNetworkConnected()) {
      clockSyncDebug("WiFi connection established, syncing time");
      
      // Configure NTP and sync time
      if (configureNTPWithTimezone()) {
        rtc_time_t ntpTime;
        if (getSystemTime(&ntpTime)) {
          if (setClockTimeFromStruct(&ntpTime)) {
            updateSyncStatus(SYNC_SOURCE_INTERNET_TEMP, true);
            clockSyncDebug("RTC synchronized from internet via WiFi connection");
            logCurrentTime("Updated RTC time:");
            
            // DO NOT disconnect from WiFi - leave connection active
            return true;
          }
        }
      }
      
      ESP_LOGW(CLOCK_SYNC_LOG, "Failed to sync time after WiFi connection");
    } else {
      ESP_LOGW(CLOCK_SYNC_LOG, "WiFi connection attempt failed");
    }
  } else {
    ESP_LOGW(CLOCK_SYNC_LOG, "No stored WiFi credentials available for connection");
  }

  ESP_LOGW(CLOCK_SYNC_LOG, "Failed to sync time - no WiFi connectivity available");
  updateSyncStatus(SYNC_SOURCE_INTERNET_TEMP, false);
  return false;
}

/**
 * @brief Set initial time with intelligent source selection
 * @param forceInternetSync Force internet sync even if RTC time seems valid
 * @return true if time was set from best available source
 */
bool syncInitialTime(bool forceInternetSync) {
  if (!isClockReady()) {
    ESP_LOGW(CLOCK_SYNC_LOG, "RTC not ready for intelligent time setting");
    return false;
  }

  bool timeWasInvalid = hasClockLostPower();
  bool internetSyncSuccess = false;

  // Priority 1: Force internet sync if requested
  if (forceInternetSync) {
    clockSyncDebug("Forcing internet time sync...");
    internetSyncSuccess = syncTimeFromInternet();
  }
  // Priority 2: Internet sync if time is invalid
  else if (timeWasInvalid) {
    clockSyncDebug("RTC time invalid - attempting internet sync...");
    internetSyncSuccess = syncTimeFromInternet();
  }
  // Priority 3: Internet sync if we're in WiFi mode (even with valid RTC time)
  else if (isWifiNetworkConnected()) {
    clockSyncDebug("WiFi mode active - updating time from internet...");
    internetSyncSuccess = syncTimeFromInternet();
  }

  // Fallback: Set to build time if RTC was invalid and internet sync failed
  if (timeWasInvalid && !internetSyncSuccess) {
    ESP_LOGW(CLOCK_SYNC_LOG,
             "Internet sync failed - setting RTC to build time");
    if (setRTCToBuildTime()) {
      updateSyncStatus(SYNC_SOURCE_BUILD_TIME, true);
      return true;
    }
  }

  // If RTC time was valid, just log current status
  if (!timeWasInvalid && !internetSyncSuccess) {
    updateSyncStatus(SYNC_SOURCE_RTC_EXISTING, true);
    logCurrentTime("Current RTC time:");
  }

  return true;
}

/**
 * @brief Check if periodic time sync is due
 * @return true if sync is due, false otherwise
 */
bool isPeriodicSyncDue(void) {
  // Only check if we're in WiFi mode and have internet connectivity
  if (!isWifiNetworkConnected()) {
    return false;
  }

  // Check if enough time has passed since last sync
  if (lastSyncFromInternet &&
      (millis() - lastInternetSyncTime) < INTERNET_SYNC_INTERVAL) {
    return false;
  }

  return true;
}

/**
 * @brief Synchronize time and update clock display
 * Combines time synchronization with display updates for seamless operation
 */
void syncAndDisplayTime() {
  // 1. Load saved timezone preference first
  if (!getTimezoneFromPreferences()) {
      ESP_LOGW("MENU_CLOCK", "No saved timezone found, using default");
  } else {
      clockSyncDebug("Loaded saved timezone preference");
  }
  
  // 2. Initialize and display the clock IMMEDIATELY
  initializeClockDisplay();
  resetClockDisplayState();
  
  // 3. Check if WiFi is enabled
  if (getWiFiModeEnabled()) {
      clockSyncDebug("WiFi mode enabled - attempting time sync");
      
      if (isWifiNetworkConnected()) {
          clockSyncDebug("WiFi connected - syncing time from internet");
          if (syncTimeFromInternet()) {
              clockSyncDebug("Time synchronized from internet with timezone: %s", getCurrentTimezone());
          } else {
              ESP_LOGW("MENU_CLOCK", "Failed to sync time from internet - showing local time");
          }
      } else {
          ESP_LOGW("MENU_CLOCK", "WiFi enabled but not connected - showing local time");
      }
  } else {
      clockSyncDebug("WiFi mode disabled - enabling WiFi temporarily for time sync");
  }
}
