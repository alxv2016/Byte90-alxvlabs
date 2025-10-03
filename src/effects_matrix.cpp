/**
 * @file effects_matrix.cpp
 * @brief Implementation of matrix effects: dot matrix and pixelation effects
 *
 * Provides comprehensive matrix-based visual effects for the BYTE-90 device,
 * including dot matrix displays and pixelation effects for retro gaming
 * console aesthetics and artistic visual processing.
 *
 * This module handles:
 * - Dot matrix effects with square and circular dot patterns
 * - Pixelation effects with block-based color averaging
 * - Color quantization and depth reduction for retro aesthetics
 * - Dot pattern generation and visibility calculations
 * - Block-based pixel processing for efficient pixelation
 * - Parameter validation and clamping for safe effect application
 * - Default parameter management for all matrix effects
 * - Integration with retro effects and color manipulation systems
 * - Performance-optimized matrix calculations and pattern generation
 */

#include "effects_matrix.h"
#include "effects_common.h"
#include "effects_retro.h"
#include "effects_tints.h"
#include <Arduino.h>

//==============================================================================
// MATRIX EFFECTS INITIALIZATION
//==============================================================================

/**
 * @brief Initialize matrix effects system
 * 
 * Initializes the matrix effects system for dot matrix and pixelation
 * effects. This function should be called during system initialization
 * to prepare the matrix effects for operation.
 */
void effectsMatrix_init(void) {
   // Initialize matrix effects system
 }

//==============================================================================
// DOT MATRIX EFFECT IMPLEMENTATIONS
//==============================================================================

/**
 * @brief Apply dot matrix effect to a scanline
 * @param pixels Array of RGB565 pixels
 * @param width Number of pixels in scanline
 * @param row Current row number
 * @param params Dot matrix parameters
 * 
 * Applies dot matrix effects to an entire scanline of pixels. This function
 * creates a dot matrix display effect by darkening pixels between dots
 * and keeping dot pixels bright, simulating retro dot matrix displays.
 */
void effectsMatrix_applyDotMatrixEffect(uint16_t *pixels, int width, int row,
                                         const dot_matrix_params_t *params) {
  if (!pixels || !params) {
    return;
  }

  for (int i = 0; i < width; i++) {
    pixels[i] = effectsMatrix_applyDotMatrixToPixel(pixels[i], params, i, row);
  }
}

/**
 * @brief Apply dot matrix effect to a single pixel
 * @param pixel Original RGB565 pixel
 * @param params Dot matrix parameters
 * @param x X coordinate of the pixel
 * @param y Y coordinate of the pixel
 * @return Dot matrix processed RGB565 pixel
 * 
 * Applies dot matrix effects to a single pixel based on the specified
 * parameters and position. This function creates authentic dot matrix
 * display effects by darkening pixels between dots and keeping dot
 * pixels bright.
 */
uint16_t effectsMatrix_applyDotMatrixToPixel(uint16_t pixel,
                                              const dot_matrix_params_t *params,
                                              int x, int y) {
  if (!params) {
    return pixel;
  }

  dot_matrix_params_t clampedParams =
      effectsMatrix_clampDotMatrixParams(params);

  if (clampedParams.mode == DOT_MATRIX_NONE ||
      clampedParams.intensity <= 0.0f) {
    return pixel;
  }

  // Extract RGB components
  uint8_t r = (pixel >> 11) & 0x1F;
  uint8_t g = (pixel >> 5) & 0x3F;
  uint8_t b = pixel & 0x1F;

  // Apply quantization first (matching original implementation)
  r = effectsRetro_quantizeColorComponent(r, 31, clampedParams.quantization,
                                          0.5f);
  g = effectsRetro_quantizeColorComponent(g, 63, clampedParams.quantization * 2,
                                          0.5f);
  b = effectsRetro_quantizeColorComponent(b, 31, clampedParams.quantization,
                                          0.5f);

  if (!effectsMatrix_isDotPixel(x, y, clampedParams.dotSize,
                                clampedParams.mode)) {
    // This pixel should be darkened by the dot matrix

    // Use more stable brightness detection - check individual components
    // (matching original)
    bool isVeryBright =
        (r >= 28 && g >= 56 &&
         b >= 28); // Fixed thresholds instead of calculated brightness

    // Apply darkening with stable logic (matching original)
    float darkening;
    if (isVeryBright) {
      // Very bright pixels: reduced darkening
      darkening = 1.0f - (clampedParams.intensity * 0.8f);
    } else {
      // All other pixels: full dot matrix darkening
      darkening = 1.0f - clampedParams.intensity;
    }

    r = (uint8_t)(r * darkening);
    g = (uint8_t)(g * darkening);
    b = (uint8_t)(b * darkening);
  }
  // Visible dot matrix pixels remain unchanged (stay bright)

  return (r << 11) | (g << 5) | b;
}

