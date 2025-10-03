/**
 * @file soundsfx_module.cpp
 * @brief Implementation of complete sound effects system - core functionality and unified interface
 *
 * Provides the main sound effects system implementation including core API functions,
 * unified interface, smart debouncing, and integration with specialized sound effect
 * modules for the BYTE-90 device.
 * 
 * This module handles:
 * - Sound effects system initialization and management
 * - Smart debouncing to prevent audio conflicts
 * - Unified API for playing sound effects asynchronously
 * - Integration with system UI, emotions, and typing sound modules
 * - Debug logging and control for sound effects operations
 * - Audio state validation and conflict prevention
 */

#include "soundsfx_module.h"
#include "speaker_module.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>

//==============================================================================
// DEBUG SYSTEM
//==============================================================================

// Debug control - set to true to enable debug logging
static bool soundsfxDebugEnabled = false;

/**
 * @brief Centralized debug logging function for sound effects operations
 * @param format Printf-style format string
 * @param ... Variable arguments for format string
 */
static void soundsfxDebug(const char* format, ...) {
  if (!soundsfxDebugEnabled) {
    return;
  }
  
  va_list args;
  va_start(args, format);
  esp_log_writev(ESP_LOG_INFO, "SFX_LOG", format, args);
  va_end(args);
}

/**
 * @brief Enable or disable debug logging for sound effects operations
 * @param enabled true to enable debug logging, false to disable
 * 
 * @example
 * // Enable debug logging
 * setSoundsfxDebug(true);
 * 
 * // Disable debug logging  
 * setSoundsfxDebug(false);
 */
void setSoundsfxDebug(bool enabled) {
  soundsfxDebugEnabled = enabled;
  if (enabled) {
    ESP_LOGI("SFX_LOG", "Sound effects debug logging enabled");
  } else {
    ESP_LOGI("SFX_LOG", "Sound effects debug logging disabled");
  }
}

//==============================================================================
// GLOBAL VARIABLES
//==============================================================================

static unsigned long lastSoundTime = 0;
static char lastSoundName[32] = {0};
static unsigned long lastSameSoundTime = 0;

static SemaphoreHandle_t audioMutex = NULL;

//==============================================================================
// TYPE DEFINITIONS
//==============================================================================

typedef struct {
    char soundName[32];
} async_sound_params_t;

//==============================================================================
// UTILITY FUNCTIONS (STATIC)
//==============================================================================

/**
 * @brief Execute the appropriate sound function based on sound name
 * @param effectName Name of the sound effect to play
 */
