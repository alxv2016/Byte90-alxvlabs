/**
 * @file menu_effects.cpp
 * @brief Individual effects toggle menu implementation
 *
 * Provides individual effects toggle functionality for the BYTE-90 device,
 * allowing users to enable/disable specific visual effects including
 * dithering, chromatic aberration, dot matrix, pixelation, scanlines, and glitch effects.
 *
 * This module handles:
 * - Individual effect toggle controls and management
 * - Effect parameter configuration and application
 * - Effects state persistence using preferences module
 * - Integration with effects core, retro, matrix, and tints modules
 * - Debug logging and system integration
 * - Menu context management and item counting
 * - Real-time effect status synchronization
 * - Visual effects system coordination
 */

#include "menu_effects.h"
#include "effects_core.h"
#include "effects_retro.h"
#include "effects_matrix.h"
#include "effects_tints.h"
#include "preferences_module.h"
#include <stdarg.h>

//==============================================================================
// DEBUG SYSTEM
//==============================================================================

// Debug control - set to true to enable debug logging
static bool menuEffectsDebugEnabled = false;

/**
 * @brief Centralized debug logging function for menu effects operations
 * @param format Printf-style format string
 * @param ... Variable arguments for format string
 */
static void menuEffectsDebug(const char* format, ...) {
  if (!menuEffectsDebugEnabled) {
    return;
  }
  
  va_list args;
  va_start(args, format);
  esp_log_writev(ESP_LOG_INFO, "EFFECTS", format, args);
  va_end(args);
}

/**
 * @brief Enable or disable debug logging for menu effects operations
 * @param enabled true to enable debug logging, false to disable
 * 
 * @example
 * // Enable debug logging
 * setMenuEffectsDebug(true);
 * 
 * // Disable debug logging  
 * setMenuEffectsDebug(false);
 */
void setMenuEffectsDebug(bool enabled) {
  menuEffectsDebugEnabled = enabled;
  if (enabled) {
    ESP_LOGI("EFFECTS", "Menu effects debug logging enabled");
  } else {
    ESP_LOGI("EFFECTS", "Menu effects debug logging disabled");
  }
}

//==============================================================================
// EFFECT STATUS FLAGS
//==============================================================================

static bool ditheringEnabled = false;
static bool chromaticEnabled = false;
static bool dotMatrixEnabled = false;
static bool pixelateEnabled = false;
static bool scanlinesEnabled = false;
static bool glitchEnabled = false;

