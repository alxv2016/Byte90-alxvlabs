/**
 * @file menu_common.h
 * @brief Shared definitions and structures for the modular menu system
 */

#ifndef MENU_COMMON_H
#define MENU_COMMON_H

#include "common.h"
#include <Adafruit_GFX.h>

//==============================================================================
// CONSTANTS & DEFINITIONS
//==============================================================================

// Hardware configuration
#if SERIES_2
  #define MENU_BUTTON_PIN D9
#else
  #define MENU_BUTTON_PIN A3
#endif

// Timing constants
#define MENU_LONG_PRESS_TIME 2000
#define MENU_DOUBLE_CLICK_TIME 500
#define MENU_DEBOUNCE_TIME 50
#define MENU_TIMEOUT 30000

// Display constants
#define MENU_TEXT_SIZE 1
#define MENU_ITEM_Y_OFFSET 3
#define MENU_ITEM_X_OFFSET 3
#define MENU_PADDING 6

// Menu labels
#define MENU_LABEL_MAIN "SETTINGS"
#define MENU_LABEL_THEMES "THEMES"
#define MENU_LABEL_EFFECTS "EFFECTS"
#define MENU_LABEL_SETTINGS "SETTINGS"
#define MENU_LABEL_GO_BACK "GO BACK"

// Main menu labels
#define MENU_LABEL_CLOCK "CLOCK"
#define MENU_LABEL_WIFI "WIFI"
#define MENU_LABEL_ESP_PAIRING "BYTE-90 PAIRING"
#define MENU_LABEL_EXIT "EXIT"

// WiFi menu labels
#define MENU_LABEL_UPDATE_MODE "UPDATE MODE"
#define MENU_LABEL_RESTART_WIFI "RESTART WIFI"
#define MENU_LABEL_FORGET_WIFI "FORGET WIFI"
#define MENU_LABEL_WIFI_STATUS "STATUS"
#define MENU_LABEL_WIFI_IP "IP"
#define MENU_LABEL_WIFI_SIGNAL "SIGNAL"

// Clock menu labels
#define MENU_LABEL_CLOCK_MODE "CLOCK MODE"
#define MENU_LABEL_TIMEZONE "TIMEZONE"
#define MENU_LABEL_TZ_EASTERN "EASTERN"
#define MENU_LABEL_TZ_CENTRAL "CENTRAL"
#define MENU_LABEL_TZ_MOUNTAIN "MOUNTAIN"
#define MENU_LABEL_TZ_PACIFIC "PACIFIC"
#define MENU_LABEL_TZ_UTC "UTC"
#define MENU_LABEL_TZ_UK "UK"
#define MENU_LABEL_TZ_EUROPE "EUROPE"
#define MENU_LABEL_TZ_JAPAN "JAPAN"
#define MENU_LABEL_TZ_CHINA "CHINA"
#define MENU_LABEL_TZ_AUSTRALIA "AUSTRALIA"

// Effects menu labels
#define MENU_LABEL_DITHERING "DITHERING"
#define MENU_LABEL_CHROMATIC "CHROMATIC"
#define MENU_LABEL_DOT_MATRIX "DOT MATRIX"
#define MENU_LABEL_PIXELATE "PIXELATE"
#define MENU_LABEL_SCANLINES "SCANLINES"
#define MENU_LABEL_GLITCH "GLITCH"

// Settings menu labels
#define MENU_LABEL_AUDIO "AUDIO"
#define MENU_LABEL_HAPTIC "HAPTIC"
#define MENU_LABEL_CLEAR_EFFECTS "CLEAR EFFECTS"

// Toggle action labels
#define MENU_LABEL_ENABLE_WIFI "ENABLE WIFI"
#define MENU_LABEL_DISABLE_WIFI "DISABLE WIFI"
#define MENU_LABEL_ENABLE_AUDIO "ENABLE AUDIO"
#define MENU_LABEL_DISABLE_AUDIO "DISABLE AUDIO"
#define MENU_LABEL_ENABLE_HAPTIC "ENABLE HAPTIC"
#define MENU_LABEL_DISABLE_HAPTIC "DISABLE HAPTIC"
#define MENU_LABEL_ENABLE_CLOCK "ENABLE CLOCK"
#define MENU_LABEL_DISABLE_CLOCK "DISABLE CLOCK"

