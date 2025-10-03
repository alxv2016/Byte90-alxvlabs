/**
 * @file effects_tints.cpp
 * @brief Implementation of color tinting and chromatic aberration effects
 *
 * Provides comprehensive color manipulation and visual effects for the BYTE-90 device,
 * including color tinting, chromatic aberration, and retro palette conversion.
 *
 * This module handles:
 * - Color tinting with selective and threshold-based application
 * - Chromatic aberration effects with directional control
 * - Four-color palette conversion with dithering
 * - Color blending and manipulation utilities
 * - Retro gaming console color palettes (Game Boy, Monochrome)
 * - Advanced dithering algorithms for smooth color transitions
 * - Edge handling for chromatic aberration effects
 * - Performance-optimized color space calculations
 * - Default parameter management for effects
 */

#include "effects_tints.h"
#include "effects_common.h"
#include <Arduino.h>

#ifndef PI
#define PI 3.14159265359f
#endif

//==============================================================================
// TINT EFFECT IMPLEMENTATIONS
//==============================================================================

/**
 * @brief Apply tint effect to a pixel
 * @param pixel Original RGB565 pixel
 * @param params Tint parameters
 * @param x X coordinate of the pixel
 * @param y Y coordinate of the pixel
 * @return Tinted RGB565 pixel
 */
uint16_t effectsTints_applyTint(uint16_t pixel, const tint_params_t *params,
                                 int x, int y) {
  if (!params) {
    return pixel;
  }

  // Handle special palette cases first (matching original implementation)
  if (params->tintColor == GAMEBOY_400) {
    return effectsTints_applyFourColorPalette(pixel, PALETTE_GAMEBOY,
                                              params->intensity, x, y);
  }

  if (params->tintColor == MONOCHROME_400) {
    return effectsTints_applyFourColorPalette(pixel, PALETTE_MONOCHROME,
                                              params->intensity, x, y);
  }

  // Regular tinting logic matching original implementation
  uint16_t tintColor = params->tintColor;
  float intensity = params->intensity;
  float threshold = params->threshold;
  bool selectiveTint = params->selectiveTint;
  float gradualTint = 0.2f;

  uint8_t r = (pixel >> 11) & 0x1F;
  uint8_t g = (pixel >> 5) & 0x3F;
  uint8_t b = pixel & 0x1F;

  float actualIntensity;

  if (selectiveTint) {
    float r_norm = (r * 255.0f) / 31.0f;
    float g_norm = (g * 255.0f) / 63.0f;
    float b_norm = (b * 255.0f) / 31.0f;

    float brightness =
        (0.299f * r_norm + 0.587f * g_norm + 0.114f * b_norm) / 255.0f;

    if (brightness < threshold) {
      actualIntensity = intensity * (brightness / threshold) * gradualTint;
    } else {
      actualIntensity =
          intensity * ((brightness - threshold) / (1.0f - threshold));
    }
  } else {
    if (threshold <= 0.0f) {
      actualIntensity = intensity;
    } else {
      float r_norm = r / 31.0f;
      float g_norm = g / 63.0f;
      float b_norm = b / 31.0f;

      float minComponent = (r_norm < g_norm) ? r_norm : g_norm;
      minComponent = (minComponent < b_norm) ? minComponent : b_norm;

      if (minComponent < threshold) {
        actualIntensity = intensity * (minComponent / threshold) * gradualTint;
      } else {
        actualIntensity = intensity;
      }
    }
  }

  uint8_t tint_r = (tintColor >> 11) & 0x1F;
  uint8_t tint_g = (tintColor >> 5) & 0x3F;
  uint8_t tint_b = tintColor & 0x1F;

  // Check if this is a "dark" tint color (like red) by checking if any
  // component is very low
  bool isDarkTint =
      (tint_g < 8 || tint_b < 8); // Low green or blue suggests red/dark tint

  if (isDarkTint && actualIntensity > 0.7f) {
    // For dark tints at high intensity, use multiplicative blending to
    // eliminate whites
    float tint_r_norm = (float)tint_r / 31.0f;
    float tint_g_norm = (float)tint_g / 63.0f;
    float tint_b_norm = (float)tint_b / 31.0f;

    float r_norm = (float)r / 31.0f;
    float g_norm = (float)g / 63.0f;
    float b_norm = (float)b / 31.0f;

    // Multiply original by tint color (this darkens whites to the tint color)
    r = (uint8_t)((r_norm * tint_r_norm * (1.0f + actualIntensity)) * 31.0f);
    g = (uint8_t)((g_norm * tint_g_norm * (1.0f + actualIntensity)) * 63.0f);
    b = (uint8_t)((b_norm * tint_b_norm * (1.0f + actualIntensity)) * 31.0f);
  } else {
    // Use normal blending for bright tints
    r = (uint8_t)(r * (1.0f - actualIntensity) + tint_r * actualIntensity);
    g = (uint8_t)(g * (1.0f - actualIntensity) + tint_g * actualIntensity);
    b = (uint8_t)(b * (1.0f - actualIntensity) + tint_b * actualIntensity);
  }

  r = (r > 31) ? 31 : r;
  g = (g > 63) ? 63 : g;
  b = (b > 31) ? 31 : b;

  return (r << 11) | (g << 5) | b;
}

