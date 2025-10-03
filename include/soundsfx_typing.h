/**
 * @file soundsfx_typing.h
 * @brief Typing and text sound effects for retro computer audio
 */

#ifndef SOUNDSFX_TYPING_H
#define SOUNDSFX_TYPING_H

#include <stdint.h>
#include <stdbool.h>

#define SFX_DEFAULT_TYPING_CHARS 10
#define SFX_DEFAULT_FAST_TYPING_CHARS 20
#define SFX_DEFAULT_TERMINAL_CHARS 15
#define SFX_DEFAULT_MATRIX_CHARS 12

#define SFX_TEST_DELAY_MS 1000
#define SFX_TYPING_MIN_DELAY_MS 40
#define SFX_TYPING_MAX_DELAY_MS 120

//==============================================================================
// TYPING SOUND FUNCTIONS
//==============================================================================

/**
 * @brief Play single keystroke beep
 */
void sfxPlayKeystroke(void);

/**
 * @brief Play typing sequence beep with optional fast mode
 * @param characterCount Number of characters to simulate
 * @param fastMode true for computer-speed, false for human-like timing
 */
void sfxPlayTyping(int characterCount, bool fastMode);

/**
 * @brief Play terminal typing beep sequence
 * @param characterCount Number of characters to simulate
 */
void sfxPlayTerminalTyping(int characterCount);

/**
 * @brief Play Matrix-style typing beep sequence
 * @param characterCount Number of characters to simulate
 */
void sfxPlayMatrixTyping(int characterCount);

/**
 * @brief Play typewriter bell
 */
void sfxPlayTypewriterBell(void);

/**
 * @brief Play backspace beep
 */
void sfxPlayBackspace(void);

/**
 * @brief Play text completion beep
 */
void sfxPlayTextComplete(void);

#endif /* SOUNDSFX_TYPING_H */