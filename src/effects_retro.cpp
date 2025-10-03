/**
 * @file effects_retro.cpp
 * @brief Implementation of retro effects: scanlines, dithering, and glitch effects
 *
 * Provides comprehensive retro visual effects for the BYTE-90 device,
 * including CRT-style scanlines, Bayer dithering, and glitch effects
 * for authentic retro gaming console aesthetics.
 *
 * This module handles:
 * - CRT scanline effects with multiple modes (classic, animated, curved)
 * - Bayer dithering with 2x2, 4x4, and 8x8 patterns for color reduction
 * - CRT glitch effects with horizontal jitter and random artifacts
 * - Color quantization and depth reduction algorithms
 * - Parameter validation and clamping for safe effect application
 * - Animation timing and synchronization for dynamic effects
 * - Fast random number generation for glitch effects
 * - Performance-optimized pixel processing and scanline operations
 * - Default parameter management for all retro effects
 */

#include "effects_retro.h"
#include "effects_common.h"
#include "effects_tints.h"
#include <Arduino.h>

//==============================================================================
// FORWARD DECLARATIONS
//==============================================================================

// Helper function to get Bayer matrix size
static int getBayerMatrixSize(DitherMode mode);

// Scanline effect functions
static uint16_t
effectsRetro_applyClassicScanline(uint16_t pixel,
                                  const scanline_params_t *params, int row);

//==============================================================================
// GLOBAL VARIABLES
//==============================================================================

// Animation and timing
static unsigned long scanlineStartTime = 0;
static bool scanlineAnimationEnabled = false;

// Glitch random seed
static unsigned long glitchSeed = 0;

// Bayer dithering matrices
static const int bayer2x2[2][2] = {{0, 2}, {3, 1}};
static const int bayer4x4[4][4] = {
    {0, 8, 2, 10}, {12, 4, 14, 6}, {3, 11, 1, 9}, {15, 7, 13, 5}};
static const int bayer8x8[8][8] = {
    {0, 32, 8, 40, 2, 34, 10, 42},  {48, 16, 56, 24, 50, 18, 58, 26},
    {12, 44, 4, 36, 14, 46, 6, 38}, {60, 28, 52, 20, 62, 30, 54, 22},
    {3, 35, 11, 43, 1, 33, 9, 41},  {51, 19, 59, 27, 49, 17, 57, 25},
    {15, 47, 7, 39, 13, 45, 5, 37}, {63, 31, 55, 23, 61, 29, 53, 21}};

//==============================================================================
// RETRO EFFECTS INITIALIZATION
//==============================================================================

/**
 * @brief Initialize retro effects system
 * 
 * Initializes the retro effects system by setting up timing variables,
 * animation states, and random number generation. This function should
 * be called during system initialization to prepare the retro effects
 * for operation.
 */
void effectsRetro_init(void) {
   scanlineStartTime = millis();
   scanlineAnimationEnabled = false;
   glitchSeed = random(0xFFFFFFFF);
 }

/**
 * @brief Shutdown retro effects system
 * 
 * Shuts down the retro effects system by disabling animations and
 * cleaning up any active effects. This function should be called
 * during system shutdown to properly clean up retro effects.
 */
void effectsRetro_shutdown(void) { scanlineAnimationEnabled = false; }

//==============================================================================
// SCANLINE EFFECT IMPLEMENTATIONS
//==============================================================================

/**
 * @brief Apply scanline effect to a pixel
 * @param pixel Original RGB565 pixel
 * @param params Scanline parameters
 * @param row Current row number
 * @return Scanline-processed RGB565 pixel
 */
uint16_t effectsRetro_applyScanline(uint16_t pixel,
                                     const scanline_params_t *params, int row) {
  if (!params) {
    return pixel;
  }

  scanline_params_t clampedParams = effectsRetro_clampScanlineParams(params);

  switch (clampedParams.mode) {
  case SCANLINE_CLASSIC:
    return effectsRetro_applyClassicScanline(pixel, &clampedParams, row);
  case SCANLINE_ANIMATED:
    return effectsRetro_applyAnimatedScanline(pixel, &clampedParams, row);
  case SCANLINE_CURVE:
    return effectsRetro_applyCurvedScanline(pixel, &clampedParams, 0, row);
  default:
    return pixel;
  }
}

