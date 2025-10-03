/**
 * @file soundsfx_emotions.h
 * @brief Emotional and game sound effects for retro computer audio
 */

#ifndef SOUNDSFX_EMOTIONS_H
#define SOUNDSFX_EMOTIONS_H

#include <stdint.h>
#include <stdbool.h>

//==============================================================================
// EMOTIONAL SOUND FUNCTIONS
//==============================================================================

/**
 * @brief Play classic "whomp whooomp" sad beep
 */
void sfxPlaySad(void);

/**
 * @brief Play rejection beep sequence
 */
void sfxPlayRejected(void);

/**
 * @brief Play digital sigh beep
 */
void sfxPlaySigh(void);

/**
 * @brief Play celebration beep sequence
 */
void sfxPlayCelebration(void);

/**
 * @brief Play classic "ta-daaaaa" triumph sound
 */
void sfxPlayTaDaaaa(void);

/**
 * @brief Play Space Invaders UFO-style dizzy sound
 */
void sfxPlayDizzy(void);

//==============================================================================
// GAME SOUND FUNCTIONS
//==============================================================================

/**
 * @brief Play winner beep sequence
 */
void sfxPlayWinner(void);

/**
 * @brief Play game over beep sequence
 */
void sfxPlayGameOver(void);

#endif /* SOUNDSFX_EMOTIONS_H */