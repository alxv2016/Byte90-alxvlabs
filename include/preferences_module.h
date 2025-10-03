/**
 * @file preferences_module.h
 * @brief Header for preferences management module
 *
 * Provides functionality for persistent storage and retrieval of system preferences,
 * user settings, WiFi credentials, and configuration data using ESP32 NVS (Non-Volatile Storage).
 */

#ifndef PREFERENCES_MODULE_H
#define PREFERENCES_MODULE_H

#include <stdint.h>
#include <stdbool.h>

//==============================================================================
// CONSTANTS & DEFINITIONS
//==============================================================================

static const char *PREFERENCES_LOG = "::PREFERENCES_MODULE::";

// WiFi credential buffer sizes - used by both modules
#define WIFI_SSID_MAX_LEN 32
#define WIFI_PASSWORD_MAX_LEN 64

//==============================================================================
// PUBLIC API FUNCTIONS
//==============================================================================

/**
 * @brief Initializes the preferences manager and NVS storage
 */
void initPreferencesManager();

/**
 * @brief Saves WiFi credentials to persistent storage
 * @param ssid WiFi network SSID
 * @param password WiFi network password
 * @return true if credentials saved successfully, false otherwise
 */
bool saveWiFiCredentials(const char* ssid, const char* password);

/**
 * @brief Loads WiFi credentials from persistent storage
 * @param ssid Buffer to store SSID (must be at least WIFI_SSID_MAX_LEN bytes)
 * @param password Buffer to store password (must be at least WIFI_PASSWORD_MAX_LEN bytes)
 * @return true if credentials loaded successfully, false otherwise
 */
bool loadWiFiCredentials(char* ssid, char* password);

/**
 * @brief Clears stored WiFi credentials from persistent storage
 */
void clearWiFiCredentials();

/**
 * @brief Gets the current WiFi mode enabled state
 * @return true if WiFi mode is enabled, false otherwise
 */
bool getWiFiModeEnabled();

/**
 * @brief Sets the WiFi mode enabled state
 * @param enabled true to enable WiFi mode, false to disable
 * @return true if state saved successfully, false otherwise
 */
bool setWiFiModeEnabled(bool enabled);

/**
 * @brief Saves the startup mode to persistent storage
 * @param mode Startup mode value to save
 */
void saveStartupMode(uint8_t mode);

/**
 * @brief Loads the startup mode from persistent storage
 * @return Startup mode value or default if not found
 */
uint8_t loadStartupMode();

/**
 * @brief Saves the last known good state to persistent storage
 * @param state State value to save
 */
void saveLastKnownGoodState(uint8_t state);

/**
 * @brief Loads the last known good state from persistent storage
 * @return State value or default if not found
 */
uint8_t loadLastKnownGoodState();

/**
 * @brief Saves timezone information to persistent storage
 * @param timezone Timezone string to save
 * @return true if timezone saved successfully, false otherwise
 */
bool saveTimezone(const char* timezone);

/**
 * @brief Loads timezone information from persistent storage
 * @param timezone Buffer to store timezone string
 * @return true if timezone loaded successfully, false otherwise
 */
bool loadTimezone(char* timezone);

/**
 * @brief Clears all system preferences from persistent storage
 */
void clearSystemPreferences();

/**
 * @brief Saves user theme preference to persistent storage
 * @param theme Theme name to save
 */
void saveUserTheme(const char* theme);

/**
 * @brief Loads user theme preference from persistent storage
 * @param theme Buffer to store theme name
 * @return true if theme loaded successfully, false otherwise
 */
bool loadUserTheme(char* theme);

/**
 * @brief Clears all user preferences from persistent storage
 */
void clearUserPreferences();

/**
 * @brief Gets the current audio enabled state
 * @return true if audio is enabled, false otherwise
 */
bool getAudioEnabled();

/**
 * @brief Sets the audio enabled state
 * @param enabled true to enable audio, false to disable
 * @return true if state saved successfully, false otherwise
 */
bool setAudioEnabled(bool enabled);

/**
 * @brief Gets the current haptic feedback enabled state
 * @return true if haptic feedback is enabled, false otherwise
 */
bool getHapticEnabled();

