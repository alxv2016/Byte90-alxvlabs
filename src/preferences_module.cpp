/**
 * @file preferences_module.cpp
 * @brief Implementation of preferences management module
 *
 * Provides functionality for persistent storage and retrieval of system preferences,
 * user settings, WiFi credentials, and configuration data using ESP32 NVS (Non-Volatile Storage)
 * for the BYTE-90 device.
 * 
 * This module handles:
 * - System preferences (WiFi mode, startup mode, last known good state)
 * - WiFi credentials storage and retrieval
 * - User preferences (themes, audio, haptic feedback)
 * - Visual effects settings (glitch, scanlines, dithering, etc.)
 * - Tint effect configuration (color and intensity)
 * - Timezone information storage
 * - Debug logging and storage information
 * - NVS storage management and error handling
 */

#include "preferences_module.h"
#include "common.h"
#include <Preferences.h>
#include <esp_log.h>
#include <stdarg.h>

//==============================================================================
// DEBUG SYSTEM
//==============================================================================

// Debug control - set to true to enable debug logging
static bool preferencesDebugEnabled = false;

/**
 * @brief Centralized debug logging function for preferences operations
 * @param format Printf-style format string
 * @param ... Variable arguments for format string
 */
static void preferencesDebug(const char* format, ...) {
  if (!preferencesDebugEnabled) {
    return;
  }
  
  va_list args;
  va_start(args, format);
  esp_log_writev(ESP_LOG_INFO, "PREF_MGR", format, args);
  va_end(args);
}

/**
 * @brief Enable or disable debug logging for preferences operations
 * @param enabled true to enable debug logging, false to disable
 * 
 * @example
 * // Enable debug logging
 * setPreferencesDebug(true);
 * 
 * // Disable debug logging  
 * setPreferencesDebug(false);
 */
void setPreferencesDebug(bool enabled) {
  preferencesDebugEnabled = enabled;
  if (enabled) {
    ESP_LOGI("PREF_MGR", "Preferences debug logging enabled");
  } else {
    ESP_LOGI("PREF_MGR", "Preferences debug logging disabled");
  }
}

//==============================================================================
// CONSTANTS & DEFINITIONS
//==============================================================================

static const char* TAG = "PREF_MGR";

// Preferences object
static Preferences preferences;

// Storage namespaces - organized by category
static const char* WIFI_NAMESPACE = "wifiPrefs";        // WiFi credentials and network settings
static const char* SYSTEM_NAMESPACE = "systemPrefs";    // System state and operational settings  
static const char* USER_NAMESPACE = "userPrefs";        // User customizable settings (TBD)
static const char* TIMEZONE_KEY = "timezone";

// WiFi preference keys
static const char* WIFI_SSID_KEY = "wifi_ssid";
static const char* WIFI_PASS_KEY = "wifi_password";
// Future WiFi keys: "static_ip", "dns_server", "auto_connect", etc.

// System preference keys
static const char* STARTUP_MODE_KEY = "startup_mode";
static const char* LAST_GOOD_STATE_KEY = "last_good_state";
static const char* USER_WIFI_MODE_ENABLED_KEY = "wifi_mode_en";

// User preference keys (example)
static const char* USER_THEME_KEY = "theme";
// Future user keys can be added here: "display_timeout", "language", etc.
static const char* USER_AUDIO_ENABLED_KEY = "audio_en";
static const char* USER_HAPTIC_ENABLED_KEY = "haptic_en";
static const char* USER_GLITCH_ENABLED_KEY = "glitch_en";
static const char* USER_SCANLINES_ENABLED_KEY = "scanlines_en";
static const char* USER_DITHERING_ENABLED_KEY = "dithering_en";
static const char* USER_CHROMATIC_ENABLED_KEY = "chromatic_en";
static const char* USER_DOT_MATRIX_ENABLED_KEY = "dot_matrix_en";
static const char* USER_PIXELATE_ENABLED_KEY = "pixelate_en";
static const char* USER_TINT_ENABLED_KEY = "tint_en";
static const char* USER_TINT_COLOR_KEY = "tint_color";
static const char* USER_TINT_INTENSITY_KEY = "tint_intensity";

// Default values - no hardcoded real credentials
static const uint8_t DEFAULT_STARTUP_MODE = 0; // IDLE_MODE
static const bool DEFAULT_WIFI_MODE_ENABLED = false;
static const char* DEFAULT_SSID = "";          // Empty - forces configuration
static const char* DEFAULT_PASSWORD = "";      // Empty - forces configuration
static const char* DEFAULT_TIMEZONE = "EST5EDT,M3.2.0,M11.1.0"; // Eastern Time with DST
static const char* DEFAULT_THEME = "dark";

