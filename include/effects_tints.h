/**
 * @file effects_tints.h
 * @brief Color tinting and chromatic aberration effects
 */

#ifndef EFFECTS_TINTS_H
#define EFFECTS_TINTS_H

#include "effects_common.h"

//==============================================================================
// TINT EFFECT PARAMETERS
//==============================================================================

/**
 * @brief Tint effect parameters
 */
typedef struct {
    uint16_t tintColor;        // RGB565 color to tint to
    float intensity;           // Tinting strength (0.0-1.0)
    float threshold;           // Brightness threshold for selective tint (0.0-1.0)
    bool selectiveTint;        // Apply only to bright/dark pixels
} tint_params_t;

/**
 * @brief Chromatic aberration parameters
 */
typedef struct {
    ChromaticMode mode;        // Type of chromatic aberration
    float intensity;           // Overall effect strength (0.0-3.0)
    float degrees;             // Angle for aberration direction (0-360)
    float redShift;            // Red channel offset pixels (+/- 2.0)
    float blueShift;           // Blue channel offset pixels (+/- 2.0)
} chromatic_params_t;

//==============================================================================
// TINT EFFECT FUNCTIONS
//==============================================================================

/**
 * @brief Apply tint effect to a pixel
 * @param pixel Original RGB565 pixel
 * @param params Tint parameters
 * @param x X coordinate of the pixel
 * @param y Y coordinate of the pixel
 * @return Tinted RGB565 pixel
 */
uint16_t effectsTints_applyTint(uint16_t pixel, const tint_params_t* params, int x, int y);

/**
 * @brief Apply tint effect to a scanline
 * @param pixels Array of RGB565 pixels
 * @param width Number of pixels in scanline
 * @param row Current row number
 * @param params Tint parameters
 */
void effectsTints_applyTintToScanline(uint16_t* pixels, int width, int row, const tint_params_t* params);


//==============================================================================
// CHROMATIC ABERRATION FUNCTIONS
//==============================================================================

/**
 * @brief Apply chromatic aberration to a scanline
 * @param pixels Array of RGB565 pixels
 * @param width Number of pixels in scanline
 * @param row Current row number
 * @param params Chromatic aberration parameters
 */
void effectsTints_applyChromaticAberration(uint16_t* pixels, int width, int row, const chromatic_params_t* params);


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
void effectsTints_calculateChromaticShift(const chromatic_params_t* params, int x, int y, 
                                         float* redShiftX, float* redShiftY, 
                                         float* blueShiftX, float* blueShiftY);

//==============================================================================
// COLOR MANIPULATION UTILITIES
//==============================================================================

/**
 * @brief Blend two colors with given ratio
 * @param color1 First RGB565 color
 * @param color2 Second RGB565 color
 * @param ratio Blend ratio (0.0-1.0, 0.0 = all color1, 1.0 = all color2)
 * @return Blended RGB565 color
 */
uint16_t effectsTints_blendColors(uint16_t color1, uint16_t color2, float ratio);

/**
 * @brief Apply color tint to a pixel
 * @param pixel Original RGB565 pixel
 * @param tintColor RGB565 tint color
 * @param intensity Tint intensity (0.0-1.0)
 * @return Tinted RGB565 pixel
 */
uint16_t effectsTints_applyColorTint(uint16_t pixel, uint16_t tintColor, float intensity);



/**
 * @brief Apply 4-color palette with improved blending
 * @param pixel Original RGB565 pixel
 * @param paletteType Type of palette to apply
 * @param intensity Effect intensity (0.0-1.0)
 * @param x X coordinate for dithering (optional)
 * @param y Y coordinate for dithering (optional)
 * @return Palette styled RGB565 pixel
 */
uint16_t effectsTints_applyFourColorPalette(uint16_t pixel, PaletteType paletteType, float intensity, int x, int y);

/**
 * @brief Convert RGB565 color to 4-color palette with dithered transitions
 * @param pixel Original RGB565 pixel
 * @param paletteType Type of palette to use
 * @param x X coordinate for dithering pattern (optional)
 * @param y Y coordinate for dithering pattern (optional)
 * @return Palette color in RGB565 format
 */
uint16_t effectsTints_convertToFourColorPaletteSmooth(uint16_t pixel, PaletteType paletteType, int x, int y);

//==============================================================================
// DEFAULT PARAMETERS
//==============================================================================

/**
 * @brief Get default tint parameters
 * @return Default tint parameters
 */
tint_params_t effectsTints_getDefaultTintParams(void);

/**
 * @brief Get default chromatic aberration parameters
 * @return Default chromatic aberration parameters
 */
chromatic_params_t effectsTints_getDefaultChromaticParams(void);

#endif /* EFFECTS_TINTS_H */
