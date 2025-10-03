/**
 * @file speaker_mp3.cpp
 * @brief Implementation of MP3 file playback and decoding functionality
 */

#include "speaker_mp3.h"
#include "speaker_module.h"
#include "common.h"
#include "preferences_module.h"
#include "driver/i2s.h"
#include "esp_log.h"
#include "flash_module.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <LittleFS.h>
#include <stdarg.h>

//==============================================================================
// DEBUG SYSTEM
//==============================================================================

// Debug control - set to true to enable debug logging
static bool speakerMp3DebugEnabled = false;

/**
 * @brief Centralized debug logging function for speaker MP3 operations
 * @param format Printf-style format string
 * @param ... Variable arguments for format string
 */
static void speakerMp3Debug(const char* format, ...) {
  if (!speakerMp3DebugEnabled) {
    return;
  }
  
  va_list args;
  va_start(args, format);
  esp_log_writev(ESP_LOG_INFO, "SPEAKER_MP3_LOG", format, args);
  va_end(args);
}

/**
 * @brief Enable or disable debug logging for speaker MP3 operations
 * @param enabled true to enable debug logging, false to disable
 * 
 * @example
 * // Enable debug logging
 * setSpeakerMp3Debug(true);
 * 
 * // Disable debug logging  
 * setSpeakerMp3Debug(false);
 */
void setSpeakerMp3Debug(bool enabled) {
  speakerMp3DebugEnabled = enabled;
  if (enabled) {
    ESP_LOGI("SPEAKER_MP3_LOG", "Speaker MP3 debug logging enabled");
  } else {
    ESP_LOGI("SPEAKER_MP3_LOG", "Speaker MP3 debug logging disabled");
  }
}

//==============================================================================
// GLOBAL VARIABLES
//==============================================================================

Audio *mp3Audio = nullptr;

static bool g_mp3DecoderInitialized = false;
static int g_mp3Volume = MP3_DEFAULT_VOLUME;
static String g_currentMP3File = "";
static mp3_state_t g_mp3State = MP3_STATE_IDLE;
static bool g_mp3Paused = false;

//==============================================================================
// UTILITY FUNCTIONS
//==============================================================================

/**
 * @brief Get the full path for an MP3 file
 */
bool getMP3FullPath(const char *filename, char *fullPath, size_t bufferSize) {
  if (!filename || !fullPath || bufferSize == 0) {
    return false;
  }

  if (filename[0] == '/') {
    strncpy(fullPath, filename, bufferSize - 1);
    fullPath[bufferSize - 1] = '\0';
  } else {
    snprintf(fullPath, bufferSize, "%s%s", SOUNDS_FOLDER, filename);
  }

  return true;
}

/**
 * @brief Check if MP3 file exists
 */
bool mp3FileExists(const char *filename) {
  char fullPath[256];
  if (!getMP3FullPath(filename, fullPath, sizeof(fullPath))) {
    return false;
  }
  return fileExists(fullPath);
}

/**
 * @brief Get MP3 file metadata
 */
bool getMP3Metadata(const char *filename, char *title, char *artist, int *duration) {
  // This is a placeholder - actual metadata extraction would require
  // parsing MP3 headers and ID3 tags
  if (title) strcpy(title, "Unknown");
  if (artist) strcpy(artist, "Unknown");
  if (duration) *duration = 0;
  return true;
}

/**
 * @brief Clean up MP3 Audio library and free I2S
 */
