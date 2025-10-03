/**
 * @file adxl_module.cpp
 * @brief Implementation of the ADXL345 accelerometer module functionality
 *
 * Handles sensor initialization, register operations, interrupt management,
 * and deep sleep functionality for the ADXL345 accelerometer.
 */

#include "adxl_module.h"
#include "i2c_module.h"
#include "common.h"
#include <stdarg.h>

//==============================================================================
// GLOBAL VARIABLES
//==============================================================================

static Adafruit_ADXL345_Unified adxl = Adafruit_ADXL345_Unified(12345);
static sensors_event_t event;
static bool ADXL345Enabled = false;
static bool interruptsEnabled = false;
static bool interruptsInitialized = false;

//==============================================================================
// UTILITY FUNCTIONS (STATIC)
//==============================================================================

// Debug control - set to true to enable debug logging
static bool adxlDebugEnabled = false;

/**
 * @brief Centralized debug logging function for ADXL module operations
 * @param format Printf-style format string
 * @param ... Variable arguments for format string
 */
static void adxlDebug(const char* format, ...) {
  if (!adxlDebugEnabled) {
    return;
  }
  
  va_list args;
  va_start(args, format);
  esp_log_writev(ESP_LOG_INFO, ADXL_LOG, format, args);
  va_end(args);
}

/**
 * @brief Enable or disable debug logging for ADXL module operations
 * @param enabled true to enable debug logging, false to disable
 * 
 * @example
 * // Enable debug logging
 * setAdxlDebug(true);
 * 
 * // Disable debug logging  
 * setAdxlDebug(false);
 */
void setAdxlDebug(bool enabled) {
  adxlDebugEnabled = enabled;
  if (enabled) {
    ESP_LOGI(ADXL_LOG, "ADXL module debug logging enabled");
  } else {
    ESP_LOGI(ADXL_LOG, "ADXL module debug logging disabled");
  }
}

/**
 * @brief Calculates scaled threshold value for G-force
 * @param gforce G-force value to scale
 * @return Scaled 8-bit threshold value (0-255)
 */
static uint8_t calcGforce(float gforce) {
  uint8_t threshold = min((uint8_t)(gforce * 1000 / FORCE_SCALE_FACTOR), (uint8_t)255);
  return threshold;
};

/**
 * @brief Calculates scaled duration value for tap detection
 * @param durationMs Duration in milliseconds
 * @return Scaled 8-bit duration value (0-255)
 */
static uint8_t calcDuration(float durationMs) {
  uint8_t duration = min((uint8_t)(durationMs / DURATION_SCALE_FACTOR), (uint8_t)255);
  return duration;
};

/**
 * @brief Calculates scaled latency value for tap detection
 * @param latencyMs Latency in milliseconds
 * @return Scaled 8-bit latency value (0-255)
 */
static uint8_t calcLatency(float latencyMs) {
  uint8_t duration = min((uint8_t)(latencyMs / LATENCY_SCALE_FACTOR), (uint8_t)255);
  return duration;
};

/**
 * @brief Attempts to initialize the ADXL345 sensor with multiple retries
 * @param attempts Maximum number of initialization attempts
 * @return true if initialization successful, false otherwise
 */
static bool retrySensorInit(uint8_t attempts = 3) {
  uint8_t retriesLeft = attempts;
  
  if (!isI2CReady()) {
    ESP_LOGE(ADXL_LOG, "I2C bus not initialized! Call initializeI2C() first.");
    return false;
  }

  while (retriesLeft--) {
    Wire.beginTransmission(ADXL345_DEFAULT_ADDRESS);
    byte error = Wire.endTransmission();
    if (error != 0) {
      ESP_LOGE(ADXL_LOG, "I2C communication failed. Error code=%d, %d retries left", error, retriesLeft);
      
      Wire.beginTransmission(error == 2 ? (ADXL345_DEFAULT_ADDRESS == 0x1D ? 0x53 : 0x1D) : ADXL345_DEFAULT_ADDRESS);
      error = Wire.endTransmission();
      if (error == 0) {
        ESP_LOGW(ADXL_LOG, "Device found at alternative address, check your wiring configuration");
      }
      if (retriesLeft > 0) {
        delay(500);
        continue;
      } else {
        return false;
      }
    }

    pinMode(INTERRUPT_PIN_D1, INPUT);

    if (adxl.begin()) {
      sensors_event_t event;
      adxl.getEvent(&event);
      uint8_t intSource = adxl.readRegister(ADXL345_REG_INT_SOURCE);
      return true;
    }

    ESP_LOGW(ADXL_LOG, "ADXL345 begin() failed, %d retries left", retriesLeft);
    delay(500);
  }

  ESP_LOGE(ADXL_LOG, "ADXL345 initialization failed after %d attempts", attempts);
  return false;
}

