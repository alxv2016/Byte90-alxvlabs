/**
 * @file menu_button.h
 * @brief Button input handling for the menu system
 */

#ifndef MENU_BUTTON_H
#define MENU_BUTTON_H

#include "menu_common.h"

//==============================================================================
// PUBLIC API
//==============================================================================

/**
 * @brief Initialize button input system
 */
void menuButton_init();

/**
 * @brief Process button input and return events
 * @return ButtonEvent that occurred, or BUTTON_NONE if no event
 */
ButtonEvent menuButton_getEvent();

/**
 * @brief Check if a long press has occurred and handle it based on menu state
 * @return true if a long press was handled, false otherwise
 */
bool menuButton_checkLongPress(unsigned long longPressTime = MENU_LONG_PRESS_TIME);

/**
 * @brief Reset button state (useful for cleanup)
 */
void menuButton_reset();

#endif /* MENU_BUTTON_H */
