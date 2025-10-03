/**
 * @file clock_module.h
 * @brief Core RTC functionality using PCF8563 RTC module
 *
 * Provides core RTC hardware management, time operations, and clock display
 * functionality. For time synchronization and timezone features, use clock_sync.h
 */

#ifndef CLOCK_MODULE_H
#define CLOCK_MODULE_H

#include "RTClib.h"
#include "common.h"
#include "soundsfx_module.h"
#include <Arduino.h>
#include <Wire.h>

//==============================================================================
// CONSTANTS & DEFINITIONS
//==============================================================================

static const char *CLOCK_LOG = "::CLOCK_MODULE::";

#define RTC_SDA_PIN D4
#define RTC_SCL_PIN D5
#define RTC_I2C_FREQUENCY 100000

//==============================================================================
// TYPE DEFINITIONS
//==============================================================================

typedef enum {
  RTC_STATE_UNINITIALIZED,
  RTC_STATE_READY,
  RTC_STATE_ERROR,
  RTC_STATE_TIME_INVALID
} rtc_state_t;

typedef struct {
  uint16_t year;
  uint8_t month;
  uint8_t day;
  uint8_t hour;
  uint8_t minute;
  uint8_t second;
  uint8_t dayOfWeek;
  uint32_t unixtime;
} rtc_time_t;

//==============================================================================
// CORE RTC INITIALIZATION & STATUS FUNCTIONS
//==============================================================================

/**
 * @brief Initialize the RTC module and I2C communication
 * @return true if initialization successful, false otherwise
 */
bool initializeClock(void);

/**
 * @brief Check if RTC is initialized and ready
 * @return true if RTC is ready, false otherwise
 */
bool isClockReady(void);

/**
 * @brief Get current RTC module state
 * @return Current rtc_state_t value
 */
rtc_state_t getClockState(void);

/**
 * @brief Check if RTC has lost power (time invalid)
 * @return true if RTC has lost power, false if time is valid
 */
bool hasClockLostPower(void);

/**
 * @brief Periodic maintenance for RTC module (hardware only)
 */
void clockMaintenance(void);

//==============================================================================
// TIME READING FUNCTIONS
//==============================================================================

/**
 * @brief Get current date and time from RTC
 * @param timeStruct Pointer to rtc_time_t structure to populate
 * @return true if time read successfully, false on error
 */
bool getCurrentTime(rtc_time_t *timeStruct);

//==============================================================================
// TIME SETTING FUNCTIONS
//==============================================================================

/**
 * @brief Set RTC time using individual components
 * @param year Year (2000-2099)
 * @param month Month (1-12)
 * @param day Day (1-31)
 * @param hour Hour (0-23)
 * @param minute Minute (0-59)
 * @param second Second (0-59)
 * @return true if time set successfully, false otherwise
 */
bool setClockTime(uint16_t year, uint8_t month, uint8_t day, uint8_t hour,
                  uint8_t minute, uint8_t second);

/**
 * @brief Set RTC time using rtc_time_t structure
 * @param timeStruct Pointer to rtc_time_t structure with time to set
 * @return true if time set successfully, false otherwise
 */
bool setClockTimeFromStruct(const rtc_time_t *timeStruct);

/**
 * @brief Set RTC time to the firmware build time
 * @return true if time was set successfully, false otherwise
 */
bool setRTCToBuildTime();

//==============================================================================
// CLOCK DISPLAY FUNCTIONS
//==============================================================================

/**
 * @brief Initialize clock display mode
 */
void initializeClockDisplay(void);

/**
 * @brief Update clock display (call regularly when in clock mode)
 */
void updateClockDisplay(void);

/**
 * @brief Draw digital clock display with AM/PM (flicker-free)
 */
void drawDigitalClock(void);

/**
 * @brief Reset clock display state (call when entering clock mode)
 */
void resetClockDisplayState(void);

//==============================================================================
// UTILITY & HELPER FUNCTIONS
//==============================================================================

/**
 * @brief Log current date and time to serial/ESP_LOG
 * @param prefix Custom prefix for log message (NULL for default)
 */
void logCurrentTime(const char *prefix = nullptr);

/**
 * @brief Get day of week as string
 * @param dayOfWeek Day of week (0=Sunday, 6=Saturday)
 * @return String representation of day
 */
const char *getDayOfWeekString(uint8_t dayOfWeek);

/**
 * @brief Get month as string
 * @param month Month (1-12)
 * @return String representation of month
 */
const char *getMonthString(uint8_t month);

#endif /* CLOCK_MODULE_H */