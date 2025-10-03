/**
 * @file effects_core.cpp
 * @brief Implementation of core effects management and pipeline functionality
 *
 * Provides comprehensive core effects management and pipeline functionality for the BYTE-90 device,
 * including effect registry, lifecycle management, parameter handling, and scanline processing
 * for the modular effects system.
 *
 * This module handles:
 * - Effect registry initialization and management
 * - Effect enable/disable/toggle operations with debouncing
 * - Effect parameter management and validation
 * - Performance monitoring and statistics tracking
 * - Preferences integration for persistent effect states
 * - Scanline processing pipeline with ordered effect application
 * - Effect lifecycle management and cleanup
 * - Integration with all effect modules (tints, retro, matrix)
 * - Default parameter management and restoration
 * - Effect state persistence and recovery
 */

#include "effects_core.h"
#include "effects_common.h"
#include "effects_tints.h"
#include "effects_retro.h"
#include "effects_matrix.h"
#include "preferences_module.h"
#include <Arduino.h>

//==============================================================================
// GLOBAL VARIABLES
//==============================================================================

// Effect registry
static effect_registry_t effectRegistry[EFFECT_COUNT];
static bool effectRegistryInitialized = false;

// Effect states
static bool effectsEnabled[EFFECT_COUNT] = {false};
static void* effectParams[EFFECT_COUNT] = {NULL};
static unsigned long lastToggleTime[EFFECT_COUNT] = {0};

// Parameter storage for each effect type - initialized from getter functions
static tint_params_t tintParams;
static chromatic_params_t chromaticParams;
static scanline_params_t scanlineParams;
static dither_params_t ditherParams;
static glitch_params_t glitchParams;
static dot_matrix_params_t dotMatrixParams;
static pixelate_params_t pixelateParams;

// Performance monitoring
static effect_performance_t performanceStats = {0};
static bool performanceMonitoringEnabled = false;
static unsigned long lastPerformanceReset = 0;



//==============================================================================
// CORE EFFECTS MANAGEMENT
//==============================================================================

/**
 * @brief Initialize the effects system
 */
void effectsCore_init(void) {
    // Initialize effect registry
    for (int i = 0; i < EFFECT_COUNT; i++) {
        effectRegistry[i].type = (effect_type_t)i;
        effectRegistry[i].name = getEffectTypeName((effect_type_t)i);
        effectRegistry[i].init = NULL;
        effectRegistry[i].apply = NULL;
        effectRegistry[i].cleanup = NULL;
    }
    
    // Register apply functions for each effect type
    effectRegistry[EFFECT_TINT].apply = (void (*)(uint16_t*, int, int, const void*))effectsTints_applyTintToScanline;
    effectRegistry[EFFECT_CHROMATIC].apply = (void (*)(uint16_t*, int, int, const void*))effectsTints_applyChromaticAberration;
    effectRegistry[EFFECT_SCANLINES].apply = (void (*)(uint16_t*, int, int, const void*))effectsRetro_applyScanlineToScanline;
    effectRegistry[EFFECT_DITHERING].apply = (void (*)(uint16_t*, int, int, const void*))effectsRetro_applyBayerDitheringToScanline;
    effectRegistry[EFFECT_GLITCH].apply = (void (*)(uint16_t*, int, int, const void*))effectsRetro_applyCRTGlitches;
    effectRegistry[EFFECT_DOT_MATRIX].apply = (void (*)(uint16_t*, int, int, const void*))effectsMatrix_applyDotMatrixEffect;
    effectRegistry[EFFECT_PIXELATE].apply = (void (*)(uint16_t*, int, int, const void*))effectsMatrix_applyPixelateEffect;
    
    effectRegistryInitialized = true;
    
    // Initialize all parameter structs with default values
    tintParams = effectsTints_getDefaultTintParams();
    chromaticParams = effectsTints_getDefaultChromaticParams();
    scanlineParams = effectsRetro_getDefaultScanlineParams();
    ditherParams = effectsRetro_getDefaultDitherParams();
    glitchParams = effectsRetro_getDefaultGlitchParams();
    dotMatrixParams = effectsMatrix_getDefaultDotMatrixParams();
    pixelateParams = effectsMatrix_getDefaultPixelateParams();
    
    // Load effect states from preferences (may override some parameter values)
    effectsCore_loadFromPreferences();
    
    // Initialize performance monitoring
    performanceMonitoringEnabled = true;
    lastPerformanceReset = millis();
}

