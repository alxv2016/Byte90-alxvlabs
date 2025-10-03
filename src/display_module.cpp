/**
 * @file display_module.cpp
 * @brief Implementation of OLED display functionality
 *
 * Provides functions for initializing and controlling the SSD1351 OLED display,
 * including rendering text, images, and controlling display parameters.
 */

#include "display_module.h"
#include "common.h"
#include "emotes_module.h"
#include "soundsfx_module.h"
#include "speaker_module.h"
#include "effects_core.h"
#include "effects_tints.h"
#include "preferences_module.h"
#include "clock_sync.h"
#include "clock_module.h"
#include "wifi_common.h"
#include "wifi_module.h"
#include "states_module.h"
#include "gif_module.h"

//==============================================================================
// GLOBAL VARIABLES
//==============================================================================

Adafruit_SSD1351 oled = Adafruit_SSD1351(DISPLAY_WIDTH, DISPLAY_HEIGHT, &SPI,
                                         CS_PIN_D7, DC_PIN_D6, RST_PIN_D0);

static int16_t dos_x = 0;
static int16_t dos_y = 0;

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
void drawBitmap(int16_t x, int16_t y, const uint16_t *bitmap, int16_t w, int16_t h) {
  oled.drawRGBBitmap(x, y, bitmap, w, h);
}

/**
 * @brief Set the active display window for drawing operations
 * @param x X-coordinate for the top-left corner
 * @param y Y-coordinate for the top-left corner
 * @param w Width of the window in pixels
 * @param h Height of the window in pixels
 */
void setAddrWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
  oled.setAddrWindow(x, y, w, h);
}

/**
 * @brief Begin a batch write operation to the display
 */
void startWrite() { 
  oled.startWrite(); 
}

/**
 * @brief End a batch write operation to the display
 */
void endWrite() { 
  oled.endWrite(); 
}

/**
 * @brief Write a batch of pixels to the display
 * @param pixels Pointer to pixel data in RGB565 format
 * @param len Number of pixels to write
 */
void writePixels(uint16_t *pixels, uint32_t len) {
  oled.writePixels(pixels, len);
}

/**
 * @brief Initialize the OLED display
 * @return true if initialization was successful, false otherwise
 */
bool initializeOLED() {
  SPI.begin(SCLK_PIN_D8, -1, MOSI_PIN_D10, CS_PIN_D7);
  SPI.setFrequency(DISPLAY_FREQUENCY);
  oled.begin();
  oled.fillScreen(COLOR_BLACK);
  return true;
}

/**
 * @brief Complete display setup after fast initialization
 */
void completeDisplaySetup() {
  oled.setTextColor(COLOR_WHITE);
  setDisplayBrightness(DISPLAY_BRIGHTNESS_FULL);
}

/**
 * @brief Set the display brightness level
 * @param contrastLevel Brightness level (0-15, with 15 being brightest)
 */
void setDisplayBrightness(uint8_t contrastLevel) {
  if (contrastLevel > DISPLAY_BRIGHTNESS_FULL)
    contrastLevel = DISPLAY_BRIGHTNESS_FULL;
  uint8_t data = contrastLevel;
  oled.sendCommand(SSD1351_CMD_CONTRASTMASTER, &data, 1);
}


/**
 * @brief Clear the display by filling it with black
 */
void clearDisplay() { 
  oled.fillScreen(COLOR_BLACK); 
}

/**
 * @brief Display a static image centered on the screen
 * @param imageData Pointer to the image data in RGB565 format
 * @param imageWidth Width of the image in pixels
 * @param imageHeight Height of the image in pixels
 * @param applyTint Whether to apply current tint effects to the image
 */
void displayStaticImage(const uint16_t *imageData, uint16_t imageWidth, uint16_t imageHeight, bool applyTint) {
  int16_t x = (DISPLAY_WIDTH - imageWidth) / 2;
  int16_t y = (DISPLAY_HEIGHT - imageHeight) / 2;
  if (x < 0) x = 0;
  if (y < 0) y = 0;

  if (applyTint && effectsCore_isEffectEnabled(EFFECT_TINT)) {
    // Get current tint parameters using the new effects system
    tint_params_t tintParams = effectsTints_getDefaultTintParams();
    effectsCore_getEffectParams(EFFECT_TINT, &tintParams);
    
    // Create a buffer for one scanline
    uint16_t lineBuffer[128];
    
    // Process the image line by line with tint effects
    for (int row = 0; row < imageHeight; row++) {
      // Copy original scanline to buffer
      for (int col = 0; col < imageWidth; col++) {
        lineBuffer[col] = imageData[row * imageWidth + col];
      }
      
      // Apply tint effect to this scanline using new system
      effectsTints_applyTintToScanline(lineBuffer, imageWidth, row, &tintParams);
      
      // Draw the processed scanline
      oled.drawRGBBitmap(x, y + row, lineBuffer, imageWidth, 1);
    }
  } else {
    // No tint effect - display normally
    oled.drawRGBBitmap(x, y, imageData, imageWidth, imageHeight);
  }
}