/**
 * @brief Check if pixel should be a dot in matrix pattern
 * @param x X coordinate of the pixel
 * @param y Y coordinate of the pixel
 * @param dotSize Size of each dot
 * @param mode Dot matrix mode
 * @return true if pixel should be a dot
 */
bool effectsMatrix_isDotPixel(int x, int y, int dotSize, DotMatrixMode mode) {
  if (dotSize < 1) {
    dotSize = 1;
  }
  if (dotSize > 8) {
    dotSize = 8;
  }

  int gridSize = dotSize + 0.5;

  int localX = x % gridSize;
  int localY = y % gridSize;

  int dotCenterX = dotSize / 2;
  int dotCenterY = dotSize / 2;

  switch (mode) {
  case DOT_MATRIX_SQUARE: {
    float border = (dotSize > 2) ? 1.0f : 0.4f;
    return (localX >= border && localX < dotSize - border && localY >= border &&
            localY < dotSize - border);
  }

  case DOT_MATRIX_CIRCLE: {
    float dx = localX - dotCenterX;
    float dy = localY - dotCenterY;
    float distance = sqrt(dx * dx + dy * dy);
    float radius = (dotSize - 1) / 2.0f;
    return distance <= radius;
  }

  default:
    return true;
  }
}

//==============================================================================
// PIXELATION EFFECT IMPLEMENTATIONS
//==============================================================================

/**
 * @brief Apply pixelation effect to a scanline
 * @param pixels Array of RGB565 pixels
 * @param width Number of pixels in scanline
 * @param row Current row number
 * @param params Pixelation parameters
 * 
 * Applies pixelation effects to an entire scanline of pixels. This function
 * creates pixelated visual effects by grouping pixels into blocks and
 * averaging their colors, simulating low-resolution displays or artistic
 * pixelation effects.
 */
void effectsMatrix_applyPixelateEffect(uint16_t *pixels, int width, int row,
                                        const pixelate_params_t *params) {
  if (!pixels || !params) {
    return;
  }

  pixelate_params_t clampedParams = effectsMatrix_clampPixelateParams(params);

  // Apply pixelation based on mode
  switch (clampedParams.mode) {
  case PIXELATE_SQUARE:
    // Apply square pixelation using block-based processing
    effectsMatrix_applySquarePixelateToScanline(pixels, width, row,
                                                &clampedParams);
    break;
  default:
    // No pixelation
    break;
  }
}

/**
 * @brief Apply square pixelation to a scanline using block-based processing
 * @param pixels Array of RGB565 pixels
 * @param width Number of pixels in scanline
 * @param row Current row number
 * @param params Pixelation parameters
 * 
 * Applies square pixelation effects to a scanline using block-based processing.
 * This function creates pixelated visual effects by grouping pixels into
 * square blocks and averaging their colors for authentic pixelation effects.
 */
void effectsMatrix_applySquarePixelateToScanline(
     uint16_t *pixels, int width, int row, const pixelate_params_t *params) {
  if (!pixels || !params) {
    return;
  }

  int blockSize = params->blockSize;
  if (blockSize <= 1) {
    return; // No pixelation
  }

  // Process each block in the scanline
  for (int blockX = 0; blockX < width; blockX += blockSize) {
    // Calculate block average color
    uint32_t totalR = 0, totalG = 0, totalB = 0;
    int pixelCount = 0;

    // Sum all pixels in this block
    for (int i = 0; i < blockSize && (blockX + i) < width; i++) {
      uint16_t pixel = pixels[blockX + i];
      totalR += (pixel >> 11) & 0x1F;
      totalG += (pixel >> 5) & 0x3F;
      totalB += pixel & 0x1F;
      pixelCount++;
    }

    if (pixelCount == 0)
      continue;

    // Calculate average color
    uint8_t avgR = totalR / pixelCount;
    uint8_t avgG = totalG / pixelCount;
    uint8_t avgB = totalB / pixelCount;
    uint16_t avgColor = (avgR << 11) | (avgG << 5) | avgB;

    // Apply average color to all pixels in the block
    for (int i = 0; i < blockSize && (blockX + i) < width; i++) {
      if (params->intensity >= 1.0f) {
        pixels[blockX + i] = avgColor;
      } else {
        // Blend with original pixel based on intensity
        pixels[blockX + i] = effectsTints_blendColors(
            pixels[blockX + i], avgColor, params->intensity);
      }
    }
  }
}

