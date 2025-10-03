/**
 * @file common.cpp
 * @brief Implementation of common utility functions
 *
 * Contains the implementation of utility functions that are used across
 * multiple modules in the application.
 */

#include "common.h"
#include <esp_chip_info.h>
#include <esp_flash.h>
#include <esp_heap_caps.h>
#include <soc/soc.h>

//==============================================================================
// PUBLIC API FUNCTIONS
//==============================================================================

/**
 * @brief Timer utility function to check if a specified time has elapsed
 * @param setTime Reference to the timestamp variable to check and update
 * @param delayTime Time period in milliseconds to check against
 * @return true if the specified time has elapsed, false otherwise
 */
bool setTimeout(unsigned long &setTime, unsigned long delayTime) {
  unsigned long currentTime = millis();
  if (currentTime - setTime >= delayTime) {
    setTime = currentTime;
    return true;
  }
  return false;
}

/**
 * @brief Convert hours and minutes to milliseconds
 * @param hours Number of hours
 * @param minutes Number of minutes
 * @return Total time in milliseconds
 */
unsigned long timeToMillis(int hours, int minutes) {
  unsigned long totalMinutes = (hours * 60) + minutes;
  return totalMinutes * 60 * 1000;
}

/**
 * @brief Get chip model name as string
 * @return String containing the chip model
 */
String getChipModel() { 
  return "ESP32-S3"; 
}

/**
 * @brief Get CPU frequency in MHz
 * @return CPU frequency as integer
 */
uint32_t getCpuFrequencyMHz() { 
  return getCpuFrequencyMhz(); 
}

/**
 * @brief Get flash size in MB
 * @return Flash size in MB
 */
uint32_t getFlashSizeMB() {
  uint32_t flash_size;
  if (esp_flash_get_size(NULL, &flash_size) == ESP_OK) {
    return flash_size / (1024 * 1024);
  }
  return 0;
}

/**
 * @brief Get PSRAM size in MB
 * @return PSRAM size in MB, 0 if no PSRAM
 */
uint32_t getPSRAMSizeMB() {
  size_t psram_size = heap_caps_get_total_size(MALLOC_CAP_SPIRAM);
  if (psram_size > 0) {
    return psram_size / (1024 * 1024);
  }
  return 0;
}

/**
 * @brief Get display controller info
 * @return String with display info
 */
String getDisplayInfo() { 
  return "SSD1351"; 
}

/**
 * @brief Check if SERIES_2 hardware features are supported
 * @return true if SERIES_2 features are enabled, false otherwise
 */
bool checkHardwareSupport() {
#if SERIES_2
  return true;
#else
  return false;
#endif
}


//==============================================================================
// STRING UTILITY FUNCTIONS
//==============================================================================

/**
 * @brief Convert bytes to human-readable format
 * @param bytes Number of bytes to convert
 * @return String representation with appropriate unit
 */
String formatBytes(size_t bytes) {
  if (bytes >= 1024 * 1024 * 1024) {
    float gb = (float)bytes / (1024 * 1024 * 1024);
    return String(gb, 1) + "GB";
  } else if (bytes >= 1024 * 1024) {
    float mb = (float)bytes / (1024 * 1024);
    return String(mb, 1) + "MB";
  } else if (bytes >= 1024) {
    float kb = (float)bytes / 1024;
    return String(kb, 1) + "KB";
  } else {
    return String(bytes) + "B";
  }
}

//==============================================================================
// COLOR UTILITY FUNCTIONS
//==============================================================================

// Usage examples:
//   uint16_t red = hexToRGB565("#FF0000");     // Pure red
//   uint16_t green = hexToRGB565("00FF00");   // Pure green (no # prefix)
//   uint16_t blue = hexToRGB565("0x0000FF");  // Pure blue (0x prefix)
//   uint16_t white = hexToRGB565("FFFFFF");   // White
//   uint16_t black = hexToRGB565("000000");   // Black

/**
 * @brief Convert hex color string to RGB565 format
 * @param hexColor Hex color string (e.g., "#FF0000", "FF0000", "0xFF0000")
 * @return RGB565 color value, or 0x0000 if invalid
 */
uint16_t hexToRGB565(const char* hexColor) {
  if (!hexColor) {
    return 0x0000;
  }
  
  // Skip leading '#' or '0x' if present
  const char* start = hexColor;
  if (*start == '#') {
    start++;
  } else if (start[0] == '0' && (start[1] == 'x' || start[1] == 'X')) {
    start += 2;
  }
  
  // Check if we have exactly 6 hex characters
  int len = strlen(start);
  if (len != 6) {
    return 0x0000;  // Invalid length
  }
  
  // Validate that all characters are hex digits
  for (int i = 0; i < 6; i++) {
    char c = start[i];
    if (!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f'))) {
      return 0x0000;  // Invalid character
    }
  }
  
  // Parse hex values
  uint32_t r = 0, g = 0, b = 0;
  
  // Parse red component (first 2 characters)
  if (sscanf(start, "%2x", &r) != 1) {
    return 0x0000;
  }
  
  // Parse green component (next 2 characters)
  if (sscanf(start + 2, "%2x", &g) != 1) {
    return 0x0000;
  }
  
  // Parse blue component (last 2 characters)
  if (sscanf(start + 4, "%2x", &b) != 1) {
    return 0x0000;
  }
  
  // Convert 8-bit RGB to RGB565
  // RGB565 format: RRRRR GGGGGG BBBBB
  uint16_t r5 = (r >> 3) & 0x1F;  // 5 bits for red
  uint16_t g6 = (g >> 2) & 0x3F;  // 6 bits for green  
  uint16_t b5 = (b >> 3) & 0x1F;  // 5 bits for blue
  
  return (r5 << 11) | (g6 << 5) | b5;
}