/**
 * @brief Type text with synchronized keystroke sounds
 * @param text String to type
 * @param delay_ms Base delay between characters
 * @param color Text color
 * @param audioAvailable Whether audio system is available
 * @param isTerminal Whether the text is being typed in a terminal context
 */
void dosTypeSynced(const char *text, int delay_ms, uint16_t color, bool audioAvailable, bool isTerminal) {
  oled.setTextColor(color);
  oled.setCursor(dos_x, dos_y);

  bool useSound = audioAvailable && strlen(text) > 3;

  while (*text) {
    if (isTerminal && useSound) {
      playBeep(1200, 30, 15);
    } else {
      if (audioAvailable) {
        sfxPlayKeystroke();
      }
    }

    oled.print(*text);
    text++;
    dos_x += 6;

    if (dos_x >= DISPLAY_WIDTH - 6) {
      dos_x = 0;
      dos_y += 10;
    }

    delay(delay_ms);
  }
}

/**
 * @brief Move cursor to the beginning of next line
 */
void dosNewLine() {
  dos_x = 0;
  dos_y += 10;
}

/**
 * @brief Display blinking cursor block at current position
 * @param blinks Number of blinks to show
 */
void dosBlinkCursor(int blinks) {
  // Get current theme colors for cursor
  uint16_t primaryColor, accentColor;
  getDOSColorsForCurrentTint(&primaryColor, &accentColor);
  
  for (int i = 0; i < blinks; i++) {
    oled.fillRect(dos_x, dos_y, 6, 8, primaryColor);
    delay(CURSOR_BLINK_MS);

    oled.fillRect(dos_x, dos_y, 6, 8, TINT_BLACK);
    delay(CURSOR_BLINK_MS);
  }
}

/**
 * @brief Get DOS colors based on current effect tint
 * @param primaryColor Pointer to store primary text color
 * @param accentColor Pointer to store accent text color
 */
void getDOSColorsForCurrentTint(uint16_t *primaryColor, uint16_t *accentColor) {
    // Check if tint effect is enabled and get its parameters
    if (effectsCore_isEffectEnabled(EFFECT_TINT)) {
        tint_params_t tintParams;
        if (effectsCore_getEffectParams(EFFECT_TINT, &tintParams)) {
            // Use the tint color as primary, with a slightly darker version as accent
            *primaryColor = tintParams.tintColor;
            // Create a darker accent color by blending with black
            *accentColor = effectsTints_blendColors(tintParams.tintColor, TINT_BLACK, 0.3f);
            
        } else {
            // Fallback to default colors if getting params failed
            *primaryColor = TINT_YELLOW_400;    
            *accentColor = TINT_YELLOW_500;
            ESP_LOGW("DISPLAY", "Failed to get tint params, using default DOS colors");
        }
    } else {
        // Default DOS colors (original behavior)
        *primaryColor = TINT_YELLOW_400;    
        *accentColor = TINT_YELLOW_500;
    }
}

/**
 * @brief Fast DOS animation maintaining retro authenticity
 */
