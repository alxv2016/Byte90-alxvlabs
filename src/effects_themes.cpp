/**
 * @file effects_themes.cpp
 * @brief Implementation of theme definitions and palette management
 *
 * Provides comprehensive theme and palette management for the BYTE-90 device,
 * including retro gaming console color palettes and advanced dithering algorithms.
 *
 * This module handles:
 * - Four-color palette definitions for retro gaming consoles
 * - Palette management and validation functions
 * - Bayer dithering matrices for smooth color transitions
 * - Pixel brightness calculation and color mapping
 * - Theme-based color palette application
 * - Dithering threshold calculations for various pattern sizes
 * - Retro gaming console color schemes (Game Boy, Monochrome)
 * - Performance-optimized palette lookups and applications
 * - Color space management and palette validation
 */

#include "effects_themes.h"
#include <Arduino.h>

//==============================================================================
// PALETTE DEFINITIONS
//==============================================================================

// 4-color palette definitions
static const FourColorPalette FOUR_COLOR_PALETTES[PALETTE_COUNT] = {
    // PALETTE_GAMEBOY
    {GAMEBOY_700, GAMEBOY_600, GAMEBOY_500, GAMEBOY_400},
    // PALETTE_MONOCHROME
    {MONOCHROME_700, MONOCHROME_600, MONOCHROME_500, MONOCHROME_400}};

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
// PALETTE MANAGEMENT
//==============================================================================

/**
 * @brief Get palette by type
 * @param type Palette type
 * @return Pointer to palette structure, or NULL if not found
 * 
 * Retrieves a four-color palette structure based on the specified palette type.
 * This function provides access to predefined retro gaming console color palettes
 * for theme-based visual effects.
 */
const FourColorPalette *effectsThemes_getPalette(PaletteType type) {
  if (type >= 0 && type < PALETTE_COUNT) {
    return &FOUR_COLOR_PALETTES[type];
  }
  return NULL;
}

/**
 * @brief Apply palette to a pixel
 * @param pixel Original RGB565 pixel
 * @param paletteType Type of palette to apply
 * @param intensity Effect intensity (0.0-1.0)
 * @param x X coordinate for dithering (optional, default 0)
 * @param y Y coordinate for dithering (optional, default 0)
 * @return Palette styled RGB565 pixel
 * 
 * Applies a four-color palette to a single pixel with optional dithering.
 * This function converts full-color pixels to retro gaming console color
 * schemes with smooth transitions and authentic dithering effects.
 */
uint16_t effectsThemes_applyPalette(uint16_t pixel, PaletteType paletteType,
                                     float intensity, int x, int y) {
  const FourColorPalette *palette = effectsThemes_getPalette(paletteType);
  if (!palette) {
    return pixel;
  }

  // Get pixel brightness
  float brightness = getPixelBrightness(pixel);

  // Apply dithering if coordinates provided
  if (x >= 0 && y >= 0) {
    int threshold = effectsThemes_getBayerThreshold(x, y, 8);
    brightness += (threshold - 32) / 256.0f;
    brightness = constrain(brightness, 0.0f, 1.0f);
  }

  // Select palette color based on brightness
  uint16_t paletteColor;
  if (brightness < 0.25f) {
    paletteColor = palette->darkest;
  } else if (brightness < 0.5f) {
    paletteColor = palette->dark;
  } else if (brightness < 0.75f) {
    paletteColor = palette->light;
  } else {
    paletteColor = palette->lightest;
  }

  // Blend with original pixel based on intensity
  // TODO: Replace with proper blend function when effects_tints is implemented
  return paletteColor;
}

/**
 * @brief Get palette name
 * @param type Palette type
 * @return Pointer to palette name string
 * 
 * Retrieves the human-readable name for a specified palette type.
 * This function provides user-friendly palette names for display
 * in menus and user interfaces.
 */
const char *effectsThemes_getPaletteName(PaletteType type) {
  static const char *paletteNames[] = {"GAME BOY", "MONOCHROME"};

  if (type >= 0 && type < PALETTE_COUNT) {
    return paletteNames[type];
  }
  return "UNKNOWN";
}

/**
 * @brief Check if palette type is valid
 * @param type Palette type to validate
 * @return true if valid
 * 
 * Validates whether a palette type is within the valid range of
 * available palettes. This function provides a simple way to check
 * palette type validity before using palette functions.
 */
bool effectsThemes_isValidPalette(PaletteType type) {
  return (type >= 0 && type < PALETTE_COUNT);
}

//==============================================================================
// DITHERING UTILITIES
//==============================================================================

/**
 * @brief Get Bayer dither threshold
 * @param x X coordinate
 * @param y Y coordinate
 * @param patternSize Dither pattern size
 * @return Dither threshold value
 * 
 * Calculates the Bayer dithering threshold value for a given position
 * and pattern size. This function provides the mathematical foundation
 * for ordered dithering algorithms used in retro visual effects.
 */
int effectsThemes_getBayerThreshold(int x, int y, int patternSize) {
  x = x % patternSize;
  y = y % patternSize;

  switch (patternSize) {
  case 2:
    return bayer2x2[y][x];
  case 4:
    return bayer4x4[y][x];
  case 8:
    return bayer8x8[y][x];
  default:
    return 0;
  }
}