static const bool DEFAULT_AUDIO_ENABLED = true;
static const bool DEFAULT_HAPTIC_ENABLED = true;
static const bool DEFAULT_GLITCH_ENABLED = false;
static const bool DEFAULT_SCANLINES_ENABLED = false;
static const bool DEFAULT_DITHERING_ENABLED = false;
static const bool DEFAULT_CHROMATIC_ENABLED = false;
static const bool DEFAULT_DOT_MATRIX_ENABLED = false;
static const bool DEFAULT_PIXELATE_ENABLED = false;
static const bool DEFAULT_TINT_ENABLED = false;
static const uint16_t DEFAULT_TINT_COLOR = 0x07E0; // Green
static const float DEFAULT_TINT_INTENSITY = 0.8f;

//==============================================================================
// CACHING VARIABLES
//==============================================================================
static bool cachedWiFiModeEnabled = DEFAULT_WIFI_MODE_ENABLED;
static bool wifiModePreferenceCached = false;

// Audio preference caching
static bool cachedAudioEnabled = DEFAULT_AUDIO_ENABLED;
static bool audioPreferenceCached = false;

// Haptic preference caching  
static bool cachedHapticEnabled = DEFAULT_HAPTIC_ENABLED;
static bool hapticPreferenceCached = false;

//==============================================================================
// PUBLIC API FUNCTIONS
//==============================================================================

/**
 * @brief Initializes the preferences manager and NVS storage
 * 
 * Initializes the ESP32 NVS (Non-Volatile Storage) system for persistent
 * preferences storage. Tests NVS availability and sets up the preferences
 * manager for use by other modules.
 */
void initPreferencesManager() {
    preferencesDebug("Initializing Preferences Manager...");
    
    // Test NVS availability with system namespace
    if (!preferences.begin(SYSTEM_NAMESPACE, true)) {
        ESP_LOGE(TAG, "Failed to initialize NVS - preferences will not work");
        return;
    }
    preferences.end();
    
    preferencesDebug("NVS storage available - preferences ready");
    preferencesDebug("Preference categories: WiFi, System, User(TBD)");
    printStorageInfo();
}

// =============================================================================
// WiFi Preferences - Network credentials and WiFi-related settings
// =============================================================================

/**
 * @brief Saves WiFi credentials to persistent storage
 * @param ssid WiFi network SSID
 * @param password WiFi network password
 * @return true if credentials saved successfully, false otherwise
 * 
 * Saves WiFi network credentials to NVS storage with validation of input
 * parameters and length limits. Handles both SSID and password storage.
 */
bool saveWiFiCredentials(const char* ssid, const char* password) {
    if (!ssid || strlen(ssid) == 0) {
        ESP_LOGW(TAG, "Invalid SSID provided - not saving credentials");
        return false;
    }
    
    if (strlen(ssid) >= WIFI_SSID_MAX_LEN) {
        ESP_LOGW(TAG, "SSID too long (%d chars, max %d) - not saving credentials", 
                 strlen(ssid), WIFI_SSID_MAX_LEN - 1);
        return false;
    }
    
    if (password && strlen(password) >= WIFI_PASSWORD_MAX_LEN) {
        ESP_LOGW(TAG, "Password too long (%d chars, max %d) - not saving credentials", 
                 strlen(password), WIFI_PASSWORD_MAX_LEN - 1);
        return false;
    }
    
    if (!preferences.begin(WIFI_NAMESPACE, false)) {
        ESP_LOGE(TAG, "Failed to open WiFi preferences for writing");
        return false;
    }
    
    bool success = true;
    
    // Save SSID
    if (preferences.putString(WIFI_SSID_KEY, ssid) == 0) {
        ESP_LOGE(TAG, "Failed to save SSID to WiFi preferences");
        success = false;
    }
    
    // Save password (can be empty)
    if (preferences.putString(WIFI_PASS_KEY, password ? password : "") == 0) {
        ESP_LOGE(TAG, "Failed to save password to WiFi preferences");
        success = false;
    }
    
    preferences.end();
    
    if (success) {
        preferencesDebug("WiFi credentials saved to wifiPrefs namespace - SSID: %s", ssid);
    }
    
    return success;
}

/**
 * @brief Loads WiFi credentials from persistent storage
 * @param ssid Buffer to store SSID (must be at least WIFI_SSID_MAX_LEN bytes)
 * @param password Buffer to store password (must be at least WIFI_PASSWORD_MAX_LEN bytes)
 * @return true if credentials loaded successfully, false otherwise
 * 
 * Loads WiFi network credentials from NVS storage into provided buffers.
 * Validates buffer pointers and handles missing credentials gracefully.
 */
