/**
 * @file haptics_module.h
 * @brief Simplified haptic feedback using DRV2605L haptic driver
 *
 * Provides basic haptic feedback functionality using the Adafruit DRV2605 library
 * with power management based on device activity.
 */

#ifndef HAPTICS_MODULE_H
#define HAPTICS_MODULE_H

#include "common.h"
#include "Adafruit_DRV2605.h"
#include <Arduino.h>
#include <Wire.h>

//==============================================================================
// CONSTANTS & DEFINITIONS
//==============================================================================

static const char *HAPTICS_LOG = "::HAPTICS::";

#define DRV2605_REG_OVERDRIVE    0x0D
#define DRV2605_REG_SUSTAINPOS   0x0E
#define DRV2605_REG_SUSTAINNEG   0x0F
#define DRV2605_REG_BREAKTIME    0x10

#define HAPTIC_INTENSITY_OFF        0
#define HAPTIC_INTENSITY_LOW        64
#define HAPTIC_INTENSITY_MEDIUM     128
#define HAPTIC_INTENSITY_HIGH       192
#define HAPTIC_INTENSITY_MAX        255

//==============================================================================
// TYPE DEFINITIONS
//==============================================================================

typedef enum {
    HAPTIC_ACTUATOR_ERM = 0,
    HAPTIC_ACTUATOR_LRA = 1
} haptic_actuator_t;

//==============================================================================
// PUBLIC API FUNCTIONS
//==============================================================================

/**
 * @brief Initialize the haptics module
 * @param actuatorType Type of haptic actuator (ERM or LRA)
 * @return true if initialization successful, false otherwise
 */
bool initializeHaptics(haptic_actuator_t actuatorType = HAPTIC_ACTUATOR_ERM);

/**
 * @brief Check if haptics module is ready to use
 * @return true if haptics is initialized and ready, false otherwise
 */
bool isHapticsReady(void);

/**
 * @brief Shutdown haptics and enter standby mode
 */
void shutdownHaptics(void);

/**
 * @brief Play a haptic effect from the ROM library
 * @param effectId Effect ID to play (1-123)
 * @return true if effect started successfully, false otherwise
 */
bool playHapticEffect(uint8_t effectId);

/**
 * @brief Start continuous vibration at specified intensity
 * @param intensity Vibration intensity (0-255)
 * @return true if vibration started successfully, false otherwise
 */
bool startContinuousVibration(uint8_t intensity);

/**
 * @brief Set vibration intensity for continuous mode
 * @param intensity New vibration intensity (0-255, 0=stop)
 * @return true if intensity set successfully, false otherwise
 */
bool setVibrationIntensity(uint8_t intensity);

/**
 * @brief Stop any currently playing haptic effect
 * @return true if stop successful, false otherwise
 */
bool stopHapticEffect(void);

/**
 * @brief Enable haptics and reset timeout
 */
void enableHaptics(void);

/**
 * @brief Disable haptics to save power
 */
void disableHaptics(void);

/**
 * @brief Check if haptics are currently active
 * @return true if haptics are enabled and ready to play effects, false if disabled for power saving
 */
bool areHapticsActive(void);

/**
 * @brief Update haptics power state based on activity level
 * @param activityLevel Current activity magnitude in m/s² (from accelerometer)
 */
void updateHapticsPowerState(float activityLevel);

/**
 * @brief Reset haptics timeout (call when user interaction detected)
 */
void resetHapticsTimeout(void);

/**
 * @brief Get diagnostic information
 * @param buffer Buffer to store diagnostic string (minimum 192 bytes)
 * @return true if diagnostics read successfully, false otherwise
 */
bool getHapticsDiagnostics(char *buffer);

/**
 * @brief Log current haptics status
 */
void logHapticsStatus(void);

/**
 * @brief Start a non-blocking pulse vibration
 * @param intensity Vibration intensity (0-255)
 * @param durationMs Duration in milliseconds
 * @return true if pulse started successfully, false otherwise
 */
bool startPulseVibration(uint8_t intensity, uint16_t durationMs);

/**
 * @brief Update non-blocking pulse vibration (call in main loop)
 * @return true if pulse is still active, false if completed
 */
bool updateHapticsPulse(void);

#endif /* HAPTICS_MODULE_H */