//==============================================================================
// EFFECT REGISTRY & LIFECYCLE
//==============================================================================

bool effectsCore_registerEffect(const effect_registry_t* effect) {
    if (!effect || !effectRegistryInitialized) {
        return false;
    }
    
    if (effect->type < 0 || effect->type >= EFFECT_COUNT) {
        return false;
    }
    
    effectRegistry[effect->type] = *effect;
    return true;
}

bool effectsCore_unregisterEffect(effect_type_t type) {
    if (!effectRegistryInitialized || type < 0 || type >= EFFECT_COUNT) {
        return false;
    }
    
    // Disable effect first
    effectsCore_disableEffect(type);
    
    // Clear registry entry
    effectRegistry[type].init = NULL;
    effectRegistry[type].apply = NULL;
    effectRegistry[type].cleanup = NULL;
    
    return true;
}

const effect_registry_t* effectsCore_getEffect(effect_type_t type) {
    if (!effectRegistryInitialized || type < 0 || type >= EFFECT_COUNT) {
        return NULL;
    }
    
    return &effectRegistry[type];
}

//==============================================================================
// EFFECT ENABLE/DISABLE
//==============================================================================

/**
 * @brief Enable a specific effect
 * @param type Effect type to enable
 * @return true if effect was enabled successfully
 */
bool effectsCore_enableEffect(effect_type_t type) {
    if (!effectRegistryInitialized || type < 0 || type >= EFFECT_COUNT) {
        ESP_LOGE(EFFECTS_LOG, "Failed to enable effect %d: invalid type or not initialized", type);
        return false;
    }
    
    if (effectsEnabled[type]) {
        ESP_LOGD(EFFECTS_LOG, "Effect %d (%s) already enabled", type, getEffectTypeName(type));
        return true; // Already enabled
    }
    
    // Ensure parameter pointer is set
    if (!effectParams[type]) {
        switch (type) {
            case EFFECT_TINT:
                effectParams[type] = &tintParams;
                break;
            case EFFECT_CHROMATIC:
                effectParams[type] = &chromaticParams;
                break;
            case EFFECT_SCANLINES:
                effectParams[type] = &scanlineParams;
                break;
            case EFFECT_DITHERING:
                effectParams[type] = &ditherParams;
                break;
            case EFFECT_GLITCH:
                effectParams[type] = &glitchParams;
                break;
            case EFFECT_DOT_MATRIX:
                effectParams[type] = &dotMatrixParams;
                break;
            case EFFECT_PIXELATE:
                effectParams[type] = &pixelateParams;
                break;
            default:
                ESP_LOGE(EFFECTS_LOG, "Unknown effect type %d", type);
                return false;
        }
    }
    
    effectsEnabled[type] = true;
    // Effect enabled silently
    return true;
}

/**
 * @brief Disable a specific effect
 * @param type Effect type to disable
 * @return true if effect was disabled successfully
 */
bool effectsCore_disableEffect(effect_type_t type) {
    if (!effectRegistryInitialized || type < 0 || type >= EFFECT_COUNT) {
        return false;
    }
    
    if (!effectsEnabled[type]) {
        return true; // Already disabled
    }
    
    effectsEnabled[type] = false;
    return true;
}

/**
 * @brief Toggle effect on/off
 * @param type Effect type to toggle
 * @return true if effect is now enabled, false if disabled
 */
bool effectsCore_toggleEffect(effect_type_t type) {
    if (!effectRegistryInitialized || type < 0 || type >= EFFECT_COUNT) {
        return false;
    }
    
    unsigned long currentTime = millis();
    if (currentTime - lastToggleTime[type] < EFFECT_DEBOUNCE_TIME) {
        return effectsEnabled[type]; // Debounce
    }
    
    lastToggleTime[type] = currentTime;
    
    if (effectsEnabled[type]) {
        effectsCore_disableEffect(type);
        return false;
    } else {
        effectsCore_enableEffect(type);
        return true;
    }
}

/**
 * @brief Check if effect is enabled
 * @param type Effect type to check
 * @return true if effect is enabled
 */
