/**
 * @file menu_display.cpp
 * @brief Universal menu drawing and UI rendering implementation
 *
 * Provides comprehensive menu display functionality for the BYTE-90 device,
 * including menu rendering, scrolling, item display, and UI state management.
 * 
 * This module handles:
 * - Menu display initialization and setup
 * - Menu context rendering and updates
 * - Individual menu item display with proper formatting
 * - Menu header drawing and styling
 * - Scrolling support for long menu lists
 * - Display state tracking and optimization
 * - Text truncation and layout management
 * - Toggle item status display and indicators
 * - Theme and timezone active state visualization
 * - Color management and DOS-style theming
 * - Performance optimization through selective redraws
 */

#include "menu_display.h"
#include "display_module.h"
#include "effects_core.h"
#include "effects_tints.h"
#include "preferences_module.h"
#include "clock_sync.h"

// External display object
extern Adafruit_SSD1351 oled;

//==============================================================================
// PRIVATE STATE - For flicker reduction and scrolling
//==============================================================================

static int lastSelectedIndex = -1;
static int lastItemCount = -1;
static String lastTitle = "";
static bool forceFullRedraw = true;
static int scrollOffset = 0;  // For scrolling long lists

//==============================================================================
// SCROLLING CONSTANTS
//==============================================================================

#define FIRST_ITEM_Y 28         // Y position where first menu item starts

/**
 * @brief Calculate maximum visible items dynamically based on display geometry
 */
static int calculateMaxVisibleItems() {
    // Calculate item spacing: text height + padding + offset
    int itemSpacing = (8 + MENU_ITEM_Y_OFFSET) + MENU_ITEM_Y_OFFSET;  // (8+3)+3 = 14 pixels
    
    // Available height for items = total height - header space
    int availableHeight = DISPLAY_HEIGHT - FIRST_ITEM_Y;  // 128 - 28 = 100 pixels
    
    // Calculate how many items fit
    int maxItems = availableHeight / itemSpacing;  // 100 / 14 = 7.14 -> 7 items
    return maxItems;
}

//==============================================================================
// PRIVATE FUNCTION DECLARATIONS
//==============================================================================

static void menuDisplay_drawScrollIndicators(int scrollOffset, int totalItems, int maxVisibleItems);

//==============================================================================
// PRIVATE FUNCTIONS
//==============================================================================

/**
 * @brief Get current DOS colors for UI
 */
static void getCurrentColors(uint16_t* primaryColor, uint16_t* accentColor) {
    getDOSColorsForCurrentTint(primaryColor, accentColor);
}

/**
 * @brief Truncate text to fit within available width, adding "..." if needed
 * @param text The text to truncate
 * @param maxWidth Maximum width in pixels
 * @return Truncated text string
 */
static String truncateText(const String& text, int maxWidth) {
    // Set text size for measurement
    oled.setTextSize(MENU_TEXT_SIZE);
    
    // Check if text fits
    int16_t x1, y1;
    uint16_t textWidth, textHeight;
    oled.getTextBounds(text.c_str(), 0, 0, &x1, &y1, &textWidth, &textHeight);
    
    if (textWidth <= maxWidth) {
        return text; // Text fits, no truncation needed
    }
    
    // Text is too long, need to truncate
    String truncated = text;
    String ellipsis = "...";
    
    // Get width of ellipsis
    oled.getTextBounds(ellipsis.c_str(), 0, 0, &x1, &y1, &textWidth, &textHeight);
    int ellipsisWidth = textWidth;
    
    // Calculate available width for text (total width minus ellipsis)
    int availableWidth = maxWidth - ellipsisWidth;
    
    // Binary search to find the right length
    int left = 0;
    int right = truncated.length();
    String bestFit = "";
    
    while (left <= right) {
        int mid = (left + right) / 2;
        String testText = truncated.substring(0, mid);
        
        oled.getTextBounds(testText.c_str(), 0, 0, &x1, &y1, &textWidth, &textHeight);
        
        if (textWidth <= availableWidth) {
            bestFit = testText;
            left = mid + 1; // Try longer
        } else {
            right = mid - 1; // Try shorter
        }
    }
    
    return bestFit + ellipsis;
}