/**
 * @brief Apply scanline effect to a scanline
 * @param pixels Array of RGB565 pixels
 * @param width Number of pixels in scanline
 * @param row Current row number
 * @param params Scanline parameters
 * 
 * Applies scanline effects to an entire scanline of pixels. This function
 * provides efficient batch processing for scanline effects during display
 * rendering.
 */
void effectsRetro_applyScanlineToScanline(uint16_t *pixels, int width, int row,
                                           const scanline_params_t *params) {
  if (!pixels || !params) {
    return;
  }

  for (int i = 0; i < width; i++) {
    pixels[i] = effectsRetro_applyScanline(pixels[i], params, row);
  }
}

/**
 * @brief Calculate animated scanline offset based on time
 * @param params Scanline parameters
 * @return Current scanline offset in pixels
 */
static int
effectsRetro_getAnimatedScanlineOffset(const scanline_params_t *params) {
  if (!scanlineAnimationEnabled || !params) {
    return 0;
  }

  unsigned long currentTime = millis();
  unsigned long elapsed = currentTime - scanlineStartTime;

  float pixelsPerMs = params->speed / 1000.0f;
  int totalOffset = (int)(elapsed * pixelsPerMs);

  return totalOffset % 2;
}

/**
 * @brief Apply animated scanline effect
 * @param pixel Original RGB565 pixel
 * @param params Scanline parameters
 * @param row Current row number
 * @return Animated scanline-processed RGB565 pixel
 * 
 * Applies animated scanline effects to a pixel with moving scanlines.
 * This function creates dynamic scanline effects that move across the
 * display over time, simulating CRT monitor characteristics.
 */
uint16_t effectsRetro_applyAnimatedScanline(uint16_t pixel,
                                             const scanline_params_t *params,
                                             int row) {
  if (!params) {
    return pixel;
  }

  // Get animated scanline offset (matching original implementation)
  int offset = effectsRetro_getAnimatedScanlineOffset(params);

  // Apply scanline effect with animation
  // Darken every other scanline, but with animated offset
  if ((row + offset) % 2 == 0) {
    return effectsTints_blendColors(pixel, TINT_BLACK, params->intensity);
  }

  return pixel;
}

/**
 * @brief Apply curved scanline effect
 * @param pixel Original RGB565 pixel
 * @param params Scanline parameters
 * @param x X coordinate of the pixel
 * @param row Current row number
 * @return Curved scanline-processed RGB565 pixel
 * 
 * Applies curved scanline effects to a pixel with center-focused curvature.
 * This function creates scanlines that are more intense at the edges and
 * less intense at the center, simulating CRT monitor curvature.
 */
uint16_t effectsRetro_applyCurvedScanline(uint16_t pixel,
                                           const scanline_params_t *params,
                                           int x, int row) {
  if (!params) {
    return pixel;
  }

  // Calculate curved scanline effect
  float centerX = 64.0f; // Assuming 128px width
  float distanceFromCenter = abs(x - centerX);
  float curveFactor = 1.0f - (distanceFromCenter / centerX) * 0.3f;

  // Apply scanline with curve
  if (row % 2 == 0) {
    float scanlineIntensity = params->intensity * curveFactor;
    return effectsTints_blendColors(pixel, TINT_BLACK, scanlineIntensity);
  }

  return pixel;
}

static uint16_t
effectsRetro_applyClassicScanline(uint16_t pixel,
                                  const scanline_params_t *params, int row) {
  if (!params) {
    return pixel;
  }

  // Classic scanline: darken every other row
  if (row % 2 == 0) {
    return effectsTints_blendColors(pixel, TINT_BLACK, params->intensity);
  }

  return pixel;
}

//==============================================================================
// DITHERING EFFECT IMPLEMENTATIONS
//==============================================================================

/**
 * @brief Apply Bayer dithering to a pixel
 * @param pixel Original RGB565 pixel
 * @param params Dithering parameters
 * @param x X coordinate of the pixel
 * @param y Y coordinate of the pixel
 * @return Dithered RGB565 pixel
 * 
 * Applies Bayer dithering effects to a single pixel for color reduction
 * and retro visual effects. This function implements ordered dithering
 * algorithms to create authentic retro gaming console aesthetics.
 */
