/**
 * @file menu_themes.cpp
 * @brief Theme selection menu implementation
 *
 * Provides theme selection functionality for the BYTE-90 device, allowing users
 * to choose from various visual themes including tint effects and color schemes.
 * 
 * This module handles:
 * - Theme selection menu items and navigation
 * - Theme application and effect management
 * - Tint effect configuration (green, blue, red, amber, purple, cyan)
 * - Theme persistence using preferences module
 * - Integration with effects core and tint modules
 * - Menu context management and item counting
 * - Theme initialization and setup
 */

#include "menu_themes.h"
#include "effects_core.h"
#include "effects_tints.h"
#include "effects_themes.h"
#include "gif_module.h"
#include "preferences_module.h"

//==============================================================================
// THEME ACTION FUNCTIONS
//==============================================================================

/**
 * @brief Apply no theme
 * 
 * Disables all tint effects and returns the display to the default state.
 * Saves the theme preference to persistent storage.
 */
static void applyNoneTheme() {
    // Disable all tint effects
    effectsCore_disableEffect(EFFECT_TINT);
    setTintEnabled(false);
    effectsCore_saveToPreferences();
}

/**
 * @brief Apply green tint theme
 * 
 * Applies a green tint effect to the display with 80% intensity.
 * Enables the tint effect, sets the color to green, and saves preferences.
 */
static void applyGreenTheme() {
    effectsCore_enableEffect(EFFECT_TINT);
    tint_params_t params = effectsTints_getDefaultTintParams();
    params.tintColor = TINT_GREEN_400;
    params.intensity = 0.8f;
    effectsCore_setEffectParams(EFFECT_TINT, &params);
    setTintEnabled(true);
    setTintColor(TINT_GREEN_400);
    setTintIntensity(0.8f);
    effectsCore_saveToPreferences();
}

/**
 * @brief Apply yellow tint theme
 * 
 * Applies a yellow tint effect to the display with 80% intensity.
 * Enables the tint effect, sets the color to yellow, and saves preferences.
 */
static void applyYellowTheme() {
    effectsCore_enableEffect(EFFECT_TINT);
    tint_params_t params = effectsTints_getDefaultTintParams();
    params.tintColor = TINT_YELLOW_400;
    params.intensity = 0.8f;
    effectsCore_setEffectParams(EFFECT_TINT, &params);
    setTintEnabled(true);
    setTintColor(TINT_YELLOW_400);
    setTintIntensity(0.8f);
    effectsCore_saveToPreferences();
}

/**
 * @brief Apply blue tint theme
 * 
 * Applies a blue tint effect to the display with 80% intensity.
 * Enables the tint effect, sets the color to blue, and saves preferences.
 */
static void applyBlueTheme() {
    effectsCore_enableEffect(EFFECT_TINT);
    tint_params_t params = effectsTints_getDefaultTintParams();
    params.tintColor = TINT_BLUE_400;
    params.intensity = 0.8f;
    effectsCore_setEffectParams(EFFECT_TINT, &params);
    setTintEnabled(true);
    setTintColor(TINT_BLUE_400);
    setTintIntensity(0.8f);
    effectsCore_saveToPreferences();
}

/**
 * @brief Apply gameboy tint theme
 * 
 * Applies a Game Boy-style green tint effect to the display with 90% intensity.
 * Enables the tint effect, sets the color to Game Boy green, and saves preferences.
 */
static void applyGameboyTheme() {
    effectsCore_enableEffect(EFFECT_TINT);
    tint_params_t params = effectsTints_getDefaultTintParams();
    params.tintColor = GAMEBOY_400;
    params.intensity = 0.9f;
    effectsCore_setEffectParams(EFFECT_TINT, &params);
    setTintEnabled(true);
    setTintColor(GAMEBOY_400);
    setTintIntensity(0.9f);
    effectsCore_saveToPreferences();
}

/**
 * @brief Apply monochrome tint theme
 * 
 * Applies a monochrome calculator-style tint effect to the display with 90% intensity.
 * Enables the tint effect, sets the color to monochrome, and saves preferences.
 */
static void applyMonochromeTheme() {
    effectsCore_enableEffect(EFFECT_TINT);
    tint_params_t params = effectsTints_getDefaultTintParams();
    params.tintColor = MONOCHROME_400;
    params.intensity = 0.9f;
    effectsCore_setEffectParams(EFFECT_TINT, &params);
    setTintEnabled(true);
    setTintColor(MONOCHROME_400);
    setTintIntensity(0.9f);
    effectsCore_saveToPreferences();
}

//==============================================================================
// MENU DEFINITION
//==============================================================================

/**
 * @brief Themes menu items
 */
static MenuItem themesMenu[] = {
    DEFINE_MENU_ACTION(none_theme, MENU_LABEL_THEME_NONE, applyNoneTheme),
    DEFINE_MENU_ACTION(green_theme, MENU_LABEL_THEME_GREEN, applyGreenTheme),
    DEFINE_MENU_ACTION(yellow_theme, MENU_LABEL_THEME_YELLOW, applyYellowTheme),
    DEFINE_MENU_ACTION(blue_theme, MENU_LABEL_THEME_BLUE, applyBlueTheme),
    DEFINE_MENU_ACTION(gameboy_theme, MENU_LABEL_THEME_GAMEBOY, applyGameboyTheme),
    DEFINE_MENU_ACTION(monochrome_theme, MENU_LABEL_THEME_MONOCHROME, applyMonochromeTheme),
    DEFINE_MENU_BACK()
};

static const int themesMenuCount = MENU_ITEM_COUNT(themesMenu);

//==============================================================================
// PUBLIC API FUNCTIONS
//==============================================================================

/**
 * @brief Initialize themes module
 * 
 * Initializes the themes module for theme selection functionality.
 * Currently no special initialization is required as themes are
 * managed through the effects and preferences modules.
 */
void menuThemes_init() {
    // Themes module initialization
    // Currently no special initialization needed
}

/**
 * @brief Get the themes menu items
 * @return Pointer to themes menu array
 * 
 * Returns a pointer to the themes menu items array containing all available
 * theme options including None, Green, Blue, Red, Amber, Purple, and Cyan themes.
 */
MenuItem* menuThemes_getItems() {
    return themesMenu;
}

/**
 * @brief Get the number of theme items
 * @return Number of theme menu items
 * 
 * Returns the total number of theme menu items available for selection.
 * Used by the menu system to determine menu size and navigation bounds.
 */
int menuThemes_getItemCount() {
    return themesMenuCount;
}

/**
 * @brief Get the themes menu context
 * @param context Pointer to context to populate
 * 
 * Populates the provided MenuContext structure with themes menu information
 * including items array, item count, selected index, and menu title. Used by
 * the menu system to initialize theme selection menus.
 */
void menuThemes_getContext(MenuContext* context) {
    if (context) {
        context->items = themesMenu;
        context->itemCount = themesMenuCount;
        context->selectedIndex = 0;
        context->title = MENU_LABEL_THEMES;
    }
}
