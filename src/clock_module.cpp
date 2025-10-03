/**
 * @file clock_module.cpp
 * @brief Core RTC functionality implementation using PCF8563 RTC module
 *
 * Provides core RTC hardware management, time operations, and clock display.
 * For time synchronization features, see clock_sync.cpp
 */

#include "clock_module.h"
#include "i2c_module.h"
#include "display_module.h"
#include "soundsfx_module.h"
#include "common.h"

//==============================================================================
// GLOBAL VARIABLES
//==============================================================================

// Core RTC variables
static RTC_PCF8563 rtc;
static rtc_state_t clockState = RTC_STATE_UNINITIALIZED;

// Clock display variables
static unsigned long lastClockUpdate = 0;
static const unsigned long CLOCK_UPDATE_INTERVAL = 1000; // Check every 1 second
static uint8_t lastDisplayedMinute = 255;
static uint8_t lastDisplayedHour = 255;
static bool clockDisplayInitialized = false;

// Flicker-free display tracking
static char lastTimeStr[32] = "";
static char lastAmpmStr[4] = "";
static char lastDateStr[32] = "";
static char lastDayStr[16] = "";

// String constants
static const char *daysOfWeek[] = {
    "Sunday", "Monday", "Tuesday", "Wednesday", 
    "Thursday", "Friday", "Saturday"
};

static const char *monthNames[] = {
    "Invalid", "January", "February", "March", "April", "May", "June",
    "July", "August", "September", "October", "November", "December"
};

//==============================================================================
// UTILITY FUNCTIONS (STATIC)
//==============================================================================


/**
 * @brief Convert DateTime to rtc_time_t structure
 * @param dt DateTime object from RTClib
 * @param timeStruct Pointer to rtc_time_t structure to populate
 * @return true if conversion successful, false otherwise
 */
static bool dateTimeToStruct(const DateTime &dt, rtc_time_t *timeStruct) {
    if (!timeStruct) {
        return false;
    }
    
    timeStruct->year = dt.year();
    timeStruct->month = dt.month();
    timeStruct->day = dt.day();
    timeStruct->hour = dt.hour();
    timeStruct->minute = dt.minute();
    timeStruct->second = dt.second();
    timeStruct->dayOfWeek = dt.dayOfTheWeek();
    timeStruct->unixtime = dt.unixtime();
    
    return true;
}

/**
 * @brief Update RTC status based on current conditions
 * @return Updated rtc_state_t value
 */
static rtc_state_t updateClockState() {
    if (clockState == RTC_STATE_UNINITIALIZED) {
        return RTC_STATE_UNINITIALIZED;
    }
    
    if (!rtc.isrunning()) {
        clockState = RTC_STATE_ERROR;
        return clockState;
    }
    
    if (rtc.lostPower()) {
        clockState = RTC_STATE_TIME_INVALID;
        return clockState;
    }
    
    clockState = RTC_STATE_READY;
    return clockState;
}

//==============================================================================
// CORE RTC INITIALIZATION & STATUS FUNCTIONS
//==============================================================================

/**
 * @brief Initialize the RTC module using shared I2C bus
 * @return true if initialization successful, false otherwise
 */
bool initializeClock(void) {
    if (!checkHardwareSupport()) return false;
    
    if (!isI2CReady()) {
        ESP_LOGE(CLOCK_LOG, "I2C bus not initialized! Call initializeI2C() first.");
        clockState = RTC_STATE_ERROR;
        return false;
    }
    
    if (!rtc.begin(getI2CBus())) {
        ESP_LOGE(CLOCK_LOG, "Could not find PCF8563 RTC module!");
        clockState = RTC_STATE_ERROR;
        return false;
    }
    
    if (rtc.lostPower()) {
        ESP_LOGW(CLOCK_LOG, "RTC has lost power or was not initialized");
        // Set to build time automatically
        DateTime buildTime(F(__DATE__), F(__TIME__));
        rtc.adjust(buildTime);
        rtc.start();
        
        // Check if this resolved the power loss
        delay(100);
        if (!rtc.lostPower()) {
            ESP_LOGI(CLOCK_LOG, "RTC time set to build time successfully");
            clockState = RTC_STATE_READY;
        } else {
            ESP_LOGW(CLOCK_LOG, "Power loss flag still present after setting time");
            clockState = RTC_STATE_TIME_INVALID;
        }
    } else {
        rtc.start();
        
        if (!rtc.isrunning()) {
            ESP_LOGE(CLOCK_LOG, "RTC is not running!");
            clockState = RTC_STATE_ERROR;
            return false;
        }
        
        clockState = RTC_STATE_READY;
    }
    
    logCurrentTime("RTC initialization complete -");
    return (clockState != RTC_STATE_ERROR);
}