/**
 * @brief Determine if a toggle item should show action text instead of dots
 */
static bool shouldShowActionText(const char* itemLabel) {
    // Check if this is a toggle item that should show action text
    return (strcmp(itemLabel, MENU_LABEL_WIFI) == 0 ||
            strcmp(itemLabel, MENU_LABEL_AUDIO) == 0 ||
            strcmp(itemLabel, MENU_LABEL_HAPTIC) == 0 ||
            strcmp(itemLabel, MENU_LABEL_CLOCK_MODE) == 0);
}

/**
 * @brief Get the appropriate action text for a toggle item
 */
static const char* getToggleActionText(const char* itemLabel, bool enabled) {
    if (strcmp(itemLabel, MENU_LABEL_WIFI) == 0) {
        return enabled ? MENU_LABEL_DISABLE_WIFI : MENU_LABEL_ENABLE_WIFI;
    } else if (strcmp(itemLabel, MENU_LABEL_AUDIO) == 0) {
        return enabled ? MENU_LABEL_DISABLE_AUDIO : MENU_LABEL_ENABLE_AUDIO;
    } else if (strcmp(itemLabel, MENU_LABEL_HAPTIC) == 0) {
        return enabled ? MENU_LABEL_DISABLE_HAPTIC : MENU_LABEL_ENABLE_HAPTIC;
    } else if (strcmp(itemLabel, MENU_LABEL_CLOCK_MODE) == 0) {
        return enabled ? MENU_LABEL_DISABLE_CLOCK : MENU_LABEL_ENABLE_CLOCK;
    }
    return itemLabel; // Fallback to original label
}

/**
 * @brief Determine if a timezone is currently selected
 */
static bool isTimezoneActive(const char* timezoneLabel) {
    // Get current timezone from preferences
    const char* currentTimezone = getCurrentTimezone();
    
    // Map timezone labels to POSIX timezone strings
    bool isActive = false;
    
    if (strcmp(timezoneLabel, MENU_LABEL_TZ_EASTERN) == 0) {
        isActive = (strcmp(currentTimezone, "EST5EDT,M3.2.0,M11.1.0") == 0);
    } else if (strcmp(timezoneLabel, MENU_LABEL_TZ_CENTRAL) == 0) {
        isActive = (strcmp(currentTimezone, "CST6CDT,M3.2.0,M11.1.0") == 0);
    } else if (strcmp(timezoneLabel, MENU_LABEL_TZ_MOUNTAIN) == 0) {
        isActive = (strcmp(currentTimezone, "MST7MDT,M3.2.0,M11.1.0") == 0);
    } else if (strcmp(timezoneLabel, MENU_LABEL_TZ_PACIFIC) == 0) {
        isActive = (strcmp(currentTimezone, "PST8PDT,M3.2.0,M11.1.0") == 0);
    } else if (strcmp(timezoneLabel, MENU_LABEL_TZ_UTC) == 0) {
        isActive = (strcmp(currentTimezone, "UTC0") == 0);
    } else if (strcmp(timezoneLabel, MENU_LABEL_TZ_UK) == 0) {
        isActive = (strcmp(currentTimezone, "GMT0BST,M3.5.0,M10.5.0") == 0);
    } else if (strcmp(timezoneLabel, MENU_LABEL_TZ_EUROPE) == 0) {
        isActive = (strcmp(currentTimezone, "CET-1CEST,M3.5.0,M10.5.0") == 0);
    } else if (strcmp(timezoneLabel, MENU_LABEL_TZ_JAPAN) == 0) {
        isActive = (strcmp(currentTimezone, "JST-9") == 0);
    } else if (strcmp(timezoneLabel, MENU_LABEL_TZ_CHINA) == 0) {
        isActive = (strcmp(currentTimezone, "CST-8") == 0);
    } else if (strcmp(timezoneLabel, MENU_LABEL_TZ_AUSTRALIA) == 0) {
        isActive = (strcmp(currentTimezone, "AEST-10AEDT,M10.1.0,M4.1.0") == 0);
    }
    
    return isActive;
}