/**
 * @brief Apply tint effect to a scanline
 * @param pixels Array of RGB565 pixels
 * @param width Number of pixels in scanline
 * @param row Current row number
 * @param params Tint parameters
 */
void effectsTints_applyTintToScanline(uint16_t *pixels, int width, int row,
                                       const tint_params_t *params) {
  if (!pixels || !params) {
    return;
  }

  for (int i = 0; i < width; i++) {
    pixels[i] = effectsTints_applyTint(pixels[i], params, i, row);
  }
}

//==============================================================================
// CHROMATIC ABERRATION IMPLEMENTATIONS
//==============================================================================

/**
 * @brief Apply chromatic aberration to a scanline
 * @param pixels Array of RGB565 pixels
 * @param width Number of pixels in scanline
 * @param row Current row number
 * @param params Chromatic aberration parameters
 */
void effectsTints_applyChromaticAberration(uint16_t *pixels, int width, int row,
                                            const chromatic_params_t *params) {
  if (!pixels || !params) {
    return;
  }

  if (params->mode == CHROMATIC_NONE || params->intensity <= 0.0f) {
    return;
  }

  // Use local buffers like original implementation
  uint8_t redChannel[128];
  uint8_t greenChannel[128];
  uint8_t blueChannel[128];

  // Extract all channels first
  for (int i = 0; i < width; i++) {
    uint16_t pixel = pixels[i];
    redChannel[i] = (pixel >> 11) & 0x1F;
    greenChannel[i] = (pixel >> 5) & 0x3F;
    blueChannel[i] = pixel & 0x1F;
  }

  // Convert degrees to radians and calculate components
  float radians = params->degrees * PI / 180.0f;
  float cosAngle = cos(radians);
  float sinAngle = sin(radians);

  // Use consistent approach for all angles
  float horizontalComponent = params->intensity * cosAngle;
  float verticalComponent = params->intensity * sinAngle;

  // Calculate base horizontal shift
  int baseRedShift = (int)(horizontalComponent * params->redShift);
  int baseBlueShift = (int)(horizontalComponent * params->blueShift);

  // Calculate vertical variation using sine wave (this prevents artifacts)
  float verticalRedVariation =
      verticalComponent * params->redShift * sin(row * 0.1f);
  float verticalBlueVariation =
      verticalComponent * params->blueShift * sin(row * 0.1f);

  // Combine components
  int redShiftPixels = baseRedShift + (int)verticalRedVariation;
  int blueShiftPixels = baseBlueShift + (int)verticalBlueVariation;

  // Apply shifts with improved edge handling
  for (int i = 0; i < width; i++) {
    uint8_t red = redChannel[i];
    uint8_t blue = blueChannel[i];
    uint8_t green = greenChannel[i];

    // Red channel shift with edge handling
    int redSourceIndex = i - redShiftPixels;
    if (redSourceIndex < 0) {
      red = (uint8_t)(redChannel[0] * 0.5f); // Fade to dark at left edge
    } else if (redSourceIndex >= width) {
      red =
          (uint8_t)(redChannel[width - 1] * 0.5f); // Fade to dark at right edge
    } else {
      red = redChannel[redSourceIndex];
    }

    // Blue channel shift with edge handling
    int blueSourceIndex = i - blueShiftPixels;
    if (blueSourceIndex < 0) {
      blue = (uint8_t)(blueChannel[0] * 0.5f); // Fade to dark at left edge
    } else if (blueSourceIndex >= width) {
      blue = (uint8_t)(blueChannel[width - 1] *
                       0.5f); // Fade to dark at right edge
    } else {
      blue = blueChannel[blueSourceIndex];
    }

    pixels[i] = (red << 11) | (green << 5) | blue;
  }
}

