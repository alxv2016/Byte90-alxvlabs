/**
 * @file test_adxl.h
 * @brief Modular test suite for ADXL Module - header declarations
 */

#ifndef TEST_ADXL_H
#define TEST_ADXL_H

#include <unity.h>
#include <Arduino.h>
#include "adxl_module.h"
#include "test_common.h"

//==============================================================================
// TEST CONFIGURATION FLAGS
//==============================================================================

#define ADXL_RUN_INITIALIZATION_TESTS
#define ADXL_RUN_STATE_MANAGEMENT_TESTS
#define ADXL_RUN_CONFIGURATION_TESTS
#define ADXL_RUN_SENSOR_OPERATION_TESTS
#define ADXL_RUN_UTILITY_TESTS

// Disable risky tests by default
// #define ADXL_RUN_HARDWARE_TESTS      // Requires actual ADXL345 sensor hardware
// #define ADXL_RUN_I2C_COMMUNICATION_TESTS  // Requires I2C bus and sensor device
// #define ADXL_RUN_CALIBRATION_TESTS   // Requires actual sensor calibration

//==============================================================================
// PUBLIC TEST FUNCTIONS
//==============================================================================

// Initialization tests
void test_adxl_module_initialization(void);
void test_adxl_constants_validation(void);
void test_adxl_module_configuration(void);

// State management tests
void test_adxl_state_management(void);
void test_adxl_state_transitions(void);
void test_adxl_error_state_handling(void);

// Configuration tests
void test_adxl_sensor_definitions(void);
void test_adxl_parameter_validation(void);
void test_adxl_timing_configuration(void);

// Sensor operation tests
void test_adxl_sensor_initialization(void);
void test_adxl_sensor_reading(void);
void test_adxl_sensor_cleanup(void);

// Utility tests
void test_adxl_utility_functions(void);
void test_adxl_error_handling(void);
void test_adxl_memory_management(void);

//==============================================================================
// TEST RUNNER FUNCTIONS
//==============================================================================

int run_adxl_initialization_tests(void);
int run_adxl_state_management_tests(void);
int run_adxl_configuration_tests(void);
int run_adxl_sensor_operation_tests(void);
int run_adxl_utility_tests(void);
int run_all_adxl_tests(void);

//==============================================================================
// UTILITY FUNCTIONS
//==============================================================================

void adxl_print_test_result(const char* testName, bool passed, const char* details = nullptr);

#endif /* TEST_ADXL_H */
