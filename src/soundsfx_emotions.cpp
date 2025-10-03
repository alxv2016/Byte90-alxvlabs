/**
 * @file soundsfx_emotions.cpp
 * @brief Implementation of emotional and game sound effects for retro computer audio
 *
 * Provides retro computer-style emotional and game sound effects including
 * sad sounds, celebrations, game outcomes, and character emotions for the
 * BYTE-90 device.
 * 
 * This module handles:
 * - Emotional sound effects (sad, rejected, sigh, celebration, triumph)
 * - Game-related sound effects (winner, game over)
 * - Character emotion audio feedback (dizzy, ta-daaaa)
 * - Audio state validation and volume control
 * - Retro gaming sound aesthetics and timing
 */

#include "soundsfx_emotions.h"
#include "soundsfx_core.h"
#include "speaker_module.h"
#include <stdlib.h>

//==============================================================================
// EMOTIONAL SOUND FUNCTIONS
//==============================================================================

/**
 * @brief Play classic "whomp whooomp" sad beep
 * 
 * Plays a descending four-note sequence (440Hz to 370Hz) with decreasing
 * volume to create a classic sad "whomp whooomp" sound effect. Each note
 * has increasing duration for a mournful effect. Followed by a wobbling
 * low-frequency sequence for added emotional impact.
 */
void sfxPlaySad(void) {
    audio_state_t audioState = getAudioState();
    if (audioState != AUDIO_STATE_READY && audioState != AUDIO_STATE_PLAYING) return;
    
    const uint16_t deathSequence[] = {
        440, 415, 392, 370
    };
    
    const int numNotes = sizeof(deathSequence) / sizeof(deathSequence[0]);
    
    for (int i = 0; i < numNotes; i++) {
        uint16_t duration = 80 + (i * 15);
        uint8_t volume = (uint8_t)(SFX_BASE_VOLUME * 0.9 * (numNotes - i) / numNotes);
        
        sfxPlayTone(deathSequence[i], duration, volume);
        
        if (i < numNotes - 1) {
            sfxDelay(20 + (i * 5));
        }
    }
    
    sfxDelay(150);
    for (int i = 0; i < 8; i++) {
        uint16_t wobbleFreq = 220 + (i % 2 == 0 ? 10 : -10);
        uint8_t wobbleVolume = (uint8_t)(SFX_BASE_VOLUME * 0.4 * (8 - i) / 8);
        sfxPlayTone(wobbleFreq, 100, wobbleVolume);
    }
}

/**
 * @brief Play rejection beep sequence
 * 
 * Plays a complex rejection sequence starting with three cycles of alternating
 * low frequencies (200Hz/180Hz), followed by a descending sequence from 1500Hz
 * to 150Hz with decreasing volume to indicate rejection or failure.
 */
void sfxPlayRejected(void) {
    audio_state_t audioState = getAudioState();
    if (audioState != AUDIO_STATE_READY && audioState != AUDIO_STATE_PLAYING) return;
    
    for (int i = 0; i < 3; i++) {
        sfxPlayTone(200, 80, SFX_BASE_VOLUME);
        sfxDelay(20);
        sfxPlayTone(180, 80, SFX_BASE_VOLUME);
        sfxDelay(20);
    }
    
    sfxDelay(100);
    
    sfxPlayTone(1500, 60, SFX_BASE_VOLUME * 0.9);
    sfxDelay(40);
    sfxPlayTone(1200, 80, SFX_BASE_VOLUME * 0.9);
    sfxDelay(40);
    sfxPlayTone(800, 120, SFX_BASE_VOLUME * 0.8);
    sfxDelay(40);
    sfxPlayTone(400, 200, SFX_BASE_VOLUME * 0.7);
    
    sfxDelay(150);
    
    sfxPlayTone(150, 300, SFX_BASE_VOLUME * 0.8);
}

/**
 * @brief Play digital sigh beep
 * 
 * Plays a frequency slide from 500Hz to 250Hz with decreasing volume
 * to simulate a digital sigh or disappointment sound effect.
 */
void sfxPlaySigh(void) {
    audio_state_t audioState = getAudioState();
    if (audioState != AUDIO_STATE_READY && audioState != AUDIO_STATE_PLAYING) return;
    
    sfxGenerateFrequencySlide(500, 250, 12, 60, SFX_BASE_VOLUME * 0.8, SFX_BASE_VOLUME * 0.4);
}

/**
 * @brief Play celebration beep sequence
 * 
 * Plays a four-tone ascending celebration sequence starting at 1000Hz
 * to indicate success, achievement, or positive events.
 */
void sfxPlayCelebration(void) {
    audio_state_t audioState = getAudioState();
    if (audioState != AUDIO_STATE_READY && audioState != AUDIO_STATE_PLAYING) return;
    
    sfxGenerateAscendingSequence(1000, 4, 200, 150, SFX_BASE_VOLUME * 0.8);
}

