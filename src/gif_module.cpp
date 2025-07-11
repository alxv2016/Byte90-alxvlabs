/**
 * @file gif_module.cpp
 * @brief Implementation of GIF animation playback functionality
 *
 * This module provides functions for loading, initializing, and playing
 * animated GIF files from the filesystem. It handles memory allocation,
 * frame buffering, and rendering to the display.
 */

#include "gif_module.h"
#include "display_module.h"
#include "flash_module.h"

//==============================================================================
// GLOBAL VARIABLES
//==============================================================================

/** @brief AnimatedGIF library instance */
AnimatedGIF gif;
/** @brief Context for GIF playback including buffer and position */
GIFContext gifContext = {nullptr, 0, 0};
/** @brief Size of frame buffer in bytes (GIF_WIDTH × GIF_HEIGHT × 2 bytes per
 * pixel) */
const size_t frameBufferSize = GIF_WIDTH * GIF_HEIGHT * 2;
/** @brief Flag indicating if GIF player is initialized */
bool isInitialized = false;
/** @brief File handle for current GIF */
File gifFile;

//==============================================================================
// FILE I/O CALLBACKS FOR ANIMATEDGIF LIBRARY
//==============================================================================

/**
 * @brief Open a GIF file from the filesystem
 *
 * @param fname Filename to open
 * @param pSize Pointer to store file size
 * @return Pointer to file handle or NULL if failed
 */
void *GIFOpenFile(const char *fname, int32_t *pSize) {
  gifFile = LittleFS.open(fname);
  if (gifFile) {
    *pSize = gifFile.size();
    return (void *)&gifFile;
  }
  return NULL;
}

/**
 * @brief Close a GIF file
 *
 * @param pHandle File handle to close
 */
void GIFCloseFile(void *pHandle) {
  File *f = static_cast<File *>(pHandle);
  if (f != NULL) {
    f->close();
  }
}

/**
 * @brief Read data from a GIF file
 *
 * @param pFile GIF file structure
 * @param pBuf Buffer to store read data
 * @param iLen Length of data to read
 * @return Number of bytes actually read
 */
int32_t GIFReadFile(GIFFILE *pFile, uint8_t *pBuf, int32_t iLen) {
  File *f = static_cast<File *>(pFile->fHandle);
  int32_t bytesToRead = min(iLen, pFile->iSize - pFile->iPos - 1);

  if (bytesToRead <= 0)
    return 0;

  int32_t bytesRead = f->read(pBuf, bytesToRead);
  pFile->iPos = f->position();
  return bytesRead;
}

/**
 * @brief Seek to a position in a GIF file
 *
 * @param pFile GIF file structure
 * @param iPosition Position to seek to
 * @return New position in file
 */
int32_t GIFSeekFile(GIFFILE *pFile, int32_t iPosition) {
  File *f = static_cast<File *>(pFile->fHandle);
  f->seek(iPosition);
  pFile->iPos = (int32_t)f->position();
  return pFile->iPos;
}

/**
 * @brief Callback function to draw a GIF frame to the display
 *
 * @param pDraw GIF drawing parameters
 */
static void GIFDraw(GIFDRAW *pDraw) {
  if (pDraw->y == 0) {
    startWrite();
    setAddrWindow(gifContext.offsetX + pDraw->iX,
                  gifContext.offsetY + pDraw->iY, pDraw->iWidth,
                  pDraw->iHeight);
  }
  uint16_t *pixels = (uint16_t *)pDraw->pPixels;
  int currentRow = gifContext.offsetY + pDraw->iY + pDraw->y;

  // Apply all visual effects using the effects module
  applyEffectsToScanline(pixels, pDraw->iWidth, currentRow);

  // If neither is enabled, pixels remain unchanged
  writePixels(pixels, pDraw->iWidth);
  if (pDraw->y == pDraw->iHeight - 1) {
    endWrite();
  }
}

//==============================================================================
// UTILITY FUNCTIONS
//==============================================================================

/**
 * @brief Check and log memory status
 *
 * Reports available heap and PSRAM memory, with warnings if levels are low.
 */
