/**
 * @file effects_retro.h
 * @brief Retro effects: scanlines, dithering, and glitch effects
 */

#ifndef EFFECTS_RETRO_H
#define EFFECTS_RETRO_H

#include "effects_common.h"

//==============================================================================
// RETRO EFFECT PARAMETERS
//==============================================================================

/**
 * @brief Scanline effect parameters
 */
typedef struct {
    ScanlineMode mode;         // Type of scanline effect
    float intensity;           // How dark the scanlines are (0.0-1.0)
    float speed;               // Animation speed for animated modes
} scanline_params_t;

/**
 * @brief Dithering effect parameters
 */
typedef struct {
    DitherMode mode;           // Dither pattern size (2x2, 4x4, 8x8)
    float intensity;           // Dithering strength (0.0-1.0)
    int quantization;          // Color reduction levels (2-16)
} dither_params_t;

/**
 * @brief Glitch effect parameters
 */
typedef struct {
    GlitchMode mode;           // Intensity level (light/medium/heavy)
    float probability;         // Chance of glitch per scanline (0.001-0.1)
} glitch_params_t;

//==============================================================================
// SCANLINE EFFECT FUNCTIONS
//==============================================================================

/**
 * @brief Initialize retro effects system
 */
void effectsRetro_init(void);

/**
 * @brief Shutdown retro effects system
 */
void effectsRetro_shutdown(void);

/**
 * @brief Apply scanline effect to a pixel
 * @param pixel Original RGB565 pixel
 * @param params Scanline parameters
 * @param row Current row number
 * @return Scanline-processed RGB565 pixel
 */
uint16_t effectsRetro_applyScanline(uint16_t pixel, const scanline_params_t* params, int row);

/**
 * @brief Apply scanline effect to a scanline
 * @param pixels Array of RGB565 pixels
 * @param width Number of pixels in scanline
 * @param row Current row number
 * @param params Scanline parameters
 */
void effectsRetro_applyScanlineToScanline(uint16_t* pixels, int width, int row, const scanline_params_t* params);

/**
 * @brief Apply animated scanline effect
 * @param pixel Original RGB565 pixel
 * @param params Scanline parameters
 * @param row Current row number
 * @return Animated scanline-processed RGB565 pixel
 */
uint16_t effectsRetro_applyAnimatedScanline(uint16_t pixel, const scanline_params_t* params, int row);

/**
 * @brief Apply curved scanline effect
 * @param pixel Original RGB565 pixel
 * @param params Scanline parameters
 * @param x X coordinate of the pixel
 * @param row Current row number
 * @return Curved scanline-processed RGB565 pixel
 */
uint16_t effectsRetro_applyCurvedScanline(uint16_t pixel, const scanline_params_t* params, int x, int row);

//==============================================================================
// DITHERING EFFECT FUNCTIONS
//==============================================================================

/**
 * @brief Apply Bayer dithering to a pixel
 * @param pixel Original RGB565 pixel
 * @param params Dithering parameters
 * @param x X coordinate of the pixel
 * @param y Y coordinate of the pixel
 * @return Dithered RGB565 pixel
 */
uint16_t effectsRetro_applyBayerDithering(uint16_t pixel, const dither_params_t* params, int x, int y);

/**
 * @brief Apply Bayer dithering to a scanline
 * @param pixels Array of RGB565 pixels
 * @param width Number of pixels in scanline
 * @param row Current row number
 * @param params Dithering parameters
 */
void effectsRetro_applyBayerDitheringToScanline(uint16_t* pixels, int width, int row, const dither_params_t* params);

/**
 * @brief Get Bayer dither threshold for a position
 * @param x X coordinate
 * @param y Y coordinate
 * @param mode Dither mode
 * @return Normalized threshold value (0.0-1.0)
 */
float effectsRetro_getBayerThreshold(int x, int y, DitherMode mode);

/**
 * @brief Quantize a color component to reduce color depth
 * @param value Original color component
 * @param maxValue Maximum value for this component
 * @param levels Number of quantization levels
 * @param threshold Bayer threshold for dithering (0.0-1.0)
 * @return Quantized color component
 */
uint8_t effectsRetro_quantizeColorComponent(uint8_t value, uint8_t maxValue, int levels, float threshold);


//==============================================================================
// GLITCH EFFECT FUNCTIONS
//==============================================================================

/**
 * @brief Apply CRT glitch effects to a scanline
 * @param pixels Array of RGB565 pixels
 * @param width Number of pixels in scanline
 * @param row Current row number
 * @param params Glitch parameters
 */
void effectsRetro_applyCRTGlitches(uint16_t* pixels, int width, int row, const glitch_params_t* params);

/**
 * @brief Apply horizontal jitter to pixel array for glitch effects
 * @param pixels Array of RGB565 pixels for current scanline
 * @param width Number of pixels in the scanline
 * @param intensity Maximum shift amount in pixels
 */
void effectsRetro_applyHorizontalJitter(uint16_t* pixels, int width, int intensity);


//==============================================================================
// UTILITY FUNCTIONS
//==============================================================================

/**
 * @brief Fast random number generator for glitch effects
 * @return Pseudo-random number between 0 and 32767
 */
uint16_t effectsRetro_fastRandom(void);


//==============================================================================
// PARAMETER VALIDATION
//==============================================================================

/**
 * @brief Validate scanline parameters
 * @param params Scanline parameters to validate
 * @return true if parameters are valid
 */
bool effectsRetro_validateScanlineParams(const scanline_params_t* params);

/**
 * @brief Validate dithering parameters
 * @param params Dithering parameters to validate
 * @return true if parameters are valid
 */
bool effectsRetro_validateDitherParams(const dither_params_t* params);

/**
 * @brief Validate glitch parameters
 * @param params Glitch parameters to validate
 * @return true if parameters are valid
 */
bool effectsRetro_validateGlitchParams(const glitch_params_t* params);

/**
 * @brief Clamp scanline parameters to valid ranges
 * @param params Scanline parameters to clamp
 * @return Clamped scanline parameters
 */
scanline_params_t effectsRetro_clampScanlineParams(const scanline_params_t* params);

/**
 * @brief Clamp dithering parameters to valid ranges
 * @param params Dithering parameters to clamp
 * @return Clamped dithering parameters
 */
dither_params_t effectsRetro_clampDitherParams(const dither_params_t* params);

/**
 * @brief Clamp glitch parameters to valid ranges
 * @param params Glitch parameters to clamp
 * @return Clamped glitch parameters
 */
glitch_params_t effectsRetro_clampGlitchParams(const glitch_params_t* params);

//==============================================================================
// DEFAULT PARAMETERS
//==============================================================================

/**
 * @brief Get default scanline parameters
 * @return Default scanline parameters
 */
scanline_params_t effectsRetro_getDefaultScanlineParams(void);

/**
 * @brief Get default dithering parameters
 * @return Default dithering parameters
 */
dither_params_t effectsRetro_getDefaultDitherParams(void);

/**
 * @brief Get default glitch parameters
 * @return Default glitch parameters
 */
glitch_params_t effectsRetro_getDefaultGlitchParams(void);

#endif /* EFFECTS_RETRO_H */