bool loadWiFiCredentials(char* ssid, char* password) {
    if (!ssid || !password) {
        ESP_LOGE(TAG, "Invalid buffers provided for credential loading");
        return false;
    }
    
    if (!preferences.begin(WIFI_NAMESPACE, true)) {
        ESP_LOGW(TAG, "Failed to open WiFi preferences - no credentials available");
        ssid[0] = '\0';
        password[0] = '\0';
        return false;
    }
    
    // Load SSID
    String storedSSID = preferences.getString(WIFI_SSID_KEY, DEFAULT_SSID);
    if (storedSSID.length() == 0) {
        ESP_LOGW(TAG, "No SSID found in WiFi preferences");
        ssid[0] = '\0';
        password[0] = '\0';
        preferences.end();
        return false;
    }
    
    // Load password
    String storedPassword = preferences.getString(WIFI_PASS_KEY, DEFAULT_PASSWORD);
    
    preferences.end();
    
    // Copy to output buffers safely
    strncpy(ssid, storedSSID.c_str(), WIFI_SSID_MAX_LEN - 1);
    ssid[WIFI_SSID_MAX_LEN - 1] = '\0';
    strncpy(password, storedPassword.c_str(), WIFI_PASSWORD_MAX_LEN - 1);
    password[WIFI_PASSWORD_MAX_LEN - 1] = '\0';
    
    preferencesDebug("WiFi credentials loaded from wifiPrefs - SSID: %s", ssid);
    return true;
}

/**
 * @brief Clears stored WiFi credentials from persistent storage
 * 
 * Removes all WiFi credentials from NVS storage, effectively forgetting
 * the stored network information. Used for network changes or security.
 */
void clearWiFiCredentials() {
    if (!preferences.begin(WIFI_NAMESPACE, false)) {
        ESP_LOGE(TAG, "Failed to open WiFi preferences for clearing");
        return;
    }
    
    preferences.remove(WIFI_SSID_KEY);
    preferences.remove(WIFI_PASS_KEY);
    preferences.end();
    
    preferencesDebug("WiFi credentials cleared from wifiPrefs namespace");
}

// =============================================================================
// System Preferences - System state and operational settings
// =============================================================================

void saveStartupMode(uint8_t mode) {
    if (!preferences.begin(SYSTEM_NAMESPACE, false)) {
        ESP_LOGE(TAG, "Failed to open system preferences for writing startup mode");
        return;
    }
    
    if (preferences.putUChar(STARTUP_MODE_KEY, mode) == 0) {
        ESP_LOGE(TAG, "Failed to save startup mode to system preferences");
    } else {
        ESP_LOGD(TAG, "Startup mode saved to systemPrefs: %d", mode);
    }
    
    preferences.end();
}

uint8_t loadStartupMode() {
    if (!preferences.begin(SYSTEM_NAMESPACE, true)) {
        ESP_LOGW(TAG, "Failed to open system preferences - using default startup mode");
        return DEFAULT_STARTUP_MODE;
    }
    
    uint8_t mode = preferences.getUChar(STARTUP_MODE_KEY, DEFAULT_STARTUP_MODE);
    preferences.end();
    
    // Validate mode against SystemState enum values:
    // 0=IDLE_MODE, 1=WIFI_MODE, 4=ESP_MODE are valid startup states
    // 2=UPDATE_MODE, 3=CLOCK_MODE are temporary and not valid for startup
    if (mode > 4 || mode == 2 || mode == 3) {
        ESP_LOGW(TAG, "Invalid startup mode %d loaded from systemPrefs - using default", mode);
        mode = DEFAULT_STARTUP_MODE;
        saveStartupMode(mode); // Save corrected value
    }
    
    ESP_LOGD(TAG, "Startup mode loaded from systemPrefs: %d", mode);
    return mode;
}

void saveLastKnownGoodState(uint8_t state) {
    if (!preferences.begin(SYSTEM_NAMESPACE, false)) {
        ESP_LOGE(TAG, "Failed to open system preferences for writing last good state");
        return;
    }
    
    if (preferences.putUChar(LAST_GOOD_STATE_KEY, state) == 0) {
        ESP_LOGE(TAG, "Failed to save last known good state to system preferences");
    } else {
        ESP_LOGD(TAG, "Last known good state saved to systemPrefs: %d", state);
    }
    
    preferences.end();
}

uint8_t loadLastKnownGoodState() {
    if (!preferences.begin(SYSTEM_NAMESPACE, true)) {
        ESP_LOGW(TAG, "Failed to open system preferences - using default last good state");
        return DEFAULT_STARTUP_MODE;
    }
    
    uint8_t state = preferences.getUChar(LAST_GOOD_STATE_KEY, DEFAULT_STARTUP_MODE);
    preferences.end();
    
    ESP_LOGD(TAG, "Last known good state loaded from systemPrefs: %d", state);
    return state;
}