/**
 * @brief Sets the haptic feedback enabled state
 * @param enabled true to enable haptic feedback, false to disable
 * @return true if state saved successfully, false otherwise
 */
bool setHapticEnabled(bool enabled);

/**
 * @brief Gets the current glitch effect enabled state
 * @return true if glitch effect is enabled, false otherwise
 */
bool getGlitchEnabled();

/**
 * @brief Sets the glitch effect enabled state
 * @param enabled true to enable glitch effect, false to disable
 * @return true if state saved successfully, false otherwise
 */
bool setGlitchEnabled(bool enabled);

/**
 * @brief Gets the current scanlines effect enabled state
 * @return true if scanlines effect is enabled, false otherwise
 */
bool getScanlinesEnabled();

/**
 * @brief Sets the scanlines effect enabled state
 * @param enabled true to enable scanlines effect, false to disable
 * @return true if state saved successfully, false otherwise
 */
bool setScanlinesEnabled(bool enabled);

/**
 * @brief Gets the current dithering effect enabled state
 * @return true if dithering effect is enabled, false otherwise
 */
bool getDitheringEnabled();

/**
 * @brief Sets the dithering effect enabled state
 * @param enabled true to enable dithering effect, false to disable
 * @return true if state saved successfully, false otherwise
 */
bool setDitheringEnabled(bool enabled);

/**
 * @brief Gets the current chromatic aberration effect enabled state
 * @return true if chromatic aberration effect is enabled, false otherwise
 */
bool getChromaticEnabled();

/**
 * @brief Sets the chromatic aberration effect enabled state
 * @param enabled true to enable chromatic aberration effect, false to disable
 * @return true if state saved successfully, false otherwise
 */
bool setChromaticEnabled(bool enabled);

/**
 * @brief Gets the current dot matrix effect enabled state
 * @return true if dot matrix effect is enabled, false otherwise
 */
bool getDotMatrixEnabled();

/**
 * @brief Sets the dot matrix effect enabled state
 * @param enabled true to enable dot matrix effect, false to disable
 * @return true if state saved successfully, false otherwise
 */
bool setDotMatrixEnabled(bool enabled);

/**
 * @brief Gets the current pixelate effect enabled state
 * @return true if pixelate effect is enabled, false otherwise
 */
bool getPixelateEnabled();

/**
 * @brief Sets the pixelate effect enabled state
 * @param enabled true to enable pixelate effect, false to disable
 * @return true if state saved successfully, false otherwise
 */
bool setPixelateEnabled(bool enabled);

/**
 * @brief Gets the current tint effect enabled state
 * @return true if tint effect is enabled, false otherwise
 */
bool getTintEnabled();

/**
 * @brief Sets the tint effect enabled state
 * @param enabled true to enable tint effect, false to disable
 * @return true if state saved successfully, false otherwise
 */
bool setTintEnabled(bool enabled);

/**
 * @brief Gets the current tint color value
 * @return Tint color as 16-bit RGB565 value
 */
uint16_t getTintColor();

/**
 * @brief Sets the tint color value
 * @param color Tint color as 16-bit RGB565 value
 * @return true if color saved successfully, false otherwise
 */
bool setTintColor(uint16_t color);

/**
 * @brief Gets the current tint intensity value
 * @return Tint intensity as float (0.0 to 1.0)
 */
float getTintIntensity();

/**
 * @brief Sets the tint intensity value
 * @param intensity Tint intensity as float (0.0 to 1.0)
 * @return true if intensity saved successfully, false otherwise
 */
bool setTintIntensity(float intensity);

/**
 * @brief Gets the current clock enabled state
 * @return true if clock is enabled, false otherwise
 */
bool getClockEnabled();

/**
 * @brief Checks if preferences storage is available and initialized
 * @return true if preferences are available, false otherwise
 */
bool isPreferencesAvailable();

/**
 * @brief Prints storage information and statistics to serial output
 */
void printStorageInfo();

/**
 * @brief Clears all preferences from persistent storage
 */
void clearAllPreferences();

/**
 * @brief Enable or disable debug logging for preferences operations
 * @param enabled true to enable debug logging, false to disable
 */
void setPreferencesDebug(bool enabled);

#endif /* PREFERENCES_MODULE_H */