/**
 * @file soundsfx_system_ui.cpp
 * @brief Implementation of system and UI sound effects for retro computer audio
 *
 * Provides retro computer-style system and user interface sound effects including
 * startup sequences, notifications, warnings, errors, and UI interaction sounds
 * for the BYTE-90 device.
 * 
 * This module handles:
 * - System startup and shutdown sound sequences
 * - Notification and alert sound patterns
 * - Error and warning audio feedback
 * - UI interaction sounds (menu, confirm, cancel)
 * - System diagnostic sounds (POST, crash, loading)
 * - Audio state validation and volume control
 */

#include "soundsfx_system_ui.h"
#include "soundsfx_core.h"
#include "speaker_module.h"

//==============================================================================
// SYSTEM SOUND FUNCTIONS
//==============================================================================

/**
 * @brief Play classic computer startup beep sequence
 * 
 * Plays a four-tone ascending startup sequence with increasing frequency
 * and volume, simulating classic computer boot sounds. Uses frequencies
 * from 600Hz to 1200Hz with progressive volume increase.
 */
void sfxPlayStartup(void) {
    audio_state_t audioState = getAudioState();
    if (audioState != AUDIO_STATE_READY && audioState != AUDIO_STATE_PLAYING) return;
    
    const uint16_t frequencies[] = {600, 800, 1000, 1200};
    const uint8_t volumes[] = {
        (uint8_t)(SFX_BASE_VOLUME * 0.7), 
        (uint8_t)(SFX_BASE_VOLUME * 0.8), 
        (uint8_t)(SFX_BASE_VOLUME * 0.9), 
        SFX_BASE_VOLUME
    };
    
    for (int i = 0; i < 4; i++) {
        sfxPlayTone(frequencies[i], 120, volumes[i]);
        if (i < 3) sfxDelay(80);
    }
}

/**
 * @brief Play gentle notification beep
 * 
 * Plays a gentle two-tone notification sound for general alerts and
 * system messages. Uses ascending frequencies (1000Hz to 1200Hz) with
 * increasing volume for a pleasant notification effect.
 */
void sfxPlayNotification(void) {
    audio_state_t audioState = getAudioState();
    if (audioState != AUDIO_STATE_READY && audioState != AUDIO_STATE_PLAYING) return;
    
    sfxPlayTone(1000, 150, SFX_BASE_VOLUME * 0.6);
    sfxDelay(50);
    sfxPlayTone(1200, 200, SFX_BASE_VOLUME * 0.7);
}

/**
 * @brief Play warning beep pattern
 * 
 * Plays a two-cycle warning pattern with alternating frequencies (800Hz/1000Hz)
 * to indicate caution or important system warnings. Uses longer delays between cycles.
 */
void sfxPlayWarning(void) {
    audio_state_t audioState = getAudioState();
    if (audioState != AUDIO_STATE_READY && audioState != AUDIO_STATE_PLAYING) return;
    
    for (int i = 0; i < 2; i++) {
        sfxPlayTone(800, 100, SFX_BASE_VOLUME * 0.8);
        sfxDelay(80);
        sfxPlayTone(1000, 100, SFX_BASE_VOLUME * 0.8);
        sfxDelay(300);
    }
}

/**
 * @brief Play error beep sequence
 * 
 * Plays a four-tone descending error sequence with decreasing frequencies
 * (1000Hz to 400Hz) and increasing durations to indicate system errors or failures.
 * Uses a pause after the third tone for emphasis.
 */
void sfxPlayError(void) {
    audio_state_t audioState = getAudioState();
    if (audioState != AUDIO_STATE_READY && audioState != AUDIO_STATE_PLAYING) return;
    
    const uint16_t frequencies[] = {1000, 800, 600, 400};
    const uint16_t durations[] = {200, 200, 300, 400};
    const uint8_t volumes[] = {
        SFX_BASE_VOLUME, 
        SFX_BASE_VOLUME, 
        SFX_BASE_VOLUME, 
        (uint8_t)(SFX_BASE_VOLUME * 0.9)
    };
    
    for (int i = 0; i < 4; i++) {
        sfxPlayTone(frequencies[i], durations[i], volumes[i]);
        if (i == 2) sfxDelay(100);
    }
}

/**
 * @brief Play shutdown beep sequence
 * 
 * Plays a frequency slide from 1200Hz to 600Hz with decreasing volume
 * to indicate system shutdown or power-off. Uses smooth frequency transition.
 */
void sfxPlayShutdown(void) {
    audio_state_t audioState = getAudioState();
    if (audioState != AUDIO_STATE_READY && audioState != AUDIO_STATE_PLAYING) return;
    
    sfxGenerateFrequencySlide(1200, 600, 8, 75, SFX_BASE_VOLUME * 0.8, SFX_BASE_VOLUME * 0.5);
}

/**
 * @brief Play loading beep pattern
 * 
 * Plays a three-tone ascending sequence starting at 900Hz to indicate
 * loading or processing operations. Uses moderate volume for subtle feedback.
 */