/**
 * @brief Determine if a theme is currently active
 */
static bool isThemeActive(const char* themeLabel) {
    // Check if tint effect is enabled
    if (!effectsCore_isEffectEnabled(EFFECT_TINT)) {
        return false;
    }
    
    // Get current tint parameters
    tint_params_t currentTint;
    if (!effectsCore_getEffectParams(EFFECT_TINT, &currentTint)) {
        return false;
    }
    
    // Map theme labels to tint colors
    if (strcmp(themeLabel, MENU_LABEL_THEME_NONE) == 0) {
        return false; // No theme is active when tint is disabled
    } else if (strcmp(themeLabel, MENU_LABEL_THEME_GREEN) == 0) {
        return (currentTint.tintColor == TINT_GREEN_400);
    } else if (strcmp(themeLabel, MENU_LABEL_THEME_YELLOW) == 0) {
        return (currentTint.tintColor == TINT_YELLOW_400);
    } else if (strcmp(themeLabel, MENU_LABEL_THEME_BLUE) == 0) {
        return (currentTint.tintColor == TINT_BLUE_400);
    } else if (strcmp(themeLabel, MENU_LABEL_THEME_GAMEBOY) == 0) {
        return (currentTint.tintColor == GAMEBOY_400);
    } else if (strcmp(themeLabel, MENU_LABEL_THEME_MONOCHROME) == 0) {
        return (currentTint.tintColor == MONOCHROME_700);
    }
    
    return false;
}

/**
 * @brief Get menu item display text
 */
static String getItemDisplayText(const MenuItem* item) {
    String text;
    
    if (item->type == MENU_TOGGLE && item->statusFlag) {
        // Check if this toggle should show action text or dots
        if (shouldShowActionText(item->label)) {
            // Show action text (ENABLE/DISABLE)
            text = String(getToggleActionText(item->label, *item->statusFlag));
        } else {
            // Show original label with space for dot if enabled
            text = String(item->label);
            if (*item->statusFlag) {
                text += " "; // Add space for the circle that will be drawn separately
            }
        }
    } else if (item->type == MENU_INFO && item->info) {
        text = String(item->label);
        text += ": ";
        text += item->info();
    } else if (item->type == MENU_ACTION) {
        // Check if this is a theme action or timezone action and if it's currently active
        text = String(item->label);
        if (isThemeActive(item->label) || isTimezoneActive(item->label)) {
            text += " "; // Add space for the circle that will be drawn separately
        }
    } else {
        text = String(item->label);
    }
    
    return text;
}

/**
 * @brief Calculate item Y position
 */
static int getItemYPosition(int index) {
    int itemHeight = 8 + MENU_ITEM_Y_OFFSET; // Text height + padding
    return 28 + (index * (itemHeight + MENU_ITEM_Y_OFFSET));
}

/**
 * @brief Calculate scroll offset for current selection
 */
static int calculateScrollOffset(int selectedIndex, int itemCount) {
    int maxVisibleItems = calculateMaxVisibleItems();
             
    if (itemCount <= maxVisibleItems) {
        return 0; // No scrolling needed
    }
    
    int newOffset = scrollOffset;
    
    // Keep selected item visible on screen
    if (selectedIndex < scrollOffset) {
        // Selected item is above visible area - scroll up
        newOffset = selectedIndex;
    } else if (selectedIndex >= scrollOffset + maxVisibleItems) {
        // Selected item is below visible area - scroll down
        newOffset = selectedIndex - maxVisibleItems + 1;
    }
    return newOffset;
}