/**
 * @brief Check if RTC is initialized and ready
 * @return true if RTC is ready, false otherwise
 */
bool isClockReady(void) {
    return (clockState == RTC_STATE_READY || clockState == RTC_STATE_TIME_INVALID);
}

/**
 * @brief Get current RTC module state
 * @return Current rtc_state_t value
 */
rtc_state_t getClockState(void) {
    if (clockState != RTC_STATE_UNINITIALIZED) {
        updateClockState();
    }
    
    return clockState;
}

/**
 * @brief Check if RTC has lost power
 * @return true if RTC has lost power, false if time is valid
 */
bool hasClockLostPower(void) {
    if (clockState == RTC_STATE_UNINITIALIZED || clockState == RTC_STATE_ERROR) {
        return true;
    }
    
    return rtc.lostPower();
}

/**
 * @brief Periodic maintenance for RTC module
 */
void clockMaintenance(void) {
    if (clockState != RTC_STATE_UNINITIALIZED) {
        rtc_state_t newState = updateClockState();
        
        if (newState != clockState) {
            clockState = newState;
        }
    }
}


//==============================================================================
// TIME READING FUNCTIONS
//==============================================================================

/**
 * @brief Get current date and time from RTC
 * @param timeStruct Pointer to rtc_time_t structure to populate
 * @return true if time read successfully, false on error
 */
bool getCurrentTime(rtc_time_t *timeStruct) {
    if (!timeStruct || clockState == RTC_STATE_UNINITIALIZED || clockState == RTC_STATE_ERROR) {
        return false;
    }
    
    DateTime now = rtc.now();
    return dateTimeToStruct(now, timeStruct);
}


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
bool setClockTime(uint16_t year, uint8_t month, uint8_t day, 
                  uint8_t hour, uint8_t minute, uint8_t second) {
    if (clockState == RTC_STATE_UNINITIALIZED || clockState == RTC_STATE_ERROR) {
        return false;
    }
    
    if (year < 2000 || year > 2099 || month < 1 || month > 12 || day < 1 || day > 31 ||
        hour > 23 || minute > 59 || second > 59) {
        ESP_LOGE(CLOCK_LOG, "Invalid time parameters");
        return false;
    }
    
    rtc.adjust(DateTime(year, month, day, hour, minute, second));
    rtc.start();
    
    if (clockState == RTC_STATE_TIME_INVALID) {
        clockState = RTC_STATE_READY;
    }
    
    return true;
}

/**
 * @brief Set RTC time using rtc_time_t structure
 * @param timeStruct Pointer to rtc_time_t structure with time to set
 * @return true if time set successfully, false otherwise
 */
bool setClockTimeFromStruct(const rtc_time_t *timeStruct) {
    if (!timeStruct || clockState == RTC_STATE_UNINITIALIZED || clockState == RTC_STATE_ERROR) {
        return false;
    }
    
    return setClockTime(
        timeStruct->year, timeStruct->month, timeStruct->day,
        timeStruct->hour, timeStruct->minute, timeStruct->second
    );
}

/**
 * @brief Set RTC time to the firmware build time
 * @return true if time was set successfully, false otherwise
 */
