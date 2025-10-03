/**
 * @file test_common.cpp
 * @brief Common utilities and helpers for all test modules - implementation
 */

#include "test_common.h"

//==============================================================================
// COMMON UTILITY FUNCTIONS
//==============================================================================

void printTestSectionHeader(const char* sectionName) {
    Serial.println();
    Serial.println("==========================================");
    Serial.printf("TEST SECTION: %s\n", sectionName);
    Serial.println("==========================================");
}

void printTestResult(const char* testName, bool passed, const char* details) {
    Serial.printf("%s %s", passed ? "✅" : "❌", testName);
    if (details) {
        Serial.printf(" - %s", details);
    }
    Serial.println();
}

//==============================================================================
// BASIC SAFETY TESTS
//==============================================================================

void test_esp32_hardware(void) {
    Serial.println("Testing ESP32 hardware (SAFE)");
    
    String chipModel = ESP.getChipModel();
    TEST_ASSERT_TRUE_MESSAGE(chipModel.length() > 0, "Chip model should not be empty");
    
    uint32_t cpuFreq = ESP.getCpuFreqMHz();
    TEST_ASSERT_GREATER_THAN_MESSAGE(0, cpuFreq, "CPU frequency should be > 0");
    
    uint32_t freeHeap = ESP.getFreeHeap();
    TEST_ASSERT_GREATER_THAN_MESSAGE(10000, freeHeap, "Should have reasonable free heap");
    
    printTestResult("ESP32 Hardware", true, 
        (chipModel + ", " + String(cpuFreq) + "MHz, " + String(freeHeap) + " bytes heap").c_str());
}

void test_memory_before_tests(void) {
    uint32_t freeHeap = ESP.getFreeHeap();
    TEST_ASSERT_GREATER_THAN_MESSAGE(10000, freeHeap, "Should have sufficient memory before testing");
    
    printTestResult("Memory Before Tests", true, (String(freeHeap) + " bytes free").c_str());
}

void test_memory_after_tests(void) {
    uint32_t freeHeap = ESP.getFreeHeap();
    TEST_ASSERT_GREATER_THAN_MESSAGE(5000, freeHeap, "Should still have reasonable memory after testing");
    
    printTestResult("Memory After All Tests", true, (String(freeHeap) + " bytes free").c_str());
}

//==============================================================================
// MEMORY TRACKING UTILITIES
//==============================================================================

uint32_t getHeapWithLogging(const char* context) {
    uint32_t heap = ESP.getFreeHeap();
    Serial.printf("🧠 Memory %s: %u bytes\n", context, heap);
    return heap;
}

bool checkMemoryLeak(uint32_t before, uint32_t after, const char* context, uint32_t maxLeak) {
    if (before > after) {
        uint32_t leak = before - after;
        Serial.printf("⚠️  Memory usage in %s: %u bytes", context, leak);
        if (leak > maxLeak) {
            Serial.printf(" (EXCESSIVE - limit: %u)", maxLeak);
            Serial.println();
            return false;
        } else {
            Serial.println(" (acceptable)");
        }
    } else {
        uint32_t gain = after - before;
        Serial.printf("📈 Memory recovered in %s: %u bytes\n", context, gain);
    }
    return true;
}

//==============================================================================
// TEST TIMING UTILITIES
//==============================================================================

unsigned long getTestUptime(void) {
    return millis();
}

void logTestTiming(const char* testName, unsigned long startTime, unsigned long endTime) {
    unsigned long duration = endTime - startTime;
    Serial.printf("⏱️  %s completed in %lu ms\n", testName, duration);
}

//==============================================================================
// SAFE DELAY UTILITIES
//==============================================================================

void safeTestDelay(unsigned long ms) {
    delay(ms);
}

void briefStabilityDelay(void) {
    delay(50);  // 50ms for general stability
}

void nvsOperationDelay(void) {
    delay(200); // 200ms for NVS operations to complete
}

//==============================================================================
// BASIC SAFETY TEST RUNNER
//==============================================================================

int run_basic_safety_tests(void) {
    int testCount = 0;
    
    #ifdef RUN_BASIC_SAFETY_TESTS
    RUN_TEST(test_esp32_hardware);
    testCount++;
    RUN_TEST(test_memory_before_tests);
    testCount++;
    #endif
    
    return testCount;
}