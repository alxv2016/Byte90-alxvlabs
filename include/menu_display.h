/**
 * @file menu_display.h
 * @brief Universal menu drawing and UI rendering
 */

#ifndef MENU_DISPLAY_H
#define MENU_DISPLAY_H

#include "menu_common.h"

//==============================================================================
// PUBLIC API
//==============================================================================

/**
 * @brief Initialize the menu display system
 */
void menuDisplay_init();

/**
 * @brief Draw the current menu context
 * @param context Menu context to draw
 */
void menuDisplay_draw(const MenuContext* context);

/**
 * @brief Clear the display
 */
void menuDisplay_clear();

/**
 * @brief Draw a menu header
 * @param title Header text to display
 */
void menuDisplay_drawHeader(const char* title);

/**
 * @brief Draw a single menu item
 * @param index Item index for positioning
 * @param item Menu item to draw
 * @param selected Whether this item is selected
 */
void menuDisplay_drawItem(int index, const MenuItem* item, bool selected);

/**
 * @brief Force a full redraw on next draw call
 * Use when toggle states or item content changes
 */
void menuDisplay_forceRedraw();

#endif /* MENU_DISPLAY_H */
