/**
 * @file menu_module.cpp
 * @brief Simplified core menu system - Modular implementation
 *
 * Provides the core menu system functionality for the BYTE-90 device,
 * including menu initialization, updates, state management, and display control.
 * 
 * This module handles:
 * - Menu system initialization and setup
 * - Menu state management and updates
 * - Menu display control and rendering
 * - Menu navigation and interaction
 * - Debug logging and system integration
 * - Menu timeout and activity tracking
 * - Integration with display, button, and other menu modules
 * - Submenu navigation and context management
 * - Button event handling and menu activation
 * - System state transitions and mode management
 */

 #include "menu_module.h"
 #include "menu_button.h"
 #include "menu_display.h"
 #include "menu_themes.h"
 #include "menu_effects.h"
 #include "menu_clock.h"
 #include "menu_wifi.h"
 #include "menu_settings.h"
 #include "gif_module.h"
 #include "motion_module.h"
 #include "common.h"
 #include "haptics_module.h"
 #include "soundsfx_module.h"
 #include "states_module.h"
 #include <stack>
 #include <stdarg.h>
 
//==============================================================================
// DEBUG SYSTEM
//==============================================================================

// Debug control - set to true to enable debug logging
static bool menuModuleDebugEnabled = false;

/**
 * @brief Centralized debug logging function for menu module operations
 * @param format Printf-style format string
 * @param ... Variable arguments for format string
 */
static void menuModuleDebug(const char* format, ...) {
  if (!menuModuleDebugEnabled) {
    return;
  }
  
  va_list args;
  va_start(args, format);
  esp_log_writev(ESP_LOG_INFO, "SETTINGS", format, args);
  va_end(args);
}

/**
 * @brief Enable or disable debug logging for menu module operations
 * @param enabled true to enable debug logging, false to disable
 * 
 * @example
 * // Enable debug logging
 * setMenuModuleDebug(true);
 * 
 * // Disable debug logging  
 * setMenuModuleDebug(false);
 */
void setMenuModuleDebug(bool enabled) {
  menuModuleDebugEnabled = enabled;
  if (enabled) {
    ESP_LOGI("SETTINGS", "Menu module debug logging enabled");
  } else {
    ESP_LOGI("SETTINGS", "Menu module debug logging disabled");
  }
}

//==============================================================================
// GLOBAL STATE
//==============================================================================