/**
 * @brief Play Space Invaders UFO-style dizzy sound
 * 
 * Plays a Space Invaders UFO-style dizzy sound with alternating frequencies
 * (400Hz/500Hz) and volume curves, followed by a fading ascending sequence
 * to simulate disorientation or confusion effects.
 */
void sfxPlayDizzy(void) {
    audio_state_t audioState = getAudioState();
    if (audioState != AUDIO_STATE_READY && audioState != AUDIO_STATE_PLAYING) return;
    
    const uint16_t freq1 = 400;
    const uint16_t freq2 = 500;
    const int cycleCount = 2;
    
    for (int i = 0; i < cycleCount; i++) {
        int duration = 120 - (i * 4);
        if (duration < 40) duration = 40;
        
        float volumeCurve;
        if (i < cycleCount / 2) {
            volumeCurve = 0.3 + (0.7 * i / (cycleCount / 2));
        } else {
            volumeCurve = 1.0 - (0.7 * (i - cycleCount / 2) / (cycleCount / 2));
        }
        uint8_t volume = (uint8_t)(SFX_BASE_VOLUME * volumeCurve);
        
        sfxPlayTone(freq1, duration, volume);
        sfxPlayTone(freq2, duration, volume);
    }
    
    for (int i = 0; i < 8; i++) {
        uint16_t fadeFreq = 600 + (i * 50);
        uint8_t fadeVolume = (uint8_t)(SFX_BASE_VOLUME * 0.4 * (8 - i) / 8);
        sfxPlayTone(fadeFreq, 30, fadeVolume);
    }
}

/**
 * @brief Play classic "ta-daaaaa" triumph sound
 * 
 * Plays a classic "ta-daaaaa" triumph sound starting with two introductory
 * notes (659Hz/784Hz), followed by a four-note fanfare sequence (523Hz to 1047Hz)
 * with increasing volume and echo effects for the final notes.
 */
void sfxPlayTaDaaaa(void) {
    audio_state_t audioState = getAudioState();
    if (audioState != AUDIO_STATE_READY && audioState != AUDIO_STATE_PLAYING) return;
    
    sfxPlayTone(659, 200, SFX_BASE_VOLUME);
    sfxDelay(50);
    sfxPlayTone(784, 150, SFX_BASE_VOLUME);
    
    sfxDelay(150);
    
    const uint16_t fanfare[] = {523, 659, 784, 1047};
    const uint16_t durations[] = {120, 120, 150, 200};
    const float volumes[] = {0.7, 0.8, 0.85, 0.9};
    
    for (int i = 0; i < 4; i++) {
        uint8_t volume = (uint8_t)(SFX_BASE_VOLUME * volumes[i]);
        sfxPlayTone(fanfare[i], durations[i], volume);
        
        if (i >= 2) {
            sfxDelay(30);
            sfxPlayTone(fanfare[i], 80, volume * 0.6);
        }
        
        if (i < 4) sfxDelay(40);
    }
}

//==============================================================================
// GAME SOUND FUNCTIONS
//==============================================================================

/**
 * @brief Play winner beep sequence
 * 
 * Plays a winner sequence starting with an introductory tone (1000Hz),
 * followed by a three-tone ascending sequence (1200Hz+) and ending with
 * a high-frequency victory tone (2000Hz) to indicate game victory.
 */
void sfxPlayWinner(void) {
    audio_state_t audioState = getAudioState();
    if (audioState != AUDIO_STATE_READY && audioState != AUDIO_STATE_PLAYING) return;
    
    sfxPlayTone(1000, 150, SFX_BASE_VOLUME * 0.8);
    sfxDelay(100);
    
    sfxGenerateAscendingSequence(1200, 3, 200, 200, SFX_BASE_VOLUME);
    sfxDelay(150);
    
    sfxPlayTone(2000, 400, SFX_BASE_VOLUME);
}

/**
 * @brief Play game over beep sequence
 * 
 * Plays a game over sequence with four descending tones (800Hz to 300Hz)
 * with increasing durations and decreasing volumes to indicate game failure
 * or end of game.
 */
void sfxPlayGameOver(void) {
    audio_state_t audioState = getAudioState();
    if (audioState != AUDIO_STATE_READY && audioState != AUDIO_STATE_PLAYING) return;
    
    const uint16_t frequencies[] = {800, 600, 400, 300};
    const uint16_t durations[] = {300, 300, 400, 600};
    const uint8_t volumes[] = {
        (uint8_t)(SFX_BASE_VOLUME * 0.9), 
        (uint8_t)(SFX_BASE_VOLUME * 0.8), 
        (uint8_t)(SFX_BASE_VOLUME * 0.7), 
        (uint8_t)(SFX_BASE_VOLUME * 0.6)
    };
    
    for (int i = 0; i < 4; i++) {
        sfxPlayTone(frequencies[i], durations[i], volumes[i]);
        if (i < 3) sfxDelay(100);
        else sfxDelay(200);
    }
}