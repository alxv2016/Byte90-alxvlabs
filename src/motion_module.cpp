/**
 * @file motion_module.cpp
 * @brief Implementation of motion detection and device orientation functionality with power management
 *
 * Provides comprehensive motion detection, device orientation tracking, and power management
 * functionality for the BYTE-90 device using the ADXL345 accelerometer.
 * 
 * This module handles:
 * - Motion detection (shaking, tapping, double-tapping, sudden acceleration)
 * - Device orientation detection (tilted, upside down, half-tilted)
 * - Inactivity monitoring and deep sleep management
 * - Power state management for haptics and display
 * - Motion state tracking and event processing
 * - Integration with haptics, display, emotes, and ESP-NOW modules
 * - Real-time accelerometer data polling and analysis
 */

#include "motion_module.h"
#include "adxl_module.h"
#include "haptics_effects.h"
#include "common.h"
#include "display_module.h"
#include "emotes_module.h"
#include "espnow_module.h"
#include "haptics_module.h"
#include "menu_module.h"
#include "soundsfx_module.h"
#include "speaker_module.h"

//==============================================================================
// CONSTANTS & DEFINITIONS
//==============================================================================

const float SHAKE_THRESHOLD = 8.0;
const float INACTIVITY_THRESHOLD = 1.5;
const float TILT_THRESHOLD = 9.0;
const float HALF_TILT_THRESHOLD = 4.2;
const float FLIP_THRESHOLD = -8;

const unsigned long ENTER_DEEP_SLEEP_TIMER = 20000;
const unsigned long INACTIVITY_TIMEOUT = timeToMillis(1, 30);
const unsigned long DISPLAY_TIMEOUT = timeToMillis(0, 30);
const unsigned long IDLE_TIMEOUT = timeToMillis(1, 00);

//==============================================================================
// GLOBAL VARIABLES
//==============================================================================

static bool g_motionStates[static_cast<size_t>(MotionStateType::MOTION_STATE_COUNT)] = {false};

unsigned long INACTIVITY_TIME = 0;
unsigned long DISPLAY_TIME = 0;
unsigned long IDLE_TIME = 0;

//==============================================================================
// UTILITY FUNCTIONS (STATIC)
//==============================================================================

/**
 * @brief Check if any states in a given list are active
 * @param states Array of motion states to check
 * @param count Number of states in the array
 * @return true if any state in the array is active
 */
static bool checkAnyMotionStates(const MotionStateType *states, int count) {
  for (int i = 0; i < count; i++) {
    if (g_motionStates[static_cast<size_t>(states[i])]) {
      return true;
    }
  }
  return false;
}

/**
 * @brief Display the appropriate static image based on device mode
 */
static void checkDeviceModes() {
#if DEVICE_MODE == MAC_MODE
  deviceMode = "MAC_MODE";
  displayStaticImage(MAC_STATIC, 128, 128);
#elif DEVICE_MODE == PC_MODE
  deviceMode = "PC_MODE";
  displayStaticImage(PC_STATIC, 128, 128);
#else
  deviceMode = "BYTE_MODE";
  displayStaticImage(BYTE_STATIC, 128, 128);
#endif
}

/**
 * @brief Detect sudden acceleration using the accelerometer
 * @param samples Number of samples to analyze
 * @return true if sudden acceleration detected, false otherwise
 */