/**
 * @brief Reset display state tracking to force full redraw
 */
static void resetDisplayState() {
    lastSelectedIndex = -1;
    lastItemCount = -1;
    lastTitle = "";
    forceFullRedraw = true;
    // Note: Don't reset scrollOffset here - it should persist during redraws
}

//==============================================================================
// PUBLIC API IMPLEMENTATION
//==============================================================================

/**
 * @brief Initialize the menu display system
 * 
 * Initializes the menu display system by resetting internal state tracking
 * variables. The actual display hardware initialization is handled by the
 * display_module, so this function focuses on preparing the menu display
 * state for proper operation.
 */
void menuDisplay_init() {
     // Display initialization is handled by display_module
     resetDisplayState();
 }

/**
 * @brief Clear the display
 * 
 * Clears the display and resets all internal state tracking variables.
 * This function ensures a clean slate for menu operations and prevents
 * display artifacts from previous operations.
 */
void menuDisplay_clear() {
     clearDisplay();
     resetDisplayState();
     scrollOffset = 0;  // Reset scroll when explicitly clearing for menu changes
     
     // Reset display font to default to prevent font persistence from other modules
     oled.setFont();
     oled.setTextSize(MENU_TEXT_SIZE);
 }

/**
 * @brief Draw a menu header
 * @param title Header text to display
 * 
 * The header is drawn at a fixed Y position (8 pixels from top)
 * and uses the current color scheme from the effects system.
 */
void menuDisplay_drawHeader(const char* title) {
     uint16_t primaryColor, accentColor;
     getCurrentColors(&primaryColor, &accentColor);
     
     // Set text properties - ensure default font is used
     oled.setFont();
     oled.setTextSize(MENU_TEXT_SIZE);
     oled.setTextColor(primaryColor);
     
     // Calculate centered position
     int16_t x1, y1;
     uint16_t textWidth, textHeight;
     oled.getTextBounds(title, 0, 0, &x1, &y1, &textWidth, &textHeight);
     
     int centerX = (DISPLAY_WIDTH - textWidth) / 2;
     int headerY = 8;
     
     // Draw header
     oled.setCursor(centerX, headerY);
     oled.println(title);
     
     // Draw underline
     int lineY = headerY + textHeight + 2;
     oled.drawLine(MENU_PADDING, lineY, DISPLAY_WIDTH - MENU_PADDING, lineY, primaryColor);
 }

/**
 * @brief Draw a single menu item
 * @param index Item index for positioning
 * @param item Menu item to draw
 * @param selected Whether this item is selected
 * 
 * The function handles text truncation, selection highlighting,
 * and various item type indicators automatically.
 */