void checkMemoryStatus() {
  const size_t LOW_HEAP_THRESHOLD = 10000;  // Warning if heap below 10KB
  const size_t LOW_PSRAM_THRESHOLD = 50000; // Warning if PSRAM below 50KB

  size_t freeHeap = heap_caps_get_free_size(MALLOC_CAP_8BIT);
  size_t freePSRAM = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);

  // Report heap status with context
  ESP_LOGW(GIF_LOG, "Free heap: %u bytes", freeHeap);
  if (freeHeap < LOW_HEAP_THRESHOLD) {
    ESP_LOGW(GIF_LOG, " (WARNING: Low heap memory!)");
  }

  // Report PSRAM status with context
  ESP_LOGW(GIF_LOG, "Free PSRAM: %u bytes", freePSRAM);
  if (freePSRAM < LOW_PSRAM_THRESHOLD) {
    ESP_LOGW(GIF_LOG, " (WARNING: Low PSRAM!)");
  }
}

//==============================================================================
// PUBLIC API FUNCTIONS
//==============================================================================

/**
 * @brief Stop GIF playback and free resources
 *
 * Releases the frame buffer memory and closes the GIF file.
 */
void stopGifPlayback() {
  if (gifContext.sharedFrameBuffer) {
    heap_caps_free(gifContext.sharedFrameBuffer);
    gifContext.sharedFrameBuffer = nullptr;
  }
  gif.close();
}

/**
 * @brief Initialize the GIF player
 *
 * Sets up the filesystem, allocates frame buffer memory, and initializes
 * the AnimatedGIF library.
 *
 * @return true if initialization was successful
 */
bool initializeGIFPlayer() {
  if (!getFSStatus()) {
    ESP_LOGE(GIF_LOG, "ERROR: LittleFS mount failed");
    isInitialized = false;
    return isInitialized;
  }

  checkMemoryStatus();
  StorageInfo storage = updateFlashStats();

  gif.begin(GIF_PALETTE_RGB565_LE);
  if (gifContext.sharedFrameBuffer == nullptr) {
    gifContext.sharedFrameBuffer =
        (uint8_t *)heap_caps_malloc(frameBufferSize, MALLOC_CAP_SPIRAM);
    if (!gifContext.sharedFrameBuffer) {
      ESP_LOGE(GIF_LOG, "ERROR: Failed to allocate shared frame buffer.");
      isInitialized = false;
      return isInitialized;
    }
  }
  isInitialized = true;
  return isInitialized;
}

/**
 * @brief Load a GIF file for playback
 *
 * Opens a GIF file, sets up rendering parameters, and prepares it for playback.
 *
 * @param filename Path to the GIF file
 * @return true if GIF was loaded successfully
 */
bool loadGIF(const char *filename) {
  if (!gif.open(filename, GIFOpenFile, GIFCloseFile, GIFReadFile, GIFSeekFile,
                GIFDraw)) {
    ESP_LOGE(GIF_LOG, "ERROR: Failed to open GIF: %s", filename);
    return false;
  }

  // Calculate the offset (position) where the GIF should be drawn on the screen
  gifContext.offsetX = (DISPLAY_WIDTH - gif.getCanvasWidth()) / 2;
  gifContext.offsetY = (DISPLAY_HEIGHT - gif.getCanvasHeight()) / 2;

  // Ensure buffer is allocated
  if (gifContext.sharedFrameBuffer == nullptr) {
    gifContext.sharedFrameBuffer =
        (uint8_t *)heap_caps_malloc(frameBufferSize, MALLOC_CAP_SPIRAM);
    if (!gifContext.sharedFrameBuffer) {
      ESP_LOGE(GIF_LOG, "ERROR: Failed to allocate %zu bytes", frameBufferSize);
      stopGifPlayback();
      return false;
    }
  }

  // Set the drawing type to "cooked" to allow the GIF library to pre-process
  gif.setDrawType(GIF_DRAW_COOKED);
  gif.setFrameBuf(gifContext.sharedFrameBuffer);
  return true;
}

/**
 * @brief Play a single frame of the current GIF
 *
 * @param bSync Whether to synchronize with the GIF timing
 * @param delayMilliseconds Pointer to store delay until next frame
 * @return Status code (0 = success, 1 = finished, negative = error)
 */
int playGIFFrame(bool bSync, int *delayMilliseconds) {
  return gif.playFrame(bSync, delayMilliseconds);
}

/**
 * @brief Check if the GIF player is initialized
 *
 * @return true if the GIF player is initialized
 */
bool gifPlayerInitialized() { return isInitialized; }