/**
 * @brief Calculate chromatic shift for a pixel
 * @param params Chromatic aberration parameters
 * @param x X coordinate of the pixel
 * @param y Y coordinate of the pixel
 * @param redShiftX Pointer to store red X shift
 * @param redShiftY Pointer to store red Y shift
 * @param blueShiftX Pointer to store blue X shift
 * @param blueShiftY Pointer to store blue Y shift
 */
void effectsTints_calculateChromaticShift(const chromatic_params_t *params,
                                           int x, int y, float *redShiftX,
                                           float *redShiftY, float *blueShiftX,
                                           float *blueShiftY) {
  if (!params || !redShiftX || !redShiftY || !blueShiftX || !blueShiftY) {
    return;
  }

  // Convert degrees to radians
  float angleRad = params->degrees * PI / 180.0f;

  // Calculate shift based on angle and position for more dynamic effect
  float distanceFromCenter = sqrt(x * x + y * y);
  float distanceFactor =
      distanceFromCenter / 100.0f; // Normalize by screen size

  // Calculate shift based on angle with distance factor
  *redShiftX = params->redShift * cos(angleRad) * distanceFactor;
  *redShiftY = params->redShift * sin(angleRad) * distanceFactor;
  *blueShiftX = params->blueShift * cos(angleRad) * distanceFactor;
  *blueShiftY = params->blueShift * sin(angleRad) * distanceFactor;
}

//==============================================================================
// FOUR-COLOR PALETTE SUPPORT
//==============================================================================

// 4-color palette definitions matching original
static const FourColorPalette FOUR_COLOR_PALETTES[PALETTE_COUNT] = {
    // PALETTE_GAMEBOY
    {GAMEBOY_700, GAMEBOY_600, GAMEBOY_500, GAMEBOY_400},
    // PALETTE_MONOCHROME
    {MONOCHROME_700, MONOCHROME_600, MONOCHROME_500, MONOCHROME_400}};

/**
 * @brief Apply 4-color palette with improved blending
 * @param pixel Original RGB565 pixel
 * @param paletteType Type of palette to apply
 * @param intensity Effect intensity (0.0-1.0)
 * @param x X coordinate for dithering (optional)
 * @param y Y coordinate for dithering (optional)
 * @return Palette styled RGB565 pixel
 */
uint16_t effectsTints_applyFourColorPalette(uint16_t pixel,
                                             PaletteType paletteType,
                                             float intensity, int x, int y) {
  if (intensity <= 0.0f || paletteType >= PALETTE_COUNT) {
    return pixel;
  }

  if (intensity >= 1.0f) {
    return effectsTints_convertToFourColorPaletteSmooth(pixel, paletteType, x,
                                                        y);
  }

  // For partial intensity, use simple blending like original implementation
  uint16_t paletteColor =
      effectsTints_convertToFourColorPaletteSmooth(pixel, paletteType, x, y);

  // Extract components for blending
  uint8_t origR = (pixel >> 11) & 0x1F;
  uint8_t origG = (pixel >> 5) & 0x3F;
  uint8_t origB = pixel & 0x1F;

  uint8_t paletteR = (paletteColor >> 11) & 0x1F;
  uint8_t paletteG = (paletteColor >> 5) & 0x3F;
  uint8_t paletteB = paletteColor & 0x1F;

  // Simple linear blending (much faster than gamma-corrected)
  uint8_t finalR = (uint8_t)(origR * (1.0f - intensity) + paletteR * intensity);
  uint8_t finalG = (uint8_t)(origG * (1.0f - intensity) + paletteG * intensity);
  uint8_t finalB = (uint8_t)(origB * (1.0f - intensity) + paletteB * intensity);

  // Ensure values don't exceed limits
  finalR = (finalR > 31) ? 31 : finalR;
  finalG = (finalG > 63) ? 63 : finalG;
  finalB = (finalB > 31) ? 31 : finalB;

  return (finalR << 11) | (finalG << 5) | finalB;
}