bool effectsCore_isEffectEnabled(effect_type_t type) {
    if (!effectRegistryInitialized || type < 0 || type >= EFFECT_COUNT) {
        return false;
    }
    
    return effectsEnabled[type];
}

/**
 * @brief Disable all effects
 */
void effectsCore_disableAllEffects(void) {
    for (int i = 0; i < EFFECT_COUNT; i++) {
        effectsCore_disableEffect((effect_type_t)i);
    }
}

//==============================================================================
// EFFECT PARAMETERS
//==============================================================================

/**
 * @brief Set effect parameters
 * @param type Effect type
 * @param params Effect parameters (void pointer to specific param struct)
 * @return true if parameters were set successfully
 */
bool effectsCore_setEffectParams(effect_type_t type, const void* params) {
    if (!effectRegistryInitialized || type < 0 || type >= EFFECT_COUNT || !params) {
        ESP_LOGE(EFFECTS_LOG, "Failed to set effect params for type %d: invalid params", type);
        return false;
    }
    
    ESP_LOGI(EFFECTS_LOG, "Setting effect params for type %d (%s)", type, getEffectTypeName(type));
    
    switch (type) {
        case EFFECT_TINT:
            tintParams = *(const tint_params_t*)params;
            effectParams[type] = &tintParams;
            break;
        case EFFECT_CHROMATIC:
            chromaticParams = *(const chromatic_params_t*)params;
            effectParams[type] = &chromaticParams;
            break;
        case EFFECT_SCANLINES:
            scanlineParams = *(const scanline_params_t*)params;
            effectParams[type] = &scanlineParams;
            break;
        case EFFECT_DITHERING:
            ditherParams = *(const dither_params_t*)params;
            effectParams[type] = &ditherParams;
            break;
        case EFFECT_GLITCH:
            glitchParams = *(const glitch_params_t*)params;
            effectParams[type] = &glitchParams;
            break;
        case EFFECT_DOT_MATRIX:
            dotMatrixParams = *(const dot_matrix_params_t*)params;
            effectParams[type] = &dotMatrixParams;
            break;
        case EFFECT_PIXELATE:
            pixelateParams = *(const pixelate_params_t*)params;
            effectParams[type] = &pixelateParams;
            break;
        default:
            return false;
    }
    
    return true;
}

/**
 * @brief Get effect parameters
 * @param type Effect type
 * @param params Pointer to store parameters (void pointer to specific param struct)
 * @return true if parameters were retrieved successfully
 */
bool effectsCore_getEffectParams(effect_type_t type, void* params) {
    if (!effectRegistryInitialized || type < 0 || type >= EFFECT_COUNT || !params) {
        return false;
    }
    
    switch (type) {
        case EFFECT_TINT:
            *(tint_params_t*)params = tintParams;
            break;
        case EFFECT_CHROMATIC:
            *(chromatic_params_t*)params = chromaticParams;
            break;
        case EFFECT_SCANLINES:
            *(scanline_params_t*)params = scanlineParams;
            break;
        case EFFECT_DITHERING:
            *(dither_params_t*)params = ditherParams;
            break;
        case EFFECT_GLITCH:
            *(glitch_params_t*)params = glitchParams;
            break;
        case EFFECT_DOT_MATRIX:
            *(dot_matrix_params_t*)params = dotMatrixParams;
            break;
        case EFFECT_PIXELATE:
            *(pixelate_params_t*)params = pixelateParams;
            break;
        default:
            return false;
    }
    
    return true;
}

//==============================================================================
// PERFORMANCE MONITORING
//==============================================================================

void effectsCore_getPerformance(effect_performance_t* stats) {
    if (stats) {
        *stats = performanceStats;
    }
}

void effectsCore_resetPerformance(void) {
    performanceStats.totalPixels = 0;
    performanceStats.effectsApplied = 0;
    performanceStats.processingTime = 0;
    performanceStats.fps = 0.0f;
    lastPerformanceReset = millis();
}

void effectsCore_setPerformanceMonitoring(bool enabled) {
    performanceMonitoringEnabled = enabled;
}

//==============================================================================
// PREFERENCES INTEGRATION
//==============================================================================

/**
 * @brief Save all effect states to preferences
 */
