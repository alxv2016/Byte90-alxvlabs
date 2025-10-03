/**
 * @file gif_module.h
 * @brief Header for GIF animation playback functionality
 *
 * Provides functions for loading, initializing, and playing animated GIF files
 * from the filesystem with memory allocation, frame buffering, and display rendering.
 */

#ifndef GIF_MODULE_H
#define GIF_MODULE_H

#include "common.h"
#include "effects_core.h"
#include <AnimatedGIF.h>

//==============================================================================
// CONSTANTS & DEFINITIONS
//==============================================================================

static const char *GIF_LOG = "::GIF_MODULE::";

#define GIF_HEIGHT 128
#define GIF_WIDTH 128
#define FRAME_DELAY_MICROSECONDS (1000000 / 16)

//==============================================================================
// TYPE DEFINITIONS
//==============================================================================

struct GIFContext {
  uint8_t *sharedFrameBuffer;
  int offsetX;
  int offsetY;
};

//==============================================================================
// PUBLIC API FUNCTIONS
//==============================================================================

/**
 * @brief Initialize the GIF player
 * @return true if initialization was successful
 */
bool initializeGIFPlayer(void);

/**
 * @brief Stop GIF playback and free resources
 */
void stopGifPlayback(void);

/**
 * @brief Check if the GIF player is initialized
 * @return true if the GIF player is initialized
 */
bool gifPlayerInitialized();

/**
 * @brief Check and log memory status
 */
void checkMemoryStatus(void);

/**
 * @brief Load a GIF file for playback
 * @param filename Path to the GIF file
 * @return true if GIF was loaded successfully
 */
bool loadGIF(const char *filename);

/**
 * @brief Play a single frame of the current GIF
 * @param bSync Whether to synchronize with the GIF timing
 * @param delayMilliseconds Pointer to store delay until next frame
 * @return Status code (0 = success, 1 = finished, negative = error)
 */
int playGIFFrame(bool bSync, int *delayMilliseconds);

#endif /* GIF_MODULE_H */