/**
 * @brief Writes a value to a specified register on the ADXL345
 * @param reg Register address
 * @param value Value to write
 */
static void writeRegister(uint8_t reg, uint8_t value) {
  if (!ADXL345Enabled)
    return;
  adxl.writeRegister(reg, value);
}

/**
 * @brief Configures ESP deep sleep mode with ADXL345 as wake-up source
 */
static void configureESPDeepSleep() {
  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
  
  clearInterrupts();
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
  esp_sleep_enable_ext0_wakeup((gpio_num_t)INTERRUPT_PIN_D1, HIGH);
}

//==============================================================================
// PUBLIC API FUNCTIONS
//==============================================================================

/**
 * @brief Temporarily disable ADXL345 INT1 interrupt
 */
void disableADXLInterrupts() {
  if (!ADXL345Enabled || !interruptsEnabled) {
    adxlDebug("Skipping interrupt disable - sensor not enabled or interrupts already disabled");
    return;
  }
  
  adxlDebug("Disabling ADXL345 interrupts...");
  writeRegister(ADXL345_REG_INT_ENABLE, 0x00);
  pinMode(INTERRUPT_PIN_D1, INPUT);
  interruptsEnabled = false;
  adxlDebug("ADXL345 interrupts disabled");
}

/**
 * @brief Re-enable ADXL345 INT1 interrupt after temporary disable
 */
void enableADXLInterrupts() {
  if (!ADXL345Enabled || !interruptsInitialized || interruptsEnabled) {
    adxlDebug("Skipping interrupt enable - sensor not ready or interrupts already enabled");
    return;
  }
  
  adxlDebug("Enabling ADXL345 interrupts...");
  vTaskDelay(pdMS_TO_TICKS(10));
  pinMode(INTERRUPT_PIN_D1, INPUT);
  clearInterrupts();
  writeRegister(ADXL345_REG_INT_ENABLE, 0x60);
  interruptsEnabled = true;
  adxlDebug("ADXL345 interrupts enabled");
}

/**
 * @brief Check if ADXL345 interrupts are currently enabled
 * @return true if interrupts are enabled, false otherwise
 */
bool areADXLInterruptsEnabled() {
  return (ADXL345Enabled && interruptsEnabled && interruptsInitialized);
}

/**
 * @brief Checks if the ADXL345 sensor is enabled
 * @return true if sensor is enabled, false otherwise
 */
bool isSensorEnabled() { 
  return ADXL345Enabled; 
}

/**
 * @brief Retrieves current sensor event data
 * @return sensors_event_t structure containing acceleration data
 */
sensors_event_t getSensorData() {
  adxl.getEvent(&event);
  return event;
}

/**
 * @brief Reads a value from a specified register on the ADXL345
 * @param reg Register address to read
 * @return Register value or 0 if sensor is disabled
 */
uint8_t readRegister(uint8_t reg) {
  if (!ADXL345Enabled) {
    ESP_LOGW(ADXL_LOG, "WARNING: Attempted to read register while sensor disabled");
    return 0;
  }
  return adxl.readRegister(reg);
}

/**
 * @brief Clears all pending interrupts by reading the interrupt source register
 */
