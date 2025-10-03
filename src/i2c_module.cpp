/**
 * @file i2c_module.cpp
 * @brief Implementation of centralized I2C bus management
 */

#include "i2c_module.h"
#include "common.h"

//==============================================================================
// GLOBAL VARIABLES
//==============================================================================

static bool i2cInitialized = false;
static const char *I2C_LOG = "::I2C_MANAGER::";

//==============================================================================
// PUBLIC API FUNCTIONS
//==============================================================================

/**
 * @brief Initialize the shared I2C bus
 * @return true if I2C initialization successful, false otherwise
 */
bool initializeI2C(void) {
    if (i2cInitialized) {
        return true;
    }
    
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    Wire.setClock(I2C_FREQUENCY);
    
    delay(10);
    
    i2cInitialized = true;
    
    uint8_t deviceCount = scanI2CDevices();
    
    return true;
}

/**
 * @brief Check if I2C bus is already initialized
 * @return true if I2C is ready, false otherwise
 */
bool isI2CReady(void) {
    return i2cInitialized;
}

/**
 * @brief Get the I2C bus instance
 * @return Pointer to the Wire instance
 */
TwoWire* getI2CBus(void) {
    return &Wire;
}

/**
 * @brief Scan I2C bus for connected devices
 * @return Number of devices found
 */
uint8_t scanI2CDevices(void) {
    if (!i2cInitialized) {
        ESP_LOGW(I2C_LOG, "Cannot scan - I2C not initialized");
        return 0;
    }
    
    uint8_t deviceCount = 0;
    
    for (uint8_t address = 1; address < 127; address++) {
        Wire.beginTransmission(address);
        uint8_t error = Wire.endTransmission();
        
        if (error == 0) {
            deviceCount++;
            
            switch (address) {
                case 0x53:
                    break;
                case 0x51:
                    break;
                case 0x5A:
                    break;
                default:
                    break;
            }
        }
    }
    
    if (deviceCount == 0) {
        ESP_LOGW(I2C_LOG, "No I2C devices found!");
    }
    
    return deviceCount;
}

/**
 * @brief Reset I2C bus in case of communication errors
 * @return true if reset successful, false otherwise
 */
bool resetI2CBus(void) {
    ESP_LOGW(I2C_LOG, "Resetting I2C bus...");
    
    Wire.end();
    
    delay(100);
    
    i2cInitialized = false;
    return initializeI2C();
}