bool setRTCToBuildTime() {
    if (clockState == RTC_STATE_UNINITIALIZED || clockState == RTC_STATE_ERROR) {
        ESP_LOGE(CLOCK_LOG, "RTC not ready - cannot set build time");
        return false;
    }
    
    // Create DateTime object from compile time
    DateTime buildTime(F(__DATE__), F(__TIME__));
    
    ESP_LOGI(CLOCK_LOG, "Setting RTC to build time: %s %s", __DATE__, __TIME__);
    ESP_LOGI(CLOCK_LOG, "Parsed build time: %04d-%02d-%02d %02d:%02d:%02d",
             buildTime.year(), buildTime.month(), buildTime.day(),
             buildTime.hour(), buildTime.minute(), buildTime.second());
    
    // Set the RTC time
    rtc.adjust(buildTime);
    rtc.start();
    
    // Update clock state
    if (clockState == RTC_STATE_TIME_INVALID) {
        clockState = RTC_STATE_READY;
    }
    
    // Verify the time was set
    delay(100); // Small delay to ensure RTC updates
    DateTime readBack = rtc.now();
    
    ESP_LOGI(CLOCK_LOG, "RTC readback: %04d-%02d-%02d %02d:%02d:%02d",
             readBack.year(), readBack.month(), readBack.day(),
             readBack.hour(), readBack.minute(), readBack.second());
    
    logCurrentTime("RTC set to build time:");
    return true;
}

//==============================================================================
// CLOCK DISPLAY FUNCTIONS
//==============================================================================


/**
 * @brief Initialize clock display mode
 */
void initializeClockDisplay(void) {
    resetClockDisplayState();
    lastClockUpdate = 0;
    ESP_LOGI(CLOCK_LOG, "Clock display mode initialized");
}

/**
 * @brief Update clock display
 */
void updateClockDisplay(void) {
    unsigned long currentTime = millis();
    
    // Check every second for minute/hour changes
    if (currentTime - lastClockUpdate >= CLOCK_UPDATE_INTERVAL) {
        rtc_time_t timeStruct;
        if (getCurrentTime(&timeStruct)) {
            // Check for hour change and play celebration sound
            if (timeStruct.hour != lastDisplayedHour && lastDisplayedHour != 255) {
                drawDigitalClock();
                sfxPlayCelebration();
            }
            
            // Only redraw if minute changed
            if (timeStruct.minute != lastDisplayedMinute) {
                drawDigitalClock();
                lastDisplayedMinute = timeStruct.minute;
            }
            
            // Update hour tracking
            lastDisplayedHour = timeStruct.hour;
        } else {
            drawDigitalClock();
        }
        lastClockUpdate = currentTime;
    }
}

/**
 * @brief Draw digital clock display with AM/PM
 */
