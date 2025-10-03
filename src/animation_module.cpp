/**
 * @file animation_module.cpp
 * @brief Implementation of animation playback and management
 *
 * Handles animation sequences, responds to device states like orientation changes
 * or sleep modes, and coordinates interactions between animations and systems.
 */

#include "animation_module.h"
#include "display_module.h"
#include "effects_core.h"
#include "emotes_module.h"
#include "espnow_module.h"
#include "gif_module.h"
#include "haptics_module.h"
#include "haptics_effects.h"
#include "menu_module.h"
#include "motion_module.h"
#include "soundsfx_module.h"
#include "states_module.h"

//==============================================================================
// GLOBAL VARIABLES
//==============================================================================

static AnimationSequence animSequence;
const char **currentEmotes;
size_t emoteCount;

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))

static CrashState currentCrashState = CrashState::NONE;
static bool wasCrashed = false;

static SleepState currentSleepState = SleepState::NONE;
static bool wasAsleep = false;

static unsigned long lastCheckComs = 0;
const unsigned long COMS_CHECK_INTERVAL = 20000;
static unsigned long lastInteractionCheck = 0;
const unsigned long INTERACTION_CHECK_DEBOUNCE = 10;

#if DEVICE_MODE == MAC_MODE | DEVICE_MODE == PC_MODE
const char *randomEmotes[] = {
    ZONED_EMOTE, DOUBTFUL_EMOTE, TALK_EMOTE, SCAN_EMOTE, ANGRY_EMOTE,
    CRY_EMOTE, PIXEL_EMOTE, GLEE_EMOTE, EXCITED_EMOTE, HEARTS_EMOTE, UWU_EMOTE,
};

const char *restingEmotes[] = {REST_EMOTE, IDLE_EMOTE, LOOK_DOWN_EMOTE,
                               LOOK_UP_EMOTE, LOOK_LEFT_RIGHT_EMOTE};
#else
const char *randomEmotes[] = {
    WINK_02_EMOTE, ZONED_EMOTE, DOUBTFUL_EMOTE, TALK_EMOTE, SCAN_EMOTE,
    ANGRY_EMOTE, CRY_EMOTE, PIXEL_EMOTE, EXCITED_EMOTE, HEARTS_EMOTE,
    UWU_EMOTE, WHISTLE_EMOTE, GLEE_EMOTE, MISCHIEF_EMOTE, HUMSUP_EMOTE,
};

const char *restingEmotes[] = {
    REST_EMOTE,
    IDLE_EMOTE,
    LOOK_DOWN_EMOTE,
    LOOK_UP_EMOTE,
    LOOK_LEFT_RIGHT_EMOTE,
};
#endif

//==============================================================================
// UTILITY FUNCTIONS (STATIC)
//==============================================================================

/**
 * @brief Play a random emote from a collection
 * @param emotes Array of emote file paths
 * @param count Number of emotes in the array
 */
void randomizeEmotes(const char **emotes, size_t count) {
  if (!emotes || count == 0) {
    return;
  }

  static uint8_t *unplayedEmotes = NULL;
  static size_t arraySize = 0;
  static size_t remainingCount = 0;

  if (arraySize != count) {
    if (unplayedEmotes)
      free(unplayedEmotes);
    unplayedEmotes = (uint8_t *)malloc(count * sizeof(uint8_t));
    arraySize = count;
    remainingCount = 0;
  }

  if (remainingCount == 0) {
    for (size_t i = 0; i < count; i++) {
      unplayedEmotes[i] = i;
    }
    remainingCount = count;
  }

  size_t randomPos = random(remainingCount);
  uint8_t selectedIndex = unplayedEmotes[randomPos];
  unplayedEmotes[randomPos] = unplayedEmotes[remainingCount - 1];
  remainingCount--;
  playGIF(emotes[selectedIndex]);
}

/**
 * @brief Handle crash animations based on device orientation
 * @return true if crash animation was played
 */
bool checkCrashOrientation() {
  if (motionTiltedLeft() || motionTiltedRight() || motionUpsideDown()) {
    if (currentCrashState == CrashState::NONE) {
      currentCrashState = CrashState::ENTERING_CRASH;
      playGIF(CRASH01_EMOTE);
      currentCrashState = CrashState::CRASHED;
      wasCrashed = true;
      return true;
    }
    else if (currentCrashState == CrashState::CRASHED) {
      playGIF(CRASH02_EMOTE);
      return true;
    }
  }
  else if (wasCrashed) {
    currentCrashState = CrashState::RECOVERING;
    playGIF(CRASH03_EMOTE);
    currentCrashState = CrashState::NONE;
    wasCrashed = false;
    return true;
  }

  return false;
}