static bool detectSuddenAcceleration(uint8_t samples) {
  if (samples < 2)
    return false;

  const float ACCELERATION_THRESHOLD = 6.0;
  const float ACCELERATION_CHANGE_THRESHOLD = 4.0;

  static unsigned long accelLockoutTime = 0;
  const unsigned long ACCEL_LOCKOUT_PERIOD = 600;

  if (checkMotionState(MotionStateType::DOUBLE_TAPPED) ||
      checkMotionState(MotionStateType::TAPPED) ||
      checkMotionState(MotionStateType::SHAKING)) {
    accelLockoutTime = millis();
    return false;
  }

  if (millis() - accelLockoutTime < ACCEL_LOCKOUT_PERIOD) {
    return false;
  }

  static float prevMagnitude = 0;
  float currentMagnitude = 0;

  sensors_event_t event = getSensorData();
  currentMagnitude = calculateCombinedMagnitude(
      event.acceleration.x, event.acceleration.y, event.acceleration.z);

  float magnitudeChange = abs(currentMagnitude - prevMagnitude);

  prevMagnitude = currentMagnitude;

  if (currentMagnitude >= ACCELERATION_THRESHOLD &&
      magnitudeChange >= ACCELERATION_CHANGE_THRESHOLD) {
    setMotionState(MotionStateType::SUDDEN_ACCELERATION, true);
    if (areHapticsActive()) {
      playHapticEffect(HAPTIC_ALERT_750MS);
    }
    return true;
  }

  setMotionState(MotionStateType::SUDDEN_ACCELERATION, false);
  return false;
}

/**
 * @brief Play haptic feedback with cooldown to prevent overwhelming feedback
 * @param effectId The haptic effect to play
 * @param cooldownMs Minimum time between playing this effect
 * @param timerId Static variable ID to track cooldown
 */
static void playOrientationHaptic(uint8_t effectId, unsigned long cooldownMs, uint8_t timerId) {
  if (!areHapticsActive())
    return;

  static unsigned long lastHapticTimes[7] = {0, 0, 0, 0, 0, 0, 0};

  if (timerId >= 7)
    return;

  if (millis() - lastHapticTimes[timerId] > cooldownMs) {
    playHapticEffect(effectId);
    lastHapticTimes[timerId] = millis();
  }
}

/**
 * @brief Automatically adjust display brightness based on activity
 * @param samples Number of samples to analyze
 */
static void autoDimDisplay(uint8_t samples) {
  if (samples == 0)
    return;

  float totalMagnitude = 0;

  for (int i = 0; i < samples; i++) {
    sensors_event_t event = getSensorData();
    totalMagnitude += calculateCombinedMagnitude(
        event.acceleration.x, event.acceleration.y, event.acceleration.z);
  }

  float avgMagnitude = totalMagnitude / samples;
  if (avgMagnitude > INACTIVITY_THRESHOLD && setTimeout(DISPLAY_TIME, 200)) {
    setDisplayBrightness(DISPLAY_BRIGHTNESS_FULL);
    DISPLAY_TIME = millis();
    setMotionState(MotionStateType::SLEEP, false);
    return;
  }

  if (millis() - DISPLAY_TIME >= DISPLAY_TIMEOUT) {
    if (!checkMotionState(MotionStateType::SLEEP) &&
        setTimeout(IDLE_TIME, IDLE_TIMEOUT)) {
      setDisplayBrightness(DISPLAY_BRIGHTNESS_LOW);
      setMotionState(MotionStateType::SLEEP, true);
    }
  }
}

/**
 * @brief Monitor and handle sleep state transitions
 * @param samples Number of samples to analyze
 */
static void monitorSleep(uint8_t samples) {
  static unsigned long lastInactivityTime = 0;

  if (detectInactivity(samples)) {
    if (lastInactivityTime == 0) {
      lastInactivityTime = millis();
    }

    if (setTimeout(lastInactivityTime, ENTER_DEEP_SLEEP_TIMER)) {
      handleDeepSleep();
    }
  } else {
    if (lastInactivityTime != 0) {
      lastInactivityTime = 0;
    }
  }
}

//==============================================================================
// PUBLIC API FUNCTIONS
//==============================================================================

/**
 * @brief Set a specific motion state
 * @param state The motion state to set
 * @param value The boolean value to set
 * 
 * Sets the specified motion state to the given value and clears any pending
 * interrupts. Used to update motion state flags during motion detection.
 */
void setMotionState(MotionStateType state, bool value) {
  g_motionStates[static_cast<size_t>(state)] = value;
  clearInterrupts();
}

/**
 * @brief Check if a specific motion state is active
 * @param state The motion state to check
 * @return true if the state is active, false otherwise
 * 
 * Returns the current value of the specified motion state flag.
 * Used to query motion states from other modules.
 */