void displayDOSStartupAnimationFast() {
  oled.fillScreen(TINT_BLACK);
  oled.setFont();
  oled.setTextSize(1);
  dos_x = 0;
  dos_y = 8;
  
  audio_state_t audioState = getAudioState();
  bool audioAvailable = (audioState == AUDIO_STATE_READY || audioState == AUDIO_STATE_PLAYING);

  // Get dynamic colors based on user's tint preference
  uint16_t primaryColor, accentColor;
  getDOSColorsForCurrentTint(&primaryColor, &accentColor);

  String chipModel = getChipModel();
  uint32_t cpuFreq = getCpuFrequencyMHz();
  uint32_t flashSize = getFlashSizeMB();
  uint32_t psramSize = getPSRAMSizeMB();
  String displayModel = getDisplayInfo();

  dosTypeSynced("ALXV LABS BIOS v1.1", TYPE_DELAY_ULTRA_FAST, accentColor, audioAvailable);
  dosNewLine();
  delay(LINE_DELAY_FAST);

  dosTypeSynced("Detecting Hardware...", TYPE_DELAY_ULTRA_FAST, primaryColor, audioAvailable, true);
  dosNewLine();
  delay(LINE_DELAY_FAST);

  String mcuLine = "MCU:" + chipModel + "R" + String(flashSize) + " [OK]";
  dosTypeSynced(mcuLine.c_str(), TYPE_DELAY_ULTRA_FAST, primaryColor, audioAvailable);
  dosNewLine();

  String memoryLine;
  if (psramSize > 0) {
    memoryLine = "PSRAM:" + String(psramSize) + "MB [OK]";
    dosTypeSynced(memoryLine.c_str(), TYPE_DELAY_ULTRA_FAST, primaryColor, audioAvailable);
  } else {
    memoryLine = "PSRAM: None [--]";
    dosTypeSynced(memoryLine.c_str(), TYPE_DELAY_ULTRA_FAST, primaryColor, audioAvailable);
  }
  dosNewLine();
  delay(LINE_DELAY_FAST);

  if (audioAvailable) {
    sfxPlayConfirm();
  }

  dosTypeSynced("///////////////////", TYPE_DELAY_ULTRA_FAST, primaryColor, audioAvailable, true);
  dosNewLine();
  delay(PAUSE_ULTRA_SHORT);

  String osVersion = "BYTE-90 OS v" + String(FIRMWARE_VERSION);
  dosTypeSynced(osVersion.c_str(), TYPE_DELAY_ULTRA_FAST, accentColor, audioAvailable, true);
  dosNewLine();
  delay(LINE_DELAY_FAST);

  dosTypeSynced("C:\\> run BYTE90.exe", TYPE_DELAY_ULTRA_FAST, primaryColor, audioAvailable, true);
  dosNewLine();
  delay(PAUSE_ULTRA_SHORT);

  oled.fillScreen(TINT_BLACK);
}


/**
 * @brief Choose the appropriate DOS animation based on boot mode
 */
void displayDOSStartupAnimation() {
  switch(BOOT_ANIMATION_MODE) {
    case BOOT_MODE_FAST:
      displayDOSStartupAnimationFast();
      break;
    case BOOT_MODE_FULL:
    default:
      displayDOSStartupAnimationFull();
      break;
  }
}

/**
 * @brief Original full DOS animation with all effects
 */
void displayDOSStartupAnimationFull() {
  oled.fillScreen(TINT_BLACK);
  oled.setFont();
  oled.setTextSize(1);
  dos_x = 0;
  dos_y = 8;
  
  audio_state_t audioState = getAudioState();
  bool audioAvailable = (audioState == AUDIO_STATE_READY || audioState == AUDIO_STATE_PLAYING);

  // Get dynamic colors based on user's tint preference
  uint16_t primaryColor, accentColor;
  getDOSColorsForCurrentTint(&primaryColor, &accentColor);

  String chipModel = getChipModel();
  uint32_t cpuFreq = getCpuFrequencyMHz();
  uint32_t flashSize = getFlashSizeMB();
  uint32_t psramSize = getPSRAMSizeMB();
  String displayModel = getDisplayInfo();

  if (audioAvailable) {
    sfxPlayPOST();
    delay(PAUSE_SHORT);
  }
  dosTypeSynced("ALXV LABS BIOS v1.1", TYPE_DELAY_FAST, accentColor, audioAvailable);
  dosNewLine();
  delay(LINE_DELAY);

  dosBlinkCursor(1);

  dosTypeSynced("Detecting Hardware...", TYPE_DELAY_FAST, primaryColor, audioAvailable, true);
  dosNewLine();
  delay(LINE_DELAY);

  dosTypeSynced("Audio: [OK]", TYPE_DELAY_FAST, primaryColor, audioAvailable);
  if (audioAvailable) {
    delay(LINE_DELAY);
    sfxPlayConfirm();
  }
  dosNewLine();
  
  String displayLine = "Display:" + displayModel + " [OK]";
  dosTypeSynced(displayLine.c_str(), TYPE_DELAY_FAST, primaryColor, audioAvailable);
  if (audioAvailable) {
    delay(LINE_DELAY);
    sfxPlayConfirm();
  }
  dosNewLine();

  esp_chip_info_t chip_info;
  esp_chip_info(&chip_info);
  String mcuLine = "MCU:" + chipModel + "R" + String(flashSize) + " [OK]";
  dosTypeSynced(mcuLine.c_str(), TYPE_DELAY_FAST, primaryColor, audioAvailable);
  if (audioAvailable) {
    delay(LINE_DELAY);
    sfxPlayConfirm();
  }
  dosNewLine();

  String cpuLine = "CPU:" + String(cpuFreq) + "MHz [OK]";
  dosTypeSynced(cpuLine.c_str(), TYPE_DELAY_FAST, primaryColor, audioAvailable);
  if (audioAvailable) {
    delay(LINE_DELAY);
    sfxPlayConfirm();
  }
  dosNewLine();

  String memoryLine;
  if (psramSize > 0) {
    memoryLine = "PSRAM:" + String(psramSize) + "MB [OK]";
    dosTypeSynced(memoryLine.c_str(), TYPE_DELAY_FAST, primaryColor, audioAvailable);
    if (audioAvailable) {
      delay(LINE_DELAY);
      sfxPlayConfirm();
    }
  } else {
    memoryLine = "PSRAM: None [--]";
    dosTypeSynced(memoryLine.c_str(), TYPE_DELAY_FAST, primaryColor, audioAvailable, true);
  }
  dosNewLine();

  dosTypeSynced("///////////////////", TYPE_DELAY_FAST, primaryColor, audioAvailable, true);
  dosNewLine();
  delay(PAUSE_SHORT);

  String osVersion = "BYTE-90 OS v" + String(FIRMWARE_VERSION);
  dosTypeSynced(osVersion.c_str(), TYPE_DELAY_FAST, accentColor, audioAvailable, true);
  dosNewLine();
  delay(LINE_DELAY);

  dosTypeSynced("C:\\> ", TYPE_DELAY_FAST, accentColor, audioAvailable, true);
  delay(LINE_DELAY);

  dosTypeSynced("run BYTE90.exe", TYPE_DELAY_FAST, primaryColor, audioAvailable, true);
  dosNewLine();
  delay(PAUSE_SHORT);
  dosBlinkCursor(2);

  oled.fillScreen(TINT_BLACK);
}