/**
 * @brief Handle sleep animations based on device state
 * @return true if sleep animation was played
 */
bool handleSleepSequence() {
  if (motionSleep()) {
    if (currentSleepState == SleepState::NONE) {
      currentSleepState = SleepState::ENTERING_SLEEP;
      playGIF(SLEEP01_EMOTE);
      currentSleepState = SleepState::SLEEPING;
      wasAsleep = true;
      return true;
    }
    else if (currentSleepState == SleepState::SLEEPING) {
      playGIF(SLEEP02_EMOTE);
      return true;
    }
  }
  else if (wasAsleep) {
    currentSleepState = SleepState::EXITING_SLEEP;
    sfxPlay("question", 20);
    if (areHapticsActive()) {
      playHapticEffect(HAPTIC_ALERT_750MS);
    }
    playGIF(SLEEP03_EMOTE);
    currentSleepState = SleepState::NONE;
    wasAsleep = false;
    return true;
  }

  return false;
}

/**
 * @brief Handle special device states and related animations
 * @return true if a special state was handled and animation played
 */
bool handleSpecialStates() {
  static unsigned long suddenAccelLockout = 0;
  const unsigned long SUDDEN_ACCEL_LOCKOUT_PERIOD = 800;

  if (espNowStateChangedState()) {
    resetEspNowStateChanged();
    if (getCurrentESPNowState() == ESPNowState::ON) {
      playGIF(COMS_CONNECT_EMOTE);
    } else {
      sfxPlay("shutdown", 20, true);
      if (areHapticsActive()) {
        playHapticEffect(HAPTIC_ALERT_750MS);
      }
      playGIF(COMS_DISCONNECT_EMOTE);
    }
    return true;
  }

  if (motionDeepSleep()) {
    setMotionState(MotionStateType::DEEP_SLEEP, false);
    stopGifPlayback();
    return true;
  }

  if (motionInteracted()) {
    if (motionSuddenAcceleration()) {
      setMotionState(MotionStateType::SUDDEN_ACCELERATION, false);
      suddenAccelLockout = millis(); 
      sfxPlay("question");
      playGIF(STARTLED_EMOTE);
      return true;
    }

    if (motionShaking() && millis() - suddenAccelLockout > SUDDEN_ACCEL_LOCKOUT_PERIOD) {
      setMotionState(MotionStateType::SHAKING, false);
      sfxPlay("dizzy", 10);
      playGIF(DIZZY_EMOTE);
      return true;
    }

    if (motionDoubleTapped()) {
      setMotionState(MotionStateType::DOUBLE_TAPPED, false);
      sfxPlay("question", 20);
      playGIF(SHOCK_EMOTE);
      return true;
    }

    if (motionTapped()) {
      setMotionState(MotionStateType::TAPPED, false);
      sfxPlay("rejected");
      playGIF(TAP_EMOTE);
      return true;
    }
  }

  if ((motionHalfTiltedLeft() || motionHalfTiltedRight()) &&
      !motionTiltedLeft() && !motionTiltedRight() && !motionUpsideDown()) {
        sfxPlay("alert");
        playGIF(SHOCK_EMOTE);
    return true;
  }

  if (motionOriented() || wasCrashed) {
    if (checkCrashOrientation()) {
      sfxPlay("crash", 60, 6000);
      return true;
    }
  }

  if (motionSleep() || wasAsleep) {
    if (handleSleepSequence()) {
      return true;
    }
  }

  return false;
}

/**
 * @brief Handle normal animation sequence when no special states are active
 * @param currentTime Current system time in milliseconds
 */
void handleAnimationSequence(unsigned long currentTime) {
  switch (animSequence.currentState) {
  case SequenceState::REST_START:
    playGIF(WINK_EMOTE);
    animSequence.stateStartTime = currentTime;
    animSequence.currentState = SequenceState::ANIMATION_CYCLE;
    break;

  case SequenceState::ANIMATION_CYCLE:
    if (currentTime - animSequence.stateStartTime >= animSequence.STATE_DELAY) {
      if (animSequence.isIdleMode) {
        currentEmotes = restingEmotes;
        emoteCount = ARRAY_SIZE(restingEmotes);
      } else {
        currentEmotes = randomEmotes;
        emoteCount = ARRAY_SIZE(randomEmotes);
      }

      randomizeEmotes(currentEmotes, emoteCount);
      animSequence.isIdleMode = !animSequence.isIdleMode;

      if (animSequence.isIdleMode) {
        animSequence.currentState = SequenceState::REST_END;
        animSequence.stateStartTime = currentTime;
      }
    }
    break;

  case SequenceState::REST_END:
    playGIF(BLINK_EMOTE);
    if (currentTime - animSequence.stateStartTime >= animSequence.IDLE_DELAY) {
      animSequence.currentState = SequenceState::REST_START;
      animSequence.stateStartTime = currentTime;
    }
    break;
  }
}

