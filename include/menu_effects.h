/**
 * @file menu_effects.h
 * @brief Individual effects toggle menu module
 */

#ifndef MENU_EFFECTS_H
#define MENU_EFFECTS_H

#include "menu_common.h"

//==============================================================================
// PUBLIC API
//==============================================================================

/**
 * @brief Get the effects menu items
 * @return Pointer to effects menu array
 */
MenuItem* menuEffects_getItems();

/**
 * @brief Get the number of effect items
 * @return Number of effect menu items
 */
int menuEffects_getItemCount();

/**
 * @brief Get the effects menu context
 * @param context Pointer to context to populate
 */
void menuEffects_getContext(MenuContext* context);

/**
 * @brief Initialize effects module
 */
void menuEffects_init();

/**
 * @brief Update effect status flags
 */
void menuEffects_updateStatus();

/**
 * @brief Enable or disable debug logging for menu effects operations
 * @param enabled true to enable debug logging, false to disable
 */
void setMenuEffectsDebug(bool enabled);

#endif /* MENU_EFFECTS_H */