bool checkMotionState(MotionStateType state) {
  return g_motionStates[static_cast<size_t>(state)];
}

/**
 * @brief Reset all motion states to inactive
 * 
 * Clears all motion state flags, setting them to false. Used to reset
 * motion detection state after processing or during initialization.
 */
void resetMotionState() {
  for (size_t i = 0; i < static_cast<size_t>(MotionStateType::MOTION_STATE_COUNT); i++) {
    g_motionStates[i] = false;
  }
}

/**
 * @brief Monitor and update haptics power state based on device activity
 * @param samples Number of samples to analyze for activity detection
 * 
 * Monitors device movement to determine if haptics should be powered on or off.
 * Powers on haptics when movement is detected and powers off when device is
 * stationary to conserve battery power.
 */
void monitorHapticsPowerState(uint8_t samples) {
  if (!isHapticsReady() || samples == 0) {
    return;
  }

  float totalMagnitude = 0;
  for (int i = 0; i < samples; i++) {
    sensors_event_t event = getSensorData();
    totalMagnitude += calculateCombinedMagnitude(
        event.acceleration.x, event.acceleration.y, event.acceleration.z);
  }
  float avgMagnitude = totalMagnitude / samples;

  updateHapticsPowerState(avgMagnitude);

  // Only reset haptics timeout if haptics are enabled by user preference
  if (motionInteracted() && areHapticsActive()) {
    resetHapticsTimeout();
  }
}

/**
 * @brief Check if device was tapped
 * @return true if tapped, false otherwise
 * 
 * Returns the current state of the single tap detection flag.
 */
bool motionTapped() { 
  return checkMotionState(MotionStateType::TAPPED); 
}

/**
 * @brief Check if device was double-tapped
 * @return true if double-tapped, false otherwise
 * 
 * Returns the current state of the double tap detection flag.
 */
bool motionDoubleTapped() {
  return checkMotionState(MotionStateType::DOUBLE_TAPPED);
}

/**
 * @brief Check if device is upside down
 * @return true if upside down, false otherwise
 * 
 * Returns the current state of the upside down orientation detection flag.
 */
bool motionUpsideDown() {
  return checkMotionState(MotionStateType::UPSIDE_DOWN);
}

/**
 * @brief Check if device is tilted left
 * @return true if tilted left, false otherwise
 * 
 * Returns the current state of the left tilt orientation detection flag.
 */
bool motionTiltedLeft() {
  return checkMotionState(MotionStateType::TILTED_LEFT);
}

/**
 * @brief Check if device is tilted right
 * @return true if tilted right, false otherwise
 * 
 * Returns the current state of the right tilt orientation detection flag.
 */
bool motionTiltedRight() {
  return checkMotionState(MotionStateType::TILTED_RIGHT);
}

/**
 * @brief Check if device is half tilted left
 * @return true if half tilted left, false otherwise
 * 
 * Returns the current state of the half left tilt orientation detection flag.
 */
bool motionHalfTiltedLeft() {
  return checkMotionState(MotionStateType::HALF_TILTED_LEFT);
}

/**
 * @brief Check if device is half tilted right
 * @return true if half tilted right, false otherwise
 * 
 * Returns the current state of the half right tilt orientation detection flag.
 */
bool motionHalfTiltedRight() {
  return checkMotionState(MotionStateType::HALF_TILTED_RIGHT);
}

/**
 * @brief Check if device has been interacted with
 * @return true if interacted with, false otherwise
 * 
 * Checks if any interaction motion states are active (shaking, tapping,
 * double-tapping, or sudden acceleration). Used to detect user interaction.
 */
bool motionInteracted() {
  static const MotionStateType interactedStates[] = {
      MotionStateType::SHAKING, MotionStateType::TAPPED,
      MotionStateType::DOUBLE_TAPPED, MotionStateType::SUDDEN_ACCELERATION};
  return checkAnyMotionStates(interactedStates, 4);
}