void drawDigitalClock(void) {
    bool timeValid = (getClockState() == RTC_STATE_READY);
    
    if (!isClockReady()) {
        // RTC hardware not available - only clear and draw if not already showing error
        if (clockDisplayInitialized) {
            clearDisplay();
            clockDisplayInitialized = false;
        }
        
        oled.setFont();
        oled.setTextSize(1);
        oled.setTextColor(COLOR_YELLOW);
        
        int16_t x1, y1;
        uint16_t w, h;
        const char* errorMsg = "RTC NOT AVAILABLE";
        oled.getTextBounds(errorMsg, 0, 0, &x1, &y1, &w, &h);
        int x = (DISPLAY_WIDTH - w) / 2;
        int y = (DISPLAY_HEIGHT - h) / 2;
        
        oled.setCursor(x, y);
        oled.print(errorMsg);
        return;
    }

    rtc_time_t currentTime;
    if (!getCurrentTime(&currentTime)) {
        return;
    }

    // Convert to 12-hour format
    uint8_t displayHour = currentTime.hour;
    bool isPM = false;
    
    if (displayHour == 0) {
        displayHour = 12; // Midnight is 12 AM
    } else if (displayHour > 12) {
        displayHour -= 12;
        isPM = true;
    } else if (displayHour == 12) {
        isPM = true; // Noon is 12 PM
    }

    // Get colors based on current effect tint
    uint16_t primaryColor, accentColor;
    getDOSColorsForCurrentTint(&primaryColor, &accentColor);
    
    // If time is invalid, use different colors to indicate the issue
    if (!timeValid) {
        primaryColor = 0xF800; // Red to indicate invalid time
        accentColor = 0xFFE0;  // Yellow
    }
    
    // Format current strings
    char timeStr[32];
    snprintf(timeStr, sizeof(timeStr), "%2d:%02d", displayHour, currentTime.minute);
    
    char ampmStr[4];
    snprintf(ampmStr, sizeof(ampmStr), "%s", isPM ? "PM" : "AM");
    
    // Short month names array (uppercase)
    const char* shortMonths[] = {
        "XXX", "JAN", "FEB", "MAR", "APR", "MAY", "JUN",
        "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"
    };
    
    // Format date string (uppercase)
    char dateStr[32];
    if (timeValid) {
        snprintf(dateStr, sizeof(dateStr), "%s %d, %d", 
                 shortMonths[currentTime.month], currentTime.day, currentTime.year);
    } else {
        snprintf(dateStr, sizeof(dateStr), "TIME NOT SET");
    }
    
    char dayStr[16];
    if (timeValid) {
        // Get day string and convert to uppercase
        const char* dayName = getDayOfWeekString(currentTime.dayOfWeek);
        snprintf(dayStr, sizeof(dayStr), "%s", dayName);
        // Convert to uppercase
        for (int i = 0; dayStr[i]; i++) {
            if (dayStr[i] >= 'a' && dayStr[i] <= 'z') {
                dayStr[i] = dayStr[i] - 'a' + 'A';
            }
        }
    } else {
        snprintf(dayStr, sizeof(dayStr), "INVALID");
    }

    // If this is the first time drawing or coming from error state, clear everything
    if (!clockDisplayInitialized) {
        clearDisplay();
        clockDisplayInitialized = true;
        // Force update of all elements by clearing the "last" strings
        strcpy(lastTimeStr, "");
        strcpy(lastAmpmStr, "");
        strcpy(lastDateStr, "");
        strcpy(lastDayStr, "");
    }

    int16_t x1, y1;
    uint16_t w, h;

    // Update time only if it changed
    if (strcmp(timeStr, lastTimeStr) != 0) {
        // Clear the old time area first (use larger area to ensure complete clearing)
        oled.setFont(&FreeSansBold9pt7b);
        oled.setTextSize(3);
        
        // Calculate bounds for old time string if it exists
        if (strlen(lastTimeStr) > 0) {
            oled.getTextBounds(lastTimeStr, 0, 0, &x1, &y1, &w, &h);
            int oldTimeX = (DISPLAY_WIDTH - w) / 2 - 6;
            int timeY = 74;
            // Clear with extra margin to ensure complete erasure
            oled.fillRect(oldTimeX - 5, timeY + y1 - 3, w + 15, h + 6, COLOR_BLACK);
        }
        
        // Draw new time
        oled.setTextWrap(false);
        oled.setTextColor(primaryColor);
        oled.getTextBounds(timeStr, 0, 0, &x1, &y1, &w, &h);
        int timeX = (DISPLAY_WIDTH - w) / 2 - 6;
        int timeY = 74;
        
        oled.setCursor(timeX, timeY);
        oled.print(timeStr);
        
        strcpy(lastTimeStr, timeStr);
    }

    // Update AM/PM only if it changed
    if (strcmp(ampmStr, lastAmpmStr) != 0) {
        // Set font for AM/PM
        oled.setFont();
        oled.setTextSize(2);
        
        // Calculate the exact bounds for both AM and PM to ensure proper clearing
        int16_t am_x1, am_y1, pm_x1, pm_y1;
        uint16_t am_w, am_h, pm_w, pm_h;
        oled.getTextBounds("AM", 0, 0, &am_x1, &am_y1, &am_w, &am_h);
        oled.getTextBounds("PM", 0, 0, &pm_x1, &pm_y1, &pm_w, &pm_h);
        
        // Use the larger of the two widths to ensure complete clearing
        uint16_t maxWidth = (am_w > pm_w) ? am_w : pm_w;
        uint16_t maxHeight = (am_h > pm_h) ? am_h : pm_h;
        
        // Clear area large enough for either AM or PM with extra margin
        int ampmX = 101;
        int ampmY = 98;
        oled.fillRect(ampmX - 2, ampmY - maxHeight - 2, maxWidth + 6, maxHeight + 6, COLOR_BLACK);
        
        // Draw new AM/PM
        oled.setTextColor(accentColor);
        oled.setCursor(ampmX, ampmY);
        oled.print(ampmStr);
        
        strcpy(lastAmpmStr, ampmStr);
    }

    // Update day only if it changed
    if (strcmp(dayStr, lastDayStr) != 0) {
        // Clear old day area with proper bounds calculation
        oled.setFont();
        oled.setTextSize(1);
        
        if (strlen(lastDayStr) > 0) {
            oled.getTextBounds(lastDayStr, 0, 0, &x1, &y1, &w, &h);
            oled.fillRect(2, 100, 80, 12, COLOR_BLACK); // Fixed height area
        }
        
        // Draw new day
        oled.setTextColor(accentColor);
        oled.setCursor(4, 105);
        oled.print(dayStr);
        
        strcpy(lastDayStr, dayStr);
    }

    // Update date only if it changed
    if (strcmp(dateStr, lastDateStr) != 0) {
        // Draw new date box
        int boxWidth = DISPLAY_WIDTH;
        int boxHeight = 13;
        int boxX = 0;
        int boxY = DISPLAY_HEIGHT - boxHeight;

        // Always clear and redraw the entire date box to prevent artifacts
        oled.fillRect(boxX, boxY, boxWidth, boxHeight, COLOR_BLACK);
        
        // Draw filled rectangle background
        oled.fillRect(boxX, boxY, boxWidth, boxHeight, accentColor);
        
        // Draw date text in contrasting color
        oled.setFont();
        oled.setTextSize(1);
        oled.setTextColor(COLOR_BLACK);
        oled.setCursor(4, 118);
        oled.print(dateStr);
        
        strcpy(lastDateStr, dateStr);
    }
    
    // Draw WiFi status indicator
    updateWiFiStatusIndicator();
}