/**
 * @brief Convert RGB565 color to 4-color palette with dithered transitions
 * @param pixel Original RGB565 pixel
 * @param paletteType Type of palette to use
 * @param x X coordinate for dithering pattern (optional)
 * @param y Y coordinate for dithering pattern (optional)
 * @return Palette color in RGB565 format
 * 
 * Converts a full-color RGB565 pixel to a four-color palette with advanced
 * dithering for smooth color transitions. This function implements sophisticated
 * dithering algorithms to create authentic retro gaming console effects.
 */
uint16_t effectsTints_convertToFourColorPaletteSmooth(uint16_t pixel,
                                                       PaletteType paletteType,
                                                       int x, int y) {
  if (paletteType >= PALETTE_COUNT) {
    return pixel;
  }

  // Optimized luminance calculation - avoid expensive conversions
  uint8_t r = (pixel >> 11) & 0x1F;
  uint8_t g = (pixel >> 5) & 0x3F;
  uint8_t b = pixel & 0x1F;

  // Calculate luminance directly in RGB565 space (much faster)
  // Approximate luminance: 0.299*R + 0.587*G + 0.114*B
  // Scale to 0.0-1.0 range
  float luminance =
      (0.299f * r / 31.0f + 0.587f * g / 63.0f + 0.114f * b / 31.0f);

  const FourColorPalette &palette = FOUR_COLOR_PALETTES[paletteType];

  // REFINED THRESHOLDS: Make the bright threshold even more aggressive
  const float thresh1 = 0.25f; // darkest -> dark
  const float thresh2 = 0.50f; // dark -> light
  const float thresh3 = 0.60f; // light -> lightest (EVEN LOWER - was 0.65f)

  // VARIABLE dithering ranges - more dithering for the problematic bright area
  const float ditherRange1 = 0.15f; // Normal dithering for lower transitions
  const float ditherRange2 = 0.17f; // Normal dithering for middle transitions
  const float ditherRange3 = 0.50f; // EXTRA dithering for bright transition

  // 4x4 Bayer dithering pattern
  static const int bayer4x4[4][4] = {
      {0, 8, 2, 10}, {12, 4, 14, 6}, {3, 11, 1, 9}, {15, 7, 13, 5}};

  // Get dither value (0-15) and normalize to 0.0-1.0
  int ditherValue = bayer4x4[y % 4][x % 4];
  float normalizedDither = ditherValue / 15.0f; // 0.0 to 1.0

  // First threshold: darkest <-> dark (unchanged)
  if (luminance <= thresh1 - ditherRange1) {
    return palette.darkest;
  } else if (luminance <= thresh1 + ditherRange1) {
    float transitionPos =
        (luminance - (thresh1 - ditherRange1)) / (2.0f * ditherRange1);
    return (normalizedDither < transitionPos) ? palette.darkest : palette.dark;
  }

  // Second threshold: dark <-> light (unchanged)
  else if (luminance <= thresh2 - ditherRange2) {
    return palette.dark;
  } else if (luminance <= thresh2 + ditherRange2) {
    float transitionPos =
        (luminance - (thresh2 - ditherRange2)) / (2.0f * ditherRange2);
    return (normalizedDither < transitionPos) ? palette.dark : palette.light;
  }

  // Third threshold: light <-> lightest (ENHANCED FOR BRIGHT AREAS)
  else if (luminance <= thresh3 - ditherRange3) {
    return palette.light;
  } else if (luminance <= thresh3 + ditherRange3) {
    float transitionPos =
        (luminance - (thresh3 - ditherRange3)) / (2.0f * ditherRange3);

    // SPECIAL HANDLING: Bias toward lightest for very bright pixels
    if (luminance > 0.65f) {
      // For very bright pixels, bias heavily toward lightest
      transitionPos = transitionPos * 0.6f + 0.4f; // Shift range to 0.4-1.0
    }

    return (normalizedDither < transitionPos) ? palette.light
                                              : palette.lightest;
  }

  // Brightest pixels - definitely lightest
  else {
    return palette.lightest;
  }
}