//==============================================================================
// EFFECT TOGGLE FUNCTIONS
//==============================================================================

 /**
  * @brief Toggle dithering effect
  * @param enabled true to enable dithering, false to disable
  * 
  * Enables or disables the dithering effect, which adds a retro-style
  * color quantization effect to the display. When enabled, applies
  * default dithering parameters from the effects retro module.
  */
 static void toggleDithering(bool enabled) {
    menuEffectsDebug("toggleDithering called with: %s", enabled ? "ON" : "OFF");
    ditheringEnabled = enabled;
    
    // Apply effect using new modular effects system
    if (enabled) {
        effectsCore_enableEffect(EFFECT_DITHERING);
        dither_params_t params = effectsRetro_getDefaultDitherParams();
        effectsCore_setEffectParams(EFFECT_DITHERING, &params);
    } else {
        effectsCore_disableEffect(EFFECT_DITHERING);
    }
    
    // Save to preferences
    effectsCore_saveToPreferences();
    menuEffectsDebug("Dithering effect applied: %s", enabled ? "ON" : "OFF");
}

 /**
  * @brief Toggle chromatic aberration effect
  * @param enabled true to enable chromatic aberration, false to disable
  * 
  * Enables or disables the chromatic aberration effect, which creates
  * a color separation effect similar to old CRT displays. When enabled,
  * applies default chromatic parameters from the effects tints module.
  */
 static void toggleChromatic(bool enabled) {
    menuEffectsDebug("toggleChromatic called with: %s", enabled ? "ON" : "OFF");
    chromaticEnabled = enabled;
    
    // Apply effect using new modular effects system
    if (enabled) {
        effectsCore_enableEffect(EFFECT_CHROMATIC);
        chromatic_params_t params = effectsTints_getDefaultChromaticParams();
        effectsCore_setEffectParams(EFFECT_CHROMATIC, &params);
    } else {
        effectsCore_disableEffect(EFFECT_CHROMATIC);
    }
    
    // Save to preferences
    effectsCore_saveToPreferences();
    menuEffectsDebug("Chromatic effect applied: %s", enabled ? "ON" : "OFF");
}

 /**
  * @brief Toggle dot matrix effect
  * @param enabled true to enable dot matrix, false to disable
  * 
  * Enables or disables the dot matrix effect, which creates a retro
  * dot matrix display appearance. When enabled, applies default
  * dot matrix parameters from the effects matrix module.
  */
 static void toggleDotMatrix(bool enabled) {
    menuEffectsDebug("toggleDotMatrix called with: %s", enabled ? "ON" : "OFF");
    dotMatrixEnabled = enabled;
    
    // Apply effect using new modular effects system
    if (enabled) {
        effectsCore_enableEffect(EFFECT_DOT_MATRIX);
        dot_matrix_params_t params = effectsMatrix_getDefaultDotMatrixParams();
        effectsCore_setEffectParams(EFFECT_DOT_MATRIX, &params);
    } else {
        effectsCore_disableEffect(EFFECT_DOT_MATRIX);
    }
    
    // Save to preferences
    effectsCore_saveToPreferences();
    menuEffectsDebug("Dot matrix effect applied: %s", enabled ? "ON" : "OFF");
}

 /**
  * @brief Toggle pixelate effect
  * @param enabled true to enable pixelation, false to disable
  * 
  * Enables or disables the pixelate effect, which creates a blocky,
  * pixelated appearance on the display. When enabled, applies default
  * pixelate parameters from the effects matrix module.
  */
 static void togglePixelate(bool enabled) {
    menuEffectsDebug("togglePixelate called with: %s", enabled ? "ON" : "OFF");
    pixelateEnabled = enabled;
    
    // Apply effect using new modular effects system
    if (enabled) {
        effectsCore_enableEffect(EFFECT_PIXELATE);
        pixelate_params_t params = effectsMatrix_getDefaultPixelateParams();
        effectsCore_setEffectParams(EFFECT_PIXELATE, &params);
    } else {
        effectsCore_disableEffect(EFFECT_PIXELATE);
    }
    
    // Save to preferences
    effectsCore_saveToPreferences();
    menuEffectsDebug("Pixelate effect applied: %s", enabled ? "ON" : "OFF");
}

 /**
  * @brief Toggle scanlines effect
  * @param enabled true to enable scanlines, false to disable
  * 
  * Enables or disables the scanlines effect, which adds horizontal
  * lines across the display to simulate old CRT monitor appearance.
  * When enabled, applies default scanline parameters from the effects retro module.
  */
 static void toggleScanlines(bool enabled) {
    menuEffectsDebug("toggleScanlines called with: %s", enabled ? "ON" : "OFF");
    scanlinesEnabled = enabled;
    
    // Apply effect using new modular effects system
    if (enabled) {
        effectsCore_enableEffect(EFFECT_SCANLINES);
        scanline_params_t params = effectsRetro_getDefaultScanlineParams();
        effectsCore_setEffectParams(EFFECT_SCANLINES, &params);
    } else {
        effectsCore_disableEffect(EFFECT_SCANLINES);
    }
    
    // Save to preferences
    effectsCore_saveToPreferences();
    menuEffectsDebug("Scanlines effect applied: %s", enabled ? "ON" : "OFF");
}

 /**
  * @brief Toggle glitch effect
  * @param enabled true to enable glitch, false to disable
  * 
  * Enables or disables the glitch effect, which creates digital
  * distortion and corruption effects on the display. When enabled,
  * applies default glitch parameters from the effects retro module.
  */
 static void toggleGlitch(bool enabled) {
    menuEffectsDebug("toggleGlitch called with: %s", enabled ? "ON" : "OFF");
    glitchEnabled = enabled;
    
    // Apply effect using new modular effects system
    if (enabled) {
        effectsCore_enableEffect(EFFECT_GLITCH);
        glitch_params_t params = effectsRetro_getDefaultGlitchParams();
        effectsCore_setEffectParams(EFFECT_GLITCH, &params);
    } else {
        effectsCore_disableEffect(EFFECT_GLITCH);
    }
    
    // Save to preferences
    effectsCore_saveToPreferences();
    menuEffectsDebug("Glitch effect applied: %s", enabled ? "ON" : "OFF");
}

