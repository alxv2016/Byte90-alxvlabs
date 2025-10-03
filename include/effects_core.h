/**
 * @file effects_core.h
 * @brief Core effects management and pipeline functionality
 */

#ifndef EFFECTS_CORE_H
#define EFFECTS_CORE_H

#include "effects_common.h"
#include "preferences_module.h"

//==============================================================================
// CORE EFFECTS MANAGEMENT
//==============================================================================

/**
 * @brief Initialize the effects system
 */
void effectsCore_init(void);

//==============================================================================
// EFFECT ENABLE/DISABLE
//==============================================================================

/**
 * @brief Enable a specific effect
 * @param type Effect type to enable
 * @return true if effect was enabled successfully
 */
bool effectsCore_enableEffect(effect_type_t type);

/**
 * @brief Disable a specific effect
 * @param type Effect type to disable
 * @return true if effect was disabled successfully
 */
bool effectsCore_disableEffect(effect_type_t type);

/**
 * @brief Toggle effect on/off
 * @param type Effect type to toggle
 * @return true if effect is now enabled, false if disabled
 */
bool effectsCore_toggleEffect(effect_type_t type);

/**
 * @brief Check if effect is enabled
 * @param type Effect type to check
 * @return true if effect is enabled
 */
bool effectsCore_isEffectEnabled(effect_type_t type);

/**
 * @brief Disable all effects
 */
void effectsCore_disableAllEffects(void);

//==============================================================================
// EFFECT PARAMETERS
//==============================================================================

/**
 * @brief Set effect parameters
 * @param type Effect type
 * @param params Effect parameters (void pointer to specific param struct)
 * @return true if parameters were set successfully
 */
bool effectsCore_setEffectParams(effect_type_t type, const void* params);

/**
 * @brief Get effect parameters
 * @param type Effect type
 * @param params Pointer to store parameters (void pointer to specific param struct)
 * @return true if parameters were retrieved successfully
 */
bool effectsCore_getEffectParams(effect_type_t type, void* params);

/**
 * @brief Apply default parameters to effect
 * @param type Effect type
 * @return true if defaults were applied successfully
 */
bool effectsCore_applyDefaultParams(effect_type_t type);

//==============================================================================
// PREFERENCES INTEGRATION
//==============================================================================

/**
 * @brief Save all effect states to preferences
 */
void effectsCore_saveToPreferences(void);

/**
 * @brief Load all effect states from preferences
 */
void effectsCore_loadFromPreferences(void);

/**
 * @brief Clear all effect preferences
 */
void effectsCore_clearPreferences(void);

//==============================================================================
// SCANLINE PROCESSING
//==============================================================================

/**
 * @brief Apply all enabled effects to a scanline
 * @param pixels Array of RGB565 pixels for current scanline
 * @param width Number of pixels in the scanline
 * @param row Current row number (Y coordinate)
 */
void effectsCore_applyToScanline(uint16_t* pixels, int width, int row);

#endif /* EFFECTS_CORE_H */