/**
 * @brief Check if device is in a non-standard orientation
 * @return true if tilted left/right or upside down, false otherwise
 * 
 * Checks if any orientation motion states are active (tilted left/right,
 * half-tilted left/right, or upside down). Used to detect non-standard orientations.
 */
bool motionOriented() {
  static const MotionStateType orientedStates[] = {
      MotionStateType::TILTED_LEFT, MotionStateType::TILTED_RIGHT,
      MotionStateType::HALF_TILTED_LEFT, MotionStateType::HALF_TILTED_RIGHT,
      MotionStateType::UPSIDE_DOWN};
  return checkAnyMotionStates(orientedStates, 5);
}

/**
 * @brief Check if device is in sleep mode
 * @return true if in sleep mode, false otherwise
 * 
 * Returns the current state of the sleep mode detection flag.
 */
bool motionSleep() { 
  return checkMotionState(MotionStateType::SLEEP); 
}

/**
 * @brief Check if device is in deep sleep mode
 * @return true if in deep sleep mode, false otherwise
 * 
 * Returns the current state of the deep sleep mode detection flag.
 */
bool motionDeepSleep() { 
  return checkMotionState(MotionStateType::DEEP_SLEEP); 
}

/**
 * @brief Check if device is being shaken
 * @return true if being shaken, false otherwise
 * 
 * Returns the current state of the shaking motion detection flag.
 */
bool motionShaking() { 
  return checkMotionState(MotionStateType::SHAKING); 
}

/**
 * @brief Handle entry into deep sleep mode
 * 
 * Prepares the device for deep sleep by dimming the display, checking device modes,
 * and initiating the deep sleep sequence. Called when inactivity is detected.
 */
void handleDeepSleep() {
  setDisplayBrightness(DISPLAY_BRIGHTNESS_DIM);
  checkDeviceModes();
  enterDeepSleep();
}

/**
 * @brief Detect tap and double-tap events using the accelerometer
 * 
 * Monitors the ADXL345 interrupt pin and processes tap/double-tap events.
 * Uses hardware interrupt detection for accurate timing and distinguishes
 * between single taps and double taps based on interrupt source.
 */
void detectTapping() {
  if (digitalRead(INTERRUPT_PIN_D1) != HIGH)
    return;

  uint8_t intSource = readRegister(ADXL345_REG_INT_SOURCE);
  uint8_t tapStatus = readRegister(ADXL345_REG_ACT_TAP_STATUS);

  if (tapStatus & ADXL345_TAP_SOURCE_Z) {
    if (intSource & ADXL345_INT_SOURCE_DOUBLETAP) {
      setMotionState(MotionStateType::DOUBLE_TAPPED, true);
      if (areHapticsActive()) {
        playHapticEffect(HAPTIC_SHARP_CLICK_100);
      }
      return;
    }

    if (intSource & ADXL345_INT_SOURCE_SINGLETAP) {
      ESP_LOGI("BYTE-90", "Single tap Z detected");
      setMotionState(MotionStateType::TAPPED, true);
      if (areHapticsActive()) {
        playHapticEffect(HAPTIC_SHARP_CLICK_100);
      }
      return;
    }
  }

  if (tapStatus & ADXL345_TAP_SOURCE_Y) {
    if (intSource & ADXL345_INT_SOURCE_DOUBLETAP) {
      setMotionState(MotionStateType::DOUBLE_TAPPED, true);
      if (areHapticsActive()) {
        playHapticEffect(HAPTIC_SHARP_CLICK_100);
      }
      return;
    }
  }

  if (tapStatus & ADXL345_TAP_SOURCE_X) {
    if (intSource & ADXL345_INT_SOURCE_DOUBLETAP) {
      setMotionState(MotionStateType::DOUBLE_TAPPED, true);
      if (areHapticsActive()) {
        playHapticEffect(HAPTIC_SHARP_CLICK_100);
      }
      return;
    }

    if (intSource & ADXL345_INT_SOURCE_SINGLETAP) {
      setMotionState(MotionStateType::TAPPED, true);
      if (areHapticsActive()) {
        playHapticEffect(HAPTIC_SHARP_CLICK_100);
      }
      return;
    }
  }

  if (intSource & ADXL345_INT_SOURCE_DOUBLETAP) {
    setMotionState(MotionStateType::DOUBLE_TAPPED, true);
    if (areHapticsActive()) {
      playHapticEffect(HAPTIC_DOUBLE_CLICK_100);
    }
    return;
  }

  if (intSource & ADXL345_INT_SOURCE_SINGLETAP) {
    setMotionState(MotionStateType::TAPPED, true);
    if (areHapticsActive()) {
      playHapticEffect(HAPTIC_SHARP_CLICK_100);
    }
  }
}

