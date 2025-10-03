/**
 * @file menu_module.h
 * @brief Simplified core menu system - Modular implementation
 */

#ifndef MENU_MODULE_H
#define MENU_MODULE_H

#include "menu_common.h"

//==============================================================================
// PUBLIC API
//==============================================================================

/**
 * @brief Initialize the menu system
 */
void menu_init();

/**
 * @brief Update menu system - call this in main loop
 */
void menu_update();

/**
 * @brief Check if menu is currently active
 * @return true if menu is active, false if in normal operation
 */
bool menu_isActive();

/**
 * @brief Exit menu system and return to normal operation
 */
void menu_exit();

/**
 * @brief Reset menu to initial state
 */
void menu_reset();

/**
 * @brief Force menu display update
 */
void menu_updateDisplay();

/**
 * @brief Enable or disable debug logging for menu module operations
 * @param enabled true to enable debug logging, false to disable
 */
void setMenuModuleDebug(bool enabled);

#endif /* MENU_MODULE_H */