void sfxPlayLoading(void) {
    audio_state_t audioState = getAudioState();
    if (audioState != AUDIO_STATE_READY && audioState != AUDIO_STATE_PLAYING) return;
    
    sfxGenerateAscendingSequence(900, 3, 100, 120, SFX_BASE_VOLUME * 0.6);
}

/**
 * @brief Play system crash beep sequence
 * 
 * Plays a chaotic crash sequence with random frequencies and durations,
 * followed by a descending sequence ending with a low-frequency tone
 * to indicate system crash or critical failure.
 */
void sfxPlayCrash(void) {
    audio_state_t audioState = getAudioState();
    if (audioState != AUDIO_STATE_READY && audioState != AUDIO_STATE_PLAYING) return;
    
    const uint16_t chaosFreqs[] = {1200, 800, 1500, 600};
    const uint16_t chaosDurations[] = {100, 80, 60, 120};
    
    for (int i = 0; i < 4; i++) {
        sfxPlayTone(chaosFreqs[i], chaosDurations[i], SFX_BASE_VOLUME);
    }
    
    sfxDelay(100);
    
    sfxGenerateAscendingSequence(400, 5, 50, 80, SFX_BASE_VOLUME * 0.8);
    sfxPlayTone(200, 500, SFX_BASE_VOLUME * 0.6);
}

/**
 * @brief Play alert beep pattern
 * 
 * Plays a four-cycle alert pattern with double beeps at 1400Hz to indicate
 * urgent alerts or important system notifications. Uses consistent timing.
 */
void sfxPlayAlert(void) {
    audio_state_t audioState = getAudioState();
    if (audioState != AUDIO_STATE_READY && audioState != AUDIO_STATE_PLAYING) return;
    
    for (int i = 0; i < 4; i++) {
        sfxPlayTone(1400, 100, SFX_BASE_VOLUME);
        sfxDelay(100);
        sfxPlayTone(1400, 100, SFX_BASE_VOLUME);
        sfxDelay(200);
    }
}

/**
 * @brief Play power-on self test beep sequence
 * 
 * Plays a POST sequence starting with a high-frequency tone (1500Hz) followed
 * by a three-tone ascending sequence to indicate successful system initialization
 * and hardware testing completion.
 */
void sfxPlayPOST(void) {
    audio_state_t audioState = getAudioState();
    if (audioState != AUDIO_STATE_READY && audioState != AUDIO_STATE_PLAYING) return;
    
    sfxPlayTone(1500, 200, SFX_BASE_VOLUME);
    sfxDelay(500);
    
    sfxGenerateAscendingSequence(800, 3, 200, 120, SFX_BASE_VOLUME * 0.7);
}

//==============================================================================
// UI INTERACTION SOUND FUNCTIONS
//==============================================================================

/**
 * @brief Play question beep
 * 
 * Plays a three-tone ascending sequence starting at 800Hz to indicate
 * questions or prompts requiring user input. Uses moderate volume and timing.
 */
void sfxPlayQuestion(void) {
    audio_state_t audioState = getAudioState();
    if (audioState != AUDIO_STATE_READY && audioState != AUDIO_STATE_PLAYING) return;
    
    sfxGenerateAscendingSequence(800, 3, 200, 150, SFX_BASE_VOLUME * 0.7);
}

/**
 * @brief Play menu navigation beep
 * 
 * Plays a short, high-pitched beep at 1200Hz with low volume for subtle
 * menu navigation feedback. Designed to be unobtrusive during menu browsing.
 */
void sfxPlayMenu(void) {
    audio_state_t audioState = getAudioState();
    if (audioState != AUDIO_STATE_READY && audioState != AUDIO_STATE_PLAYING) return;
    
    sfxPlayTone(1200, 50, SFX_BASE_VOLUME * 0.4);
}

/**
 * @brief Play confirmation beep
 * 
 * Plays a two-tone ascending confirmation sequence (1000Hz to 1300Hz) with
 * increasing volume to indicate successful confirmation or positive action.
 */
void sfxPlayConfirm(void) {
    audio_state_t audioState = getAudioState();
    if (audioState != AUDIO_STATE_READY && audioState != AUDIO_STATE_PLAYING) return;
    
    sfxPlayTone(1000, 80, SFX_BASE_VOLUME * 0.6);
    sfxDelay(50);
    sfxPlayTone(1300, 120, SFX_BASE_VOLUME * 0.7);
}

/**
 * @brief Play cancellation beep
 * 
 * Plays a two-tone descending cancellation sequence (1000Hz to 700Hz) with
 * decreasing volume to indicate cancellation or negative action.
 */
void sfxPlayCancel(void) {
    audio_state_t audioState = getAudioState();
    if (audioState != AUDIO_STATE_READY && audioState != AUDIO_STATE_PLAYING) return;
    
    sfxPlayTone(1000, 100, SFX_BASE_VOLUME * 0.6);
    sfxPlayTone(700, 150, SFX_BASE_VOLUME * 0.5);
}