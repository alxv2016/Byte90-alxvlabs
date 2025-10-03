/**
 * @file common.h
 * @brief Common definitions and utility functions
 *
 * Provides common includes, constants, and utility functions that are used
 * across multiple modules in the application.
 */

#ifndef COMMON_H
#define COMMON_H

#include <Arduino.h>
#include <ESP_log.h>
#include <SPI.h>
#include <Wire.h>

//==============================================================================
// CONSTANTS & DEFINITIONS
//==============================================================================

#define DEVICE_MODE BYTE_MODE
#define MAC_MODE 1
#define PC_MODE 2
#define BYTE_MODE 3

#define SERIES_2 true

#define DISPLAY_WIDTH 128
#define DISPLAY_HEIGHT 128

//==============================================================================
// PUBLIC API FUNCTIONS
//==============================================================================

/**
 * @brief Timer utility function to check if a specified time has elapsed
 * @param setTime Reference to the timestamp variable to check and update
 * @param delayTime Time period in milliseconds to check against
 * @return true if the specified time has elapsed, false otherwise
 */
bool setTimeout(unsigned long &setTime, unsigned long delayTime);

/**
 * @brief Convert hours and minutes to milliseconds
 * @param hours Number of hours
 * @param minutes Number of minutes
 * @return Total time in milliseconds
 */
unsigned long timeToMillis(int hours, int minutes);

/**
 * @brief Get chip model name as string
 * @return String containing the chip model
 */
String getChipModel();

/**
 * @brief Get CPU frequency in MHz
 * @return CPU frequency as integer
 */
uint32_t getCpuFrequencyMHz();

/**
 * @brief Get flash size in MB
 * @return Flash size in MB
 */
uint32_t getFlashSizeMB();

/**
 * @brief Get PSRAM size in MB
 * @return PSRAM size in MB, 0 if no PSRAM
 */
uint32_t getPSRAMSizeMB();

/**
 * @brief Get display controller info
 * @return String with display info
 */
String getDisplayInfo();

/**
 * @brief Check if SERIES_2 hardware features are supported
 * @return true if SERIES_2 features are enabled, false otherwise
 */
bool checkHardwareSupport();

//==============================================================================
// STRING UTILITY FUNCTIONS
//==============================================================================

/**
 * @brief Convert bytes to human-readable format (B, KB, MB, GB)
 * @param bytes Number of bytes to convert
 * @return String representation with appropriate unit
 */
String formatBytes(size_t bytes);

//==============================================================================
// COLOR UTILITY FUNCTIONS
//==============================================================================

/**
 * @brief Convert hex color string to RGB565 format
 * @param hexColor Hex color string (e.g., "#FF0000", "FF0000", "0xFF0000")
 * @return RGB565 color value, or 0x0000 if invalid
 */
uint16_t hexToRGB565(const char* hexColor);


#endif /* COMMON_H */