// Global state variables (defined in menu_common.h)
bool menuActive = false;
 MenuContext currentMenuContext;
 
 // Private state
 static std::stack<MenuContext> menuStack;
 static unsigned long lastMenuActivity = 0;
 static bool menuInitialized = false;
 
 //==============================================================================
 // MENU ACTION FUNCTIONS
 //==============================================================================
 
 /**
  * @brief Exit menu action function
  */
 static void exitMenuAction() {
     menuModuleDebug("Exit menu action triggered");
     menu_exit();
 }
 
 /**
  * @brief Toggle ESP-NOW pairing mode
  */
 static void toggleESPPairingMode() {
     menuModuleDebug("Starting ESP pairing mode preparation");
 
     menu_exit();
     
     transitionToState(SystemState::ESP_MODE);
     if (getCurrentState() == SystemState::ESP_MODE) {
         menuModuleDebug("Successfully entered ESP pairing mode");
     } else {
         ESP_LOGE("MENU", "Failed to transition to update mode");
     }
 }
 
 /**
  * @brief Handle button press when in update mode to exit
  */
 bool handleESPModeButtonPress() {
     if (getCurrentState() != SystemState::ESP_MODE) {
         return false; // Not in ESP mode - let other handlers deal with it
     }
     // ESP mode should always exit to IDLE mode
     menuModuleDebug("Exiting ESP mode - transitioning to IDLE_MODE");
     transitionToState(SystemState::IDLE_MODE);
     return true;
 }
 
 //==============================================================================
 // CONSERVATIVE DYNAMIC MENU APPROACH
 //==============================================================================
 
 // Pre-defined menu items (safe initialization)
 static MenuItem themesItem = DEFINE_MENU_SUBMENU(themes, MENU_LABEL_THEMES, nullptr, 0);
 static MenuItem effectsItem = DEFINE_MENU_SUBMENU(effects, MENU_LABEL_EFFECTS, nullptr, 0);
 static MenuItem clockItem = DEFINE_MENU_SUBMENU(clock, MENU_LABEL_CLOCK, nullptr, 0);
 static MenuItem wifiItem = DEFINE_MENU_SUBMENU(wifi, MENU_LABEL_WIFI, nullptr, 0);
 static MenuItem espPairingItem = DEFINE_MENU_ACTION(esp_pairing, MENU_LABEL_ESP_PAIRING, toggleESPPairingMode);
 static MenuItem settingsItem = DEFINE_MENU_SUBMENU(settings, MENU_LABEL_SETTINGS, nullptr, 0);
 static MenuItem exitItem = DEFINE_MENU_ACTION(exit, MENU_LABEL_EXIT, exitMenuAction);
 
 // Dynamic menu array (pointers to pre-defined items)
 static MenuItem* dynamicMainMenu[10];
 static int dynamicMainMenuCount = 0;
 
 //==============================================================================
 // SAFE MENU BUILDING
 //==============================================================================
 
 /**
  * @brief Build main menu using safe pointer assignments
  */
 static void buildMainMenu() {
     dynamicMainMenuCount = 0;
     
     // Always include these items
     dynamicMainMenu[dynamicMainMenuCount++] = &themesItem;
     dynamicMainMenu[dynamicMainMenuCount++] = &effectsItem;
     
     // Conditionally include clock menu for SERIES_2
     if (checkHardwareSupport()) {
         menuModuleDebug("Adding clock menu for SERIES_2 hardware");
         dynamicMainMenu[dynamicMainMenuCount++] = &clockItem;
     }
     
     // Always include these items
     dynamicMainMenu[dynamicMainMenuCount++] = &wifiItem;
     dynamicMainMenu[dynamicMainMenuCount++] = &espPairingItem;
     dynamicMainMenu[dynamicMainMenuCount++] = &settingsItem;
     dynamicMainMenu[dynamicMainMenuCount++] = &exitItem;
     
     menuModuleDebug("Built main menu with %d items", dynamicMainMenuCount);
 }
 
 /**
  * @brief Initialize submenu pointers
  */
 static void initializeSubmenus() {
     // Set up submenu pointers for all items
     themesItem.submenu = menuThemes_getItems();
     themesItem.childCount = menuThemes_getItemCount();
     
     effectsItem.submenu = menuEffects_getItems();
     effectsItem.childCount = menuEffects_getItemCount();
     
     clockItem.submenu = menuClock_getItems();
     clockItem.childCount = menuClock_getItemCount();
     
     wifiItem.submenu = menuWifi_getItems();
     wifiItem.childCount = menuWifi_getItemCount();
     
     settingsItem.submenu = menuSettings_getItems();
     settingsItem.childCount = menuSettings_getItemCount();
     
     menuModuleDebug("Initialized all submenus");
 }
 
 /**
  * @brief Convert pointer array to regular array for MenuContext
  */
 static MenuItem actualMenuItems[10];
 
 static void setupMainMenuContext() {
     buildMainMenu();
     initializeSubmenus();
     
     // Copy pointers to actual items
     for (int i = 0; i < dynamicMainMenuCount; i++) {
         actualMenuItems[i] = *dynamicMainMenu[i];
     }
     
     currentMenuContext.items = actualMenuItems;
     currentMenuContext.itemCount = dynamicMainMenuCount;
     currentMenuContext.selectedIndex = 0;
     currentMenuContext.title = MENU_LABEL_MAIN;
     
     menuModuleDebug("Main menu context set up with %d items", dynamicMainMenuCount);
 }
 
 //==============================================================================
 // PRIVATE FUNCTIONS (unchanged from your original)
 //==============================================================================
 
 /**
  * @brief Navigate to next menu item
  */
 static void navigateNext() {
     int startIndex = currentMenuContext.selectedIndex;
     int currentIndex = (startIndex + 1) % currentMenuContext.itemCount;
     
     // Skip over MENU_INFO items (they are read-only)
     while (currentIndex != startIndex && 
            currentMenuContext.items[currentIndex].type == MENU_INFO) {
         currentIndex = (currentIndex + 1) % currentMenuContext.itemCount;
     }
     
     currentMenuContext.selectedIndex = currentIndex;
     lastMenuActivity = millis();
 }
 
 /**
  * @brief Enter a submenu
  */
 static void enterSubmenu(const MenuItem* item) {
     if (item->type != MENU_SUBMENU || !item->submenu) {
         ESP_LOGE("MENU", "Invalid submenu item - type: %d, submenu: %p", item->type, item->submenu);
         return;
     }
     
     menuModuleDebug("Entering submenu: %s", item->label);
     
     // Save current context to stack
     menuStack.push(currentMenuContext);
     
     // Set up new context
     if (item->submenu == menuThemes_getItems()) {
         menuModuleDebug("Setting up THEMES submenu");
         menuThemes_getContext(&currentMenuContext);
     } else if (item->submenu == menuEffects_getItems()) {
         menuModuleDebug("Setting up EFFECTS submenu");
         menuEffects_getContext(&currentMenuContext);
     } else if (item->submenu == menuClock_getItems()) {
         menuModuleDebug("Setting up CLOCK submenu");
         menuClock_getContext(&currentMenuContext);
     } else if (item->submenu == menuWifi_getItems()) {
         menuModuleDebug("Setting up WIFI submenu");
         menuWifi_getContext(&currentMenuContext);
     } else if (item->submenu == menuSettings_getItems()) {
         menuModuleDebug("Setting up SETTINGS submenu");
         menuSettings_getContext(&currentMenuContext);
     } else if (item->submenu == menuTimezone_getItems()) {
         menuModuleDebug("Setting up TIMEZONE submenu");
         menuTimezone_getContext(&currentMenuContext);
     } else if (item->submenu == menuWifiStatus_getItems()) {
         menuModuleDebug("Setting up WIFI STATUS submenu");
         menuWifiStatus_getContext(&currentMenuContext);
     } else {
         ESP_LOGE("MENU", "Unknown submenu pointer: %p", item->submenu);
     }
     
     menuModuleDebug("Submenu context: items=%p, count=%d, title=%s", 
              currentMenuContext.items, currentMenuContext.itemCount, currentMenuContext.title);
     
     lastMenuActivity = millis();
 }
 
 /**
  * @brief Go back to parent menu
  */
 static void goBack() {
     if (!menuStack.empty()) {
         // Restore previous context
         currentMenuContext = menuStack.top();
         menuStack.pop();
     } else {
         // Exit to normal operation
         menu_exit();
         return;
     }
     
     lastMenuActivity = millis();
 }
 
 /**
  * @brief Execute current menu item action
  */
 static void selectCurrentItem() {
     if (currentMenuContext.selectedIndex >= currentMenuContext.itemCount) {
         ESP_LOGE("MENU", "Invalid selection: %d >= %d", currentMenuContext.selectedIndex, currentMenuContext.itemCount);
         return;
     }
     
     MenuItem* item = &currentMenuContext.items[currentMenuContext.selectedIndex];
     menuModuleDebug("Selecting item: %s (type: %d)", item->label, item->type);
     
     switch (item->type) {
         case MENU_SUBMENU:
             menuModuleDebug("Entering submenu...");
             enterSubmenu(item);
             break;
             
         case MENU_TOGGLE:
             if (item->toggle && item->statusFlag) {
                 bool currentState = *item->statusFlag;
                 bool newState = !currentState;
                 menuModuleDebug("Toggling %s: %s -> %s", item->label, 
                          currentState ? "ON" : "OFF", newState ? "ON" : "OFF");
                 item->toggle(newState);
                 *item->statusFlag = newState;
                 menuModuleDebug("Toggle complete - new state: %s", (*item->statusFlag) ? "ON" : "OFF");
                 
                 // Force redraw to update the toggle status display
                 menuDisplay_forceRedraw();
             } else {
                 ESP_LOGE("MENU", "Invalid toggle item - toggle: %p, statusFlag: %p", item->toggle, item->statusFlag);
             }
             break;
             
         case MENU_ACTION:
             if (item->action) {
                 item->action();
                 // Auto-exit after actions (like applying themes)
                 // But don't exit for UPDATE_MODE since it needs to stay in that mode
                 if (strcmp(item->label, MENU_LABEL_UPDATE_MODE) != 0) {
                     menu_exit();
                     return;
                 }
             }
             break;
             
         case MENU_INFO:
             menuModuleDebug("Info item selected: %s (read-only)", item->label);
             // Info items are read-only, no action needed
             break;
             
         case MENU_BACK:
             goBack();
             return;
     }
 
     lastMenuActivity = millis();
 }
 
 /**
  * @brief Check for menu timeout
  */
 static void checkMenuTimeout() {
     if (menuActive && (millis() - lastMenuActivity) > MENU_TIMEOUT) {
         menu_exit();
     }
 }
 
 /**
  * @brief Update status of all modules
  */
 static void updateModuleStatus() {
     menuEffects_updateStatus();
     menuSettings_updateStatus();
 }
 
 //==============================================================================
 // PUBLIC API IMPLEMENTATION
 //==============================================================================
 
