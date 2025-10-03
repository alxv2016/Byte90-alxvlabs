/**
 * @file effects_common.h
 * @brief Shared constants, types, and utilities for the modular effects system
 */

#ifndef EFFECTS_COMMON_H
#define EFFECTS_COMMON_H

#include "common.h"
#include <stdint.h>

//==============================================================================
// CONSTANTS & DEFINITIONS
//==============================================================================

static const char *EFFECTS_LOG = "::EFFECTS_MODULE::";

#define EFFECT_DEBOUNCE_TIME 300

// Color definitions using hexToRGB565() - computed once at compile time
const uint16_t TINT_BLACK = hexToRGB565("#000000");      // #000000 -> RGB565
const uint16_t TINT_WHITE = hexToRGB565("#ffffff");     // #ffffff -> RGB565

const uint16_t TINT_YELLOW_400 = hexToRGB565("#ffff00"); // #ffff00 -> RGB565
const uint16_t TINT_YELLOW_500 = hexToRGB565("#ffbe00"); // #ffbe00 -> RGB565

const uint16_t TINT_GREEN_400 = hexToRGB565("#00ff21");  // #00ff21 -> RGB565
const uint16_t TINT_GREEN_500 = hexToRGB565("#00db00");  // #00db00 -> RGB565

const uint16_t TINT_BLUE_400 = hexToRGB565("#3100ce");   // #3100ce -> RGB565
const uint16_t TINT_BLUE_500 = hexToRGB565("#21009c");   // #21009c -> RGB565

// Game Boy palette colors using hexToRGB565() - computed once at compile time
const uint16_t GAMEBOY_700 = hexToRGB565("#081408");     // #081408 -> RGB565
const uint16_t GAMEBOY_600 = hexToRGB565("#192d19");     // #192d19 -> RGB565
const uint16_t GAMEBOY_500 = hexToRGB565("#52aa52");     // #52aa52 -> RGB565
const uint16_t GAMEBOY_400 = hexToRGB565("#6bdf6b");     // #6bdf6b -> RGB565

const uint16_t MONOCHROME_700 = hexToRGB565("#161411");  // #161411 -> RGB565
const uint16_t MONOCHROME_600 = hexToRGB565("#525152");  // #525152 -> RGB565
const uint16_t MONOCHROME_500 = hexToRGB565("#b5b2b5");  // #b5b2b5 -> RGB565
const uint16_t MONOCHROME_400 = hexToRGB565("#d6d7d6");  // #d6d7d6 -> RGB565

// Effect labels
#define EFFECT_LABEL_NONE "NONE"
#define EFFECT_LABEL_GREEN_TINT "HACKER GREEN"
#define EFFECT_LABEL_YELLOW_TINT "RETRO YELLOW"
#define EFFECT_LABEL_RED_TINT "DIGITAL BLUE"
#define EFFECT_LABEL_GAMEBOY_TINT "GAME BOY"
#define EFFECT_LABEL_MONOCHROME_TINT "MONOCHROME"

//==============================================================================
// TYPE DEFINITIONS
//==============================================================================

// Unified effect types
typedef enum {
  EFFECT_SCANLINES,  // CRT scanline simulation
  EFFECT_DITHERING,  // Bayer dithering patterns
  EFFECT_GLITCH,     // CRT glitch effects
  EFFECT_TINT,       // Color tinting
  EFFECT_CHROMATIC,  // Chromatic aberration
  EFFECT_DOT_MATRIX, // Dot matrix display
  EFFECT_PIXELATE,   // Pixelation/blocks
  EFFECT_COUNT
} effect_type_t;

// Palette types
typedef enum { PALETTE_GAMEBOY = 0, PALETTE_MONOCHROME, PALETTE_COUNT } PaletteType;

// Four-color palette structure
typedef struct {
  uint16_t darkest;  // 700 level
  uint16_t dark;     // 600 level
  uint16_t light;    // 500 level
  uint16_t lightest; // 400 level
} FourColorPalette;

// Mode enums
typedef enum {
  SCANLINE_NONE,
  SCANLINE_CLASSIC,
  SCANLINE_ANIMATED,
  SCANLINE_CURVE
} ScanlineMode;

typedef enum {
  GLITCH_NONE,
  GLITCH_LIGHT,
  GLITCH_MEDIUM,
  GLITCH_HEAVY
} GlitchMode;

typedef enum { DITHER_NONE, DITHER_2X2, DITHER_4X4, DITHER_8X8 } DitherMode;

typedef enum { CHROMATIC_NONE, CHROMATIC_ANGLE } ChromaticMode;

typedef enum {
  DOT_MATRIX_NONE,
  DOT_MATRIX_SQUARE,
  DOT_MATRIX_CIRCLE
} DotMatrixMode;

typedef enum { PIXELATE_NONE, PIXELATE_SQUARE } PixelateMode;

// Effect registry structure
typedef struct {
  effect_type_t type;
  const char *name;
  void (*init)(void *params);
  void (*apply)(uint16_t *pixels, int width, int row, const void *params);
  void (*cleanup)(void *params);
} effect_registry_t;

// Performance tracking
typedef struct {
  uint32_t totalPixels;
  uint32_t effectsApplied;
  uint32_t processingTime;
  float fps;
} effect_performance_t;

// Effect pipeline structure
typedef struct {
  effect_type_t type;
  bool enabled;
  float intensity;
  void (*apply)(uint16_t *, int, int, const void *);
  void *params;
} effect_pipeline_t;

//==============================================================================
// UTILITY FUNCTIONS
//==============================================================================

/**
 * @brief Convert RGB888 to RGB565
 * @param r Red component (0-255)
 * @param g Green component (0-255)
 * @param b Blue component (0-255)
 * @return RGB565 color value
 */
uint16_t rgb888_to_rgb565(uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief Convert RGB565 to RGB888
 * @param rgb565 RGB565 color value
 * @param r Pointer to store red component (0-255)
 * @param g Pointer to store green component (0-255)
 * @param b Pointer to store blue component (0-255)
 */
void rgb565_to_rgb888(uint16_t rgb565, uint8_t *r, uint8_t *g, uint8_t *b);

/**
 * @brief Get pixel brightness (0.0-1.0)
 * @param pixel RGB565 pixel value
 * @return Brightness as float (0.0-1.0)
 */
float getPixelBrightness(uint16_t pixel);

/**
 * @brief Validate intensity value
 * @param intensity Intensity value to validate
 * @return true if valid (0.0-1.0), false otherwise
 */
bool validateIntensity(float intensity);

/**
 * @brief Validate color value
 * @param color RGB565 color value
 * @return true if valid, false otherwise
 */
bool validateColor(uint16_t color);

/**
 * @brief Get effect type name
 * @param type Effect type
 * @return Pointer to effect name string
 */
const char *getEffectTypeName(effect_type_t type);

#endif /* EFFECTS_COMMON_H */
