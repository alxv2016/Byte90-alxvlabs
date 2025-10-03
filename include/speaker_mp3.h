/**
 * @file speaker_mp3.h
 * @brief MP3 file playback and decoding functionality
 */

#ifndef SPEAKER_MP3_H
#define SPEAKER_MP3_H

#include "common.h"
#include <Arduino.h>
#include "Audio.h"
#include "flash_module.h"
#include "speaker_common.h"

//==============================================================================
// CONSTANTS & DEFINITIONS
//==============================================================================

static const char *SPEAKER_MP3_LOG = "::SPEAKER_MP3::";

//==============================================================================
// MP3 PLAYBACK API
//==============================================================================

/**
 * @brief Initialize MP3 decoder and audio system
 * @return true if initialization successful, false otherwise
 */
bool initMP3Decoder();

/**
 * @brief Shutdown MP3 decoder and cleanup resources
 */
void shutdownMP3Decoder();

/**
 * @brief Play an MP3 file from SPIFFS storage
 * @param filename MP3 filename (with or without path)
 * @param volume Volume level (0-21), use -1 for default volume
 * @param blocking If true, function blocks until playback complete
 * @return true if playback started successfully, false otherwise
 */
bool playMP3File(const char* filename, int volume = -1, bool blocking = false);

/**
 * @brief Stop current MP3 playback
 * @param cleanup If true, deletes Audio object; if false, just stops playback
 * @return true if stopped successfully, false otherwise
 */
bool stopMP3Playback(bool cleanup = true);

/**
 * @brief Pause current MP3 playback
 * @return true if paused successfully, false otherwise
 */
bool pauseMP3Playback();

/**
 * @brief Resume paused MP3 playback
 * @return true if resumed successfully, false otherwise
 */
bool resumeMP3Playback();

/**
 * @brief Set volume for MP3 playback
 * @param volume Volume level (0-21)
 * @return true if volume set successfully, false otherwise
 */
bool setMP3Volume(int volume);

/**
 * @brief Get current MP3 volume
 * @return Current volume level (0-21)
 */
int getMP3Volume();

/**
 * @brief Check if MP3 is currently playing
 * @return true if MP3 is playing, false otherwise
 */
bool isMP3Playing();

/**
 * @brief Check if MP3 is paused
 * @return true if MP3 is paused, false otherwise
 */
bool isMP3Paused();

/**
 * @brief Get current MP3 playback state
 * @return mp3_state_t indicating current state
 */
mp3_state_t getMP3State();

/**
 * @brief Get current playing MP3 filename
 * @return String with current filename, empty if not playing
 */
String getCurrentMP3File();

/**
 * @brief MP3 loop function for audio processing
 * Should be called regularly in main loop when MP3 is active
 */
void mp3Loop();

//==============================================================================
// UTILITY FUNCTIONS
//==============================================================================

/**
 * @brief Get the full path for an MP3 file
 * @param filename MP3 filename
 * @param fullPath Buffer to store the full path
 * @param bufferSize Size of the fullPath buffer
 * @return true if path constructed successfully, false otherwise
 */
bool getMP3FullPath(const char *filename, char *fullPath, size_t bufferSize);

/**
 * @brief Check if MP3 file exists
 * @param filename MP3 filename
 * @return true if file exists, false otherwise
 */
bool mp3FileExists(const char *filename);

/**
 * @brief Get MP3 file metadata (if available)
 * @param filename MP3 filename
 * @param title Buffer for title (optional)
 * @param artist Buffer for artist (optional)
 * @param duration Buffer for duration in seconds (optional)
 * @return true if metadata retrieved successfully, false otherwise
 */
bool getMP3Metadata(const char *filename, char *title = nullptr, char *artist = nullptr, int *duration = nullptr);

/**
 * @brief Enable or disable debug logging for speaker MP3 operations
 * @param enabled true to enable debug logging, false to disable
 */
void setSpeakerMp3Debug(bool enabled);

#endif /* SPEAKER_MP3_H */
