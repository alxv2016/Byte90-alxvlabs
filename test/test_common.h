/**
 * @file test_common.h
 * @brief Common utilities and helpers for all test modules
 */

#ifndef TEST_COMMON_H
#define TEST_COMMON_H

#include <unity.h>
#include <Arduino.h>

//==============================================================================
// COMMON TEST CONFIGURATION
//==============================================================================

// Enable/disable entire test suites
#define RUN_BASIC_SAFETY_TESTS
#define RUN_PREFERENCES_TESTS
#define RUN_SYSTEM_MODULE_TESTS
#define RUN_WIFI_MODULE_TESTS
#define RUN_SPEAKER_MODULE_TESTS
#define RUN_SOUNDSFX_MODULE_TESTS
#define RUN_SERIAL_MODULE_TESTS
#define RUN_OTA_MODULE_TESTS
#define RUN_MOTION_MODULE_TESTS
#define RUN_MENU_MODULE_TESTS
#define RUN_I2C_MODULE_TESTS
#define RUN_HAPTICS_MODULE_TESTS
#define RUN_COMMON_MODULE_TESTS
#define RUN_DISPLAY_MODULE_TESTS
#define RUN_CLOCK_MODULE_TESTS
#define RUN_EFFECTS_CORE_MODULE_TESTS
#define RUN_ADXL_MODULE_TESTS
#define RUN_GIF_MODULE_TESTS
#define RUN_FLASH_MODULE_TESTS
#define RUN_ESPNOW_MODULE_TESTS
#define RUN_ANIMATION_MODULE_TESTS
#define RUN_INTEGRATION_TESTS

//==============================================================================
// COMMON UTILITY FUNCTIONS
//==============================================================================

/**
 * @brief Print a formatted test section header
 * @param sectionName Name of the test section
 */
void printTestSectionHeader(const char* sectionName);

/**
 * @brief Print test result with consistent formatting
 * @param testName Name of the test
 * @param passed Whether test passed
 * @param details Additional details (optional)
 */
void printTestResult(const char* testName, bool passed, const char* details = nullptr);

/**
 * @brief Test ESP32 hardware basics (memory, CPU, etc.)
 */
void test_esp32_hardware(void);

/**
 * @brief Test memory before main tests
 */
void test_memory_before_tests(void);

/**
 * @brief Test memory after main tests
 */
void test_memory_after_tests(void);

/**
 * @brief Run all basic safety tests
 * @return Number of tests run
 */
int run_basic_safety_tests(void);

//==============================================================================
// MEMORY TRACKING UTILITIES
//==============================================================================

/**
 * @brief Get current free heap with logging
 * @param context Description of when this is called
 * @return Free heap in bytes
 */
uint32_t getHeapWithLogging(const char* context);

/**
 * @brief Check for memory leaks between two measurements
 * @param before Previous heap measurement
 * @param after Current heap measurement
 * @param context Description for logging
 * @param maxLeak Maximum acceptable leak in bytes (default: 1000)
 * @return true if within acceptable limits
 */
bool checkMemoryLeak(uint32_t before, uint32_t after, const char* context, uint32_t maxLeak = 1000);

//==============================================================================
// TEST TIMING UTILITIES
//==============================================================================

/**
 * @brief Get current uptime in milliseconds
 * @return Uptime in ms
 */
unsigned long getTestUptime(void);

/**
 * @brief Log test timing information
 * @param testName Name of the test
 * @param startTime Start time in ms
 * @param endTime End time in ms
 */
void logTestTiming(const char* testName, unsigned long startTime, unsigned long endTime);

//==============================================================================
// SAFE DELAY UTILITIES
//==============================================================================

/**
 * @brief Safe delay between tests
 * @param ms Milliseconds to delay
 */
void safeTestDelay(unsigned long ms);

/**
 * @brief Brief delay for system stability
 */
void briefStabilityDelay(void);

/**
 * @brief Longer delay for NVS operations
 */
void nvsOperationDelay(void);

#endif /* TEST_COMMON_H */