/**
 * @file test_display.h
 * @brief Modular test suite for Display Module - header declarations
 */

#ifndef TEST_DISPLAY_H
#define TEST_DISPLAY_H

#include <unity.h>
#include <Arduino.h>
#include "display_module.h"
#include "test_common.h"

//==============================================================================
// TEST CONFIGURATION FLAGS
//==============================================================================

#define DISPLAY_RUN_INITIALIZATION_TESTS
#define DISPLAY_RUN_CONFIGURATION_TESTS
#define DISPLAY_RUN_CONSTANT_TESTS
#define DISPLAY_RUN_API_TESTS
#define DISPLAY_RUN_UTILITY_TESTS

// Disable risky tests by default
// #define DISPLAY_RUN_HARDWARE_TESTS      // Requires actual OLED display hardware
// #define DISPLAY_RUN_RENDERING_TESTS     // Requires actual display rendering
// #define DISPLAY_RUN_SPI_COMMUNICATION_TESTS  // Requires SPI bus and display
// #define DISPLAY_RUN_BRIGHTNESS_TESTS    // Requires actual brightness control

//==============================================================================
// PUBLIC TEST FUNCTIONS
//==============================================================================

// Initialization tests
void test_display_module_initialization(void);
void test_display_constants_validation(void);
void test_display_pin_configuration(void);

// Configuration tests
void test_display_pin_definitions(void);
void test_display_brightness_settings(void);
void test_display_dimensions(void);

// Constant tests
void test_display_color_definitions(void);
void test_display_timing_constants(void);
void test_display_boot_modes(void);

// API tests
void test_display_api_functions(void);
void test_display_window_operations(void);
void test_display_batch_operations(void);

// Utility tests
void test_display_utility_functions(void);
void test_display_error_handling(void);
void test_display_memory_management(void);

//==============================================================================
// TEST RUNNER FUNCTIONS
//==============================================================================

int run_display_initialization_tests(void);
int run_display_configuration_tests(void);
int run_display_constant_tests(void);
int run_display_api_tests(void);
int run_display_utility_tests(void);
int run_all_display_tests(void);

//==============================================================================
// UTILITY FUNCTIONS
//==============================================================================

void display_print_test_result(const char* testName, bool passed, const char* details = nullptr);

#endif /* TEST_DISPLAY_H */