// Timezone labels
#define MENU_LABEL_TZ_EASTERN_ACTIVE "EASTERN ○"
#define MENU_LABEL_TZ_CENTRAL_ACTIVE "CENTRAL ○"
#define MENU_LABEL_TZ_MOUNTAIN_ACTIVE "MOUNTAIN ○"
#define MENU_LABEL_TZ_PACIFIC_ACTIVE "PACIFIC ○"
#define MENU_LABEL_TZ_UTC_ACTIVE "UTC ○"
#define MENU_LABEL_TZ_UK_ACTIVE "UK ○"
#define MENU_LABEL_TZ_EUROPE_ACTIVE "EUROPE ○"
#define MENU_LABEL_TZ_JAPAN_ACTIVE "JAPAN ○"
#define MENU_LABEL_TZ_CHINA_ACTIVE "CHINA ○"
#define MENU_LABEL_TZ_AUSTRALIA_ACTIVE "AUSTRALIA ○"

// Themes menu labels
#define MENU_LABEL_THEME_NONE "NONE"
#define MENU_LABEL_THEME_GREEN "GREEN"
#define MENU_LABEL_THEME_YELLOW "YELLOW"
#define MENU_LABEL_THEME_BLUE "BLUE"
#define MENU_LABEL_THEME_GAMEBOY "GAMEBOY"
#define MENU_LABEL_THEME_MONOCHROME "MONOCHROME"

//==============================================================================
// TYPE DEFINITIONS
//==============================================================================

/**
 * @brief Types of menu items
 */
enum MenuItemType {
    MENU_SUBMENU,      // Navigate to submenu
    MENU_TOGGLE,       // Boolean toggle with status
    MENU_ACTION,       // Execute action function
    MENU_INFO,         // Read-only information display
    MENU_BACK          // Go back to parent menu
};

/**
 * @brief Button events
 */
enum ButtonEvent {
    BUTTON_NONE,
    BUTTON_SINGLE_CLICK,
    BUTTON_DOUBLE_CLICK,
    BUTTON_LONG_PRESS
};

/**
 * @brief Button states for state machine
 */
enum ButtonState {
    BTN_IDLE,
    BTN_PRESSED,
    BTN_RELEASED,
    BTN_POTENTIAL_DOUBLE
};

// Forward declaration
struct MenuItem;

/**
 * @brief Function pointer types
 */
typedef void (*ActionFunction)();
typedef void (*ToggleFunction)(bool enabled);
typedef String (*InfoFunction)();

/**
 * @brief Universal menu item structure
 */
struct MenuItem {
    const char* label;          // Display text
    MenuItemType type;          // Type of menu item
    
    union {
        ActionFunction action;   // For MENU_ACTION
        ToggleFunction toggle;   // For MENU_TOGGLE
        MenuItem* submenu;       // For MENU_SUBMENU
        InfoFunction info;       // For MENU_INFO
    };
    
    bool* statusFlag;           // For MENU_TOGGLE (nullptr if not applicable)
    int childCount;             // For MENU_SUBMENU (0 if not applicable)
};

/**
 * @brief Menu context for navigation
 */
struct MenuContext {
    MenuItem* items;            // Current menu items
    int itemCount;             // Number of items in current menu
    int selectedIndex;         // Currently selected item
    const char* title;         // Menu title
};

//==============================================================================
// GLOBAL STATE VARIABLES
//==============================================================================

extern bool menuActive;
extern MenuContext currentMenuContext;

//==============================================================================
// UTILITY MACROS
//==============================================================================

#define MENU_ITEM_COUNT(array) (sizeof(array) / sizeof(array[0]))

#define DEFINE_MENU_TOGGLE(name, label, toggleFunc, statusPtr) \
    {label, MENU_TOGGLE, {.toggle = toggleFunc}, statusPtr, 0}

#define DEFINE_MENU_ACTION(name, label, actionFunc) \
    {label, MENU_ACTION, {.action = actionFunc}, nullptr, 0}

#define DEFINE_MENU_SUBMENU(name, label, submenuPtr, count) \
    {label, MENU_SUBMENU, {.submenu = submenuPtr}, nullptr, count}

#define DEFINE_MENU_INFO(name, label, infoFunc) \
    {label, MENU_INFO, {.info = infoFunc}, nullptr, 0}

#define DEFINE_MENU_BACK() \
    {"GO BACK", MENU_BACK, {.action = nullptr}, nullptr, 0}

#endif /* MENU_COMMON_H */
