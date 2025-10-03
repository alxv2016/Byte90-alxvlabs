/**
 * @file speaker_module.cpp
 * @brief Implementation of enhanced audio functionality with ESPHome ESP32-audioI2S support
 */

#include "speaker_module.h"
#include "speaker_mp3.h"
#include "common.h"
#include "preferences_module.h"
#include "driver/i2s.h"
#include "esp_log.h"
#include "flash_module.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <LittleFS.h>
#include <math.h>
#include <stdarg.h>

//==============================================================================
// DEBUG SYSTEM
//==============================================================================

// Debug control - set to true to enable debug logging
static bool speakerDebugEnabled = false;

/**
 * @brief Centralized debug logging function for speaker operations
 * @param format Printf-style format string
 * @param ... Variable arguments for format string
 */
static void speakerDebug(const char* format, ...) {
  if (!speakerDebugEnabled) {
    return;
  }
  
  va_list args;
  va_start(args, format);
  esp_log_writev(ESP_LOG_INFO, "SPEAKER_LOG", format, args);
  va_end(args);
}

/**
 * @brief Enable or disable debug logging for speaker operations
 * @param enabled true to enable debug logging, false to disable
 * 
 * @example
 * // Enable debug logging
 * setSpeakerDebug(true);
 * 
 * // Disable debug logging  
 * setSpeakerDebug(false);
 */
void setSpeakerDebug(bool enabled) {
  speakerDebugEnabled = enabled;
  if (enabled) {
    ESP_LOGI("SPEAKER_LOG", "Speaker debug logging enabled");
  } else {
    ESP_LOGI("SPEAKER_LOG", "Speaker debug logging disabled");
  }
}

//==============================================================================
// GLOBAL VARIABLES
//==============================================================================

Audio *audio = nullptr;

static bool g_audioInitialized = false;
static bool g_audioShutdown = false;
static audio_mode_t g_audioMode = AUDIO_MODE_SHUTDOWN;
static bool g_beepInProgress = false;
static bool g_i2sInitializedForBeep = false;
static float g_sinePhase = 0.0f;

//==============================================================================
// UTILITY FUNCTIONS (STATIC)
//==============================================================================

/**
 * @brief Clean up Audio library and free I2S
 */