//==============================================================================
// COLOR MANIPULATION UTILITIES
//==============================================================================

/**
 * @brief Blend two colors with given ratio
 * @param color1 First RGB565 color
 * @param color2 Second RGB565 color
 * @param ratio Blend ratio (0.0-1.0, 0.0 = all color1, 1.0 = all color2)
 * @return Blended RGB565 color
 * 
 * Blends two RGB565 colors using linear interpolation based on the
 * provided ratio. This function provides smooth color transitions
 * for various visual effects.
 */
uint16_t effectsTints_blendColors(uint16_t color1, uint16_t color2,
                                   float ratio) {
  // Clamp ratio to valid range
  ratio = constrain(ratio, 0.0f, 1.0f);

  // Extract RGB components from both colors
  uint8_t r1 = (color1 >> 11) & 0x1F;
  uint8_t g1 = (color1 >> 5) & 0x3F;
  uint8_t b1 = color1 & 0x1F;

  uint8_t r2 = (color2 >> 11) & 0x1F;
  uint8_t g2 = (color2 >> 5) & 0x3F;
  uint8_t b2 = color2 & 0x1F;

  // Blend components
  uint8_t r_blend = (uint8_t)(r1 * (1.0f - ratio) + r2 * ratio);
  uint8_t g_blend = (uint8_t)(g1 * (1.0f - ratio) + g2 * ratio);
  uint8_t b_blend = (uint8_t)(b1 * (1.0f - ratio) + b2 * ratio);

  return (r_blend << 11) | (g_blend << 5) | b_blend;
}

/**
 * @brief Apply color tint to a pixel
 * @param pixel Original RGB565 pixel
 * @param tintColor RGB565 tint color
 * @param intensity Tint intensity (0.0-1.0)
 * @return Tinted RGB565 pixel
 * 
 * Applies a simple color tint to a pixel using linear blending.
 * This function provides a straightforward way to tint pixels
 * without the complexity of selective tinting or thresholds.
 */
uint16_t effectsTints_applyColorTint(uint16_t pixel, uint16_t tintColor,
                                      float intensity) {
  return effectsTints_blendColors(pixel, tintColor, intensity);
}

//==============================================================================
// DEFAULT PARAMETERS
//==============================================================================

/**
 * @brief Get default tint parameters
 * @return Default tint parameters
 * 
 * Returns a set of default parameters for tint effects. These parameters
 * provide a good starting point for tint effects and can be customized
 * as needed for specific applications.
 */
tint_params_t effectsTints_getDefaultTintParams(void) {
  tint_params_t params;
  params.tintColor = TINT_GREEN_400;
  params.intensity = 1.0f;
  params.threshold = 0.1f;
  params.selectiveTint = false;
  return params;
}

/**
 * @brief Get default chromatic aberration parameters
 * @return Default chromatic aberration parameters
 * 
 * Returns a set of default parameters for chromatic aberration effects.
 * These parameters provide a good starting point for chromatic aberration
 * effects and can be customized as needed for specific applications.
 */
chromatic_params_t effectsTints_getDefaultChromaticParams(void) {
  chromatic_params_t params;
  params.mode = CHROMATIC_ANGLE;
  params.intensity = 0.6f;  // Match original default
  params.degrees = 90.0f;   // Match original default
  params.redShift = 3.0f;   // Match original default
  params.blueShift = -3.0f; // Match original default
  return params;
}
