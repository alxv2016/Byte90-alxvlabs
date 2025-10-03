/**
 * @file display_module.h
 * @brief Header for OLED display functionality
 *
 * Provides functions for initializing and controlling the SSD1351 OLED display,
 * including rendering text, images, and controlling display parameters.
 */

#ifndef DISPLAY_MODULE_H
#define DISPLAY_MODULE_H

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1351.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include "common.h"
#include "states_module.h"

//==============================================================================
// CONSTANTS & DEFINITIONS
//==============================================================================

extern Adafruit_SSD1351 oled;
static const char *DISPLAY_LOG = "::DISPLAY_MODULE::";

#define MOSI_PIN_D10 D10
#define SCLK_PIN_D8 D8
#define CS_PIN_D7 D7
#define DC_PIN_D6 D6
#define RST_PIN_D0 D0

#define DISPLAY_BRIGHTNESS_DIM 0x00
#define DISPLAY_BRIGHTNESS_LOW 0x02
#define DISPLAY_BRIGHTNESS_MEDIUM 0x05
#define DISPLAY_BRIGHTNESS_HIGH 0x07
#define DISPLAY_BRIGHTNESS_FULL 0x0F
#define DISPLAY_FREQUENCY 20000000
#define DISPLAY_WIDTH 128
#define DISPLAY_HEIGHT 128
#define ERROR_ICON_SIZE 128
const uint16_t COLOR_BLACK = hexToRGB565("#000000");  // #000000 -> RGB565
const uint16_t COLOR_WHITE = hexToRGB565("#ffffff");  // #ffffff -> RGB565
const uint16_t COLOR_YELLOW = hexToRGB565("#ffff00"); // #ffff00 -> RGB565

#define BOOT_MODE_FULL 0
#define BOOT_MODE_FAST 1
#define BOOT_ANIMATION_MODE BOOT_MODE_FAST

#define TYPE_DELAY_ULTRA_FAST 15
#define TYPE_DELAY_SUPER_FAST 20
#define LINE_DELAY_FAST 100
#define PAUSE_ULTRA_SHORT 200

#define TYPE_DELAY_FAST 25
#define TYPE_DELAY_NORMAL 40
#define TYPE_DELAY_SLOW 60
#define LINE_DELAY 150
#define PAUSE_SHORT 300
#define PAUSE_LONG 500
#define CURSOR_BLINK_MS 400
#define CURSOR_BLINK_COUNT 3

//==============================================================================
// PUBLIC API FUNCTIONS
//==============================================================================

/**
 * @brief Draw a bitmap image on the display
 * @param x X-coordinate for the top-left corner
 * @param y Y-coordinate for the top-left corner
 * @param bitmap Pointer to the bitmap image data in RGB565 format
 * @param w Width of the bitmap in pixels
 * @param h Height of the bitmap in pixels
 */
void drawBitmap(int16_t x, int16_t y, const uint16_t *bitmap, int16_t w, int16_t h);

/**
 * @brief Set the active display window for drawing operations
 * @param x X-coordinate for the top-left corner
 * @param y Y-coordinate for the top-left corner
 * @param w Width of the window in pixels
 * @param h Height of the window in pixels
 */
void setAddrWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h);

/**
 * @brief Begin a batch write operation to the display
 */
void startWrite();

/**
 * @brief End a batch write operation to the display
 */
void endWrite();

/**
 * @brief Write a batch of pixels to the display
 * @param pixels Pointer to pixel data in RGB565 format
 * @param len Number of pixels to write
 */
void writePixels(uint16_t *pixels, uint32_t len);

/**
 * @brief Initialize the OLED display (optimized for fast boot)
 * @return true if initialization was successful, false otherwise
 */
bool initializeOLED();

/**
 * @brief Complete display setup after fast initialization
 */
void completeDisplaySetup();



/**
 * @brief Display a static image centered on the screen
 * @param imageData Pointer to the image data in RGB565 format
 * @param imageWidth Width of the image in pixels
 * @param imageHeight Height of the image in pixels
 * @param applyTint Whether to apply current tint effects to the image
 */
void displayStaticImage(const uint16_t *imageData, uint16_t imageWidth, uint16_t imageHeight, bool applyTint = false);

/**
 * @brief Set the display brightness level
 * @param contrastLevel Brightness level (0-15, with 15 being brightest)
 */
void setDisplayBrightness(uint8_t contrastLevel);

/**
 * @brief Clear the display by filling it with black
 */
void clearDisplay(void);

/**
 * @brief Run optimized DOS startup animation with multiple modes
 */
void displayDOSStartupAnimation();

/**
 * @brief Fast DOS animation maintaining retro authenticity
 */
void displayDOSStartupAnimationFast();

/**
 * @brief Original full DOS animation
 */
void displayDOSStartupAnimationFull();

/**
 * @brief Display a loading screen with custom messages
 * @param headerMessage Header message (e.g., "Entering update mode")
 * @param mainMessage Main message to display
 * @param footerMessage Footer message (e.g., "AP Starting...[OK]")
 * @param showProgress Whether to show animated progress dots
 */
void displayLoadingScreen(const char* headerMessage, const char* mainMessage, const char* footerMessage, bool showProgress = true);

/**
 * @brief Type text with synchronized keystroke sounds
 * @param text String to type
 * @param delay_ms Base delay between characters in milliseconds
 * @param color Text color
 * @param audioAvailable Whether audio system is available
 * @param isTerminal Whether the text is being typed in a terminal context
 */
void dosTypeSynced(const char *text, int delay_ms = TYPE_DELAY_NORMAL,
                   uint16_t color = COLOR_YELLOW, bool audioAvailable = false, bool isTerminal = false);
/**
 * @brief Move cursor to the beginning of next line
 */
void dosNewLine();

/**
 * @brief Display blinking cursor block at current position
 * @param blinks Number of blinks to show
 */
void dosBlinkCursor(int blinks = CURSOR_BLINK_COUNT);

/**
 * @brief Get DOS colors based on current effect tint
 * @param primaryColor Pointer to store primary text color
 * @param secondaryColor Pointer to store secondary text color
 * @param accentColor Pointer to store accent text color
 */
void getDOSColorsForCurrentTint(uint16_t *primaryColor, uint16_t *accentColor);

/**
 * @brief Draw update mode screen
 */
void drawUpdateModeScreen();

/**
 * @brief Update display for a specific system mode
 * @param newMode The system mode to display
 */
void updateDisplayForMode(SystemState newMode);

/**
 * @brief Draw WiFi status indicator dot in top-right corner
 * @param showGreen Show green dot for connected WiFi
 * @param showYellow Show yellow dot for disconnected WiFi
 */
void drawWiFiStatusIndicator(bool showGreen, bool showYellow);

/**
 * @brief Update and draw WiFi status indicator based on current state
 */
void updateWiFiStatusIndicator();

#endif /* DISPLAY_MODULE_H */