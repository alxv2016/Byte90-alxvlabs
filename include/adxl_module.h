/**
 * @file adxl_module.h
 * @brief Header for ADXL345 accelerometer module
 *
 * Provides functionality for ADXL345 accelerometer initialization, configuration,
 * data reading, and interrupt-based deep sleep functionality.
 */

#ifndef ADXL_MODULE_H
#define ADXL_MODULE_H

#include "common.h"
#include <Adafruit_ADXL345_U.h>
#include <Adafruit_Sensor.h>

//==============================================================================
// CONSTANTS & DEFINITIONS
//==============================================================================

static const char *ADXL_LOG = "::ADXL_MODULE::";

#define INT_PIN_BITMASK (1ULL << GPIO_NUM_1)
#define INTERRUPT_PIN_D1 D1
#define SDA_PIN_D4 D4
#define SCL_PIN_D5 D5

#define FORCE_SCALE_FACTOR 62.5
#define DURATION_SCALE_FACTOR 0.625
#define LATENCY_SCALE_FACTOR 1.25

#define ADXL345_INT_SOURCE_OVERRUN 0x01
#define ADXL345_INT_SOURCE_WATERMARK 0x02
#define ADXL345_INT_SOURCE_FREEFALL 0x04
#define ADXL345_INT_SOURCE_INACTIVITY 0x08
#define ADXL345_INT_SOURCE_ACTIVITY 0x10
#define ADXL345_INT_SOURCE_DOUBLETAP 0x20
#define ADXL345_INT_SOURCE_SINGLETAP 0x40
#define ADXL345_INT_SOURCE_DATAREADY 0x80
#define ADXL345_FIFO_BYPASS_MODE 0x00

#define ADXL345_TAP_SOURCE_X 0x04
#define ADXL345_TAP_SOURCE_Y 0x02
#define ADXL345_TAP_SOURCE_Z 0x01

//==============================================================================
// PUBLIC API FUNCTIONS
//==============================================================================

/**
 * @brief Initializes and configures the ADXL345 accelerometer
 * @return true if initialization successful, false otherwise
 */
bool initializeADXL345();

/**
 * @brief Clears all pending interrupts by reading the interrupt source register
 */
void clearInterrupts();

/**
 * @brief Puts the ESP into deep sleep mode
 */
void enterDeepSleep();

/**
 * @brief Calculates the combined magnitude of acceleration across all axes
 * @param accelX X-axis acceleration in m/s²
 * @param accelY Y-axis acceleration in m/s²
 * @param accelZ Z-axis acceleration in m/s²
 * @return Smoothed, gravity-compensated acceleration magnitude as integer
 */
int calculateCombinedMagnitude(float accelX, float accelY, float accelZ);

/**
 * @brief Gets the number of samples available in the FIFO buffer
 * @return Number of samples (0-32) or 0 if sensor is disabled
 */
uint8_t getFifoSampleData();

/**
 * @brief Retrieves current sensor event data
 * @return sensors_event_t structure containing acceleration data
 */
sensors_event_t getSensorData();

/**
 * @brief Checks if the ADXL345 sensor is enabled
 * @return true if sensor is enabled, false otherwise
 */
bool isSensorEnabled();

/**
 * @brief Reads a value from a specified register on the ADXL345
 * @param reg Register address to read
 * @return Register value or 0 if sensor is disabled
 */
uint8_t readRegister(uint8_t reg);

/**
 * @brief Temporarily disable ADXL345 INT1 interrupt
 */
void disableADXLInterrupts(void);

/**
 * @brief Re-enable ADXL345 INT1 interrupt after temporary disable
 */
void enableADXLInterrupts(void);

/**
 * @brief Check if ADXL345 interrupts are currently enabled
 * @return true if interrupts are enabled, false otherwise
 */
bool areADXLInterruptsEnabled(void);

/**
 * @brief Enable or disable debug logging for ADXL module operations
 * @param enabled true to enable debug logging, false to disable
 */
void setAdxlDebug(bool enabled);

#endif /* ADXL_MODULE_H */