void drawUpdateModeScreen() {
  ESP_LOGI("DISPLAY", "Drawing update mode screen...");
  
  // Clear the screen first to ensure clean display
  oled.fillScreen(COLOR_BLACK);
  
  // Setup display
  oled.setFont();
  oled.setTextSize(1);
  oled.setTextWrap(false);
  
  // Get theme colors
  uint16_t primaryColor, accentColor;
  getDOSColorsForCurrentTint(&primaryColor, &accentColor);
  
  // Draw BYTE-90 logo
  oled.drawBitmap(0, 24, BYTE90_LOGO, 128, 19, primaryColor);
  
  // Helper function to draw text with background
  auto drawTextWithBackground = [&](const char* text, int y, bool useBackground = true) {
    int16_t x1, y1;
    uint16_t textWidth, textHeight;
    oled.getTextBounds(text, 0, 0, &x1, &y1, &textWidth, &textHeight);
    
    if (useBackground) {
      int itemHeight = textHeight + 4;
      int itemWidth = textWidth + 8;
      oled.fillRect(0, y - 2, itemWidth, itemHeight, primaryColor);
      oled.setTextColor(COLOR_BLACK);
    } else {
      oled.setTextColor(primaryColor);
    }
    
    oled.setCursor(4, y);
    oled.println(text);
  };
  
  // Draw main elements
  drawTextWithBackground("SOFTWARE CONFIG", 50);
  drawTextWithBackground("192.168.4.1", 70, false);
  drawTextWithBackground("PWD: 00000000", 80, false);
  // Add firmware version
  String versionText = "Firmware: V" + String(FIRMWARE_VERSION);
  drawTextWithBackground(versionText.c_str(), 90, false);
  drawTextWithBackground("BYTE90_Setup", DISPLAY_HEIGHT - 20);
}

//==============================================================================
// WIFI STATUS INDICATOR FUNCTIONS
//==============================================================================

/**
 * @brief Draw WiFi status indicator dot in top-right corner
 * @param showGreen Show green dot for connected WiFi
 * @param showYellow Show yellow dot for disconnected WiFi
 */
void drawWiFiStatusIndicator(bool showGreen, bool showYellow) {
    // Position: top-right corner, 2px from edges
    int x = DISPLAY_WIDTH - 4;  // 2px from right edge
    int y = 4;                  // 2px from top edge
    
    if (showGreen) {
        // Draw green dot for connected WiFi
        oled.fillCircle(x, y, 2, hexToRGB565("#00FF00")); // #00FF00 -> RGB565
    } else if (showYellow) {
        // Draw yellow dot for disconnected WiFi
        oled.fillCircle(x, y, 2, hexToRGB565("#FFFF00")); // #FFFF00 -> RGB565
    }
    // If both false, no dot is drawn
}