static void executeSoundByName(const char* effectName) {
    audio_state_t audioState = getAudioState();
    if (audioState != AUDIO_STATE_READY && audioState != AUDIO_STATE_PLAYING) {
        ESP_LOGW(SFX_LOG, "Audio not ready for sound playback");
        return;
    }
    
    if (!effectName) {
        ESP_LOGW(SFX_LOG, "Invalid parameters for sound playback");
        return;
    }
    
    char lowerName[32];
    strncpy(lowerName, effectName, sizeof(lowerName) - 1);
    lowerName[sizeof(lowerName) - 1] = '\0';
    
    for (int i = 0; lowerName[i]; i++) {
        lowerName[i] = tolower(lowerName[i]);
    }
    
    // System and UI sounds
    if (strcmp(lowerName, "startup") == 0) sfxPlayStartup();
    else if (strcmp(lowerName, "notification") == 0) sfxPlayNotification();
    else if (strcmp(lowerName, "warning") == 0) sfxPlayWarning();
    else if (strcmp(lowerName, "error") == 0) sfxPlayError();
    else if (strcmp(lowerName, "shutdown") == 0) sfxPlayShutdown();
    else if (strcmp(lowerName, "loading") == 0) sfxPlayLoading();
    else if (strcmp(lowerName, "crash") == 0) sfxPlayCrash();
    else if (strcmp(lowerName, "alert") == 0) sfxPlayAlert();
    else if (strcmp(lowerName, "post") == 0) sfxPlayPOST();
    else if (strcmp(lowerName, "question") == 0) sfxPlayQuestion();
    else if (strcmp(lowerName, "menu") == 0) sfxPlayMenu();
    else if (strcmp(lowerName, "confirm") == 0) sfxPlayConfirm();
    else if (strcmp(lowerName, "cancel") == 0) sfxPlayCancel();
    
    // Emotional and game sounds
    else if (strcmp(lowerName, "sad") == 0) sfxPlaySad();
    else if (strcmp(lowerName, "rejected") == 0) sfxPlayRejected();
    else if (strcmp(lowerName, "sigh") == 0) sfxPlaySigh();
    else if (strcmp(lowerName, "celebration") == 0) sfxPlayCelebration();
    else if (strcmp(lowerName, "tadaaaa") == 0) sfxPlayTaDaaaa();
    else if (strcmp(lowerName, "dizzy") == 0) sfxPlayDizzy();
    else if (strcmp(lowerName, "winner") == 0) sfxPlayWinner();
    else if (strcmp(lowerName, "gameover") == 0) sfxPlayGameOver();
    
    // Typing sounds
    else if (strcmp(lowerName, "keystroke") == 0) sfxPlayKeystroke();
    else if (strcmp(lowerName, "typing") == 0) sfxPlayTyping(SFX_DEFAULT_TYPING_CHARS, false);
    else if (strcmp(lowerName, "fasttyping") == 0) sfxPlayTyping(SFX_DEFAULT_FAST_TYPING_CHARS, true);
    else if (strcmp(lowerName, "terminal") == 0) sfxPlayTerminalTyping(SFX_DEFAULT_TERMINAL_CHARS);
    else if (strcmp(lowerName, "matrix") == 0) sfxPlayMatrixTyping(SFX_DEFAULT_MATRIX_CHARS);
    else if (strcmp(lowerName, "bell") == 0) sfxPlayTypewriterBell();
    else if (strcmp(lowerName, "backspace") == 0) sfxPlayBackspace();
    else if (strcmp(lowerName, "textcomplete") == 0) sfxPlayTextComplete();

    // Communication soundss
    else if (strcmp(lowerName, "talking") == 0) sfxPlayTalking();
    else if (strcmp(lowerName, "alttalking") == 0) sfxPlayAltTalking();
    else if (strcmp(lowerName, "laughing") == 0) sfxPlayLaughing();
    else if (strcmp(lowerName, "angry") == 0) sfxPlayAngry();
    else if (strcmp(lowerName, "shock") == 0) sfxPlayShock();
    
    else {
        ESP_LOGW(SFX_LOG, "Unknown sound effect: %s, playing notification as fallback", effectName);
        sfxPlayNotification();
    }
}

/**
 * @brief Simple async sound task that directly executes sound functions
 * @param parameter Task parameters containing sound name
 */
static void simpleAsyncSoundTask(void *parameter) {
    async_sound_params_t* params = (async_sound_params_t*)parameter;
    
    if (params) {
        audio_state_t audioState = getAudioState();
        if (audioState == AUDIO_STATE_READY || audioState == AUDIO_STATE_PLAYING) {
            vTaskDelay(pdMS_TO_TICKS(10));
            executeSoundByName(params->soundName);
        }
    }
    
    if (params) free(params);
    vTaskDelete(NULL);
}

//==============================================================================
// CORE API FUNCTIONS
//==============================================================================

/**
 * @brief Initialize the complete sound effects system
 * 
 * Initializes the sound effects system by creating the audio mutex for
 * thread-safe sound operations. This function should be called once
 * during system startup before using any sound effects.
 */
void sfxInit(void) {
    if (!audioMutex) {
        audioMutex = xSemaphoreCreateMutex();
        if (audioMutex) {
            soundsfxDebug("Complete sound effects system initialized with audio protection");
        } else {
            ESP_LOGE(SFX_LOG, "Failed to initialize sound effects system!");
        }
    }
}