void clearInterrupts() {
  if (!ADXL345Enabled)
    return;
  uint8_t interruptSource = adxl.readRegister(ADXL345_REG_INT_SOURCE);
  adxl.readRegister(ADXL345_REG_INT_SOURCE);
}

/**
 * @brief Calculates the combined magnitude of acceleration across all axes
 * @param accelX X-axis acceleration in m/s²
 * @param accelY Y-axis acceleration in m/s²
 * @param accelZ Z-axis acceleration in m/s²
 * @return Smoothed, gravity-compensated acceleration magnitude as integer
 */
int calculateCombinedMagnitude(float accelX, float accelY, float accelZ) {
  if (!ADXL345Enabled)
    return 0;
  static float smoothedMagnitude = 0;
  static const float SMOOTHING_FACTOR = 0.1;
  
  float rawAccelMagnitude = sqrt(sq(accelX) + sq(accelY) + sq(accelZ));
  float dynamicAccelMagnitude = abs(rawAccelMagnitude - SENSORS_GRAVITY_EARTH);
  smoothedMagnitude = (SMOOTHING_FACTOR * dynamicAccelMagnitude) + ((1 - SMOOTHING_FACTOR) * smoothedMagnitude);

  return (int)round(smoothedMagnitude);
}

/**
 * @brief Puts the ESP into deep sleep mode
 */
void enterDeepSleep() {
  if (!ADXL345Enabled) {
    adxlDebug("Skipping deep sleep - ADXL345 not enabled");
    return;
  }

  adxlDebug("Entering deep sleep mode...");
  clearInterrupts();
  delay(100);
  adxlDebug("Starting ESP32 deep sleep");
  esp_deep_sleep_start();
}

/**
 * @brief Initializes and configures the ADXL345 accelerometer
 * @return true if initialization successful, false otherwise
 */
bool initializeADXL345() {
  adxlDebug("Starting ADXL345 initialization...");
  
  if (!retrySensorInit()) {
    ESP_LOGE(ADXL_LOG, "ERROR: ADXL345 initialization failed");
    ADXL345Enabled = false;
    return false;
  }

  ADXL345Enabled = true;
  adxlDebug("ADXL345 sensor initialized successfully");
  
  adxlDebug("Configuring ADXL345 settings...");
  adxl.setRange(ADXL345_RANGE_16_G);
  adxl.setDataRate(ADXL345_DATARATE_100_HZ);
  adxl.writeRegister(ADXL345_REG_INT_ENABLE, 0x00);
  adxl.writeRegister(ADXL345_REG_THRESH_TAP, calcGforce(14.0));
  adxl.writeRegister(ADXL345_REG_DUR, calcDuration(30.0));
  adxl.writeRegister(ADXL345_REG_LATENT, calcLatency(100.0));
  adxl.writeRegister(ADXL345_REG_WINDOW, calcLatency(250.0));
  adxl.writeRegister(ADXL345_REG_TAP_AXES, 0x0F);
  adxl.writeRegister(ADXL345_REG_INT_MAP, 0x00);
  adxl.writeRegister(ADXL345_REG_INT_ENABLE, 0x60);
  adxl.writeRegister(ADXL345_REG_FIFO_CTL, 0x80 | 0x10);
  clearInterrupts();
  
  adxlDebug("ADXL345 configuration completed - Range: 16G, Rate: 100Hz, Tap threshold: 14G");
  configureESPDeepSleep();
  interruptsInitialized = true;
  interruptsEnabled = true;
  
  return true;
}

/**
 * @brief Gets the number of samples available in the FIFO buffer
 * @return Number of samples (0-32) or 0 if sensor is disabled
 */
uint8_t getFifoSampleData() {
  if (!ADXL345Enabled) {
    adxlDebug("FIFO read skipped - ADXL345 not enabled");
    return 0;
  }
  
  uint8_t fifoStatus = adxl.readRegister(ADXL345_REG_FIFO_STATUS);
  uint8_t samplesAvailable = fifoStatus & 0x3F;
  adxlDebug("FIFO status: %d samples available", samplesAvailable);
  return samplesAvailable;
}