/**
 * @brief Gets the current WiFi mode enabled state
 * @return true if WiFi mode is enabled, false otherwise
 * 
 * Retrieves the WiFi mode enabled state from NVS storage with caching
 * for performance. Returns default value if not found or on error.
 */
bool getWiFiModeEnabled() {
    // Return cached value if available
    if (wifiModePreferenceCached) {
        return cachedWiFiModeEnabled;
    }
    
    // Load from preferences and cache
    if (!preferences.begin(USER_NAMESPACE, true)) {
        cachedWiFiModeEnabled = DEFAULT_WIFI_MODE_ENABLED;
        wifiModePreferenceCached = true;
        return cachedWiFiModeEnabled;
    }
    
    cachedWiFiModeEnabled = preferences.getBool(USER_WIFI_MODE_ENABLED_KEY, DEFAULT_WIFI_MODE_ENABLED);
    preferences.end();
    wifiModePreferenceCached = true;
    
    return cachedWiFiModeEnabled;
}

/**
 * @brief Sets the WiFi mode enabled state
 * @param enabled true to enable WiFi mode, false to disable
 * @return true if state saved successfully, false otherwise
 * 
 * Saves the WiFi mode enabled state to NVS storage and updates the cache.
 * Used to control whether WiFi functionality is enabled on the device.
 */
bool setWiFiModeEnabled(bool enabled) {
    if (!preferences.begin(USER_NAMESPACE, false)) {
        ESP_LOGE(TAG, "Failed to open user preferences for writing WiFi mode");
        return false;
    }
    
    if (preferences.putBool(USER_WIFI_MODE_ENABLED_KEY, enabled) == 0) {
        ESP_LOGE(TAG, "Failed to save WiFi mode to user preferences");
        preferences.end();
        return false;
    }
    
    preferences.end();
    
    // Update cache
    cachedWiFiModeEnabled = enabled;
    wifiModePreferenceCached = true;
    
    preferencesDebug("WiFi mode saved: %s", enabled ? "enabled" : "disabled");
    return true;
}

bool saveTimezone(const char* timezone) {
    if (!timezone || strlen(timezone) == 0) {
        ESP_LOGW(TAG, "Invalid timezone provided - not saving");
        return false;
    }
    
    if (strlen(timezone) >= 64) {  // Reasonable timezone string length limit
        ESP_LOGW(TAG, "Timezone string too long (%d chars, max 63) - not saving", strlen(timezone));
        return false;
    }
    
    if (!preferences.begin(SYSTEM_NAMESPACE, false)) {
        ESP_LOGE(TAG, "Failed to open system preferences for writing timezone");
        return false;
    }
    
    if (preferences.putString(TIMEZONE_KEY, timezone) == 0) {
        ESP_LOGE(TAG, "Failed to save timezone to system preferences");
        preferences.end();
        return false;
    }
    
    preferences.end();
    preferencesDebug("Timezone saved to systemPrefs: %s", timezone);
    return true;
}

bool loadTimezone(char* timezone) {
    if (!timezone) {
        ESP_LOGE(TAG, "Invalid buffer provided for timezone loading");
        return false;
    }
    
    if (!preferences.begin(SYSTEM_NAMESPACE, true)) {
        ESP_LOGW(TAG, "Failed to open system preferences - using default timezone");
        strncpy(timezone, DEFAULT_TIMEZONE, 63);
        timezone[63] = '\0';
        return false;
    }
    
    String storedTimezone = preferences.getString(TIMEZONE_KEY, DEFAULT_TIMEZONE);
    preferences.end();
    
    strncpy(timezone, storedTimezone.c_str(), 63);
    timezone[63] = '\0';
    
    ESP_LOGD(TAG, "Timezone loaded from systemPrefs: %s", timezone);
    return true;
}

void clearSystemPreferences() {
    if (!preferences.begin(SYSTEM_NAMESPACE, false)) {
        ESP_LOGE(TAG, "Failed to open system preferences for clearing");
        return;
    }
    
    preferences.remove(STARTUP_MODE_KEY);
    preferences.remove(LAST_GOOD_STATE_KEY);
    preferences.remove(TIMEZONE_KEY);
    preferences.end();
    
    preferencesDebug("System preferences cleared from systemPrefs namespace");
}

// =============================================================================
// User Preferences - User customizable settings (Example Implementation)
// =============================================================================

