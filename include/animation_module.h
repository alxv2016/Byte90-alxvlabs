/**
 * @file animation_module.h
 * @brief Header for animation playback and management
 *
 * Provides functions for managing and playing GIF animations, handling animation
 * sequences, and coordinating interactions between animations and device systems.
 */

#ifndef ANIMATION_MODULE_H
#define ANIMATION_MODULE_H

#include "common.h"
#include "gif_module.h"

//==============================================================================
// CONSTANTS & DEFINITIONS
//==============================================================================

static const char* ANIM_LOG = "::ANIMATION_MODULE::";

//==============================================================================
// TYPE DEFINITIONS
//==============================================================================

enum class SequenceState {
  REST_START,
  ANIMATION_CYCLE,
  REST_END
};

enum class SleepState {
  NONE,
  ENTERING_SLEEP,
  SLEEPING,
  EXITING_SLEEP
};

enum class CrashState {
  NONE,
  ENTERING_CRASH,
  CRASHED,
  RECOVERING
};

struct AnimationSequence {
  SequenceState currentState = SequenceState::REST_START;
  unsigned long stateStartTime = 0;
  bool isIdleMode = true;
  const unsigned long STATE_DELAY = 3000;
  const unsigned long IDLE_DELAY = 20000;
};

//==============================================================================
// PUBLIC API FUNCTIONS
//==============================================================================

/**
 * @brief Initialize the animation module
 */
void initializeAnimationModule(void);

/**
 * @brief Play a GIF animation with interaction detection
 * @param filename Path to the GIF file to play
 * @return true if playback completed successfully
 */
bool playGIF(const char* filename);

/**
 * @brief Main function to handle emote playback
 */
void playEmotes(void);

/**
 * @brief Play the boot animation
 */
void playBootAnimation(void);

#endif /* ANIMATION_MODULE_H */