uint16_t effectsRetro_applyBayerDithering(uint16_t pixel,
                                           const dither_params_t *params, int x,
                                           int y) {
  if (!params) {
    return pixel;
  }

  dither_params_t clampedParams = effectsRetro_clampDitherParams(params);

  // Get Bayer threshold for this position (normalized 0.0-1.0)
  float threshold = effectsRetro_getBayerThreshold(x, y, clampedParams.mode);

  // Extract RGB components
  uint8_t r = (pixel >> 11) & 0x1F;
  uint8_t g = (pixel >> 5) & 0x3F;
  uint8_t b = pixel & 0x1F;

  // Apply dithering to each component
  r = effectsRetro_quantizeColorComponent(r, 31, clampedParams.quantization,
                                          threshold);
  g = effectsRetro_quantizeColorComponent(g, 63, clampedParams.quantization,
                                          threshold);
  b = effectsRetro_quantizeColorComponent(b, 31, clampedParams.quantization,
                                          threshold);

  // Apply intensity (blend with original)
  float intensity = clampedParams.intensity;
  r = (uint8_t)(r * intensity + ((pixel >> 11) & 0x1F) * (1.0f - intensity));
  g = (uint8_t)(g * intensity + ((pixel >> 5) & 0x3F) * (1.0f - intensity));
  b = (uint8_t)(b * intensity + (pixel & 0x1F) * (1.0f - intensity));

  return (r << 11) | (g << 5) | b;
}

/**
 * @brief Apply Bayer dithering to a scanline
 * @param pixels Array of RGB565 pixels
 * @param width Number of pixels in scanline
 * @param row Current row number
 * @param params Dithering parameters
 * 
 * Applies Bayer dithering effects to an entire scanline of pixels.
 * This function provides efficient batch processing for dithering
 * effects during display rendering.
 */
void effectsRetro_applyBayerDitheringToScanline(uint16_t *pixels, int width,
                                                 int row,
                                                 const dither_params_t *params) {
  if (!pixels || !params) {
    return;
  }

  for (int i = 0; i < width; i++) {
    pixels[i] = effectsRetro_applyBayerDithering(pixels[i], params, i, row);
  }
}

/**
 * @brief Get Bayer dither threshold for a position
 * @param x X coordinate
 * @param y Y coordinate
 * @param mode Dither mode
 * @return Normalized threshold value (0.0-1.0)
 * 
 * Calculates the Bayer dithering threshold value for a given position
 * and dithering mode. This function provides the mathematical foundation
 * for ordered dithering algorithms used in retro visual effects.
 */
float effectsRetro_getBayerThreshold(int x, int y, DitherMode mode) {
  int threshold = 0;
  int maxValue = 0;

  switch (mode) {
  case DITHER_2X2:
    threshold = bayer2x2[y % 2][x % 2];
    maxValue = 3;
    break;
  case DITHER_4X4:
    threshold = bayer4x4[y % 4][x % 4];
    maxValue = 15;
    break;
  case DITHER_8X8:
    threshold = bayer8x8[y % 8][x % 8];
    maxValue = 63;
    break;
  default:
    return 0.0f;
  }

  return (float)threshold / (float)maxValue;
}

/**
 * @brief Quantize a color component to reduce color depth
 * @param value Original color component
 * @param maxValue Maximum value for this component
 * @param levels Number of quantization levels
 * @param threshold Bayer threshold for dithering (0.0-1.0)
 * @return Quantized color component
 * 
 * Quantizes a color component to reduce color depth with dithering support.
 * This function implements color quantization algorithms used in retro
 * visual effects to create authentic color reduction.
 */
uint8_t effectsRetro_quantizeColorComponent(uint8_t value, uint8_t maxValue,
                                             int levels, float threshold) {
  if (levels <= 1) {
    return 0;
  }
  if (levels >= maxValue) {
    return value;
  }

  // Apply dithering threshold
  float normalized = (float)value / (float)maxValue;
  normalized += (threshold - 0.5f) * 0.1f; // Small dithering effect
  normalized = constrain(normalized, 0.0f, 1.0f);

  // Quantize
  int quantized = (int)(normalized * (levels - 1) + 0.5f);
  return (uint8_t)((quantized * maxValue) / (levels - 1));
}

