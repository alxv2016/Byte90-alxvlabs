/**
 * @file haptics_module.cpp
 * @brief Implementation of simplified haptic feedback functionality
 */

#include "haptics_module.h"
#include "haptics_effects.h"
#include "display_module.h"
#include "i2c_module.h"
#include "preferences_module.h"
#include "common.h"
#include <Wire.h>

//==============================================================================
// GLOBAL VARIABLES
//==============================================================================

static Adafruit_DRV2605 drv;
static bool hapticsInitialized = false;
static haptic_actuator_t currentActuatorType = HAPTIC_ACTUATOR_ERM;

static bool hapticsActive = true;
static unsigned long hapticsTimeoutStart = 0;
static const unsigned long HAPTICS_TIMEOUT = 45000;
static const float HAPTICS_ACTIVITY_THRESHOLD = 1.5;

static bool pulseActive = false;
static unsigned long pulseStartTime = 0;
static uint16_t pulseDuration = 0;

//==============================================================================
// PUBLIC API FUNCTIONS
//==============================================================================

/**
 * @brief Initialize the haptics module
 * @param actuatorType Type of haptic actuator
 * @return true if initialization successful, false otherwise
 */
bool initializeHaptics(haptic_actuator_t actuatorType) {
    if (!checkHardwareSupport()) {
        ESP_LOGE(HAPTICS_LOG, "Haptics support not enabled at compile time");
        return false;
    }
    
    if (!isI2CReady()) {
        ESP_LOGE(HAPTICS_LOG, "I2C bus not initialized! Call initializeI2C() first");
        return false;
    }
    
    if (!drv.begin(getI2CBus())) {
        ESP_LOGE(HAPTICS_LOG, "Could not find DRV2605L haptic driver");
        return false;
    }
    
    currentActuatorType = actuatorType;
    
    if (actuatorType == HAPTIC_ACTUATOR_LRA) {
        drv.selectLibrary(6);
        drv.useLRA();
    } else {
        drv.selectLibrary(1);
        drv.useERM();
    }
    
    drv.setMode(DRV2605_MODE_INTTRIG);

    drv.writeRegister8(DRV2605_REG_OVERDRIVE, 0xFF);
    drv.writeRegister8(DRV2605_REG_SUSTAINPOS, 0xFF);
    drv.writeRegister8(DRV2605_REG_SUSTAINNEG, 0xFF);
    drv.writeRegister8(DRV2605_REG_BREAKTIME, 0xFF);
    
    uint8_t overdrive = drv.readRegister8(DRV2605_REG_OVERDRIVE);
    uint8_t sustainPos = drv.readRegister8(DRV2605_REG_SUSTAINPOS);
    uint8_t sustainNeg = drv.readRegister8(DRV2605_REG_SUSTAINNEG);
    
    hapticsInitialized = true;
    
    // Load haptic preference from storage
    hapticsActive = getHapticEnabled();
    hapticsTimeoutStart = millis();
    
    return true;
}

/**
 * @brief Check if haptics module is ready
 * @return true if haptics is initialized and ready, false otherwise
 */
bool isHapticsReady(void) {
    return hapticsInitialized;
}

/**
 * @brief Shutdown haptics and enter standby mode
 */
void shutdownHaptics(void) {
    if (!hapticsInitialized) {
        return;
    }
    
    drv.stop();
    
    uint8_t mode = drv.readRegister8(DRV2605_REG_MODE);
    drv.writeRegister8(DRV2605_REG_MODE, mode | 0x40);
}

/**
 * @brief Play a haptic effect from the ROM library
 * @param effectId Effect ID to play (1-123)
 * @return true if effect started successfully, false otherwise
 */
bool playHapticEffect(uint8_t effectId) {
    if (!hapticsInitialized) {
        ESP_LOGE(HAPTICS_LOG, "Haptics not initialized");
        return false;
    }
    
    if (!hapticsActive) {
        return false;
    }
    
    if (effectId < 1 || effectId > 123) {
        ESP_LOGE(HAPTICS_LOG, "Invalid effect ID: %d (valid range: 1-123)", effectId);
        return false;
    }
    
    drv.setWaveform(0, effectId);
    drv.setWaveform(1, 0);
    
    drv.go();
    
    return true;
}

/**
 * @brief Start continuous vibration at specified intensity
 * @param intensity Vibration intensity (0-255)
 * @return true if vibration started successfully, false otherwise
 */
bool startContinuousVibration(uint8_t intensity) {
    if (!hapticsInitialized) {
        ESP_LOGE(HAPTICS_LOG, "Haptics not initialized");
        return false;
    }
    
    if (!hapticsActive) {
        return false;
    }
    
    drv.setMode(DRV2605_MODE_REALTIME);
    drv.setRealtimeValue(intensity);
    
    return true;
}

/**
 * @brief Set vibration intensity for continuous mode
 * @param intensity New vibration intensity (0-255, 0=stop)
 * @return true if intensity set successfully, false otherwise
 */
bool setVibrationIntensity(uint8_t intensity) {
    if (!hapticsInitialized) {
        ESP_LOGE(HAPTICS_LOG, "Haptics not initialized");
        return false;
    }
    
    if (!hapticsActive) {
        return false;
    }
    
    uint8_t mode = drv.readRegister8(DRV2605_REG_MODE) & 0x07;
    if (mode != DRV2605_MODE_REALTIME) {
        drv.setMode(DRV2605_MODE_REALTIME);
    }
    
    drv.setRealtimeValue(intensity);
    
    return true;
}