/**
 * @brief Detect shaking motion using the accelerometer
 * @param samples Number of samples to analyze
 * 
 * Analyzes accelerometer data to detect shaking motion patterns. Uses a threshold-based
 * approach to identify rapid acceleration changes characteristic of shaking. Includes
 * interaction lockout to prevent conflicts with other motion detection algorithms.
 */
void detectShakes(uint8_t samples) {
  static unsigned long interactionLockoutTime = 0;
  const unsigned long INTERACTION_LOCKOUT_PERIOD = 500;

  if (checkMotionState(MotionStateType::TAPPED) ||
      checkMotionState(MotionStateType::DOUBLE_TAPPED) ||
      checkMotionState(MotionStateType::SUDDEN_ACCELERATION)) {
    interactionLockoutTime = millis();
    return;
  }

  if (millis() - interactionLockoutTime < INTERACTION_LOCKOUT_PERIOD) {
    return;
  }

  float totalMagnitude = 0;
  for (int i = 0; i < samples; i++) {
    sensors_event_t event = getSensorData();
    totalMagnitude += calculateCombinedMagnitude(
        event.acceleration.x, event.acceleration.y, event.acceleration.z);
  }

  float avgMagnitude = totalMagnitude / samples;
  if (avgMagnitude >= SHAKE_THRESHOLD) {
    setMotionState(MotionStateType::SHAKING, true);
    
    if (areHapticsActive()) {
      static unsigned long lastShakeHaptic = 0;
      if (millis() - lastShakeHaptic > 500) {
        playHapticEffect(HAPTIC_STRONG_BUZZ_100);
        lastShakeHaptic = millis();
      }
    }
  }
}

/**
 * @brief Detect device orientation changes
 * @param samples Number of samples to analyze
 * 
 * Analyzes accelerometer data to determine device orientation including tilted,
 * upside down, and half-tilted states. Uses averaged acceleration data over
 * multiple samples for stable orientation detection.
 */
void detectOrientation(uint8_t samples) {
  if (samples == 0)
    return;

  float avgX = 0, avgY = 0, avgZ = 0;

  for (int i = 0; i < samples; i++) {
    sensors_event_t event = getSensorData();
    avgX += event.acceleration.x;
    avgY += event.acceleration.y;
    avgZ += event.acceleration.z;
  }

  avgX /= samples;
  avgY /= samples;
  avgZ /= samples;

  setMotionState(MotionStateType::UPSIDE_DOWN, false);
  setMotionState(MotionStateType::TILTED_LEFT, false);
  setMotionState(MotionStateType::TILTED_RIGHT, false);
  setMotionState(MotionStateType::HALF_TILTED_LEFT, false);
  setMotionState(MotionStateType::HALF_TILTED_RIGHT, false);

  if (avgZ <= FLIP_THRESHOLD) {
    setMotionState(MotionStateType::UPSIDE_DOWN, true);
    playOrientationHaptic(HAPTIC_RAMP_DOWN_LONG_SMOOTH_1_100, 200, 0);
  } else if (avgY >= TILT_THRESHOLD) {
    setMotionState(MotionStateType::TILTED_RIGHT, true);
    playOrientationHaptic(HAPTIC_STRONG_BUZZ_100, 200, 1);
  } else if (avgY <= -TILT_THRESHOLD) {
    setMotionState(MotionStateType::TILTED_LEFT, true);
    playOrientationHaptic(HAPTIC_STRONG_BUZZ_100, 200, 2);
  } else if (avgY >= HALF_TILT_THRESHOLD && avgY < TILT_THRESHOLD) {
    setMotionState(MotionStateType::HALF_TILTED_RIGHT, true);
    playOrientationHaptic(HAPTIC_LONG_DOUBLE_SHARP_CLICK_STRONG_1_100, 150, 3);
  } else if (avgY <= -HALF_TILT_THRESHOLD && avgY > -TILT_THRESHOLD) {
    setMotionState(MotionStateType::HALF_TILTED_LEFT, true);
    playOrientationHaptic(HAPTIC_LONG_DOUBLE_SHARP_CLICK_STRONG_1_100, 150, 4);
  } else if (avgX >= HALF_TILT_THRESHOLD && avgX < TILT_THRESHOLD) {
    //setMotionState(MotionStateType::HALF_TILTED_RIGHT, true);
    playOrientationHaptic(HAPTIC_LONG_DOUBLE_SHARP_CLICK_STRONG_1_100, 150, 5);
  } else if (avgX <= -HALF_TILT_THRESHOLD && avgX > -TILT_THRESHOLD) {
    //setMotionState(MotionStateType::HALF_TILTED_LEFT, true);
    playOrientationHaptic(HAPTIC_LONG_DOUBLE_SHARP_CLICK_STRONG_1_100, 150, 6);
  } 
}

