/**
 * @file effects_common.cpp
 * @brief Implementation of shared utilities for the modular effects system
 *
 * Provides comprehensive shared utilities and helper functions for the BYTE-90 device
 * effects system, including color space conversions, pixel manipulation, validation
 * functions, and effect type management for the modular effects framework.
 *
 * This module handles:
 * - RGB888 to RGB565 color space conversions
 * - RGB565 to RGB888 color space conversions
 * - Pixel brightness calculations using luminance formulas
 * - Parameter validation for intensity and color values
 * - Effect type name resolution and management
 * - Color space manipulation and normalization
 * - Utility functions for effects processing
 * - Shared constants and type definitions
 * - Cross-module compatibility and integration
 */

#include "effects_common.h"
#include <Arduino.h>

//==============================================================================
// UTILITY FUNCTION IMPLEMENTATIONS
//==============================================================================

/**
 * @brief Convert RGB888 to RGB565
 * @param r Red component (0-255)
 * @param g Green component (0-255)
 * @param b Blue component (0-255)
 * @return RGB565 color value
 * 
 * Converts 8-bit RGB color values to 16-bit RGB565 format used by the display.
 * This function performs bit shifting and masking to compress the color data
 * while maintaining reasonable color accuracy.
 */
uint16_t rgb888_to_rgb565(uint8_t r, uint8_t g, uint8_t b) {
    // Convert 8-bit RGB to 5-6-5 RGB565
    uint16_t r5 = (r >> 3) & 0x1F;
    uint16_t g6 = (g >> 2) & 0x3F;
    uint16_t b5 = (b >> 3) & 0x1F;
    
    return (r5 << 11) | (g6 << 5) | b5;
}

/**
 * @brief Convert RGB565 to RGB888
 * @param rgb565 RGB565 color value
 * @param r Pointer to store red component (0-255)
 * @param g Pointer to store green component (0-255)
 * @param b Pointer to store blue component (0-255)
 * 
 * Converts 16-bit RGB565 color values to 8-bit RGB888 format. This function
 * extracts the color components and scales them back to 8-bit precision
 * for standard RGB color operations.
 */
void rgb565_to_rgb888(uint16_t rgb565, uint8_t* r, uint8_t* g, uint8_t* b) {
    // Extract 5-6-5 components and convert to 8-bit
    uint8_t r5 = (rgb565 >> 11) & 0x1F;
    uint8_t g6 = (rgb565 >> 5) & 0x3F;
    uint8_t b5 = rgb565 & 0x1F;
    
    // Scale to 8-bit (multiply by 255/max_value)
    *r = (r5 * 255) / 31;
    *g = (g6 * 255) / 63;
    *b = (b5 * 255) / 31;
}

/**
 * @brief Get pixel brightness (0.0-1.0)
 * @param pixel RGB565 pixel value
 * @return Brightness as float (0.0-1.0)
 * 
 * Calculates the perceived brightness of an RGB565 pixel using the standard
 * luminance formula. This function provides accurate brightness calculations
 * for effects that depend on pixel brightness.
 */
float getPixelBrightness(uint16_t pixel) {
    // Extract RGB components
    uint8_t r = (pixel >> 11) & 0x1F;
    uint8_t g = (pixel >> 5) & 0x3F;
    uint8_t b = pixel & 0x1F;
    
    // Convert to 8-bit
    float r_norm = (r * 255.0f) / 31.0f;
    float g_norm = (g * 255.0f) / 63.0f;
    float b_norm = (b * 255.0f) / 31.0f;
    
    // Calculate brightness using luminance formula
    return (0.299f * r_norm + 0.587f * g_norm + 0.114f * b_norm) / 255.0f;
}

/**
 * @brief Validate intensity value
 * @param intensity Intensity value to validate
 * @return true if valid (0.0-1.0), false otherwise
 * 
 * Validates that an intensity value is within the valid range for effects.
 * This function ensures that intensity parameters are safe to use in
 * effects processing.
 */
bool validateIntensity(float intensity) {
    return (intensity >= 0.0f && intensity <= 1.0f);
}

/**
 * @brief Validate color value
 * @param color RGB565 color value
 * @return true if valid, false otherwise
 * 
 * Validates that an RGB565 color value is within valid ranges for all
 * color components. This function ensures that color parameters are
 * safe to use in effects processing.
 */
bool validateColor(uint16_t color) {
    // Basic validation - check if it's a valid RGB565 value
    // All bits should be within valid ranges
    uint8_t r = (color >> 11) & 0x1F;
    uint8_t g = (color >> 5) & 0x3F;
    uint8_t b = color & 0x1F;
    
    return (r <= 31 && g <= 63 && b <= 31);
}

/**
 * @brief Get effect type name
 * @param type Effect type
 * @return Pointer to effect name string
 * 
 * Returns a human-readable string name for a given effect type. This function
 * provides a convenient way to get effect names for logging, debugging, and
 * user interface purposes.
 */
const char* getEffectTypeName(effect_type_t type) {
    static const char* effectNames[] = {
        "SCANLINES",
        "DITHERING", 
        "GLITCH",
        "TINT",
        "CHROMATIC",
        "DOT_MATRIX",
        "PIXELATE"
    };
    
    if (type >= 0 && type < EFFECT_COUNT) {
        return effectNames[type];
    }
    return "UNKNOWN";
}