/**
 * @brief Stop any currently playing haptic effect
 * @return true if stop successful, false otherwise
 */
bool stopHapticEffect(void) {
    if (!hapticsInitialized) {
        return false;
    }
    
    drv.stop();
    
    uint8_t mode = drv.readRegister8(DRV2605_REG_MODE) & 0x07;
    if (mode == DRV2605_MODE_REALTIME) {
        drv.setRealtimeValue(0);
        drv.setMode(DRV2605_MODE_INTTRIG);
    }
    
    return true;
}

/**
 * @brief Enable haptics and reset timeout
 */
void enableHaptics(void) {
    if (!hapticsInitialized) {
        return;
    }
    
    if (!hapticsActive) {
        hapticsActive = true;
        hapticsTimeoutStart = millis();
        
        uint8_t mode = drv.readRegister8(DRV2605_REG_MODE);
        drv.writeRegister8(DRV2605_REG_MODE, mode & ~0x40);
        
        delay(10);
        playHapticEffect(HAPTIC_SHARP_CLICK_100);
    } else {
        hapticsTimeoutStart = millis();
    }
}

/**
 * @brief Disable haptics to save power
 */
void disableHaptics(void) {
    if (!hapticsInitialized || !hapticsActive) {
        return;
    }
    
    hapticsActive = false;
    
    stopHapticEffect();
    shutdownHaptics();
}

/**
 * @brief Check if haptics are currently active
 * @return true if haptics are enabled, false otherwise
 */
bool areHapticsActive(void) {
    return hapticsActive && hapticsInitialized;
}

/**
 * @brief Update haptics power state based on activity level
 * @param activityLevel Current activity magnitude (m/s²)
 */
void updateHapticsPowerState(float activityLevel) {
    if (!hapticsInitialized) {
        return;
    }
    
    // Only enable haptics if user preference allows it
    if (activityLevel > HAPTICS_ACTIVITY_THRESHOLD && getHapticEnabled()) {
        enableHaptics();
    } else {
        if (hapticsActive && millis() - hapticsTimeoutStart >= HAPTICS_TIMEOUT) {
            disableHaptics();
        }
    }
}

/**
 * @brief Reset haptics timeout
 */
void resetHapticsTimeout(void) {
    if (hapticsInitialized) {
        hapticsTimeoutStart = millis();
        // Only enable haptics if user preference allows it
        if (!hapticsActive && getHapticEnabled()) {
            enableHaptics();
        }
    }
}

/**
 * @brief Get diagnostic information
 * @param buffer Buffer to store diagnostic string (minimum 192 bytes)
 * @return true if diagnostics read successfully, false otherwise
 */
bool getHapticsDiagnostics(char *buffer) {
    if (!buffer || !hapticsInitialized) {
        return false;
    }
    
    uint8_t status = drv.readRegister8(DRV2605_REG_STATUS);
    uint8_t mode = drv.readRegister8(DRV2605_REG_MODE);
    uint8_t rtpInput = drv.readRegister8(DRV2605_REG_RTPIN);
    uint8_t library = drv.readRegister8(DRV2605_REG_LIBRARY);
    uint8_t feedback = drv.readRegister8(DRV2605_REG_FEEDBACK);
    
    snprintf(buffer, 192,
        "DRV2605L Status:\n"
        "  Status: 0x%02X (Playing: %s, Standby: %s)\n"
        "  Mode: %d, Library: %d\n"
        "  RTP Input: %d, Actuator: %s\n"
        "  Power State: %s, Timeout: %lus",
        status,
        (status & 0x01) ? "Yes" : "No",
        (mode & 0x40) ? "Yes" : "No",
        mode & 0x07, library,
        rtpInput,
        (currentActuatorType == HAPTIC_ACTUATOR_LRA) ? "LRA" : "ERM",
        hapticsActive ? "Active" : "Disabled",
        hapticsActive ? (millis() - hapticsTimeoutStart) / 1000 : 0
    );
    
    return true;
}

/**
 * @brief Log current haptics status
 */
void logHapticsStatus(void) {
    if (!hapticsInitialized) {
        return;
    }
    
    char buffer[192];
    if (getHapticsDiagnostics(buffer)) {
        ESP_LOGI(HAPTICS_LOG, "%s", buffer);
    }
}

/**
 * @brief Start a non-blocking pulse vibration
 * @param intensity Vibration intensity (0-255)
 * @param durationMs Duration in milliseconds
 * @return true if pulse started successfully, false otherwise
 */
bool startPulseVibration(uint8_t intensity, uint16_t durationMs) {
    if (!startContinuousVibration(intensity)) {
        return false;
    }
    
    pulseActive = true;
    pulseStartTime = millis();
    pulseDuration = durationMs;
    
    resetHapticsTimeout();
    
    return true;
}

/**
 * @brief Update non-blocking pulse vibration
 * @return true if pulse is still active, false if completed
 */
bool updateHapticsPulse(void) {
    if (!pulseActive) {
        return false;
    }
    
    if (millis() - pulseStartTime >= pulseDuration) {
        stopHapticEffect();
        pulseActive = false;
        return false;
    }
    
    return true;
}