//==============================================================================
// MENU DEFINITION
//==============================================================================

/**
 * @brief Individual effects menu items
 */
static MenuItem effectsMenu[] = {
    DEFINE_MENU_TOGGLE(dithering, MENU_LABEL_DITHERING, toggleDithering, &ditheringEnabled),
    DEFINE_MENU_TOGGLE(chromatic, MENU_LABEL_CHROMATIC, toggleChromatic, &chromaticEnabled),
    DEFINE_MENU_TOGGLE(dot_matrix, MENU_LABEL_DOT_MATRIX, toggleDotMatrix, &dotMatrixEnabled),
    DEFINE_MENU_TOGGLE(pixelate, MENU_LABEL_PIXELATE, togglePixelate, &pixelateEnabled),
    DEFINE_MENU_TOGGLE(scanlines, MENU_LABEL_SCANLINES, toggleScanlines, &scanlinesEnabled),
    DEFINE_MENU_TOGGLE(glitch, MENU_LABEL_GLITCH, toggleGlitch, &glitchEnabled),
    DEFINE_MENU_BACK()
};

static const int effectsMenuCount = MENU_ITEM_COUNT(effectsMenu);

//==============================================================================
// PUBLIC API IMPLEMENTATION
//==============================================================================

/**
 * @brief Initialize effects module
 * 
 * Initializes the effects menu module by loading current effect states
 * from the preferences module. This function should be called during
 * system initialization to set up the effects menu with the current
 * effect preferences.
 */
void menuEffects_init() {
     // Load current effect states from preferences
     menuEffects_updateStatus();
 }

/**
 * @brief Update effect status flags
 * 
 * Updates the internal status flags for all effects by reading the current
 * state from the effects core system. This function ensures the menu
 * reflects the actual state of the effects system.
 */
void menuEffects_updateStatus() {
     // Update status flags from new effects system
     ditheringEnabled = effectsCore_isEffectEnabled(EFFECT_DITHERING);
     chromaticEnabled = effectsCore_isEffectEnabled(EFFECT_CHROMATIC);
     dotMatrixEnabled = effectsCore_isEffectEnabled(EFFECT_DOT_MATRIX);
     pixelateEnabled = effectsCore_isEffectEnabled(EFFECT_PIXELATE);
     scanlinesEnabled = effectsCore_isEffectEnabled(EFFECT_SCANLINES);
     glitchEnabled = effectsCore_isEffectEnabled(EFFECT_GLITCH);
     
     // Menu status updated silently
 }

/**
 * @brief Get the effects menu items
 * @return Pointer to effects menu array
 * 
 */
MenuItem* menuEffects_getItems() {
     return effectsMenu;
 }

/**
 * @brief Get the number of effect items
 * @return Number of effect menu items
 * 
 */
int menuEffects_getItemCount() {
     return effectsMenuCount;
 }

/**
 * @brief Get the effects menu context
 * @param context Pointer to context to populate
 * 
 */
void menuEffects_getContext(MenuContext* context) {
     if (context) {
         context->items = effectsMenu;
         context->itemCount = effectsMenuCount;
         context->selectedIndex = 0;
         context->title = MENU_LABEL_EFFECTS;
     }
 }