static void cleanupAudioLibrary() {
  if (audio) {
    ESP_LOGE(SPEAKER_LOG, "Cleaning up Audio library");
    audio->stopSong();
    delete audio;
    audio = nullptr;
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

/**
 * @brief Configure I2S system
 * @param pinsOnly If true, only configure pins; if false, configure full I2S system
 * @return true if configuration successful
 */
static bool configureI2SSystem(bool pinsOnly = false) {
  if (!checkHardwareSupport())
    return false;

  if (!pinsOnly) {
    // Configure I2S peripheral first
    esp_err_t uninstall_err = i2s_driver_uninstall(I2S_NUM);
    if (uninstall_err == ESP_OK) {
      ESP_LOGE(SPEAKER_LOG, "Uninstalled existing I2S driver");
      vTaskDelay(pdMS_TO_TICKS(10));
    }

    i2s_config_t i2s_config = {
        .mode = static_cast<i2s_mode_t>(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = I2S_SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE,
        .channel_format = I2S_CHANNEL_FORMAT,
        .communication_format = static_cast<i2s_comm_format_t>(I2S_COMM_FORMAT_STAND_I2S),
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL2,
        .dma_buf_count = 2,
        .dma_buf_len = 64,
        .use_apll = false,
        .tx_desc_auto_clear = true,
        .fixed_mclk = 0};

    esp_err_t err = i2s_driver_install(I2S_NUM, &i2s_config, 0, NULL);
    if (err != ESP_OK) {
      ESP_LOGE(SPEAKER_LOG, "Failed to install I2S driver: %s", esp_err_to_name(err));
      return false;
    }

    i2s_zero_dma_buffer(I2S_NUM);
    i2s_stop(I2S_NUM);
  }

  // Configure I2S pins
  i2s_pin_config_t pin_config = {
      .bck_io_num = I2S_BCK_IO,
      .ws_io_num = I2S_PIN_NO_CHANGE,
      .data_out_num = I2S_DO_IO,
      .data_in_num = I2S_PIN_NO_CHANGE};

  esp_err_t err = i2s_set_pin(I2S_NUM, &pin_config);
  if (err != ESP_OK) {
    ESP_LOGE(SPEAKER_LOG, "Failed to set I2S pins: %s", esp_err_to_name(err));
    return false;
  }

  return true;
}

/**
 * @brief Generate sine wave samples with phase continuity
 * @param buffer Audio buffer to fill
 * @param samples Number of samples to generate
 * @param frequency Frequency in Hz
 * @param amplitude Amplitude (0.0-1.0)
 */
static void generateSineWave(int16_t *buffer, size_t samples, uint16_t frequency, float amplitude) {
  if (!checkHardwareSupport())
    return;

  const float phase_increment = 2.0f * M_PI * frequency / I2S_SAMPLE_RATE;

  for (size_t i = 0; i < samples; i++) {
    buffer[i] = (int16_t)(sinf(g_sinePhase) * amplitude * 32767.0f);
    g_sinePhase += phase_increment;

    if (g_sinePhase >= 2.0f * M_PI) {
      g_sinePhase -= 2.0f * M_PI;
    }
  }
}

/**
 * @brief MP3 end of file callback
 * @param info EOF information
 */
// MP3 EOF handling moved to speaker_mp3 module

//==============================================================================
// PUBLIC API FUNCTIONS
//==============================================================================

bool initializeSpeaker(bool checkPreferences) {
  ESP_LOGE(SPEAKER_LOG, "=== initializeSpeaker() called ===");
  ESP_LOGE(SPEAKER_LOG, "SERIES_2 compile flag: %s", SERIES_2 ? "ENABLED" : "DISABLED");
  ESP_LOGE(SPEAKER_LOG, "checkHardwareSupport(): %s", checkHardwareSupport() ? "true" : "false");
  ESP_LOGE(SPEAKER_LOG, "Check preferences: %s", checkPreferences ? "YES" : "NO");
  
  if (!checkHardwareSupport()) {
    ESP_LOGE(SPEAKER_LOG, "Speaker support disabled at compile time - skipping initialization");
    g_audioInitialized = false;
    g_audioShutdown = true;
    g_audioMode = AUDIO_MODE_SHUTDOWN;
    return false;
  }

  if (checkPreferences) {
    bool audioShouldBeEnabled = getAudioEnabled();
    ESP_LOGE(SPEAKER_LOG, "User preference: audio should be %s", audioShouldBeEnabled ? "ENABLED" : "DISABLED");

    if (!audioShouldBeEnabled) {
      ESP_LOGE(SPEAKER_LOG, "Audio disabled by user preference - skipping initialization");
      g_audioInitialized = false;
      g_audioShutdown = true;
      g_audioMode = AUDIO_MODE_SHUTDOWN;
      return false;
    }
  }

  if (g_audioInitialized) {
    ESP_LOGW(SPEAKER_LOG, "Speaker already initialized");
    return true;
  }

  ESP_LOGE(SPEAKER_LOG, "Initializing speaker module...");

  if (getFSStatus()) {
    ESP_LOGE(SPEAKER_LOG, "Filesystem already initialized - ready for MP3 playback");
  } else {
    ESP_LOGW(SPEAKER_LOG, "Filesystem not available - MP3 playback will be unavailable");
    ESP_LOGW(SPEAKER_LOG, "Note: Filesystem should be initialized before audio module");
  }

  vTaskDelay(pdMS_TO_TICKS(50));

  g_audioInitialized = true;
  g_audioShutdown = false;
  g_audioMode = AUDIO_MODE_IDLE;

  // Initialize MP3 decoder
  if (!initMP3Decoder()) {
    ESP_LOGW(SPEAKER_LOG, "MP3 decoder initialization failed - MP3 playback will be unavailable");
  } else {
    speakerDebug("MP3 decoder initialized successfully");
  }

  ESP_LOGE(SPEAKER_LOG, "Speaker module initialized successfully");
  ESP_LOGE(SPEAKER_LOG, "I2S Pins - BCLK: %d, LRCLK: GND, DOUT: %d", I2S_BCK_IO, I2S_DO_IO);
  return true;
}

void shutdownAudio(bool saveAsDisabled) {
  if (!g_audioInitialized || g_audioShutdown) {
    if (saveAsDisabled) {
      setAudioEnabled(false);
      ESP_LOGE(SPEAKER_LOG, "Audio already shutdown - only saving disabled preference");
    }
    return;
  }

  if (!checkHardwareSupport()) {
    g_audioShutdown = true;
    g_audioInitialized = false;
    g_audioMode = AUDIO_MODE_SHUTDOWN;
    if (saveAsDisabled) {
      setAudioEnabled(false);
    }
    return;
  }

  ESP_LOGE(SPEAKER_LOG, "Shutting down audio system...");

  g_audioShutdown = true;
  g_audioMode = AUDIO_MODE_SHUTDOWN;

  // Stop MP3 playback if active
  if (isMP3Playing()) {
    ESP_LOGE(SPEAKER_LOG, "Stopping MP3 playback...");
    stopMP3Playback(true);
    vTaskDelay(pdMS_TO_TICKS(50));
  }

  if (g_beepInProgress) {
    ESP_LOGE(SPEAKER_LOG, "Stopping beep generation...");
    g_beepInProgress = false;
    vTaskDelay(pdMS_TO_TICKS(10));
  }

  if (audio != nullptr) {
    ESP_LOGE(SPEAKER_LOG, "Cleaning up Audio library...");
    try {
      audio->stopSong();
      vTaskDelay(pdMS_TO_TICKS(20));
      delete audio;
      audio = nullptr;
      ESP_LOGE(SPEAKER_LOG, "Audio library cleaned up successfully");
    } catch (...) {
      ESP_LOGE(SPEAKER_LOG, "Exception during Audio library cleanup");
      audio = nullptr;
    }
    vTaskDelay(pdMS_TO_TICKS(50));
  }

  if (g_i2sInitializedForBeep) {
    ESP_LOGE(SPEAKER_LOG, "Cleaning up I2S driver...");
    esp_err_t stop_err = i2s_stop(I2S_NUM);
    if (stop_err != ESP_OK) {
      ESP_LOGW(SPEAKER_LOG, "I2S stop failed: %s", esp_err_to_name(stop_err));
    }

    esp_err_t zero_err = i2s_zero_dma_buffer(I2S_NUM);
    if (zero_err != ESP_OK) {
      ESP_LOGW(SPEAKER_LOG, "I2S zero buffer failed: %s", esp_err_to_name(zero_err));
    }

    esp_err_t uninstall_err = i2s_driver_uninstall(I2S_NUM);
    if (uninstall_err != ESP_OK) {
      ESP_LOGW(SPEAKER_LOG, "I2S uninstall failed: %s", esp_err_to_name(uninstall_err));
    } else {
      ESP_LOGE(SPEAKER_LOG, "I2S driver uninstalled successfully");
    }

    g_i2sInitializedForBeep = false;
    vTaskDelay(pdMS_TO_TICKS(20));
  }

  ESP_LOGE(SPEAKER_LOG, "Resetting GPIO pins...");
  gpio_reset_pin((gpio_num_t)I2S_BCK_IO);
  gpio_reset_pin((gpio_num_t)I2S_DO_IO);

  g_beepInProgress = false;
  g_i2sInitializedForBeep = false;
  g_audioInitialized = false;

  // Shutdown MP3 decoder
  shutdownMP3Decoder();

  if (saveAsDisabled) {
    setAudioEnabled(false);
    ESP_LOGE(SPEAKER_LOG, "Audio shutdown and saved as disabled");
  }

  ESP_LOGE(SPEAKER_LOG, "Audio system shutdown complete");
}

bool restartAudio() {
    if (!checkHardwareSupport()) return false;

    if (g_audioInitialized && !g_audioShutdown) {
        ESP_LOGW(SPEAKER_LOG, "Audio already running");
        return true;
    }

    ESP_LOGE(SPEAKER_LOG, "Restarting audio system...");
    
    g_audioShutdown = false;
    g_audioInitialized = false;
    
    return initializeSpeaker(true);
}

audio_state_t getAudioState() {
  if (!checkHardwareSupport()) {
    return AUDIO_STATE_DISABLED;
  }

  if (!getAudioEnabled()) {
    return AUDIO_STATE_DISABLED;
  }

  if (!g_audioInitialized || g_audioShutdown) {
    return AUDIO_STATE_NOT_READY;
  }

  if ((g_audioMode == AUDIO_MODE_MP3) || (g_audioMode == AUDIO_MODE_BEEP) || g_beepInProgress) {
    return AUDIO_STATE_PLAYING;
  }

  return AUDIO_STATE_READY;
}



bool stopAudio(bool cleanup) {
  audio_state_t audioState = getAudioState();
  if (audioState != AUDIO_STATE_READY && audioState != AUDIO_STATE_PLAYING) {
    return false;
  }

  bool wasStopped = false;

  // Stop MP3 playback if active
  if (isMP3Playing()) {
    stopMP3Playback(cleanup);
    wasStopped = true;
    ESP_LOGE(SPEAKER_LOG, "MP3 playback stopped");
  }

  if (g_beepInProgress) {
    g_beepInProgress = false;
    wasStopped = true;
  }

  g_audioMode = AUDIO_MODE_IDLE;

  return wasStopped;
}

void playBeep(uint16_t frequency, uint16_t duration, uint8_t volume) {
  if (!checkHardwareSupport())
    return;

  audio_state_t audioState = getAudioState();
  if ((audioState != AUDIO_STATE_READY && audioState != AUDIO_STATE_PLAYING) || g_audioShutdown) {
    ESP_LOGW(SPEAKER_LOG, "Audio not ready or shutting down - skipping beep");
    return;
  }

  if (isMP3Playing()) {
    ESP_LOGE(SPEAKER_LOG, "Stopping MP3 to play beep");
    stopMP3Playback(true);
    vTaskDelay(pdMS_TO_TICKS(50));

    if (g_audioShutdown) {
      ESP_LOGW(SPEAKER_LOG, "Audio shutdown detected during MP3 cleanup - aborting beep");
      return;
    }
  }

  g_audioMode = AUDIO_MODE_BEEP;
  g_beepInProgress = true;

  if (!g_i2sInitializedForBeep) {
    if (configureI2SSystem()) {
      g_i2sInitializedForBeep = true;
    } else {
      ESP_LOGE(SPEAKER_LOG, "Failed to initialize I2S for beep");
      g_beepInProgress = false;
      g_audioMode = AUDIO_MODE_IDLE;
      return;
    }
  }

  esp_err_t err = i2s_start(I2S_NUM);
  if (err != ESP_OK) {
    ESP_LOGE(SPEAKER_LOG, "Failed to start I2S: %s", esp_err_to_name(err));
    g_beepInProgress = false;
    g_audioMode = AUDIO_MODE_IDLE;
    return;
  }

  float amplitude = volume / 100.0f;
  size_t total_samples = (I2S_SAMPLE_RATE * duration) / 1000;
  size_t samples_per_chunk = 256;

  int16_t *audio_buffer = (int16_t *)malloc(samples_per_chunk * sizeof(int16_t));
  if (!audio_buffer) {
    ESP_LOGE(SPEAKER_LOG, "Failed to allocate audio buffer");
    i2s_stop(I2S_NUM);
    g_beepInProgress = false;
    g_audioMode = AUDIO_MODE_IDLE;
    return;
  }

  g_sinePhase = 0.0f;

  size_t samples_remaining = total_samples;
  while (samples_remaining > 0 && !g_audioShutdown) {
    size_t chunk_samples = (samples_remaining < samples_per_chunk) ? samples_remaining : samples_per_chunk;

    generateSineWave(audio_buffer, chunk_samples, frequency, amplitude);

    size_t bytes_written;
    esp_err_t write_err = i2s_write(I2S_NUM, audio_buffer, chunk_samples * sizeof(int16_t), &bytes_written, pdMS_TO_TICKS(100));

    if (write_err != ESP_OK) {
      ESP_LOGW(SPEAKER_LOG, "I2S write failed: %s", esp_err_to_name(write_err));
      break;
    }

    samples_remaining -= chunk_samples;
  }

  free(audio_buffer);

  if (!g_audioShutdown) {
    i2s_stop(I2S_NUM);
    i2s_zero_dma_buffer(I2S_NUM);
  }

  g_beepInProgress = false;
  g_audioMode = AUDIO_MODE_IDLE;
}

// MP3 playback functions moved to speaker_mp3 module

void audioLoop() {
  audio_state_t audioState = getAudioState();
  if (audioState != AUDIO_STATE_READY && audioState != AUDIO_STATE_PLAYING)
    return;

  // Handle MP3 playback through the MP3 module
  mp3Loop();
}