void saveUserTheme(const char* theme) {
    if (!theme || strlen(theme) == 0) {
        ESP_LOGW(TAG, "Invalid theme provided - not saving");
        return;
    }
    
    if (strlen(theme) >= 16) {  // Reasonable theme name length limit
        ESP_LOGW(TAG, "Theme name too long (%d chars, max 15) - not saving", strlen(theme));
        return;
    }
    
    if (!preferences.begin(USER_NAMESPACE, false)) {
        ESP_LOGE(TAG, "Failed to open user preferences for writing theme");
        return;
    }
    
    if (preferences.putString(USER_THEME_KEY, theme) == 0) {
        ESP_LOGE(TAG, "Failed to save theme to user preferences");
    } else {
        preferencesDebug("User theme saved to userPrefs: %s", theme);
    }
    
    preferences.end();
}

bool loadUserTheme(char* theme) {
    if (!theme) {
        ESP_LOGE(TAG, "Invalid buffer provided for theme loading");
        return false;
    }
    
    if (!preferences.begin(USER_NAMESPACE, true)) {
        ESP_LOGW(TAG, "Failed to open user preferences - using default theme");
        strncpy(theme, DEFAULT_THEME, 15);
        theme[15] = '\0';
        return false;
    }
    
    String storedTheme = preferences.getString(USER_THEME_KEY, DEFAULT_THEME);
    preferences.end();
    
    strncpy(theme, storedTheme.c_str(), 15);
    theme[15] = '\0';
    
    ESP_LOGD(TAG, "User theme loaded from userPrefs: %s", theme);
    return true;
}

void clearUserPreferences() {
    if (!preferences.begin(USER_NAMESPACE, false)) {
        ESP_LOGE(TAG, "Failed to open user preferences for clearing");
        return;
    }
    
    preferences.remove(USER_THEME_KEY);
    preferences.remove(USER_AUDIO_ENABLED_KEY);
    preferences.remove(USER_HAPTIC_ENABLED_KEY);
    preferences.remove(USER_GLITCH_ENABLED_KEY);
    preferences.remove(USER_SCANLINES_ENABLED_KEY);
    preferences.remove(USER_DITHERING_ENABLED_KEY);
    preferences.remove(USER_CHROMATIC_ENABLED_KEY);
    preferences.remove(USER_DOT_MATRIX_ENABLED_KEY);
    preferences.remove(USER_PIXELATE_ENABLED_KEY);
    preferences.remove(USER_TINT_ENABLED_KEY);
    preferences.remove(USER_TINT_COLOR_KEY);
    preferences.remove(USER_TINT_INTENSITY_KEY);
    preferences.end();
    
    // Clear cached values so they get reloaded from defaults
    audioPreferenceCached = false;
    hapticPreferenceCached = false;
    
    preferencesDebug("User preferences cleared from userPrefs namespace");
}

// =============================================================================
// Audio Preferences
// =============================================================================

/**
 * @brief Gets the current audio enabled state
 * @return true if audio is enabled, false otherwise
 * 
 * Retrieves the audio enabled state from NVS storage with caching for performance.
 * Checks hardware support and returns false if audio hardware is not available.
 */
bool getAudioEnabled() {
    // SERIES_2 flag overrides user preferences
    if (!checkHardwareSupport()) {
        return false;
    }
    
    // Return cached value if available
    if (audioPreferenceCached) {
        return cachedAudioEnabled;
    }
    
    // Load from preferences and cache the result
    if (!preferences.begin(USER_NAMESPACE, true)) {
        ESP_LOGW(TAG, "Failed to open user preferences - using default audio setting");
        cachedAudioEnabled = DEFAULT_AUDIO_ENABLED;
        audioPreferenceCached = true;
        return cachedAudioEnabled;
    }
    
    cachedAudioEnabled = preferences.getBool(USER_AUDIO_ENABLED_KEY, DEFAULT_AUDIO_ENABLED);
    preferences.end();
    audioPreferenceCached = true;
    
    preferencesDebug("Audio enabled loaded from userPrefs: %s", cachedAudioEnabled ? "true" : "false");
    return cachedAudioEnabled;
}

/**
 * @brief Sets the audio enabled state
 * @param enabled true to enable audio, false to disable
 * @return true if state saved successfully, false otherwise
 * 
 * Saves the audio enabled state to NVS storage and updates the cache.
 * Checks hardware support and prevents enabling if audio hardware is not available.
 */
bool setAudioEnabled(bool enabled) {
    if (!preferences.begin(USER_NAMESPACE, false)) {
        ESP_LOGE(TAG, "Failed to open user preferences for writing audio enabled");
        return false;
    }
    
    if (preferences.putBool(USER_AUDIO_ENABLED_KEY, enabled) == 0) {
        ESP_LOGE(TAG, "Failed to save audio enabled to user preferences");
        preferences.end();
        return false;
    }
    
    preferences.end();
    
    // Update cache with new value
    cachedAudioEnabled = enabled;
    audioPreferenceCached = true;
    
    preferencesDebug("Audio enabled saved to userPrefs: %s", enabled ? "true" : "false");
    return true;
}

