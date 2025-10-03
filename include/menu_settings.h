/**
 * @file menu_settings.h
 * @brief System settings menu module (WiFi, Audio, Clock, etc.)
 */

#ifndef MENU_SETTINGS_H
#define MENU_SETTINGS_H

#include "menu_common.h"

//==============================================================================
// PUBLIC API
//==============================================================================

/**
 * @brief Get the settings menu items
 * @return Pointer to settings menu array
 */
MenuItem* menuSettings_getItems();

/**
 * @brief Get the number of setting items
 * @return Number of setting menu items
 */
int menuSettings_getItemCount();

/**
 * @brief Get the settings menu context
 * @param context Pointer to context to populate
 */
void menuSettings_getContext(MenuContext* context);

/**
 * @brief Initialize settings module
 */
void menuSettings_init();

/**
 * @brief Update settings status flags
 */
void menuSettings_updateStatus();

/**
 * @brief Enable or disable debug logging for menu settings operations
 * @param enabled true to enable debug logging, false to disable
 */
void setMenuSettingsDebug(bool enabled);

#endif /* MENU_SETTINGS_H */