//==============================================================================
// GLITCH EFFECT IMPLEMENTATIONS
//==============================================================================

/**
 * @brief Apply CRT glitch effects to a scanline
 * @param pixels Array of RGB565 pixels
 * @param width Number of pixels in scanline
 * @param row Current row number
 * @param params Glitch parameters
 * 
 * Applies CRT glitch effects to an entire scanline of pixels. This function
 * creates authentic CRT monitor glitch effects including horizontal jitter
 * and random artifacts for retro visual aesthetics.
 */
void effectsRetro_applyCRTGlitches(uint16_t *pixels, int width, int row,
                                    const glitch_params_t *params) {
  if (!pixels || !params) {
    return;
  }

  glitch_params_t clampedParams = effectsRetro_clampGlitchParams(params);
  if (clampedParams.mode == GLITCH_NONE) {
    return;
  }

  // Update glitch seed with row number (matching original implementation)
  glitchSeed ^= (row * 7919);

  // Check probability (matching original implementation)
  float random = (float)(effectsRetro_fastRandom() % 1000) / 1000.0f;
  if (random > clampedParams.probability) {
    return;
  }

  // Determine jitter intensity based on mode (matching original implementation)
  int jitterIntensity = 0;
  switch (clampedParams.mode) {
  case GLITCH_LIGHT:
    jitterIntensity = 1;
    break;
  case GLITCH_MEDIUM:
    jitterIntensity = 2;
    break;
  case GLITCH_HEAVY:
    jitterIntensity = 3;
    break;
  default:
    return;
  }

  // Apply horizontal jitter (matching original implementation)
  effectsRetro_applyHorizontalJitter(pixels, width, jitterIntensity);
}

/**
 * @brief Apply horizontal jitter to pixel array for glitch effects
 * @param pixels Array of RGB565 pixels for current scanline
 * @param width Number of pixels in the scanline
 * @param intensity Maximum shift amount in pixels
 * 
 * Applies horizontal jitter effects to a pixel array by shifting pixels
 * horizontally with random amounts. This function creates authentic CRT
 * monitor glitch effects with horizontal displacement.
 */
void effectsRetro_applyHorizontalJitter(uint16_t *pixels, int width,
                                         int intensity) {
  if (intensity <= 0) {
    return;
  }

  int shift = (effectsRetro_fastRandom() % (intensity * 2 + 1)) - intensity;
  if (shift == 0) {
    return;
  }

  static uint16_t tempBuffer[128];

  for (int i = 0; i < width; i++) {
    int sourceIndex = i - shift;

    while (sourceIndex < 0) {
      sourceIndex += width;
    }
    while (sourceIndex >= width) {
      sourceIndex -= width;
    }

    tempBuffer[i] = pixels[sourceIndex];
  }

  for (int i = 0; i < width; i++) {
    pixels[i] = tempBuffer[i];
  }
}

// Removed - using simpler applyHorizontalJitter approach instead

//==============================================================================
// UTILITY FUNCTIONS
//==============================================================================

/**
 * @brief Fast random number generator for glitch effects
 * @return Pseudo-random number between 0 and 32767
 * 
 * Generates pseudo-random numbers using a linear congruential generator
 * optimized for glitch effects. This function provides fast random number
 * generation for CRT glitch effects and other retro visual effects.
 */
uint16_t effectsRetro_fastRandom(void) {
  glitchSeed = glitchSeed * 1103515245 + 12345;
  return (glitchSeed >> 16) & 0x7FFF;
}

//==============================================================================
// PARAMETER VALIDATION
//==============================================================================

/**
 * @brief Validate scanline parameters
 * @param params Scanline parameters to validate
 * @return true if parameters are valid
 * 
 * Validates scanline parameters to ensure they are within acceptable
 * ranges for safe effect application. This function provides parameter
 * validation for scanline effects.
 */
bool effectsRetro_validateScanlineParams(const scanline_params_t *params) {
  if (!params) {
    return false;
  }

  return params->intensity >= 0.0f && params->intensity <= 1.0f &&
         params->speed >= 0.1f && params->speed <= 10.0f;
}