// =============================================================================
// Haptic Preferences
// =============================================================================

/**
 * @brief Gets the current haptic feedback enabled state
 * @return true if haptic feedback is enabled, false otherwise
 * 
 * Retrieves the haptic feedback enabled state from NVS storage with caching for performance.
 * Checks hardware support and returns false if haptic hardware is not available.
 */
bool getHapticEnabled() {
    // SERIES_2 flag overrides user preferences
    if (!checkHardwareSupport()) {
        return false;
    }
    
    // Return cached value if available
    if (hapticPreferenceCached) {
        return cachedHapticEnabled;
    }
    
    // Load from preferences and cache the result
    if (!preferences.begin(USER_NAMESPACE, true)) {
        ESP_LOGW(TAG, "Failed to open user preferences - using default haptic setting");
        cachedHapticEnabled = DEFAULT_HAPTIC_ENABLED;
        hapticPreferenceCached = true;
        return cachedHapticEnabled;
    }
    
    cachedHapticEnabled = preferences.getBool(USER_HAPTIC_ENABLED_KEY, DEFAULT_HAPTIC_ENABLED);
    preferences.end();
    hapticPreferenceCached = true;
    
    preferencesDebug("Haptic enabled loaded from userPrefs: %s", cachedHapticEnabled ? "true" : "false");
    return cachedHapticEnabled;
}

/**
 * @brief Sets the haptic feedback enabled state
 * @param enabled true to enable haptic feedback, false to disable
 * @return true if state saved successfully, false otherwise
 * 
 * Saves the haptic feedback enabled state to NVS storage and updates the cache.
 * Checks hardware support and prevents enabling if haptic hardware is not available.
 */
bool setHapticEnabled(bool enabled) {
    if (!preferences.begin(USER_NAMESPACE, false)) {
        ESP_LOGE(TAG, "Failed to open user preferences for writing haptic enabled");
        return false;
    }
    
    if (preferences.putBool(USER_HAPTIC_ENABLED_KEY, enabled) == 0) {
        ESP_LOGE(TAG, "Failed to save haptic enabled to user preferences");
        preferences.end();
        return false;
    }
    
    preferences.end();
    
    // Update cache with new value
    cachedHapticEnabled = enabled;
    hapticPreferenceCached = true;
    
    preferencesDebug("Haptic enabled saved to userPrefs: %s", enabled ? "true" : "false");
    return true;
}

// =============================================================================
// Visual Effect Preferences
// =============================================================================

bool getGlitchEnabled() {
    if (!preferences.begin(USER_NAMESPACE, true)) {
        ESP_LOGW(TAG, "Failed to open user preferences - using default glitch setting");
        return DEFAULT_GLITCH_ENABLED;
    }
    
    bool enabled = preferences.getBool(USER_GLITCH_ENABLED_KEY, DEFAULT_GLITCH_ENABLED);
    preferences.end();
    return enabled;
}

bool setGlitchEnabled(bool enabled) {
    if (!preferences.begin(USER_NAMESPACE, false)) {
        ESP_LOGE(TAG, "Failed to open user preferences for writing glitch enabled");
        return false;
    }
    
    bool success = preferences.putBool(USER_GLITCH_ENABLED_KEY, enabled) > 0;
    preferences.end();
    
    if (success) {
        preferencesDebug("Glitch effect saved to userPrefs: %s", enabled ? "enabled" : "disabled");
    }
    return success;
}

bool getScanlinesEnabled() {
    if (!preferences.begin(USER_NAMESPACE, true)) {
        return DEFAULT_SCANLINES_ENABLED;
    }
    
    bool enabled = preferences.getBool(USER_SCANLINES_ENABLED_KEY, DEFAULT_SCANLINES_ENABLED);
    preferences.end();
    return enabled;
}

bool setScanlinesEnabled(bool enabled) {
    if (!preferences.begin(USER_NAMESPACE, false)) {
        return false;
    }
    
    bool success = preferences.putBool(USER_SCANLINES_ENABLED_KEY, enabled) > 0;
    preferences.end();
    return success;
}

bool getDitheringEnabled() {
    if (!preferences.begin(USER_NAMESPACE, true)) {
        return DEFAULT_DITHERING_ENABLED;
    }
    
    bool enabled = preferences.getBool(USER_DITHERING_ENABLED_KEY, DEFAULT_DITHERING_ENABLED);
    preferences.end();
    return enabled;
}

