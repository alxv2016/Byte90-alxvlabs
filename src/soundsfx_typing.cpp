/**
 * @file soundsfx_typing.cpp
 * @brief Implementation of typing and text sound effects for retro computer audio
 *
 * Provides retro computer-style typing sound effects including keystrokes,
 * typing sequences, terminal sounds, Matrix-style effects, and typewriter
 * audio feedback for the BYTE-90 device.
 * 
 * This module handles:
 * - Individual keystroke sound effects
 * - Typing sequence simulations with variable timing
 * - Terminal and Matrix-style typing effects
 * - Typewriter bell and backspace sounds
 * - Text completion audio feedback
 * - Audio state validation and volume control
 */

#include "soundsfx_typing.h"
#include "soundsfx_core.h"
#include "speaker_module.h"
#include <stdlib.h>

//==============================================================================
// TYPING SOUND FUNCTIONS
//==============================================================================

/**
 * @brief Play single keystroke beep
 * 
 * Plays a short, high-pitched beep sound simulating a single keystroke.
 * Validates audio state before playing and uses reduced volume for subtlety.
 */
void sfxPlayKeystroke(void) {
    audio_state_t audioState = getAudioState();
    if (audioState != AUDIO_STATE_READY && audioState != AUDIO_STATE_PLAYING) return;
    
    sfxPlayTone(1500, 25, SFX_BASE_VOLUME * 0.3);
}

/**
 * @brief Play typing sequence beep with optional fast mode
 * @param characterCount Number of characters to simulate
 * @param fastMode true for computer-speed, false for human-like timing
 * 
 * Plays a sequence of keystroke sounds with variable timing and frequency.
 * Fast mode uses shorter delays and higher frequencies for computer-like typing.
 * Human mode uses longer delays and varied frequencies for realistic typing.
 */
void sfxPlayTyping(int characterCount, bool fastMode) {
    audio_state_t audioState = getAudioState();
    if (audioState != AUDIO_STATE_READY && audioState != AUDIO_STATE_PLAYING) return;
    
    for (int i = 0; i < characterCount; i++) {
        uint16_t freq;
        uint8_t volume;
        int delay_ms;
        
        if (fastMode) {
            freq = 1600 + (rand() % 100);
            volume = SFX_BASE_VOLUME * 0.2;
            delay_ms = 20 + (rand() % 20);
        } else {
            freq = 1400 + (rand() % 200);
            volume = (SFX_BASE_VOLUME * 0.25) + (rand() % (SFX_BASE_VOLUME / 10));
            delay_ms = SFX_TYPING_MIN_DELAY_MS + (rand() % (SFX_TYPING_MAX_DELAY_MS - SFX_TYPING_MIN_DELAY_MS));
        }
        
        sfxPlayTone(freq, fastMode ? 15 : 20, volume);
        sfxDelay(delay_ms);
    }
}

/**
 * @brief Play terminal typing beep sequence
 * @param characterCount Number of characters to simulate
 * 
 * Plays a sequence of terminal-style typing sounds with consistent timing
 * and frequency, simulating old computer terminal keystrokes.
 */
void sfxPlayTerminalTyping(int characterCount) {
    audio_state_t audioState = getAudioState();
    if (audioState != AUDIO_STATE_READY && audioState != AUDIO_STATE_PLAYING) return;
    
    for (int i = 0; i < characterCount; i++) {
        sfxPlayTone(1200, 30, SFX_BASE_VOLUME * 0.25);
        sfxDelay(80);
    }
}

/**
 * @brief Play Matrix-style typing beep sequence
 * @param characterCount Number of characters to simulate
 * 
 * Plays a sequence of Matrix-style typing sounds with rapid, staccato timing
 * and varied frequencies, simulating the iconic Matrix digital rain effect.
 */
void sfxPlayMatrixTyping(int characterCount) {
    audio_state_t audioState = getAudioState();
    if (audioState != AUDIO_STATE_READY && audioState != AUDIO_STATE_PLAYING) return;
    
    for (int i = 0; i < characterCount; i++) {
        uint16_t freq = 1800 - (600 * i / characterCount);
        sfxPlayTone(freq, 25, SFX_BASE_VOLUME * 0.3);
        
        int delay_ms = 100 - (i * 5);
        if (delay_ms < 30) delay_ms = 30;
        sfxDelay(delay_ms);
    }
}

/**
 * @brief Play typewriter bell
 * 
 * Plays a typewriter bell sound effect, typically used to indicate
 * end of line or completion of typing. Uses a two-tone sequence with
 * decreasing volume for a classic typewriter bell sound.
 */
void sfxPlayTypewriterBell(void) {
    audio_state_t audioState = getAudioState();
    if (audioState != AUDIO_STATE_READY && audioState != AUDIO_STATE_PLAYING) return;
    
    sfxPlayTone(2000, 200, SFX_BASE_VOLUME * 0.6);
    sfxDelay(50);
    sfxPlayTone(2000, 100, SFX_BASE_VOLUME * 0.4);
}

/**
 * @brief Play backspace beep
 * 
 * Plays a backspace sound effect with a lower frequency and short duration,
 * simulating the sound of deleting or correcting text input.
 */
void sfxPlayBackspace(void) {
    audio_state_t audioState = getAudioState();
    if (audioState != AUDIO_STATE_READY && audioState != AUDIO_STATE_PLAYING) return;
    
    sfxPlayTone(800, 40, SFX_BASE_VOLUME * 0.4);
}

/**
 * @brief Play text completion beep
 * 
 * Plays a two-tone ascending sequence to indicate completion of text
 * input or successful text processing. Uses increasing frequencies and
 * volume for a satisfying completion sound.
 */
void sfxPlayTextComplete(void) {
    audio_state_t audioState = getAudioState();
    if (audioState != AUDIO_STATE_READY && audioState != AUDIO_STATE_PLAYING) return;
    
    sfxPlayTone(1000, 100, SFX_BASE_VOLUME * 0.5);
    sfxDelay(50);
    sfxPlayTone(1300, 150, SFX_BASE_VOLUME * 0.6);
}