void sfxPlayTone(uint16_t frequency, uint16_t duration, uint8_t volume) {
    audio_state_t audioState = getAudioState();
    if (audioState != AUDIO_STATE_READY && audioState != AUDIO_STATE_PLAYING) {
        ESP_LOGW(SFX_LOG, "Audio system not ready for SFX playback");
        return;
    }
    
    if (!audioMutex) {
        audioMutex = xSemaphoreCreateMutex();
        if (audioMutex) {
            ESP_LOGE(SFX_LOG, "Audio mutex initialized for hardware protection");
        } else {
            ESP_LOGE(SFX_LOG, "Failed to create audio mutex!");
        }
    }
    
    if (audioMutex && xSemaphoreTake(audioMutex, pdMS_TO_TICKS(SFX_MUTEX_TIMEOUT_MS)) == pdTRUE) {
        playBeep(frequency, duration, volume);
        xSemaphoreGive(audioMutex);
    } else {
        ESP_LOGW(SFX_LOG, "Audio mutex timeout - sound dropped to prevent scrambling");
    }
}

void sfxDelay(uint32_t milliseconds) {
    vTaskDelay(pdMS_TO_TICKS(milliseconds));
}

/**
 * @brief Check if enough time has passed to play a sound safely
 * @param soundName Name of the sound to check
 * @param customDebounceMs Custom debounce time (0 = use default)
 * @return true if sound can be played, false if would cause audio conflicts
 * 
 * Checks if enough time has passed since the last sound was played to avoid
 * audio conflicts. Uses either custom debounce time or default system debounce.
 * Also checks for same-sound debouncing to prevent rapid repetition.
 */
bool sfxCanPlaySound(const char* soundName, uint32_t customDebounceMs) {
    if (!soundName) return false;
    
    unsigned long currentTime = xTaskGetTickCount() * portTICK_PERIOD_MS;

    uint32_t basicDebounce = (customDebounceMs > 0) ? customDebounceMs : SFX_DEBOUNCE_TIME_MS;
    uint32_t sameDebounce = (customDebounceMs > 0) ? customDebounceMs : SFX_SAME_SOUND_DEBOUNCE_MS;
    
    if (currentTime - lastSoundTime < basicDebounce) {
        return false;
    }
    
    if (strcmp(lastSoundName, soundName) == 0) {
        if (currentTime - lastSameSoundTime < sameDebounce) {
            return false;
        }
    }
    
    return true;
}

/**
 * @brief Update debouncing state after sound approval
 * @param soundName Name of the sound that will be played
 * 
 * Updates the debouncing timestamps after a sound has been approved for playback.
 * Tracks both general sound timing and same-sound repetition timing.
 */
void sfxUpdateSoundDebounce(const char* soundName) {
    if (!soundName) return;
    
    unsigned long currentTime = xTaskGetTickCount() * portTICK_PERIOD_MS;
    
    lastSoundTime = currentTime;
    
    if (strcmp(lastSoundName, soundName) == 0) {
        lastSameSoundTime = currentTime;
    } else {
        strncpy(lastSoundName, soundName, sizeof(lastSoundName) - 1);
        lastSoundName[sizeof(lastSoundName) - 1] = '\0';
        lastSameSoundTime = currentTime;
    }
}

void sfxGenerateFrequencySlide(uint16_t startFreq, uint16_t endFreq, 
                              int segments, int segmentDuration,
                              uint8_t startVolume, uint8_t endVolume) {
    audio_state_t audioState = getAudioState();
    if (audioState != AUDIO_STATE_READY && audioState != AUDIO_STATE_PLAYING) return;
    if (segments <= 0) return;
    
    for (int i = 0; i < segments; i++) {
        uint16_t freq = startFreq + ((endFreq - startFreq) * i / segments);
        uint8_t volume = startVolume + ((endVolume - startVolume) * i / segments);
        
        sfxPlayTone(freq, segmentDuration, volume);
    }
}

void sfxGenerateAscendingSequence(uint16_t baseFreq, int steps, 
                                 uint16_t stepSize, int duration, uint8_t volume) {
    audio_state_t audioState = getAudioState();
    if (audioState != AUDIO_STATE_READY && audioState != AUDIO_STATE_PLAYING) return;
    if (steps <= 0) return;
    
    for (int i = 0; i < steps; i++) {
        uint16_t freq = baseFreq + (stepSize * i);
        sfxPlayTone(freq, duration, volume);
        if (i < steps - 1) {
            sfxDelay(duration / 2);
        }
    }
}

