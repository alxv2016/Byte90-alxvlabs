/**
 * @file menu_themes.h
 * @brief Theme selection menu module
 */

#ifndef MENU_THEMES_H
#define MENU_THEMES_H

#include "menu_common.h"

//==============================================================================
// PUBLIC API
//==============================================================================

/**
 * @brief Get the themes menu items
 * @return Pointer to themes menu array
 */
MenuItem* menuThemes_getItems();

/**
 * @brief Get the number of theme items
 * @return Number of theme menu items
 */
int menuThemes_getItemCount();

/**
 * @brief Get the themes menu context
 * @param context Pointer to context to populate
 */
void menuThemes_getContext(MenuContext* context);

/**
 * @brief Initialize themes module
 */
void menuThemes_init();

#endif /* MENU_THEMES_H */