//==============================================================================
// UTILITY FUNCTIONS
//==============================================================================

/**
 * @brief Calculate distance from center of dot
 * @param x X coordinate of the pixel
 * @param y Y coordinate of the pixel
 * @param dotCenterX X coordinate of dot center
 * @param dotCenterY Y coordinate of dot center
 * @return Distance from dot center
 * 
 * Calculates the Euclidean distance from a pixel to the center of a dot.
 * This function provides the mathematical foundation for circular dot
 * matrix patterns and radius-based visibility calculations.
 */
float effectsMatrix_calculateDotDistance(int x, int y, int dotCenterX,
                                          int dotCenterY) {
  float dx = x - dotCenterX;
  float dy = y - dotCenterY;
  return sqrt(dx * dx + dy * dy);
}

/**
 * @brief Check if pixel is within dot radius
 * @param x X coordinate of the pixel
 * @param y Y coordinate of the pixel
 * @param dotCenterX X coordinate of dot center
 * @param dotCenterY Y coordinate of dot center
 * @param radius Dot radius
 * @return true if pixel is within dot radius
 * 
 * Determines whether a pixel is within the specified radius of a dot center.
 * This function provides radius-based visibility calculations for circular
 * dot matrix patterns and other radius-based effects.
 */
bool effectsMatrix_isWithinDotRadius(int x, int y, int dotCenterX,
                                      int dotCenterY, float radius) {
  float distance =
      effectsMatrix_calculateDotDistance(x, y, dotCenterX, dotCenterY);
  return distance <= radius;
}

//==============================================================================
// PARAMETER VALIDATION
//==============================================================================

/**
 * @brief Clamp dot matrix parameters to valid ranges
 * @param params Dot matrix parameters to clamp
 * @return Clamped dot matrix parameters
 * 
 * Clamps dot matrix parameters to valid ranges to ensure safe effect
 * application. This function provides parameter safety for dot matrix effects.
 */
dot_matrix_params_t
 effectsMatrix_clampDotMatrixParams(const dot_matrix_params_t *params) {
  dot_matrix_params_t clamped = *params;

  clamped.intensity = constrain(clamped.intensity, 0.0f, 1.0f);
  clamped.dotSize = constrain(clamped.dotSize, 1, 8);
  clamped.quantization = constrain(clamped.quantization, 2, 16);

  return clamped;
}

/**
 * @brief Clamp pixelation parameters to valid ranges
 * @param params Pixelation parameters to clamp
 * @return Clamped pixelation parameters
 * 
 * Clamps pixelation parameters to valid ranges to ensure safe effect
 * application. This function provides parameter safety for pixelation effects.
 */
pixelate_params_t
 effectsMatrix_clampPixelateParams(const pixelate_params_t *params) {
  pixelate_params_t clamped = *params;

  clamped.intensity = constrain(clamped.intensity, 0.0f, 1.0f);
  clamped.blockSize = constrain(clamped.blockSize, 2, 16);

  return clamped;
}

//==============================================================================
// DEFAULT PARAMETERS
//==============================================================================

/**
 * @brief Get default dot matrix parameters
 * @return Default dot matrix parameters
 * 
 * Returns a set of default parameters for dot matrix effects. These parameters
 * provide a good starting point for dot matrix effects and can be customized
 * as needed for specific applications.
 */
dot_matrix_params_t effectsMatrix_getDefaultDotMatrixParams(void) {
  dot_matrix_params_t params;
  params.mode = DOT_MATRIX_SQUARE;
  params.intensity = 1.0f;
  params.dotSize = 4;
  params.quantization = 4;
  return params;
}

/**
 * @brief Get default pixelation parameters
 * @return Default pixelation parameters
 * 
 * Returns a set of default parameters for pixelation effects. These parameters
 * provide a good starting point for pixelation effects and can be customized
 * as needed for specific applications.
 */
pixelate_params_t effectsMatrix_getDefaultPixelateParams(void) {
  pixelate_params_t params;
  params.mode = PIXELATE_SQUARE;
  params.intensity = 0.9f;
  params.blockSize = 3;
  return params;
}