//==============================================================================
// PUBLIC API FUNCTIONS
//==============================================================================

/**
 * @brief Play a GIF animation with interaction detection
 * @param filename Path to the GIF file to play
 * @return true if playback completed successfully
 */
bool playGIF(const char *filename) {
  const unsigned long TIMEOUT_MS = 10000;
  unsigned long startTime = millis();
  unsigned long frameTime = micros();
  unsigned long lastCheck = 0;
  const unsigned long INTERACTION_CHECK_DEBOUNCE = 10;

  if (!loadGIF(filename)) {
    return false;
  }

  while (playGIFFrame(false, NULL)) {
    // Update WiFi status indicator on each frame
    updateWiFiStatusIndicator();
    
    unsigned long currentTime = micros();
    unsigned long elapsed = currentTime - frameTime;
    if (elapsed < FRAME_DELAY_MICROSECONDS) {
      delayMicroseconds(FRAME_DELAY_MICROSECONDS - elapsed);
    }
    frameTime = micros();

    currentTime = millis();
    if (currentTime - lastCheck >= INTERACTION_CHECK_DEBOUNCE) {
      ADXLDataPolling();

      if (menu_isActive()) {
        break;
      }

      if (getCurrentState() == SystemState::UPDATE_MODE) {
        break;
      }

      if (espNowStateChangedState()) {
        break;
      }

      if (motionInteracted()) {
        if (motionDoubleTapped()) {
          break;
        }
        if (motionTapped()) {
          break;
        }
        if (motionShaking()) {
          break;
        }
        if (motionSuddenAcceleration()) {
          break;
        }
      }

      if ((motionTiltedLeft() || motionTiltedRight() || motionUpsideDown()) && strcmp(filename, CRASH01_EMOTE) != 0 &&
          strcmp(filename, CRASH02_EMOTE) != 0 && strcmp(filename, SHOCK_EMOTE) != 0) {
        break;
      }

      lastCheck = currentTime;
    }

    if (currentTime - startTime > TIMEOUT_MS) {
      break;
    }
  }

  stopGifPlayback();
  return true;
}

/**
 * @brief Initialize the animation module
 */
void initializeAnimationModule() {
  currentCrashState = CrashState::NONE;
  wasCrashed = false;
  currentSleepState = SleepState::NONE;
  wasAsleep = false;
  lastCheckComs = 0;

  animSequence.currentState = SequenceState::REST_START;
  animSequence.stateStartTime = 0;
  animSequence.isIdleMode = true;
}

/**
 * @brief Main function to handle emote playback
 */
void playEmotes() {
  if (getCurrentState() == SystemState::UPDATE_MODE) {
    return;
  }

  if (menu_isActive()) {
    return;
  }

  if (!gifPlayerInitialized()) {
    return;
  }

  unsigned long currentTime = millis();

  if (handleSpecialStates()) {
    return;
  }

  if (animSequence.currentState == SequenceState::ANIMATION_CYCLE) {
    if (currentTime - lastCheckComs >= COMS_CHECK_INTERVAL) {
      if (getCurrentESPNowState() == ESPNowState::ON && !isPaired()) {
        playGIF(COMS_CONNECT_EMOTE);
        lastCheckComs = currentTime;
      }
    }
  }

  if (isPaired()) {
    switch (getCurrentComState()) {
    case ComState::PROCESSING:
      if (getCurrentAnimationPath() != nullptr) {
        ConversationType convType = getCurrentConversationType();
        const char* soundType = getCurrentConversationSoundType(convType);
        int soundDelay = getCurrentConversationSoundDelay(convType);
        sfxPlay(soundType, soundDelay);
        playGIF(getCurrentAnimationPath());

      }
      break;
    case ComState::WAITING:
      playGIF(COMS_IDLE_EMOTE);
      break;
    }
  } else {
    resetAnimationPath();
    handleAnimationSequence(millis());
  }
}

/**
 * @brief Play the boot animation
 */
void playBootAnimation() {
  if (!gifPlayerInitialized()) {
    return;
  }
  sfxPlay("startup");
  if (areHapticsActive()) {
    pulseVibration(100, 1200);
  }
  playGIF(STARTUP_EMOTE);
}