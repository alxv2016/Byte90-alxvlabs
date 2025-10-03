/**
 * @file motion_module.h
 * @brief Motion detection and device orientation functionality with power management
 */

#ifndef MOTION_MODULE_H
#define MOTION_MODULE_H

#include "adxl_module.h"
#include "common.h"

//==============================================================================
// CONSTANTS & DEFINITIONS
//==============================================================================

static const char *MOTION_LOG = "::MOTION_MODULE::";

//==============================================================================
// TYPE DEFINITIONS
//==============================================================================

enum class MotionStateType {
  SHAKING = 0,
  TAPPED,
  DOUBLE_TAPPED,
  SLEEP,
  DEEP_SLEEP,
  UPSIDE_DOWN,
  TILTED_LEFT,
  TILTED_RIGHT,
  HALF_TILTED_LEFT,
  HALF_TILTED_RIGHT,
  SUDDEN_ACCELERATION,
  MOTION_STATE_COUNT
};

//==============================================================================
// PUBLIC API FUNCTIONS
//==============================================================================

/**
 * @brief Main function to poll accelerometer data and process all motion events
 */
void ADXLDataPolling(void);

/**
 * @brief Check if device detected sudden acceleration
 * @return true if sudden acceleration detected, false otherwise
 */
bool motionSuddenAcceleration();

/**
 * @brief Detect shaking motion using the accelerometer
 * @param samples Number of samples to analyze
 */
void detectShakes(uint8_t samples);

/**
 * @brief Detect tap and double-tap events using the accelerometer
 */
void detectTapping(void);

/**
 * @brief Detect device orientation changes
 * @param samples Number of samples to analyze
 */
void detectOrientation(uint8_t samples);

/**
 * @brief Detect lack of movement over time
 * @param samples Number of samples to analyze
 * @return true if device should enter deep sleep
 */
bool detectInactivity(uint8_t samples);

/**
 * @brief Handle entry into deep sleep mode
 */
void handleDeepSleep(void);

/**
 * @brief Set a specific motion state
 * @param state The motion state to set
 * @param value The boolean value to set
 */
void setMotionState(MotionStateType state, bool value);

/**
 * @brief Check if a specific motion state is active
 * @param state The motion state to check
 * @return true if the state is active, false otherwise
 */
bool checkMotionState(MotionStateType state);

/**
 * @brief Reset all motion states to inactive
 */
void resetMotionState();

/**
 * @brief Monitor and update haptics power state based on device activity
 * @param samples Number of samples to analyze for activity detection
 */
void monitorHapticsPowerState(uint8_t samples);

/**
 * @brief Check if device was tapped
 * @return true if tapped, false otherwise
 */
bool motionTapped();

/**
 * @brief Check if device was double-tapped
 * @return true if double-tapped, false otherwise
 */
bool motionDoubleTapped();

/**
 * @brief Check if device is upside down
 * @return true if upside down, false otherwise
 */
bool motionUpsideDown();

/**
 * @brief Check if device is tilted left
 * @return true if tilted left, false otherwise
 */
bool motionTiltedLeft();

/**
 * @brief Check if device is tilted right
 * @return true if tilted right, false otherwise
 */
bool motionTiltedRight();

/**
 * @brief Check if device is half tilted left
 * @return true if half tilted left, false otherwise
 */
bool motionHalfTiltedLeft();

/**
 * @brief Check if device is half tilted right
 * @return true if half tilted right, false otherwise
 */
bool motionHalfTiltedRight();

/**
 * @brief Check if device has been interacted with
 * @return true if interacted with, false otherwise
 */
bool motionInteracted();

/**
 * @brief Check if device is in a non-standard orientation
 * @return true if tilted left/right or upside down, false otherwise
 */
bool motionOriented();

/**
 * @brief Check if device is in sleep mode
 * @return true if in sleep mode, false otherwise
 */
bool motionSleep();

/**
 * @brief Check if device is in deep sleep mode
 * @return true if in deep sleep mode, false otherwise
 */
bool motionDeepSleep();

/**
 * @brief Check if device is being shaken
 * @return true if being shaken, false otherwise
 */
bool motionShaking();

#endif /* MOTION_MODULE_H */