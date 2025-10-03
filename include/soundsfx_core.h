/**
 * @file soundsfx_core.h
 * @brief Core sound effects definitions and shared functions
 */

#ifndef SOUNDSFX_CORE_H
#define SOUNDSFX_CORE_H

#include <stdint.h>
#include <stdbool.h>

//==============================================================================
// CONSTANTS & DEFINITIONS
//==============================================================================

static const char* SFX_LOG = "::SFX_MODULE::";

#define SFX_BASE_VOLUME 100
#define SFX_DEBOUNCE_TIME_MS 600
#define SFX_SAME_SOUND_DEBOUNCE_MS 800
#define SFX_MUTEX_TIMEOUT_MS 100

#define SFX_DEFAULT_TYPING_CHARS 10
#define SFX_DEFAULT_FAST_TYPING_CHARS 20
#define SFX_DEFAULT_TERMINAL_CHARS 15
#define SFX_DEFAULT_MATRIX_CHARS 12

#define SFX_TEST_DELAY_MS 1000
#define SFX_TYPING_MIN_DELAY_MS 40
#define SFX_TYPING_MAX_DELAY_MS 120

//==============================================================================
// SHARED CORE FUNCTIONS
//==============================================================================

/**
 * @brief Internal helper to play a beep with mutex protection
 * @param frequency Frequency in Hz
 * @param duration Duration in milliseconds
 * @param volume Volume level (0-100)
 */
void sfxPlayTone(uint16_t frequency, uint16_t duration, uint8_t volume);

/**
 * @brief Internal helper for task delays with consistent timing
 * @param milliseconds Delay time in milliseconds
 */
void sfxDelay(uint32_t milliseconds);

/**
 * @brief Generate a frequency slide effect
 * @param startFreq Starting frequency in Hz
 * @param endFreq Ending frequency in Hz
 * @param segments Number of frequency steps
 * @param segmentDuration Duration of each segment in ms
 * @param startVolume Starting volume (0-100)
 * @param endVolume Ending volume (0-100)
 */
void sfxGenerateFrequencySlide(uint16_t startFreq, uint16_t endFreq, 
                              int segments, int segmentDuration,
                              uint8_t startVolume, uint8_t endVolume);

/**
 * @brief Generate ascending tone sequence
 * @param baseFreq Base frequency in Hz
 * @param steps Number of steps
 * @param stepSize Frequency increment per step
 * @param duration Duration of each tone in ms
 * @param volume Volume level (0-100)
 */
void sfxGenerateAscendingSequence(uint16_t baseFreq, int steps, 
                                 uint16_t stepSize, int duration, uint8_t volume);

#endif /* SOUNDSFX_CORE_H */