static void cleanupMP3AudioLibrary() {
  if (mp3Audio) {
    ESP_LOGE(SPEAKER_MP3_LOG, "Cleaning up MP3 Audio library");
    mp3Audio->stopSong();
    delete mp3Audio;
    mp3Audio = nullptr;
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

//==============================================================================
// MP3 PLAYBACK API IMPLEMENTATION
//==============================================================================

/**
 * @brief Initialize MP3 decoder and audio system
 */
bool initMP3Decoder() {
  if (!checkHardwareSupport()) {
    ESP_LOGW(SPEAKER_MP3_LOG, "Speaker support disabled");
    return false;
  }

  if (g_mp3DecoderInitialized) {
    speakerMp3Debug("MP3 decoder already initialized");
    return true;
  }

  speakerMp3Debug("Initializing MP3 decoder");

  // Ensure core speaker is initialized first
  audio_state_t audioState = getAudioState();
  if (audioState != AUDIO_STATE_READY && audioState != AUDIO_STATE_PLAYING) {
    ESP_LOGE(SPEAKER_MP3_LOG, "Core speaker not ready");
    return false;
  }

  g_mp3DecoderInitialized = true;
  g_mp3State = MP3_STATE_IDLE;
  g_mp3Paused = false;

  speakerMp3Debug("MP3 decoder initialized successfully");
  return true;
}

/**
 * @brief Shutdown MP3 decoder and cleanup resources
 */
void shutdownMP3Decoder() {
  speakerMp3Debug("Shutting down MP3 decoder");
  
  stopMP3Playback(true);
  cleanupMP3AudioLibrary();
  
  g_mp3DecoderInitialized = false;
  g_mp3State = MP3_STATE_IDLE;
  g_mp3Paused = false;
  g_currentMP3File = "";
  
  speakerMp3Debug("MP3 decoder shutdown complete");
}

/**
 * @brief Play an MP3 file from SPIFFS storage
 */
bool playMP3File(const char* filename, int volume, bool blocking) {
  if (!g_mp3DecoderInitialized) {
    ESP_LOGE(SPEAKER_MP3_LOG, "MP3 decoder not initialized");
    return false;
  }

  if (!filename) {
    ESP_LOGW(SPEAKER_MP3_LOG, "Invalid filename for MP3 playback");
    return false;
  }

  // Stop any current playback
  stopMP3Playback(true);

  // Create Audio object if needed
  if (!mp3Audio) {
    speakerMp3Debug("Creating MP3 Audio object");
    mp3Audio = new Audio(false, 1, I2S_NUM_0);
    if (!mp3Audio) {
      ESP_LOGE(SPEAKER_MP3_LOG, "Failed to create MP3 Audio object");
      return false;
    }
    vTaskDelay(pdMS_TO_TICKS(50));
  }

  // Get full file path
  char fullPath[256];
  if (!getMP3FullPath(filename, fullPath, sizeof(fullPath))) {
    ESP_LOGE(SPEAKER_MP3_LOG, "Failed to construct full path for: %s", filename);
    return false;
  }

  // Check if file exists
  if (!fileExists(fullPath)) {
    ESP_LOGE(SPEAKER_MP3_LOG, "MP3 file not found: %s", fullPath);
    return false;
  }

  speakerMp3Debug("Playing MP3 file: %s", fullPath);

  // Set volume if specified
  if (volume >= 0 && volume <= 21) {
    setMP3Volume(volume);
  }

  // Configure audio object
  mp3Audio->setPinout(I2S_BCK_IO, I2S_WS_IO, I2S_DO_IO);
  mp3Audio->forceMono(true);
  mp3Audio->setTone(-100, -100, 0);

  // Connect to MP3 file
  if (!mp3Audio->connecttoFS(LittleFS, fullPath)) {
    ESP_LOGE(SPEAKER_MP3_LOG, "Failed to connect to MP3 file: %s", fullPath);
    return false;
  }

  g_mp3State = MP3_STATE_PLAYING;
  g_mp3Paused = false;
  g_currentMP3File = String(fullPath);

  if (blocking) {
    speakerMp3Debug("Blocking MP3 playback started");
    while (g_mp3State == MP3_STATE_PLAYING && mp3Audio) {
      mp3Audio->loop();
      vTaskDelay(pdMS_TO_TICKS(1));
    }
    speakerMp3Debug("Blocking MP3 playback completed");
  }

  return true;
}

/**
 * @brief Stop current MP3 playback
 */
bool stopMP3Playback(bool cleanup) {
  if (!mp3Audio) {
    return true; // Nothing to stop
  }

  speakerMp3Debug("Stopping MP3 playback");
  
  mp3Audio->stopSong();
  
  if (cleanup) {
    cleanupMP3AudioLibrary();
  }

  g_mp3State = MP3_STATE_IDLE;
  g_mp3Paused = false;
  g_currentMP3File = "";

  return true;
}

/**
 * @brief Pause current MP3 playback
 */
bool pauseMP3Playback() {
  if (!mp3Audio || g_mp3State != MP3_STATE_PLAYING) {
    return false;
  }

  speakerMp3Debug("Pausing MP3 playback");
  
  // Note: ESP32-audioI2S doesn't have a direct pause function
  // This is a placeholder for future implementation
  g_mp3Paused = true;
  g_mp3State = MP3_STATE_PAUSED;

  return true;
}

/**
 * @brief Resume paused MP3 playback
 */
bool resumeMP3Playback() {
  if (!mp3Audio || g_mp3State != MP3_STATE_PAUSED) {
    return false;
  }

  speakerMp3Debug("Resuming MP3 playback");
  
  // Note: ESP32-audioI2S doesn't have a direct resume function
  // This is a placeholder for future implementation
  g_mp3Paused = false;
  g_mp3State = MP3_STATE_PLAYING;

  return true;
}

/**
 * @brief Set volume for MP3 playback
 */
bool setMP3Volume(int volume) {
  if (volume < 0) volume = 0;
  if (volume > 21) volume = 21;

  g_mp3Volume = volume;
  
  if (mp3Audio) {
    mp3Audio->setVolume(volume);
  }

  speakerMp3Debug("MP3 volume set to: %d", volume);
  return true;
}

/**
 * @brief Get current MP3 volume
 */
int getMP3Volume() {
  return g_mp3Volume;
}

/**
 * @brief Check if MP3 is currently playing
 */
bool isMP3Playing() {
  return (g_mp3State == MP3_STATE_PLAYING && mp3Audio);
}

/**
 * @brief Check if MP3 is paused
 */
bool isMP3Paused() {
  return (g_mp3State == MP3_STATE_PAUSED && g_mp3Paused);
}

/**
 * @brief Get current MP3 playback state
 */
mp3_state_t getMP3State() {
  return g_mp3State;
}

/**
 * @brief Get current playing MP3 filename
 */
String getCurrentMP3File() {
  return g_currentMP3File;
}

/**
 * @brief MP3 loop function for audio processing
 */
void mp3Loop() {
  if (!g_mp3DecoderInitialized || !mp3Audio) {
    return;
  }

  if (g_mp3State == MP3_STATE_PLAYING) {
    mp3Audio->loop();
    
    // Check if playback has finished
    if (!mp3Audio->isRunning()) {
      speakerMp3Debug("MP3 playback finished");
      g_mp3State = MP3_STATE_IDLE;
      g_currentMP3File = "";
    }
  }
}