bool setDitheringEnabled(bool enabled) {
    if (!preferences.begin(USER_NAMESPACE, false)) {
        return false;
    }
    
    bool success = preferences.putBool(USER_DITHERING_ENABLED_KEY, enabled) > 0;
    preferences.end();
    return success;
}

bool getChromaticEnabled() {
    if (!preferences.begin(USER_NAMESPACE, true)) {
        return DEFAULT_CHROMATIC_ENABLED;
    }
    
    bool enabled = preferences.getBool(USER_CHROMATIC_ENABLED_KEY, DEFAULT_CHROMATIC_ENABLED);
    preferences.end();
    return enabled;
}

bool setChromaticEnabled(bool enabled) {
    if (!preferences.begin(USER_NAMESPACE, false)) {
        return false;
    }
    
    bool success = preferences.putBool(USER_CHROMATIC_ENABLED_KEY, enabled) > 0;
    preferences.end();
    return success;
}

bool getDotMatrixEnabled() {
    if (!preferences.begin(USER_NAMESPACE, true)) {
        return DEFAULT_DOT_MATRIX_ENABLED;
    }
    
    bool enabled = preferences.getBool(USER_DOT_MATRIX_ENABLED_KEY, DEFAULT_DOT_MATRIX_ENABLED);
    preferences.end();
    return enabled;
}

bool setDotMatrixEnabled(bool enabled) {
    if (!preferences.begin(USER_NAMESPACE, false)) {
        return false;
    }
    
    bool success = preferences.putBool(USER_DOT_MATRIX_ENABLED_KEY, enabled) > 0;
    preferences.end();
    return success;
}

bool getPixelateEnabled() {
    if (!preferences.begin(USER_NAMESPACE, true)) {
        return DEFAULT_PIXELATE_ENABLED;
    }
    
    bool enabled = preferences.getBool(USER_PIXELATE_ENABLED_KEY, DEFAULT_PIXELATE_ENABLED);
    preferences.end();
    return enabled;
}

bool setPixelateEnabled(bool enabled) {
    if (!preferences.begin(USER_NAMESPACE, false)) {
        return false;
    }
    
    bool success = preferences.putBool(USER_PIXELATE_ENABLED_KEY, enabled) > 0;
    preferences.end();
    return success;
}

bool getTintEnabled() {
    if (!preferences.begin(USER_NAMESPACE, true)) {
        return DEFAULT_TINT_ENABLED;
    }
    
    bool enabled = preferences.getBool(USER_TINT_ENABLED_KEY, DEFAULT_TINT_ENABLED);
    preferences.end();
    return enabled;
}

bool setTintEnabled(bool enabled) {
    if (!preferences.begin(USER_NAMESPACE, false)) {
        return false;
    }
    
    bool success = preferences.putBool(USER_TINT_ENABLED_KEY, enabled) > 0;
    preferences.end();
    return success;
}

/**
 * @brief Gets the current tint color value
 * @return Tint color as 16-bit RGB565 value
 * 
 * Retrieves the tint color from NVS storage. Returns default color if not found.
 */
uint16_t getTintColor() {
    if (!preferences.begin(USER_NAMESPACE, true)) {
        return DEFAULT_TINT_COLOR;
    }
    
    uint16_t color = preferences.getUShort(USER_TINT_COLOR_KEY, DEFAULT_TINT_COLOR);
    preferences.end();
    return color;
}

/**
 * @brief Sets the tint color value
 * @param color Tint color as 16-bit RGB565 value
 * @return true if color saved successfully, false otherwise
 * 
 * Saves the tint color to NVS storage for visual effect customization.
 */
bool setTintColor(uint16_t color) {
    if (!preferences.begin(USER_NAMESPACE, false)) {
        return false;
    }
    
    bool success = preferences.putUShort(USER_TINT_COLOR_KEY, color) > 0;
    preferences.end();
    
    if (success) {
        preferencesDebug("Tint color saved to userPrefs: 0x%04X", color);
    }
    return success;
}

/**
 * @brief Gets the current tint intensity value
 * @return Tint intensity as float (0.0 to 1.0)
 * 
 * Retrieves the tint intensity from NVS storage. Returns default intensity if not found.
 */
float getTintIntensity() {
    if (!preferences.begin(USER_NAMESPACE, true)) {
        return DEFAULT_TINT_INTENSITY;
    }
    
    float intensity = preferences.getFloat(USER_TINT_INTENSITY_KEY, DEFAULT_TINT_INTENSITY);
    preferences.end();
    return intensity;
}