void menuDisplay_drawItem(int index, const MenuItem* item, bool selected) {
     uint16_t primaryColor, accentColor;
     getCurrentColors(&primaryColor, &accentColor);
     
     // Get display text
     String displayText = getItemDisplayText(item);
     
     // Calculate position
     int y = getItemYPosition(index);
     int x = MENU_PADDING + MENU_ITEM_X_OFFSET;
     
     // Calculate available width for text (accounting for padding and potential arrow)
     int availableWidth = DISPLAY_WIDTH - (MENU_PADDING * 2) - MENU_ITEM_X_OFFSET;
     if (item->type == MENU_SUBMENU) {
         availableWidth -= 20; // Reserve space for arrow
     }
     
     // Truncate text if needed
     String truncatedText = truncateText(displayText, availableWidth);
     const char* text = truncatedText.c_str();
     
     // Set text size FIRST before getting bounds - ensure default font is used
     oled.setFont();
     oled.setTextSize(MENU_TEXT_SIZE);
     
     // Get text bounds with correct text size
     int16_t x1, y1;
     uint16_t textWidth, textHeight;
     oled.getTextBounds(text, 0, 0, &x1, &y1, &textWidth, &textHeight);
     
     int itemHeight = textHeight + 4;
     int fullMenuWidth = DISPLAY_WIDTH - (MENU_PADDING * 2);

     int circleX = x + textWidth + 2; 
     int circleY = y + (textHeight / 2) - 1; 
     uint16_t circleColor = selected ? COLOR_BLACK : primaryColor;
     
     // Clear item area
     oled.fillRect(MENU_PADDING, y - MENU_ITEM_Y_OFFSET, fullMenuWidth, itemHeight, COLOR_BLACK);
     
     // Calculate if this item needs extra space for circle
     bool needsCircleSpace = (item->type == MENU_ACTION && (isThemeActive(item->label) || isTimezoneActive(item->label))) ||
                            (item->type == MENU_TOGGLE && item->statusFlag && *item->statusFlag && !shouldShowActionText(item->label));
     int extraSpace = needsCircleSpace ? 8 : 0; // 6px margin + 6px circle diameter
     
     // Draw selection highlight
     if (selected) {
         int highlightWidth = item->type == MENU_SUBMENU ? textWidth + 15 : textWidth + 6 + extraSpace;
         oled.fillRect(x - MENU_ITEM_X_OFFSET, y - MENU_ITEM_Y_OFFSET, 
                      highlightWidth, itemHeight, primaryColor);
         oled.setTextColor(COLOR_BLACK);
     } else {
         oled.setTextColor(primaryColor);
     }
     
     // Draw text (text size already set above)
     oled.setCursor(x, y);
     oled.println(text);
     
     // Add icon for menu item types
     if (item->type == MENU_SUBMENU) {
         // Draw arrow for submenus
         int arrowX = x + textWidth + 4;
         int arrowY = y + (textHeight / 2);
         uint16_t arrowColor = selected ? COLOR_BLACK : primaryColor;
         
         // Simple right arrow: > (ensure consistent text size)
         oled.setTextColor(arrowColor);
         oled.setTextSize(MENU_TEXT_SIZE);
         oled.setCursor(arrowX, y);
         oled.print(">");
     } else if (item->type == MENU_ACTION) {
         // Check if this is a theme action or timezone action and if it's currently active
         if (isThemeActive(item->label) || isTimezoneActive(item->label)) {
             // Draw filled circle (3px radius = 6px diameter)
             oled.fillCircle(circleX, circleY, 2, circleColor);
         }
     } else if (item->type == MENU_TOGGLE && item->statusFlag) {
         // Check if this toggle item is enabled and should show dots (not action text)
         if (*item->statusFlag && !shouldShowActionText(item->label)) {
             // Draw filled circle (3px radius = 6px diameter)
             oled.fillCircle(circleX, circleY, 2, circleColor);
         }
     }
 }

/**
 * @brief Draw the current menu context
 * @param context Menu context to draw
 * 
 * The function automatically handles scrolling, text truncation,
 * and performance optimization through selective redraws.
 */