void effectsCore_saveToPreferences(void) {
    // Save effect enabled states 
    bool success = true;
    
    success &= setGlitchEnabled(effectsEnabled[EFFECT_GLITCH]);
    success &= setScanlinesEnabled(effectsEnabled[EFFECT_SCANLINES]);
    success &= setDitheringEnabled(effectsEnabled[EFFECT_DITHERING]);
    success &= setChromaticEnabled(effectsEnabled[EFFECT_CHROMATIC]);
    success &= setDotMatrixEnabled(effectsEnabled[EFFECT_DOT_MATRIX]);
    success &= setPixelateEnabled(effectsEnabled[EFFECT_PIXELATE]);
    success &= setTintEnabled(effectsEnabled[EFFECT_TINT]);
    
    if (!success) {
        ESP_LOGE(EFFECTS_LOG, "Failed to save some effect preferences");
    } else {
        ESP_LOGI(EFFECTS_LOG, "All effect preferences saved successfully");
    }
}

/**
 * @brief Load all effect states from preferences
 */
void effectsCore_loadFromPreferences(void) {
    // Load effect enabled states
    effectsEnabled[EFFECT_GLITCH] = getGlitchEnabled();
    effectsEnabled[EFFECT_SCANLINES] = getScanlinesEnabled();
    effectsEnabled[EFFECT_DITHERING] = getDitheringEnabled();
    effectsEnabled[EFFECT_CHROMATIC] = getChromaticEnabled();
    effectsEnabled[EFFECT_DOT_MATRIX] = getDotMatrixEnabled();
    effectsEnabled[EFFECT_PIXELATE] = getPixelateEnabled();
    
    // Load tint effect state from preferences
    effectsEnabled[EFFECT_TINT] = getTintEnabled();
    
    // Load tint parameters from preferences
    tintParams.tintColor = getTintColor();
    tintParams.intensity = getTintIntensity();
    
    // Set parameter pointers
    effectParams[EFFECT_TINT] = &tintParams;
    effectParams[EFFECT_CHROMATIC] = &chromaticParams;
    effectParams[EFFECT_SCANLINES] = &scanlineParams;
    effectParams[EFFECT_DITHERING] = &ditherParams;
    effectParams[EFFECT_GLITCH] = &glitchParams;
    effectParams[EFFECT_DOT_MATRIX] = &dotMatrixParams;
    effectParams[EFFECT_PIXELATE] = &pixelateParams;
}

//==============================================================================
// SCANLINE PROCESSING
//==============================================================================

/**
 * @brief Apply all enabled effects to a scanline
 * @param pixels Array of RGB565 pixels for current scanline
 * @param width Number of pixels in the scanline
 * @param row Current row number (Y coordinate)
 */
