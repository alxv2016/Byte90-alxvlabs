/**
 * @file effects_matrix.h
 * @brief Matrix effects: dot matrix and pixelation effects
 */

#ifndef EFFECTS_MATRIX_H
#define EFFECTS_MATRIX_H

#include "effects_common.h"

//==============================================================================
// MATRIX EFFECT PARAMETERS
//==============================================================================

/**
 * @brief Dot matrix effect parameters
 */
typedef struct {
    DotMatrixMode mode;        // Shape of matrix dots (square/circle)
    float intensity;           // How dark the gaps are (0.0-1.0)
    int dotSize;               // Size of each dot in pixels (1-8)
    int quantization;          // Color reduction levels (2-16)
} dot_matrix_params_t;

/**
 * @brief Pixelation effect parameters
 */
typedef struct {
    PixelateMode mode;         // Type of pixelation
    float intensity;           // Pixelation strength (0.0-1.0)
    int blockSize;             // Size of pixel blocks (2-16)
} pixelate_params_t;

//==============================================================================
// DOT MATRIX EFFECT FUNCTIONS
//==============================================================================

/**
 * @brief Initialize matrix effects system
 */
void effectsMatrix_init(void);

/**
 * @brief Apply dot matrix effect to a scanline
 * @param pixels Array of RGB565 pixels
 * @param width Number of pixels in scanline
 * @param row Current row number
 * @param params Dot matrix parameters
 */
void effectsMatrix_applyDotMatrixEffect(uint16_t* pixels, int width, int row, const dot_matrix_params_t* params);

/**
 * @brief Apply dot matrix effect to a single pixel
 * @param pixel Original RGB565 pixel
 * @param params Dot matrix parameters
 * @param x X coordinate of the pixel
 * @param y Y coordinate of the pixel
 * @return Dot matrix processed RGB565 pixel
 */
uint16_t effectsMatrix_applyDotMatrixToPixel(uint16_t pixel, const dot_matrix_params_t* params, int x, int y);

/**
 * @brief Check if pixel should be a dot in matrix pattern
 * @param x X coordinate of the pixel
 * @param y Y coordinate of the pixel
 * @param dotSize Size of each dot
 * @param mode Dot matrix mode
 * @return true if pixel should be a dot
 */
bool effectsMatrix_isDotPixel(int x, int y, int dotSize, DotMatrixMode mode);



//==============================================================================
// PIXELATION EFFECT FUNCTIONS
//==============================================================================

/**
 * @brief Apply pixelation effect to a scanline
 * @param pixels Array of RGB565 pixels
 * @param width Number of pixels in scanline
 * @param row Current row number
 * @param params Pixelation parameters
 */
void effectsMatrix_applyPixelateEffect(uint16_t* pixels, int width, int row, const pixelate_params_t* params);


/**
 * @brief Apply square pixelation to a scanline using block-based processing
 * @param pixels Array of RGB565 pixels
 * @param width Number of pixels in scanline
 * @param row Current row number
 * @param params Pixelation parameters
 */
void effectsMatrix_applySquarePixelateToScanline(uint16_t* pixels, int width, int row, const pixelate_params_t* params);


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
 */
float effectsMatrix_calculateDotDistance(int x, int y, int dotCenterX, int dotCenterY);

/**
 * @brief Check if pixel is within dot radius
 * @param x X coordinate of the pixel
 * @param y Y coordinate of the pixel
 * @param dotCenterX X coordinate of dot center
 * @param dotCenterY Y coordinate of dot center
 * @param radius Dot radius
 * @return true if pixel is within dot radius
 */
bool effectsMatrix_isWithinDotRadius(int x, int y, int dotCenterX, int dotCenterY, float radius);



//==============================================================================
// PARAMETER VALIDATION
//==============================================================================

/**
 * @brief Clamp dot matrix parameters to valid ranges
 * @param params Dot matrix parameters to clamp
 * @return Clamped dot matrix parameters
 */
dot_matrix_params_t effectsMatrix_clampDotMatrixParams(const dot_matrix_params_t* params);

/**
 * @brief Clamp pixelation parameters to valid ranges
 * @param params Pixelation parameters to clamp
 * @return Clamped pixelation parameters
 */
pixelate_params_t effectsMatrix_clampPixelateParams(const pixelate_params_t* params);

//==============================================================================
// DEFAULT PARAMETERS
//==============================================================================

/**
 * @brief Get default dot matrix parameters
 * @return Default dot matrix parameters
 */
dot_matrix_params_t effectsMatrix_getDefaultDotMatrixParams(void);

/**
 * @brief Get default pixelation parameters
 * @return Default pixelation parameters
 */
pixelate_params_t effectsMatrix_getDefaultPixelateParams(void);

#endif /* EFFECTS_MATRIX_H */
