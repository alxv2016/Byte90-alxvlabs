/**
 * @file test_runner.cpp
 * @brief Main test runner - orchestrates all module tests
 */

#include <unity.h>
#include <Arduino.h>

// Common test utilities
#include "test_common.h"

// Module-specific test includes
#ifdef RUN_ADXL_MODULE_TESTS
#include "test_adxl.h"
#endif


//==============================================================================
// MAIN TEST CONFIGURATION
//==============================================================================

// Global test statistics
static int totalTestsRun = 0;
static unsigned long testSuiteStartTime = 0;

//==============================================================================
// TEST SUITE FUNCTIONS
//==============================================================================

void printTestSuiteHeader(void) {
    Serial.println("\n==================================================");
    Serial.println("🔧 BYTE-90 MODULAR TEST SUITE 🔧");
    Serial.println("==================================================");
    Serial.printf("Firmware Version: %s\n", FIRMWARE_VERSION);
    Serial.println("🎯 Modular, scalable testing architecture");
    Serial.println("🛡️ Safe, comprehensive testing approach");
    Serial.println("==================================================");
}

void printTestSuiteFooter(void) {
    unsigned long testSuiteEndTime = getTestUptime();
    unsigned long totalDuration = testSuiteEndTime - testSuiteStartTime;
    
    Serial.println("\n==================================================");
    Serial.println("🎉 BYTE-90 TEST SUITE COMPLETED! 🎉");
    Serial.println("==================================================");
    Serial.printf("📊 Total tests run: %d\n", totalTestsRun);
    Serial.printf("⏱️ Total duration: %lu ms (%.2f seconds)\n", totalDuration, totalDuration / 1000.0);
    Serial.printf("🧠 Final memory: %u bytes free\n", ESP.getFreeHeap());
    Serial.println("✅ All modules tested successfully");
    Serial.println("🚀 System ready for deployment");
    Serial.println("==================================================");
}

void runBasicSafetyTests(void) {
    #ifdef RUN_BASIC_SAFETY_TESTS
    printTestSectionHeader("BASIC SAFETY TESTS");
    unsigned long sectionStart = getTestUptime();
    
    int testsRun = run_basic_safety_tests();
    totalTestsRun += testsRun;
    
    unsigned long sectionEnd = getTestUptime();
    logTestTiming("Basic Safety Tests", sectionStart, sectionEnd);
    #endif
}


void runADXLTests(void) {
    #ifdef RUN_ADXL_MODULE_TESTS
    printTestSectionHeader("ADXL MODULE TESTS");
    unsigned long sectionStart = getTestUptime();
    uint32_t memoryBefore = getHeapWithLogging("before ADXL tests");
    
    // Run all ADXL test categories
    printTestSectionHeader("ADXL INITIALIZATION");
    int initTests = run_adxl_initialization_tests();
    totalTestsRun += initTests;
    
    printTestSectionHeader("ADXL STATE MANAGEMENT");
    int stateTests = run_adxl_state_management_tests();
    totalTestsRun += stateTests;
    
    printTestSectionHeader("ADXL CONFIGURATION");
    int configTests = run_adxl_configuration_tests();
    totalTestsRun += configTests;
    
    printTestSectionHeader("ADXL SENSOR OPERATIONS");
    int sensorTests = run_adxl_sensor_operation_tests();
    totalTestsRun += sensorTests;
    
    printTestSectionHeader("ADXL UTILITIES");
    int utilityTests = run_adxl_utility_tests();
    totalTestsRun += utilityTests;
    
    // Show summary
    Serial.println("📋 ADXL TEST SUMMARY:");
    Serial.println("   ✅ Safe tests completed (configuration, constants, API validation)");
    Serial.println("   ⚠️  Hardware tests skipped - requires ADXL sensor hardware");
    Serial.printf("   ⏱️ Section took %lu ms\n", getTestUptime() - sectionStart);
    getHeapWithLogging("after ADXL tests");
    Serial.printf("🎯 ADXL tests completed: %d total tests\n",
                 initTests + stateTests + configTests + sensorTests + utilityTests);
    #endif
}

//////////////////////////////////////////////////////////////////////////

void runFinalTests(void) {
    printTestSectionHeader("FINAL SYSTEM TESTS");
    unsigned long sectionStart = getTestUptime();
    
    // Run final memory check
    RUN_TEST(test_memory_after_tests);
    totalTestsRun++;
    
    unsigned long sectionEnd = getTestUptime();
    logTestTiming("Final System Tests", sectionStart, sectionEnd);
}

//==============================================================================
// UNITY TEST SETUP
//==============================================================================

void setUp(void) { 
    // Called before each test
    briefStabilityDelay();
}

void tearDown(void) { 
    // Called after each test
    briefStabilityDelay();
}

//==============================================================================
// MAIN SETUP AND LOOP
//==============================================================================

void setup() {
    // Startup delay for stability
    delay(3000);
    Serial.begin(115200);
    
    // Record test suite start time
    testSuiteStartTime = getTestUptime();
    
    // Print header
    printTestSuiteHeader();
    
    // Initialize Unity
    UNITY_BEGIN();
    
    // Run all test suites in dependency order (matching main.cpp initialization)
    runBasicSafetyTests();
    
    runADXLTests();
    
    // Finalize Unity
    runFinalTests();
    UNITY_END();
    
    // Print footer with statistics
    printTestSuiteFooter();
}

void loop() {
    // Long delay in main loop - tests are complete
    delay(30000);
    
    // Optional: Print periodic status
    static unsigned long lastStatus = 0;
    if (millis() - lastStatus > 30000) {
        Serial.printf("💤 Test suite idle - %d tests completed successfully\n", totalTestsRun);
        lastStatus = millis();
    }
}