/**
 * @brief Validate dithering parameters
 * @param params Dithering parameters to validate
 * @return true if parameters are valid
 * 
 * Validates dithering parameters to ensure they are within acceptable
 * ranges for safe effect application. This function provides parameter
 * validation for dithering effects.
 */
bool effectsRetro_validateDitherParams(const dither_params_t *params) {
  if (!params) {
    return false;
  }

  return params->intensity >= 0.0f && params->intensity <= 1.0f &&
         params->quantization >= 2 && params->quantization <= 16;
}

/**
 * @brief Validate glitch parameters
 * @param params Glitch parameters to validate
 * @return true if parameters are valid
 * 
 * Validates glitch parameters to ensure they are within acceptable
 * ranges for safe effect application. This function provides parameter
 * validation for glitch effects.
 */
bool effectsRetro_validateGlitchParams(const glitch_params_t *params) {
  if (!params) {
    return false;
  }

  return params->probability >= 0.001f && params->probability <= 0.1f;
}

/**
 * @brief Clamp scanline parameters to valid ranges
 * @param params Scanline parameters to clamp
 * @return Clamped scanline parameters
 * 
 * Clamps scanline parameters to valid ranges to ensure safe effect
 * application. This function provides parameter safety for scanline effects.
 */
scanline_params_t
 effectsRetro_clampScanlineParams(const scanline_params_t *params) {
  scanline_params_t clamped = *params;

  clamped.intensity = constrain(clamped.intensity, 0.0f, 1.0f);
  clamped.speed = constrain(clamped.speed, 0.1f, 10.0f);

  return clamped;
}

/**
 * @brief Clamp dithering parameters to valid ranges
 * @param params Dithering parameters to clamp
 * @return Clamped dithering parameters
 * 
 * Clamps dithering parameters to valid ranges to ensure safe effect
 * application. This function provides parameter safety for dithering effects.
 */
dither_params_t effectsRetro_clampDitherParams(const dither_params_t *params) {
  dither_params_t clamped = *params;

  clamped.intensity = constrain(clamped.intensity, 0.0f, 1.0f);
  clamped.quantization = constrain(clamped.quantization, 2, 16);

  return clamped;
}

/**
 * @brief Clamp glitch parameters to valid ranges
 * @param params Glitch parameters to clamp
 * @return Clamped glitch parameters
 * 
 * Clamps glitch parameters to valid ranges to ensure safe effect
 * application. This function provides parameter safety for glitch effects.
 */
glitch_params_t effectsRetro_clampGlitchParams(const glitch_params_t *params) {
  glitch_params_t clamped = *params;

  clamped.probability = constrain(clamped.probability, 0.001f, 0.1f);

  return clamped;
}

//==============================================================================
// DEFAULT PARAMETERS
//==============================================================================

/**
 * @brief Get default scanline parameters
 * @return Default scanline parameters
 * 
 * Returns a set of default parameters for scanline effects. These parameters
 * provide a good starting point for scanline effects and can be customized
 * as needed for specific applications.
 */
scanline_params_t effectsRetro_getDefaultScanlineParams(void) {
  scanline_params_t params;
  params.mode = SCANLINE_CLASSIC;
  params.intensity = 0.5f;
  params.speed = 2.0f;
  return params;
}

/**
 * @brief Get default dithering parameters
 * @return Default dithering parameters
 * 
 * Returns a set of default parameters for dithering effects. These parameters
 * provide a good starting point for dithering effects and can be customized
 * as needed for specific applications.
 */
dither_params_t effectsRetro_getDefaultDitherParams(void) {
  dither_params_t params;
  params.mode = DITHER_8X8;        // Bayer dithering pattern
  params.intensity = 1.0f;         // 100% intensity
  params.quantization = 3;         // 3-bit color quantization
  return params;
}

/**
 * @brief Get default glitch parameters
 * @return Default glitch parameters
 * 
 * Returns a set of default parameters for glitch effects. These parameters
 * provide a good starting point for glitch effects and can be customized
 * as needed for specific applications.
 */
glitch_params_t effectsRetro_getDefaultGlitchParams(void) {
  glitch_params_t params;
  params.mode = GLITCH_HEAVY;
  params.probability = 0.08f;
  return params;
}