/**
 * @brief Sets the tint intensity value
 * @param intensity Tint intensity as float (0.0 to 1.0)
 * @return true if intensity saved successfully, false otherwise
 * 
 * Saves the tint intensity to NVS storage for visual effect customization.
 */
bool setTintIntensity(float intensity) {
    if (!preferences.begin(USER_NAMESPACE, false)) {
        return false;
    }
    
    // Clamp intensity to valid range
    if (intensity < 0.0f) intensity = 0.0f;
    if (intensity > 1.0f) intensity = 1.0f;
    
    bool success = preferences.putFloat(USER_TINT_INTENSITY_KEY, intensity) > 0;
    preferences.end();
    
    if (success) {
        preferencesDebug("Tint intensity saved to userPrefs: %.2f", intensity);
    }
    return success;
}

// =============================================================================
// Utility Functions
// =============================================================================

bool isPreferencesAvailable() {
    bool available = preferences.begin(SYSTEM_NAMESPACE, true);
    if (available) {
        preferences.end();
    }
    return available;
}

void printStorageInfo() {
    preferencesDebug("=== NVS Storage Info by Category ===");
    
    // Check WiFi preferences namespace
    if (preferences.begin(WIFI_NAMESPACE, true)) {
        String ssid = preferences.getString(WIFI_SSID_KEY, "");
        String password = preferences.getString(WIFI_PASS_KEY, "");
        size_t wifiUsed = ssid.length() + password.length();
        preferencesDebug("WiFi Preferences (wifiPrefs): %d bytes used", wifiUsed);
        preferences.end();
    } else {
        preferencesDebug("WiFi Preferences (wifiPrefs): Not initialized");
    }
    
    // Check system preferences namespace
    if (preferences.begin(SYSTEM_NAMESPACE, true)) {
        size_t systemUsed = preferences.getBytesLength(STARTUP_MODE_KEY) + 
            preferences.getBytesLength(LAST_GOOD_STATE_KEY) +
            preferences.getBytesLength(TIMEZONE_KEY);
        preferencesDebug("System Preferences (systemPrefs): %d bytes used", systemUsed);
        preferences.end();
    } else {
        preferencesDebug("System Preferences (systemPrefs): Not initialized");
    }
    
    // Check user preferences namespace
    if (preferences.begin(USER_NAMESPACE, true)) {
        size_t userUsed = preferences.getBytesLength(USER_THEME_KEY) +
            preferences.getBytesLength(USER_AUDIO_ENABLED_KEY) +
            preferences.getBytesLength(USER_HAPTIC_ENABLED_KEY) +
            preferences.getBytesLength(USER_GLITCH_ENABLED_KEY) +
            preferences.getBytesLength(USER_SCANLINES_ENABLED_KEY) +
            preferences.getBytesLength(USER_DITHERING_ENABLED_KEY) +
            preferences.getBytesLength(USER_CHROMATIC_ENABLED_KEY) +
            preferences.getBytesLength(USER_DOT_MATRIX_ENABLED_KEY) +
            preferences.getBytesLength(USER_PIXELATE_ENABLED_KEY) +
            preferences.getBytesLength(USER_TINT_ENABLED_KEY) +
            preferences.getBytesLength(USER_TINT_COLOR_KEY) +
            preferences.getBytesLength(USER_TINT_INTENSITY_KEY);
        preferencesDebug("User Preferences (userPrefs): %d bytes used", userUsed);
        preferences.end();
    } else {
        preferencesDebug("User Preferences (userPrefs): Not initialized");
    }
    
    // Get free heap as a proxy for available memory
    preferencesDebug("Free heap: %d bytes", ESP.getFreeHeap());
    preferencesDebug("=====================================");
}

void clearAllPreferences() {
    preferencesDebug("Clearing all preference categories...");
    
    // Clear WiFi preferences
    if (preferences.begin(WIFI_NAMESPACE, false)) {
        preferences.clear();
        preferences.end();
        preferencesDebug("WiFi preferences cleared");
    }
    
    // Clear system preferences  
    if (preferences.begin(SYSTEM_NAMESPACE, false)) {
        preferences.clear();
        preferences.end();
        preferencesDebug("System preferences cleared");
    }
    
    // Clear user preferences
    if (preferences.begin(USER_NAMESPACE, false)) {
        preferences.clear();
        preferences.end();
        preferencesDebug("User preferences cleared");
    }
    
    // Clear cached values so they get reloaded from defaults
    audioPreferenceCached = false;
    hapticPreferenceCached = false;
    
    preferencesDebug("All preference categories cleared");
}

// =============================================================================
// Clock Preferences
// =============================================================================

bool getClockEnabled() {
    // SERIES_2 flag overrides user preferences
    return checkHardwareSupport();
}