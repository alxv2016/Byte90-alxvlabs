/**
 * @file menu_clock.h
 * @brief Clock and time settings menu module
 */

#ifndef MENU_CLOCK_H
#define MENU_CLOCK_H

#include "menu_common.h"
#include "menu_button.h"

//==============================================================================
// PUBLIC API
//==============================================================================

/**
 * @brief Get the clock menu items
 * @return Pointer to clock menu array
 */
MenuItem* menuClock_getItems();

/**
 * @brief Get the number of clock items
 * @return Number of clock menu items
 */
int menuClock_getItemCount();

/**
 * @brief Get the clock menu context
 * @param context Pointer to context to populate
 */
void menuClock_getContext(MenuContext* context);

/**
 * @brief Initialize clock menu module
 */
void menuClock_init();

/**
 * @brief Handle button press when in clock mode (single-click exit)
 * @return true if clock mode was exited, false if not in clock mode
 */
bool handleClockModeButtonPress();

/**
 * @brief Get the timezone submenu items
 * @return Pointer to timezone menu array
 */
MenuItem* menuTimezone_getItems();

/**
 * @brief Get the number of timezone submenu items
 * @return Number of timezone menu items
 */
int menuTimezone_getItemCount();

/**
 * @brief Get the timezone submenu context
 * @param context Pointer to context to populate
 */
void menuTimezone_getContext(MenuContext* context);

/**
 * @brief Enable or disable debug logging for menu clock operations
 * @param enabled true to enable debug logging, false to disable
 */
void setMenuClockDebug(bool enabled);

#endif /* MENU_CLOCK_H */
