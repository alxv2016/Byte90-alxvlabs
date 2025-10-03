/**
 * @file effects_themes.h
 * @brief Theme definitions and palette management
 */

#ifndef EFFECTS_THEMES_H
#define EFFECTS_THEMES_H

#include "effects_common.h"

//==============================================================================
// PALETTE MANAGEMENT
//==============================================================================

/**
 * @brief Get palette by type
 * @param type Palette type
 * @return Pointer to palette structure, or NULL if not found
 */
const FourColorPalette* effectsThemes_getPalette(PaletteType type);

/**
 * @brief Apply palette to a pixel
 * @param pixel Original RGB565 pixel
 * @param paletteType Type of palette to apply
 * @param intensity Effect intensity (0.0-1.0)
 * @param x X coordinate for dithering (optional, default 0)
 * @param y Y coordinate for dithering (optional, default 0)
 * @return Palette styled RGB565 pixel
 */
uint16_t effectsThemes_applyPalette(uint16_t pixel, PaletteType paletteType, float intensity, int x = 0, int y = 0);

/**
 * @brief Get palette name
 * @param type Palette type
 * @return Pointer to palette name string
 */
const char* effectsThemes_getPaletteName(PaletteType type);

/**
 * @brief Check if palette type is valid
 * @param type Palette type to validate
 * @return true if valid
 */
bool effectsThemes_isValidPalette(PaletteType type);

//==============================================================================
// DITHERING UTILITIES
//==============================================================================

/**
 * @brief Get Bayer dither threshold
 * @param x X coordinate
 * @param y Y coordinate
 * @param patternSize Dither pattern size
 * @return Dither threshold value
 */
int effectsThemes_getBayerThreshold(int x, int y, int patternSize);

#endif /* EFFECTS_THEMES_H */