/**
 * @brief Detect lack of movement over time
 * @param samples Number of samples to analyze
 * @return true if device should enter deep sleep
 * 
 * Monitors device movement over time to detect inactivity periods. Calculates
 * combined acceleration magnitude and compares against threshold to determine
 * if device should enter deep sleep mode for power conservation.
 */
bool detectInactivity(uint8_t samples) {
  if (samples == 0)
    return false;

  float totalMagnitude = 0;
  for (int i = 0; i < samples; i++) {
    sensors_event_t event = getSensorData();
    totalMagnitude += calculateCombinedMagnitude(
        event.acceleration.x, event.acceleration.y, event.acceleration.z);
  }

  float avgMagnitude = totalMagnitude / samples;
  if (avgMagnitude < INACTIVITY_THRESHOLD) {
    if (INACTIVITY_TIME == 0) {
      INACTIVITY_TIME = millis();
    } else if (millis() - INACTIVITY_TIME >= INACTIVITY_TIMEOUT) {
      setMotionState(MotionStateType::DEEP_SLEEP, true);
      return true;
    }
  } else {
    INACTIVITY_TIME = 0;
    setMotionState(MotionStateType::DEEP_SLEEP, false);
  }
  return false;
}

/**
 * @brief Check if device detected sudden acceleration
 * @return true if sudden acceleration detected, false otherwise
 * 
 * Returns the current state of the sudden acceleration detection flag.
 * Used to detect rapid acceleration changes that may indicate device movement.
 */
bool motionSuddenAcceleration() {
  return checkMotionState(MotionStateType::SUDDEN_ACCELERATION);
}

//==============================================================================
// PUBLIC API FUNCTIONS
//==============================================================================

/**
 * @brief Main function to poll accelerometer data and process all motion events
 * 
 * Continuously polls the ADXL345 accelerometer for new data and processes all motion
 * detection algorithms including shaking, tapping, orientation, and inactivity detection.
 * Also handles menu updates and power management based on device activity.
 */
void ADXLDataPolling() {
  menu_update();

  if (!isSensorEnabled())
    return;

  uint8_t samplesAvailable = getFifoSampleData();
  if (samplesAvailable == 0)
    return;

  detectShakes(samplesAvailable);
  if (!checkMotionState(MotionStateType::SHAKING)) {
    detectTapping();
    detectInactivity(samplesAvailable);
  }

  detectSuddenAcceleration(samplesAvailable);
  detectOrientation(samplesAvailable);
  monitorSleep(samplesAvailable);
  autoDimDisplay(samplesAvailable);
  monitorHapticsPowerState(samplesAvailable);
}