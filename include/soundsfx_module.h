/**
 * @file soundsfx_module.h
 * @brief Complete sound effects system - core functionality and unified interface
 */

#ifndef SOUNDSFX_MODULE_H
#define SOUNDSFX_MODULE_H

#include <stdint.h>
#include <stdbool.h>

// Include core definitions and shared functions
#include "soundsfx_core.h"

// Include specialized sound effect modules
#include "soundsfx_system_ui.h"
#include "soundsfx_emotions.h"
#include "soundsfx_typing.h"
#include "soundsfx_communication.h"

//==============================================================================
// CORE API FUNCTIONS
//==============================================================================

/**
 * @brief Initialize the complete sound effects system
 */
void sfxInit(void);

/**
 * @brief Check if enough time has passed to play a sound safely
 * @param soundName Name of the sound to check
 * @param customDebounceMs Custom debounce time (0 = use default)
 * @return true if sound can be played, false if would cause audio conflicts
 */
bool sfxCanPlaySound(const char* soundName, uint32_t customDebounceMs = 0);

/**
 * @brief Update debouncing state after sound approval
 * @param soundName Name of the sound that will be played
 */
void sfxUpdateSoundDebounce(const char* soundName);

/**
 * @brief Reset debouncing state (allow immediate next sound)
 */
void sfxResetDebounce(void);

/**
 * @brief Check if a sound would be debounced without playing it
 * @param soundName String identifier for the sound effect
 * @return true if sound would play, false if it would be debounced
 */
bool sfxWouldPlaySound(const char* soundName);

/**
 * @brief Get milliseconds until a sound can be played
 * @param soundName String identifier for the sound effect
 * @return Milliseconds until sound can play (0 if can play now)
 */
uint32_t sfxGetDebounceRemaining(const char* soundName);

//==============================================================================
// UNIFIED API FUNCTIONS
//==============================================================================

/**
 * @brief Play sound effect asynchronously with smart debouncing
 * @param soundName String identifier for the sound effect
 * @param delayMS Delay before playing sound
 * @param debounceMS Custom debounce time
 * @param force If true, bypasses debouncing for critical sounds
 */
void sfxPlay(const char* soundName, uint32_t delayMS = 2, uint32_t debounceMS = 0, bool force = false);

/**
 * @brief Enable or disable debug logging for sound effects operations
 * @param enabled true to enable debug logging, false to disable
 */
void setSoundsfxDebug(bool enabled);

#endif /* SOUNDSFX_MODULE_H */