/**
 * @brief Reset debouncing state
 * 
 * Resets all debouncing timestamps and sound name tracking to allow
 * immediate playback of the next sound effect. Useful for critical
 * system sounds that should not be debounced.
 */
void sfxResetDebounce(void) {
    lastSoundTime = 0;
    lastSameSoundTime = 0;
    memset(lastSoundName, 0, sizeof(lastSoundName));
}

/**
 * @brief Check if a sound would be debounced without playing it
 * @param soundName String identifier for the sound effect
 * @return true if sound would play, false if it would be debounced
 * 
 * Checks if a sound would be allowed to play without actually playing it.
 * Useful for UI feedback or conditional logic based on sound availability.
 */
bool sfxWouldPlaySound(const char* soundName) {
    return sfxCanPlaySound(soundName);
}

/**
 * @brief Get milliseconds until a sound can be played
 * @param soundName String identifier for the sound effect
 * @return Milliseconds until sound can play (0 if can play now)
 * 
 * Calculates the remaining debounce time for a specific sound, considering
 * both general debouncing and same-sound repetition debouncing.
 */
uint32_t sfxGetDebounceRemaining(const char* soundName) {
    if (!soundName) return 0;
    
    unsigned long currentTime = xTaskGetTickCount() * portTICK_PERIOD_MS;
    
    uint32_t basicRemaining = 0;
    if (currentTime - lastSoundTime < SFX_DEBOUNCE_TIME_MS) {
        basicRemaining = SFX_DEBOUNCE_TIME_MS - (currentTime - lastSoundTime);
    }
    
    uint32_t sameRemaining = 0;
    if (strcmp(lastSoundName, soundName) == 0) {
        if (currentTime - lastSameSoundTime < SFX_SAME_SOUND_DEBOUNCE_MS) {
            sameRemaining = SFX_SAME_SOUND_DEBOUNCE_MS - (currentTime - lastSameSoundTime);
        }
    }
    
    return (sameRemaining > basicRemaining) ? sameRemaining : basicRemaining;
}

//==============================================================================
// UNIFIED API FUNCTIONS
//==============================================================================

/**
 * @brief Play sound effect asynchronously with smart debouncing
 * @param soundName String identifier for the sound effect
 * @param delayMS Delay before playing sound
 * @param debounceMS Custom debounce time
 * @param force If true, bypasses debouncing for critical sounds
 * 
 * Plays a sound effect asynchronously with smart debouncing to prevent
 * audio conflicts. Creates a separate task for sound playback to avoid
 * blocking the main thread. Supports custom delays and force override.
 */
void sfxPlay(const char* soundName, uint32_t delayMS, uint32_t debounceMS, bool force) {
    audio_state_t audioState = getAudioState();
    if (!soundName || (audioState != AUDIO_STATE_READY && audioState != AUDIO_STATE_PLAYING)) {
        return;
    }
    
    if (!force && !sfxCanPlaySound(soundName, debounceMS)) {
        return;
    }

    if (delayMS > 0) {
        sfxDelay(delayMS);
    }
    
    sfxUpdateSoundDebounce(soundName);
    
    async_sound_params_t* params = (async_sound_params_t*)malloc(sizeof(async_sound_params_t));
    if (!params) {
        ESP_LOGW(SFX_LOG, "Memory allocation failed for async sound: %s", soundName);
        return;
    }
    
    strncpy(params->soundName, soundName, sizeof(params->soundName) - 1);
    params->soundName[sizeof(params->soundName) - 1] = '\0';
    
    BaseType_t result = xTaskCreate(
        simpleAsyncSoundTask,
        "AsyncSFX",
        4096,
        params,
        2,
        NULL
    );
    
    if (result != pdPASS) {
        ESP_LOGW(SFX_LOG, "Task creation failed for sound: %s", soundName);
        free(params);
        return;
    }
}