/**
 * @brief Initialize the menu system
 * 
 * Initializes the core menu system including all submenu modules,
 * button handling, display management, and menu state tracking.
 * Sets up the main menu context and initializes the menu stack.
 * 
 */
void menu_init() {
     if (menuInitialized) {
         return;
     }
     
     // Initialize all modules
     menuButton_init();
     menuDisplay_init();
     menuThemes_init();
     menuEffects_init();
     menuClock_init();
     menuWifi_init();
     menuSettings_init();
     
     // Set up menu structure
     setupMainMenuContext();
     
     // Initialize state
     menuActive = false;
     lastMenuActivity = millis();
     
     // Clear menu stack
     while (!menuStack.empty()) {
         menuStack.pop();
     }
     
     menuInitialized = true;
     menuModuleDebug("Menu system initialized successfully");
 }
 
/**
 * @brief Update menu system - call this in main loop
 * 
 * Updates the menu system including button handling, timeout checking,
 * and menu state management. Should be called regularly in the main loop
 * to ensure proper menu functionality and responsiveness.
 * 
 */
void menu_update() {
     if (!menuInitialized) {
         return;
     }
     
     // Check for menu timeout
     checkMenuTimeout();
     
     // Check for long press first (regardless of menu state)
     if (menuButton_checkLongPress(MENU_LONG_PRESS_TIME)) {
         ButtonEvent event = menuButton_getEvent();
         if (event == BUTTON_LONG_PRESS) {
             if (menuActive) {
                 // Long press in menu - exit menu only
                 menuModuleDebug("Long press detected in menu - exiting menu");
                 menu_exit();
             } else {
                 // Long press outside menu - enter deep sleep
                 menuModuleDebug("Long press detected outside menu - entering deep sleep");
                 handleDeepSleep();
             }
             return;
         }
     }
     
     // If menu is not active, check for activation
     if (!menuActive) {
         ButtonEvent event = menuButton_getEvent();
         
         // Check if we're in clock mode and handle single-click exit
         if (event == BUTTON_SINGLE_CLICK && handleClockModeButtonPress()) {
             // Clock mode was exited, don't activate menu
             return;
         }
         
         // Check if we're in update mode and handle single-click exit
         if (event == BUTTON_SINGLE_CLICK && handleUpdateModeButtonPress()) {
             // Update mode was exited, don't activate menu
             return;
         }

         // Check if we're in ESP mode and handle single-click exit
         if (event == BUTTON_SINGLE_CLICK && handleESPModeButtonPress()) {
             // ESP mode was exited, don't activate menu
             return;
         }
         
         if (event == BUTTON_SINGLE_CLICK || event == BUTTON_DOUBLE_CLICK) {
             // Activate menu
             menuActive = true;
             stopGifPlayback();
             menuDisplay_clear();  // Clear display to remove any GIF artifacts
             setupMainMenuContext();
             updateModuleStatus();
             lastMenuActivity = millis();
             menu_updateDisplay();
         }
         return;
     }
     
     // Process button events when menu is active
     ButtonEvent event = menuButton_getEvent();
     
     switch (event) {
         case BUTTON_SINGLE_CLICK:
             // Single click - navigating to next item
             navigateNext();
             menu_updateDisplay();
             break;
             
         case BUTTON_DOUBLE_CLICK:
             // Double click - selecting current item
             selectCurrentItem();
             if (menuActive) { // May have been deactivated by action
                 menu_updateDisplay();
             }
             break;
             
         default:
             // No event
             break;
     }
 }
 
