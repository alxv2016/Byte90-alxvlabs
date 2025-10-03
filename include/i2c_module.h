/**
 * @file i2c_module.h
 * @brief Centralized I2C bus management for BYTE-90
 *
 * Manages the shared I2C bus used by multiple modules to prevent conflicts
 * and duplicate initialization.
 */

#ifndef I2C_MODULE_H
#define I2C_MODULE_H

#include <Arduino.h>
#include <Wire.h>

//==============================================================================
// CONSTANTS & DEFINITIONS
//==============================================================================

#define I2C_SDA_PIN             D4
#define I2C_SCL_PIN             D5
#define I2C_FREQUENCY           400000

//==============================================================================
// PUBLIC API FUNCTIONS
//==============================================================================

/**
 * @brief Initialize the shared I2C bus
 * @return true if I2C initialization successful, false otherwise
 */
bool initializeI2C(void);

/**
 * @brief Check if I2C bus is already initialized
 * @return true if I2C is ready, false otherwise
 */
bool isI2CReady(void);

/**
 * @brief Get the I2C bus instance
 * @return Pointer to the Wire instance
 */
TwoWire* getI2CBus(void);

/**
 * @brief Scan I2C bus for connected devices
 * @return Number of devices found
 */
uint8_t scanI2CDevices(void);

/**
 * @brief Reset I2C bus in case of communication errors
 * @return true if reset successful, false otherwise
 */
bool resetI2CBus(void);

#endif /* I2C_MODULE_H */