/**
 * @brief Update and draw WiFi status indicator based on current state
 */
void updateWiFiStatusIndicator() {
    // Check if we're in ESP MODE (don't show WiFi indicator)
    if (getCurrentState() == SystemState::ESP_MODE) {
        return; // Don't show any WiFi indicator in ESP MODE
    }
    
    // Check if WiFi is enabled
    if (getCurrentState() != SystemState::WIFI_MODE) {
        return; // Don't show any WiFi indicator if WiFi is disabled
    }
    
    // Use WiFi module API to check connection status
    bool isConnected = isWifiNetworkConnected();
    
    // Draw appropriate indicator
    if (isConnected) {
        drawWiFiStatusIndicator(true, false);  // Green dot for connected
    } else {
        drawWiFiStatusIndicator(false, true);  // Yellow dot for disconnected
    }
}

/**
 * @brief Display a loading screen with custom messages
 * @param headerMessage Header message (e.g., "Entering update mode")
 * @param mainMessage Main message to display
 * @param footerMessage Footer message (e.g., "AP Starting...[OK]")
 * @param showProgress Whether to show animated progress dots
 */
void displayLoadingScreen(const char* headerMessage, const char* mainMessage, const char* footerMessage, bool showProgress) {
  oled.fillScreen(TINT_BLACK);
  oled.setFont();
  oled.setTextSize(1);
  dos_x = 0;
  dos_y = 8;
  
  audio_state_t audioState = getAudioState();
  bool audioAvailable = (audioState == AUDIO_STATE_READY || audioState == AUDIO_STATE_PLAYING);

  // Get dynamic colors based on user's tint preference
  uint16_t primaryColor, accentColor;
  getDOSColorsForCurrentTint(&primaryColor, &accentColor);

  // BIOS header (configurable)
  dosTypeSynced(headerMessage, TYPE_DELAY_ULTRA_FAST, accentColor, audioAvailable);
  dosNewLine();
  delay(LINE_DELAY_FAST);

  // Main message (configurable)
  dosTypeSynced(mainMessage, TYPE_DELAY_ULTRA_FAST, primaryColor, audioAvailable, true);
  dosNewLine();
  delay(LINE_DELAY_FAST);

  if (showProgress) {
    // Show animated progress dots
    oled.setTextColor(primaryColor);
    oled.setCursor(dos_x, dos_y);
    dosBlinkCursor(3);
  }

  dosNewLine();
  // Footer message (configurable)
  dosTypeSynced(footerMessage, TYPE_DELAY_ULTRA_FAST, accentColor, audioAvailable, true);
  dosNewLine();
  
  if (audioAvailable) {
    sfxPlayConfirm();
  }
}

/**
 * @brief Update display for a specific system mode
 * @param newMode The system mode to display
 */
void updateDisplayForMode(SystemState newMode) {
    ESP_LOGI(DISPLAY_LOG, "Updating display for mode: %s", 
             (newMode == SystemState::UPDATE_MODE) ? "UPDATE_MODE" :
             (newMode == SystemState::CLOCK_MODE) ? "CLOCK_MODE" :
             (newMode == SystemState::ESP_MODE) ? "ESP_MODE" :
             (newMode == SystemState::WIFI_MODE) ? "WIFI_MODE" :
             "IDLE_MODE");
    
    if (newMode == SystemState::IDLE_MODE) {
      // Don't stop GIF playback for IDLE mode
        return;
    }
    // Stop any ongoing GIF playback
    stopGifPlayback();
    
    if (newMode == SystemState::UPDATE_MODE) {
        // Draw update mode screen
        displayLoadingScreen("Entering update mode", "Setting up AP...", "AP Starting...[OK]", true);
        delay(300);
        drawUpdateModeScreen();
    } else if (newMode == SystemState::CLOCK_MODE) {
        // Clock mode display is handled by clock module
        displayLoadingScreen("Entering clock mode", "Syncing time...", "Time synced...[OK]", true);
        delay(300);
        resetClockDisplayState();
    } else if (newMode == SystemState::ESP_MODE) {
        displayLoadingScreen("Enabling signals", "Broadcasting ID...", "Discovering...[OK]", true);
        delay(300);
    }
    // Other modes (ESP_MODE, WIFI_MODE, IDLE_MODE) don't need special display handling
}