void effectsCore_applyToScanline(uint16_t* pixels, int width, int row) {
    if (!pixels || width <= 0 || !effectRegistryInitialized) {
        return;
    }
    
    unsigned long startTime = micros();
    
    // Apply effects in specific order matching original implementation
    // 1. Tint effect first (affects all pixels)
    if (effectsEnabled[EFFECT_TINT] && effectRegistry[EFFECT_TINT].apply && effectParams[EFFECT_TINT]) {
        for (int i = 0; i < width; i++) {
            uint16_t originalPixel = pixels[i];
            pixels[i] = effectsTints_applyTint(originalPixel, (const tint_params_t*)effectParams[EFFECT_TINT], i, row);
        }
        performanceStats.effectsApplied++;
    }
    
    // 2. Dithering (per-pixel effect)
    if (effectsEnabled[EFFECT_DITHERING] && effectRegistry[EFFECT_DITHERING].apply && effectParams[EFFECT_DITHERING]) {
        for (int i = 0; i < width; i++) {
            pixels[i] = effectsRetro_applyBayerDithering(pixels[i], (const dither_params_t*)effectParams[EFFECT_DITHERING], i, row);
        }
        performanceStats.effectsApplied++;
    }
    
    // 3. Chromatic aberration (scanline effect)
    if (effectsEnabled[EFFECT_CHROMATIC] && effectRegistry[EFFECT_CHROMATIC].apply && effectParams[EFFECT_CHROMATIC]) {
        effectsTints_applyChromaticAberration(pixels, width, row, (const chromatic_params_t*)effectParams[EFFECT_CHROMATIC]);
        performanceStats.effectsApplied++;
    }
    
    // 4. Dot matrix (per-pixel effect)
    if (effectsEnabled[EFFECT_DOT_MATRIX] && effectRegistry[EFFECT_DOT_MATRIX].apply && effectParams[EFFECT_DOT_MATRIX]) {
        effectsMatrix_applyDotMatrixEffect(pixels, width, row, (const dot_matrix_params_t*)effectParams[EFFECT_DOT_MATRIX]);
        performanceStats.effectsApplied++;
    }
    
    // 5. Pixelate (block effect)
    if (effectsEnabled[EFFECT_PIXELATE] && effectRegistry[EFFECT_PIXELATE].apply && effectParams[EFFECT_PIXELATE]) {
        effectsMatrix_applyPixelateEffect(pixels, width, row, (const pixelate_params_t*)effectParams[EFFECT_PIXELATE]);
        performanceStats.effectsApplied++;
    }
    
    // 6. Scanlines (per-pixel effect)
    if (effectsEnabled[EFFECT_SCANLINES] && effectRegistry[EFFECT_SCANLINES].apply && effectParams[EFFECT_SCANLINES]) {
        for (int i = 0; i < width; i++) {
            pixels[i] = effectsRetro_applyScanline(pixels[i], (const scanline_params_t*)effectParams[EFFECT_SCANLINES], row);
        }
        performanceStats.effectsApplied++;
    }
    
    // 7. Glitch effects (scanline effect) - should be last
    if (effectsEnabled[EFFECT_GLITCH] && effectRegistry[EFFECT_GLITCH].apply && effectParams[EFFECT_GLITCH]) {
        effectsRetro_applyCRTGlitches(pixels, width, row, (const glitch_params_t*)effectParams[EFFECT_GLITCH]);
        performanceStats.effectsApplied++;
    }
    
    performanceStats.totalPixels += width;
    performanceStats.processingTime += (micros() - startTime);
}

/**
 * @brief Clear all effect preferences
 */
void effectsCore_clearPreferences(void) {
    // Disable all effects
    effectsCore_disableAllEffects();
    
    // Reset to default parameters
    effectsCore_applyDefaultParams(EFFECT_TINT);
    effectsCore_applyDefaultParams(EFFECT_CHROMATIC);
    effectsCore_applyDefaultParams(EFFECT_SCANLINES);
    effectsCore_applyDefaultParams(EFFECT_DITHERING);
    effectsCore_applyDefaultParams(EFFECT_GLITCH);
    effectsCore_applyDefaultParams(EFFECT_DOT_MATRIX);
    effectsCore_applyDefaultParams(EFFECT_PIXELATE);
    
    // Save the cleared state
    effectsCore_saveToPreferences();
}

/**
 * @brief Apply default parameters to effect
 * @param type Effect type
 * @return true if defaults were applied successfully
 */
bool effectsCore_applyDefaultParams(effect_type_t type) {
    if (!effectRegistryInitialized || type < 0 || type >= EFFECT_COUNT) {
        return false;
    }
    
    switch (type) {
        case EFFECT_TINT:
            tintParams = effectsTints_getDefaultTintParams();
            effectParams[type] = &tintParams;
            break;
        case EFFECT_CHROMATIC:
            chromaticParams = effectsTints_getDefaultChromaticParams();
            effectParams[type] = &chromaticParams;
            break;
        case EFFECT_SCANLINES:
            scanlineParams = effectsRetro_getDefaultScanlineParams();
            effectParams[type] = &scanlineParams;
            break;
        case EFFECT_DITHERING:
            ditherParams = effectsRetro_getDefaultDitherParams();
            effectParams[type] = &ditherParams;
            break;
        case EFFECT_GLITCH:
            glitchParams = effectsRetro_getDefaultGlitchParams();
            effectParams[type] = &glitchParams;
            break;
        case EFFECT_DOT_MATRIX:
            dotMatrixParams = effectsMatrix_getDefaultDotMatrixParams();
            effectParams[type] = &dotMatrixParams;
            break;
        case EFFECT_PIXELATE:
            pixelateParams = effectsMatrix_getDefaultPixelateParams();
            effectParams[type] = &pixelateParams;
            break;
        default:
            return false;
    }
    
    return true;
}
