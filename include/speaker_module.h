/**
 * @file speaker_module.h
 * @brief Enhanced audio functionality with ESPHome ESP32-audioI2S support for MAX98357A codec
 */

#ifndef SPEAKER_MODULE_H
#define SPEAKER_MODULE_H

#include "common.h"
#include <Arduino.h>
#include <Preferences.h>
#include "Audio.h"
#include "flash_module.h"
#include "speaker_common.h"

//==============================================================================
// CONSTANTS & DEFINITIONS
//==============================================================================

static const char *SPEAKER_LOG = "::SPEAKER_MODULE::";

//==============================================================================
// PUBLIC API FUNCTIONS
//==============================================================================

/**
 * @brief Initialize the speaker module with optional preference checking
 * @param checkPreferences If true, checks user preferences before initialization
 * @return true if initialization successful, false otherwise
 */
bool initializeSpeaker(bool checkPreferences = true);

/**
 * @brief Shutdown audio system with optional preference saving
 * @param saveAsDisabled If true, saves the disabled state to persistent storage
 */
void shutdownAudio(bool saveAsDisabled = false);

/**
 * @brief Restart audio system after shutdown
 * @return true if restart successful, false otherwise
 */
bool restartAudio(void);

/**
 * @brief Get comprehensive audio system state
 * @return audio_state_t indicating current state
 */
audio_state_t getAudioState(void);

/**
 * @brief Stop any currently playing audio
 * @param cleanup If true, deletes Audio object; if false, just stops playback
 * @return true if stopped successfully, false otherwise
 */
bool stopAudio(bool cleanup = true);

/**
 * @brief Generate and play a single beep - Core audio function
 * @param frequency Frequency in Hz
 * @param duration Duration in milliseconds
 * @param volume Volume level (0-100)
 */
void playBeep(uint16_t frequency, uint16_t duration, uint8_t volume);

// MP3 functions moved to speaker_mp3.h

/**
 * @brief Audio loop function for core speaker operations
 */
void audioLoop(void);

/**
 * @brief Enable or disable debug logging for speaker operations
 * @param enabled true to enable debug logging, false to disable
 */
void setSpeakerDebug(bool enabled);

#endif /* SPEAKER_MODULE_H */