/**
 * @brief Check if menu is currently active
 * @return true if menu is active, false if in normal operation
 * 
 * Returns the current state of the menu system. Used by other modules
 * to determine if the device is in menu mode or normal operation mode.
 *
 */
bool menu_isActive() {
     return menuActive;
 }
 
/**
 * @brief Exit menu system and return to normal operation
 * 
 * Exits the menu system and returns the device to normal operation mode.
 * Resets menu state and updates the display to show normal operation content.
 * 
 */
void menu_exit() {
     menuActive = false;
     
     // Clear menu stack
     while (!menuStack.empty()) {
         menuStack.pop();
     }
     
     // Reset to main menu for next time
     setupMainMenuContext();
     
     // Clear display and resume normal operation
     menuDisplay_clear();
     
     lastMenuActivity = millis();
 }
 
/**
 * @brief Reset menu to initial state
 * 
 * Resets the menu system to its initial state, clearing all navigation
 * history and returning to the main menu. Useful for error recovery
 * or when returning to a known good state.
 * 
 */
void menu_reset() {
     menu_exit();
     menuButton_reset();
 }
 
/**
 * @brief Force menu display update
 * 
 * Forces an immediate update of the menu display. Useful when menu
 * content has changed and needs to be refreshed immediately.
 * Handles both menu and normal operation display modes.
 * 
 */
void menu_updateDisplay() {
     if (menuActive) {
         updateModuleStatus(); // Ensure status is current
         menuDisplay_draw(&currentMenuContext);
     }
 }