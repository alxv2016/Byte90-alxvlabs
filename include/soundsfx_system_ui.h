/**
 * @file soundsfx_system_ui.h
 * @brief System and UI sound effects for retro computer audio
 */

#ifndef SOUNDSFX_SYSTEM_UI_H
#define SOUNDSFX_SYSTEM_UI_H

#include <stdint.h>
#include <stdbool.h>

//==============================================================================
// SYSTEM SOUND FUNCTIONS
//==============================================================================

/**
 * @brief Play classic computer startup beep sequence
 */
void sfxPlayStartup(void);

/**
 * @brief Play gentle notification beep
 */
void sfxPlayNotification(void);

/**
 * @brief Play warning beep pattern
 */
void sfxPlayWarning(void);

/**
 * @brief Play error beep sequence
 */
void sfxPlayError(void);

/**
 * @brief Play shutdown beep sequence
 */
void sfxPlayShutdown(void);

/**
 * @brief Play loading beep pattern
 */
void sfxPlayLoading(void);

/**
 * @brief Play system crash beep sequence
 */
void sfxPlayCrash(void);

/**
 * @brief Play alert beep pattern
 */
void sfxPlayAlert(void);

/**
 * @brief Play power-on self test (POST) beep sequence
 */
void sfxPlayPOST(void);

//==============================================================================
// UI INTERACTION SOUND FUNCTIONS
//==============================================================================

/**
 * @brief Play question beep
 */
void sfxPlayQuestion(void);

/**
 * @brief Play menu navigation beep
 */
void sfxPlayMenu(void);

/**
 * @brief Play confirmation beep
 */
void sfxPlayConfirm(void);

/**
 * @brief Play cancellation beep
 */
void sfxPlayCancel(void);

#endif /* SOUNDSFX_SYSTEM_UI_H */