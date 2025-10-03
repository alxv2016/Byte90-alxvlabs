/**
 * @file speaker_common.h
 * @brief Shared constants, types, and utilities for speaker system modules
 */

#ifndef SPEAKER_COMMON_H
#define SPEAKER_COMMON_H

#include "common.h"
#include <Arduino.h>

//==============================================================================
// CONSTANTS & DEFINITIONS
//==============================================================================

// I2S Configuration (shared between speaker_module and speaker_mp3)
#define I2S_NUM I2S_NUM_0
#define I2S_SAMPLE_RATE 44100
#define I2S_BITS_PER_SAMPLE I2S_BITS_PER_SAMPLE_24BIT
#define I2S_CHANNEL_FORMAT I2S_CHANNEL_FMT_ONLY_LEFT

// I2S Pin Configuration (shared between speaker_module and speaker_mp3)
#define I2S_BCK_IO A3
#define I2S_WS_IO I2S_PIN_NO_CHANGE
#define I2S_DO_IO A2

// Beep Configuration (from speaker_module)
#define BEEP_FREQUENCY 800
#define BEEP_DURATION 150
#define BEEP_COUNT 6
#define BEEP_VOLUME 30
#define BEEP_PAUSE 100

// MP3 Configuration (from speaker_mp3)
#define MP3_DEFAULT_VOLUME 1
#define SOUNDS_FOLDER "/sounds/"

//==============================================================================
// TYPE DEFINITIONS
//==============================================================================

/**
 * @brief Audio operation modes (from speaker_module)
 */
typedef enum {
    AUDIO_MODE_IDLE,
    AUDIO_MODE_BEEP,
    AUDIO_MODE_MP3,
    AUDIO_MODE_SHUTDOWN
} audio_mode_t;

/**
 * @brief Audio system states (from speaker_module)
 */
typedef enum {
    AUDIO_STATE_DISABLED,
    AUDIO_STATE_NOT_READY,
    AUDIO_STATE_READY,
    AUDIO_STATE_PLAYING
} audio_state_t;

/**
 * @brief MP3 playback states (from speaker_mp3)
 */
typedef enum {
    MP3_STATE_IDLE,
    MP3_STATE_LOADING,
    MP3_STATE_PLAYING,
    MP3_STATE_PAUSED,
    MP3_STATE_ERROR
} mp3_state_t;


#endif /* SPEAKER_COMMON_H */