/**
 * @brief Reset clock display state
 */
void resetClockDisplayState(void) {
    clockDisplayInitialized = false;
    lastDisplayedMinute = 255;
    lastDisplayedHour = 255;
    strcpy(lastTimeStr, "");
    strcpy(lastAmpmStr, "");
    strcpy(lastDateStr, "");
    strcpy(lastDayStr, "");
    ESP_LOGI(CLOCK_LOG, "Clock display state reset");
}

//==============================================================================
// UTILITY & HELPER FUNCTIONS
//==============================================================================

/**
 * @brief Log current date and time to serial/ESP_LOG
 * @param prefix Custom prefix for log message (NULL for default)
 */
void logCurrentTime(const char *prefix) {
    if (clockState == RTC_STATE_UNINITIALIZED) {
        ESP_LOGW(CLOCK_LOG, "RTC not initialized");
        return;
    }
    
    if (clockState == RTC_STATE_ERROR) {
        ESP_LOGE(CLOCK_LOG, "RTC error - cannot read time");
        return;
    }
    
    DateTime now = rtc.now();
    
    if (prefix) {
        ESP_LOGI(CLOCK_LOG, "%s %04d-%02d-%02d %02d:%02d:%02d (%s)",
                prefix,
                now.year(), now.month(), now.day(),
                now.hour(), now.minute(), now.second(),
                daysOfWeek[now.dayOfTheWeek()]);
    } else {
        ESP_LOGI(CLOCK_LOG, "Current time: %04d-%02d-%02d %02d:%02d:%02d (%s)",
                now.year(), now.month(), now.day(),
                now.hour(), now.minute(), now.second(),
                daysOfWeek[now.dayOfTheWeek()]);
    }
}

/**
 * @brief Get day of week as string
 * @param dayOfWeek Day of week (0=Sunday, 6=Saturday)
 * @return String representation of day
 */
const char* getDayOfWeekString(uint8_t dayOfWeek) {
    if (dayOfWeek > 6) {
        return "Invalid";
    }
    
    return daysOfWeek[dayOfWeek];
}

/**
 * @brief Get month as string
 * @param month Month (1-12)
 * @return String representation of month
 */
const char* getMonthString(uint8_t month) {
    if (month < 1 || month > 12) {
        return "Invalid";
    }
    
    return monthNames[month];
}
