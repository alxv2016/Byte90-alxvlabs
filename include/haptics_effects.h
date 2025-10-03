/**
 * @file haptics_effects.h
 * @brief Haptic effect definitions and high-level effect functions
 *
 * Provides haptic effect constants and high-level effect functions
 * that use the core haptics module for device control.
 */

#ifndef HAPTICS_EFFECTS_H
#define HAPTICS_EFFECTS_H

#include "haptics_module.h"
#include <Arduino.h>

//==============================================================================
// HAPTIC EFFECT DEFINITIONS
//==============================================================================

// Basic click effects
#define HAPTIC_STRONG_CLICK_100     1
#define HAPTIC_SHARP_CLICK_100      4
#define HAPTIC_SOFT_BUMP_100        7
#define HAPTIC_DOUBLE_CLICK_100     10
#define HAPTIC_TRIPLE_CLICK_100     12

// Buzz and alert effects
#define HAPTIC_STRONG_BUZZ_100      14
#define HAPTIC_ALERT_750MS          15
#define HAPTIC_ALERT_1000MS         16
#define HAPTIC_BUZZ_1_100           47
#define HAPTIC_LONG_BUZZ_100        118

// Click variations
#define HAPTIC_STRONG_CLICK_1_100   17
#define HAPTIC_MEDIUM_CLICK_1_100   21
#define HAPTIC_SHARP_TICK_1_100     24

// Double click variations
#define HAPTIC_SHORT_DOUBLE_CLICK_STRONG_1_100      27
#define HAPTIC_SHORT_DOUBLE_CLICK_MEDIUM_1_100      31
#define HAPTIC_SHORT_DOUBLE_SHARP_TICK_1_100        34
#define HAPTIC_LONG_DOUBLE_SHARP_CLICK_STRONG_1_100 37
#define HAPTIC_LONG_DOUBLE_SHARP_CLICK_MEDIUM_1_100 41
#define HAPTIC_LONG_DOUBLE_SHARP_TICK_1_100         44

// Pulsing effects
#define HAPTIC_PULSING_STRONG_1_100  52
#define HAPTIC_PULSING_MEDIUM_1_100  54
#define HAPTIC_PULSING_SHARP_1_100   56

// Transition effects
#define HAPTIC_TRANSITION_CLICK_1_100    58
#define HAPTIC_TRANSITION_HUM_1_100      64

// Ramp down effects
#define HAPTIC_RAMP_DOWN_LONG_SMOOTH_1_100      70
#define HAPTIC_RAMP_DOWN_LONG_SMOOTH_2_100      71
#define HAPTIC_RAMP_DOWN_MEDIUM_SMOOTH_1_100    72
#define HAPTIC_RAMP_DOWN_MEDIUM_SMOOTH_2_100    73
#define HAPTIC_RAMP_DOWN_SHORT_SMOOTH_1_100     74
#define HAPTIC_RAMP_DOWN_SHORT_SMOOTH_2_100     75
#define HAPTIC_RAMP_DOWN_LONG_SHARP_1_100       76
#define HAPTIC_RAMP_DOWN_LONG_SHARP_2_100       77
#define HAPTIC_RAMP_DOWN_MEDIUM_SHARP_1_100     78
#define HAPTIC_RAMP_DOWN_MEDIUM_SHARP_2_100     79
#define HAPTIC_RAMP_DOWN_SHORT_SHARP_1_100      80
#define HAPTIC_RAMP_DOWN_SHORT_SHARP_2_100      81

// Ramp up effects
#define HAPTIC_RAMP_UP_LONG_SMOOTH_1_100        82
#define HAPTIC_RAMP_UP_LONG_SHARP_1_100         88

//==============================================================================
// HIGH-LEVEL EFFECT FUNCTIONS
//==============================================================================

/**
 * @brief Quick click feedback (like button press)
 * @param intensity 1=light, 2=medium, 3=strong
 * @return true if effect started successfully, false otherwise
 */
bool quickClick(uint8_t intensity = 2);

/**
 * @brief Quick notification buzz
 * @param intensity 1=light, 2=medium, 3=strong
 * @return true if effect started successfully, false otherwise
 */
bool notificationBuzz(uint8_t intensity = 2);

/**
 * @brief Quick pulse vibration (BLOCKING)
 * @param intensity Vibration intensity (0-255)
 * @param durationMs Duration in milliseconds
 * @return true if pulse completed successfully, false otherwise
 */
bool pulseVibration(uint8_t intensity, uint16_t durationMs);

/**
 * @brief Play 3 quick clicks followed by a strong buzz
 * @return true if effect sequence completed successfully, false otherwise
 */
bool tripleClickBuzz();

/**
 * @brief Test all haptic effects with display output
 */
void testHapticsEffects();

#endif /* HAPTICS_EFFECTS_H */