void menuDisplay_draw(const MenuContext* context) {
     if (!context || !context->items) {
         return;
     }
     
     // Ensure default font is set to prevent font persistence from other modules
     oled.setFont();
     oled.setTextSize(MENU_TEXT_SIZE);
     
     // Calculate scroll offset for current selection
     int newScrollOffset = calculateScrollOffset(context->selectedIndex, context->itemCount);
     bool scrollChanged = (newScrollOffset != scrollOffset);
     
     // Check if we need a full redraw (context changed)
     bool titleChanged = (lastTitle != String(context->title ? context->title : ""));
     bool itemCountChanged = (lastItemCount != context->itemCount);
     bool selectionChanged = (lastSelectedIndex != context->selectedIndex);
     
     // Force full redraw for context changes, scroll changes, or first time
     if (forceFullRedraw || titleChanged || itemCountChanged || scrollChanged) {
         // Update scroll offset
         scrollOffset = newScrollOffset;
         
         // Clear display for full redraw
         clearDisplay();
         
         // Draw header
         if (context->title) {
             menuDisplay_drawHeader(context->title);
         }
         
         // Draw visible menu items (with scrolling)
         int maxVisibleItems = calculateMaxVisibleItems();
         int endIndex = min(scrollOffset + maxVisibleItems, context->itemCount);
         for (int i = scrollOffset; i < endIndex; i++) {
             bool selected = (i == context->selectedIndex);
             int displayIndex = i - scrollOffset; // Adjust for scrolling
             menuDisplay_drawItem(displayIndex, &context->items[i], selected);
         }
         
         // Draw scroll indicators if needed
         if (context->itemCount > maxVisibleItems) {
             menuDisplay_drawScrollIndicators(scrollOffset, context->itemCount, maxVisibleItems);
         }
         
         // Update tracking state
         lastTitle = String(context->title ? context->title : "");
         lastItemCount = context->itemCount;
         lastSelectedIndex = context->selectedIndex;
         forceFullRedraw = false;
         
     } else if (selectionChanged) {
         // Only selection changed - redraw affected items (much faster!)
         
         // Calculate which items are visible
         int maxVisibleItems = calculateMaxVisibleItems();
         int oldVisibleIndex = lastSelectedIndex - scrollOffset;
         int newVisibleIndex = context->selectedIndex - scrollOffset;
         
         // Clear old selection highlight (if still visible)
         if (oldVisibleIndex >= 0 && oldVisibleIndex < maxVisibleItems && 
             lastSelectedIndex >= scrollOffset && lastSelectedIndex < scrollOffset + maxVisibleItems) {
             menuDisplay_drawItem(oldVisibleIndex, &context->items[lastSelectedIndex], false);
         }
         
         // Draw new selection highlight (if visible)
         if (newVisibleIndex >= 0 && newVisibleIndex < maxVisibleItems &&
             context->selectedIndex >= scrollOffset && context->selectedIndex < scrollOffset + maxVisibleItems) {
             menuDisplay_drawItem(newVisibleIndex, &context->items[context->selectedIndex], true);
         }
         
         // Update selection tracking
         lastSelectedIndex = context->selectedIndex;
     }
 }

/**
 * @brief Draw scroll indicators when list is longer than screen
 */
static void menuDisplay_drawScrollIndicators(int scrollOffset, int totalItems, int maxVisibleItems) {
    uint16_t primaryColor, accentColor;
    getCurrentColors(&primaryColor, &accentColor);
    
    int indicatorX = DISPLAY_WIDTH - 6;  // Right edge
    
    // Store current text size to restore later
    uint8_t originalTextSize = MENU_TEXT_SIZE;
    
    // Draw up arrow if we can scroll up
    if (scrollOffset > 0) {
        // Simple up arrow at top right
        oled.setFont(); // Ensure default font
        oled.setTextColor(primaryColor);
        oled.setTextSize(1);
        oled.setCursor(indicatorX, FIRST_ITEM_Y);
        oled.print("^");
    }
    
    // Draw down arrow if we can scroll down
    if (scrollOffset + maxVisibleItems < totalItems) {
        // Simple down arrow at bottom right  
        // Calculate item spacing dynamically
        int itemSpacing = (8 + MENU_ITEM_Y_OFFSET) + MENU_ITEM_Y_OFFSET;
        int bottomY = FIRST_ITEM_Y + (maxVisibleItems - 1) * itemSpacing + 8;
        oled.setFont(); // Ensure default font
        oled.setTextColor(primaryColor);
        oled.setTextSize(1);
        oled.setCursor(indicatorX, bottomY);
        oled.print("v");
    }
    
    // Restore original text size
    oled.setTextSize(originalTextSize);
}

/**
 * @brief Force a full redraw on next draw call
 * 
 * Forces a complete redraw of the menu on the next call to menuDisplay_draw().
 * This is useful when toggle states or item content has changed and the
 * display needs to be refreshed to reflect the new state.
 */
void menuDisplay_forceRedraw() {
     forceFullRedraw = true;
 }




