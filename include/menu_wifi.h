/**
 * @file menu_wifi.h
 * @brief WiFi and network settings menu module (includes update mode)
 */

#ifndef MENU_WIFI_H
#define MENU_WIFI_H

#include "menu_common.h"

//==============================================================================
// PUBLIC API
//==============================================================================

/**
 * @brief Get the WiFi menu items
 * @return Pointer to WiFi menu array
 */
MenuItem* menuWifi_getItems();

/**
 * @brief Get the number of WiFi items
 * @return Number of WiFi menu items
 */
int menuWifi_getItemCount();

/**
 * @brief Get the WiFi menu context
 * @param context Pointer to context to populate
 */
void menuWifi_getContext(MenuContext* context);

/**
 * @brief Initialize WiFi menu module
 */
void menuWifi_init();

/**
 * @brief Enable or disable debug logging for menu WiFi operations
 * @param enabled true to enable debug logging, false to disable
 */
void setMenuWifiDebug(bool enabled);

/**
 * @brief Handle button press when in update mode to exit
 * @return true if button press was handled (update mode was exited), false otherwise
 */
bool handleUpdateModeButtonPress();

//==============================================================================
// STATUS SUBMENU API
//==============================================================================

/**
 * @brief Get the WiFi status submenu items
 * @return Pointer to WiFi status submenu array
 */
MenuItem* menuWifiStatus_getItems();

/**
 * @brief Get the number of WiFi status items
 * @return Number of WiFi status menu items
 */
int menuWifiStatus_getItemCount();

/**
 * @brief Get the WiFi status submenu context
 * @param context Pointer to context to populate
 */
void menuWifiStatus_getContext(MenuContext* context);

#endif /* MENU_WIFI_H */
