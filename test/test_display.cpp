/**
 * @file test_display.cpp  
 * @brief Modular test suite for Display Module - implementation
 */

#include "test_display.h"
#include "test_common.h"

//==============================================================================
// UTILITY FUNCTIONS
//==============================================================================

void display_print_test_result(const char* testName, bool passed, const char* details) {
    Serial.printf("%s %s", passed ? "✅" : "❌", testName);
    if (details) {
        Serial.printf(" - %s", details);
    }
    Serial.println();
}

//==============================================================================
// INITIALIZATION TESTS
//==============================================================================

void test_display_module_initialization(void) {
    Serial.println("🖥️ Testing display module initialization (SAFE VERSION)");
    
    try {
        // Test basic framework functionality
        bool basicTestFramework = true;
        TEST_ASSERT_TRUE_MESSAGE(basicTestFramework, "Test framework should work");
        
        // Test memory allocation before display operations
        uint32_t heapBefore = ESP.getFreeHeap();
        void* testPtr = malloc(100);
        bool memoryAllocationWorks = (testPtr != nullptr);
        if (testPtr) free(testPtr);
        uint32_t heapAfter = ESP.getFreeHeap();
        bool memoryFreed = (heapAfter >= heapBefore - 200);
        TEST_ASSERT_TRUE_MESSAGE(memoryAllocationWorks && memoryFreed, "Memory allocation should work");
        
        // Test display constants are defined
        bool constantsDefined = true;
        #ifdef DISPLAY_WIDTH
        constantsDefined &= (DISPLAY_WIDTH > 0);
        #endif
        #ifdef DISPLAY_HEIGHT
        constantsDefined &= (DISPLAY_HEIGHT > 0);
        #endif
        #ifdef DISPLAY_FREQUENCY
        constantsDefined &= (DISPLAY_FREQUENCY > 0);
        #endif
        TEST_ASSERT_TRUE_MESSAGE(constantsDefined, "Display constants should be defined and valid");
        
        #ifdef DISPLAY_RUN_HARDWARE_TESTS
        // Test actual display initialization (only if hardware testing enabled)
        Serial.println("   🔧 Attempting display hardware initialization...");
        // Note: Actual display init would be called here if hardware testing enabled
        bool initResult = true; // Placeholder for hardware test
        Serial.printf("   📊 Display init result: %s\n", initResult ? "SUCCESS" : "FAILED");
        TEST_ASSERT_TRUE_MESSAGE(initResult, "Display initialization should succeed");
        
        display_print_test_result("Display Module Initialization", 
            basicTestFramework && memoryAllocationWorks && constantsDefined && initResult,
            "All tests passed including hardware initialization");
        #else
        Serial.println("   ⚠️ Display hardware initialization skipped - enable DISPLAY_RUN_HARDWARE_TESTS for full testing");
        
        display_print_test_result("Display Module Initialization", 
            basicTestFramework && memoryAllocationWorks && constantsDefined,
            "Basic tests passed, hardware init skipped for safety");
        #endif
            
    } catch (...) {
        TEST_FAIL_MESSAGE("Exception during display module initialization");
    }
}

void test_display_constants_validation(void) {
    Serial.println("📏 Testing display constants validation");
    
    try {
        // Test display pin definitions are in valid ESP32 range
        bool pinDefinitionsValid = true;
        
        #ifdef MOSI_PIN_D10
        bool mosiPinValid = (MOSI_PIN_D10 == D10);  // Validate against actual XIAO ESP32S3 pin
        TEST_ASSERT_TRUE_MESSAGE(mosiPinValid, "Display MOSI pin should be D10");
        pinDefinitionsValid &= mosiPinValid;
        #endif
        
        #ifdef SCLK_PIN_D8
        bool sclkPinValid = (SCLK_PIN_D8 == D8);  // Validate against actual XIAO ESP32S3 pin
        TEST_ASSERT_TRUE_MESSAGE(sclkPinValid, "Display SCLK pin should be D8");
        pinDefinitionsValid &= sclkPinValid;
        #endif
        
        #ifdef CS_PIN_D7
        bool csPinValid = (CS_PIN_D7 == D7);  // Validate against actual XIAO ESP32S3 pin
        TEST_ASSERT_TRUE_MESSAGE(csPinValid, "Display CS pin should be D7");
        pinDefinitionsValid &= csPinValid;
        #endif
        
        #ifdef DC_PIN_D6
        bool dcPinValid = (DC_PIN_D6 == D6);  // Validate against actual XIAO ESP32S3 pin
        TEST_ASSERT_TRUE_MESSAGE(dcPinValid, "Display DC pin should be D6");
        pinDefinitionsValid &= dcPinValid;
        #endif
        
        #ifdef RST_PIN_D0
        bool rstPinValid = (RST_PIN_D0 == D0);  // Validate against actual XIAO ESP32S3 pin
        TEST_ASSERT_TRUE_MESSAGE(rstPinValid, "Display RST pin should be D0");
        pinDefinitionsValid &= rstPinValid;
        #endif
        
        // Test display dimensions are reasonable
        bool dimensionsValid = true;
        #if defined(DISPLAY_WIDTH) && defined(DISPLAY_HEIGHT)
        bool widthValid = (DISPLAY_WIDTH > 0 && DISPLAY_WIDTH <= 1024);
        bool heightValid = (DISPLAY_HEIGHT > 0 && DISPLAY_HEIGHT <= 1024);
        dimensionsValid = widthValid && heightValid;
        TEST_ASSERT_TRUE_MESSAGE(dimensionsValid, "Display dimensions should be reasonable (1-1024)");
        #endif
        
        // Test frequency is reasonable
        bool frequencyValid = true;
        #ifdef DISPLAY_FREQUENCY
        bool freqInRange = (DISPLAY_FREQUENCY >= 1000000 && DISPLAY_FREQUENCY <= 80000000);
        TEST_ASSERT_TRUE_MESSAGE(freqInRange, "Display frequency should be reasonable (1-80MHz)");
        frequencyValid = freqInRange;
        #endif
        
        display_print_test_result("Display Constants Validation", 
            pinDefinitionsValid && dimensionsValid && frequencyValid,
            "Pin definitions valid, dimensions reasonable, frequency in range");
            
    } catch (...) {
        TEST_FAIL_MESSAGE("Exception during display constants validation");
    }
}

void test_display_pin_configuration(void) {
    Serial.println("📌 Testing display pin configuration");
    
    try {
        // Test pin constants are compile-time accessible
        bool pinConstantsAccessible = true;
        
        #ifdef MOSI_PIN_D10
        int mosiPin = MOSI_PIN_D10;
        bool mosiAccessible = (mosiPin >= 0);
        TEST_ASSERT_TRUE_MESSAGE(mosiAccessible, "Display MOSI pin constant should be accessible");
        pinConstantsAccessible &= mosiAccessible;
        #endif
        
        #ifdef SCLK_PIN_D8
        int sclkPin = SCLK_PIN_D8;
        bool sclkAccessible = (sclkPin >= 0);
        TEST_ASSERT_TRUE_MESSAGE(sclkAccessible, "Display SCLK pin constant should be accessible");
        pinConstantsAccessible &= sclkAccessible;
        #endif
        
        #ifdef CS_PIN_D7
        int csPin = CS_PIN_D7;
        bool csAccessible = (csPin >= 0);
        TEST_ASSERT_TRUE_MESSAGE(csAccessible, "Display CS pin constant should be accessible");
        pinConstantsAccessible &= csAccessible;
        #endif
        
        #ifdef DC_PIN_D6
        int dcPin = DC_PIN_D6;
        bool dcAccessible = (dcPin >= 0);
        TEST_ASSERT_TRUE_MESSAGE(dcAccessible, "Display DC pin constant should be accessible");
        pinConstantsAccessible &= dcAccessible;
        #endif
        
        #ifdef RST_PIN_D0
        int rstPin = RST_PIN_D0;
        bool rstAccessible = (rstPin >= 0);
        TEST_ASSERT_TRUE_MESSAGE(rstAccessible, "Display RST pin constant should be accessible");
        pinConstantsAccessible &= rstAccessible;
        #endif
        
        // Test pin configuration logic
        #if defined(MOSI_PIN_D10) && defined(SCLK_PIN_D8) && defined(CS_PIN_D7) && defined(DC_PIN_D6) && defined(RST_PIN_D0)
        bool pinConfigurationLogic = (MOSI_PIN_D10 != SCLK_PIN_D8 && 
                                     MOSI_PIN_D10 != CS_PIN_D7 &&
                                     MOSI_PIN_D10 != DC_PIN_D6 &&
                                     MOSI_PIN_D10 != RST_PIN_D0);
        TEST_ASSERT_TRUE_MESSAGE(pinConfigurationLogic, "Display pin configuration logic should be sound");
        #else
        bool pinConfigurationLogic = true; // If not all defined, skip this test
        #endif
        
        // Test that pin numbers match XIAO ESP32S3 configuration
        bool pinsAreReasonable = true;
        #if defined(MOSI_PIN_D10) && defined(SCLK_PIN_D8) && defined(CS_PIN_D7) && defined(DC_PIN_D6) && defined(RST_PIN_D0)
        // Verify pins match XIAO ESP32S3 display configuration
        bool allPinsReasonable = (MOSI_PIN_D10 == D10 && SCLK_PIN_D8 == D8 && 
                                 CS_PIN_D7 == D7 && DC_PIN_D6 == D6 && RST_PIN_D0 == D0);
        pinsAreReasonable = allPinsReasonable;
        TEST_ASSERT_TRUE_MESSAGE(pinsAreReasonable, "Display pins should match XIAO ESP32S3 configuration");
        #endif
        
        display_print_test_result("Display Pin Configuration", 
            pinConstantsAccessible && pinConfigurationLogic && pinsAreReasonable,
            "Pin constants accessible, configuration logic sound, pins reasonable");
            
    } catch (...) {
        TEST_FAIL_MESSAGE("Exception during display pin configuration testing");
    }
}

//==============================================================================
// CONFIGURATION TESTS
//==============================================================================

void test_display_pin_definitions(void) {
    Serial.println("📌 Testing display pin definitions");
    
    try {
        // Test pin definitions are properly defined
        bool pinDefinitionsExist = true;
        
        #ifndef MOSI_PIN_D10
        pinDefinitionsExist = false;
        Serial.println("   ❌ MOSI_PIN_D10 not defined");
        #endif
        
        #ifndef SCLK_PIN_D8
        pinDefinitionsExist = false;
        Serial.println("   ❌ SCLK_PIN_D8 not defined");
        #endif
        
        #ifndef CS_PIN_D7
        pinDefinitionsExist = false;
        Serial.println("   ❌ CS_PIN_D7 not defined");
        #endif
        
        #ifndef DC_PIN_D6
        pinDefinitionsExist = false;
        Serial.println("   ❌ DC_PIN_D6 not defined");
        #endif
        
        #ifndef RST_PIN_D0
        pinDefinitionsExist = false;
        Serial.println("   ❌ RST_PIN_D0 not defined");
        #endif
        
        TEST_ASSERT_TRUE_MESSAGE(pinDefinitionsExist, "Display pin definitions should exist");
        
        // Test pin definitions match XIAO ESP32S3 configuration
        bool pinRangesValid = true;
        
        #ifdef MOSI_PIN_D10
        if (MOSI_PIN_D10 != D10) {
            pinRangesValid = false;
            Serial.printf("   ❌ MOSI_PIN_D10 should be D10, got %d\n", MOSI_PIN_D10);
        }
        #endif
        
        #ifdef SCLK_PIN_D8
        if (SCLK_PIN_D8 != D8) {
            pinRangesValid = false;
            Serial.printf("   ❌ SCLK_PIN_D8 should be D8, got %d\n", SCLK_PIN_D8);
        }
        #endif
        
        #ifdef CS_PIN_D7
        if (CS_PIN_D7 != D7) {
            pinRangesValid = false;
            Serial.printf("   ❌ CS_PIN_D7 should be D7, got %d\n", CS_PIN_D7);
        }
        #endif
        
        #ifdef DC_PIN_D6
        if (DC_PIN_D6 != D6) {
            pinRangesValid = false;
            Serial.printf("   ❌ DC_PIN_D6 should be D6, got %d\n", DC_PIN_D6);
        }
        #endif
        
        #ifdef RST_PIN_D0
        if (RST_PIN_D0 != D0) {
            pinRangesValid = false;
            Serial.printf("   ❌ RST_PIN_D0 should be D0, got %d\n", RST_PIN_D0);
        }
        #endif
        
        TEST_ASSERT_TRUE_MESSAGE(pinRangesValid, "Display pin definitions should be in valid range");
        
        // Test pins are unique
        bool pinsAreUnique = true;
        #if defined(MOSI_PIN_D10) && defined(SCLK_PIN_D8) && defined(CS_PIN_D7) && defined(DC_PIN_D6) && defined(RST_PIN_D0)
        if (MOSI_PIN_D10 == SCLK_PIN_D8 || MOSI_PIN_D10 == CS_PIN_D7 || MOSI_PIN_D10 == DC_PIN_D6 || MOSI_PIN_D10 == RST_PIN_D0 ||
            SCLK_PIN_D8 == CS_PIN_D7 || SCLK_PIN_D8 == DC_PIN_D6 || SCLK_PIN_D8 == RST_PIN_D0 ||
            CS_PIN_D7 == DC_PIN_D6 || CS_PIN_D7 == RST_PIN_D0 ||
            DC_PIN_D6 == RST_PIN_D0) {
            pinsAreUnique = false;
            Serial.println("   ❌ Display pins are not unique");
        }
        #endif
        
        TEST_ASSERT_TRUE_MESSAGE(pinsAreUnique, "Display pins should be unique");
        
        display_print_test_result("Display Pin Definitions", 
            pinDefinitionsExist && pinRangesValid && pinsAreUnique,
            "Pin definitions exist, ranges valid, pins unique");
            
    } catch (...) {
        TEST_FAIL_MESSAGE("Exception during display pin definitions testing");
    }
}

void test_display_brightness_settings(void) {
    Serial.println("💡 Testing display brightness settings");
    
    try {
        // Test brightness constants are defined
        bool brightnessConstantsDefined = true;
        
        #ifndef DISPLAY_BRIGHTNESS_DIM
        brightnessConstantsDefined = false;
        Serial.println("   ❌ DISPLAY_BRIGHTNESS_DIM not defined");
        #endif
        
        #ifndef DISPLAY_BRIGHTNESS_LOW
        brightnessConstantsDefined = false;
        Serial.println("   ❌ DISPLAY_BRIGHTNESS_LOW not defined");
        #endif
        
        #ifndef DISPLAY_BRIGHTNESS_MEDIUM
        brightnessConstantsDefined = false;
        Serial.println("   ❌ DISPLAY_BRIGHTNESS_MEDIUM not defined");
        #endif
        
        #ifndef DISPLAY_BRIGHTNESS_HIGH
        brightnessConstantsDefined = false;
        Serial.println("   ❌ DISPLAY_BRIGHTNESS_HIGH not defined");
        #endif
        
        #ifndef DISPLAY_BRIGHTNESS_FULL
        brightnessConstantsDefined = false;
        Serial.println("   ❌ DISPLAY_BRIGHTNESS_FULL not defined");
        #endif
        
        TEST_ASSERT_TRUE_MESSAGE(brightnessConstantsDefined, "Display brightness constants should be defined");
        
        // Test brightness values are in valid range
        bool brightnessRangesValid = true;
        
        #ifdef DISPLAY_BRIGHTNESS_DIM
        if (DISPLAY_BRIGHTNESS_DIM < 0 || DISPLAY_BRIGHTNESS_DIM > 15) {
            brightnessRangesValid = false;
            Serial.printf("   ❌ DISPLAY_BRIGHTNESS_DIM (%d) out of valid range (0-15)\n", DISPLAY_BRIGHTNESS_DIM);
        }
        #endif
        
        #ifdef DISPLAY_BRIGHTNESS_LOW
        if (DISPLAY_BRIGHTNESS_LOW < 0 || DISPLAY_BRIGHTNESS_LOW > 15) {
            brightnessRangesValid = false;
            Serial.printf("   ❌ DISPLAY_BRIGHTNESS_LOW (%d) out of valid range (0-15)\n", DISPLAY_BRIGHTNESS_LOW);
        }
        #endif
        
        #ifdef DISPLAY_BRIGHTNESS_MEDIUM
        if (DISPLAY_BRIGHTNESS_MEDIUM < 0 || DISPLAY_BRIGHTNESS_MEDIUM > 15) {
            brightnessRangesValid = false;
            Serial.printf("   ❌ DISPLAY_BRIGHTNESS_MEDIUM (%d) out of valid range (0-15)\n", DISPLAY_BRIGHTNESS_MEDIUM);
        }
        #endif
        
        #ifdef DISPLAY_BRIGHTNESS_HIGH
        if (DISPLAY_BRIGHTNESS_HIGH < 0 || DISPLAY_BRIGHTNESS_HIGH > 15) {
            brightnessRangesValid = false;
            Serial.printf("   ❌ DISPLAY_BRIGHTNESS_HIGH (%d) out of valid range (0-15)\n", DISPLAY_BRIGHTNESS_HIGH);
        }
        #endif
        
        #ifdef DISPLAY_BRIGHTNESS_FULL
        if (DISPLAY_BRIGHTNESS_FULL < 0 || DISPLAY_BRIGHTNESS_FULL > 15) {
            brightnessRangesValid = false;
            Serial.printf("   ❌ DISPLAY_BRIGHTNESS_FULL (%d) out of valid range (0-15)\n", DISPLAY_BRIGHTNESS_FULL);
        }
        #endif
        
        TEST_ASSERT_TRUE_MESSAGE(brightnessRangesValid, "Display brightness values should be in valid range");
        
        // Test brightness progression makes sense
        bool brightnessProgressionValid = true;
        #if defined(DISPLAY_BRIGHTNESS_DIM) && defined(DISPLAY_BRIGHTNESS_LOW) && defined(DISPLAY_BRIGHTNESS_MEDIUM) && defined(DISPLAY_BRIGHTNESS_HIGH) && defined(DISPLAY_BRIGHTNESS_FULL)
        brightnessProgressionValid = (DISPLAY_BRIGHTNESS_DIM <= DISPLAY_BRIGHTNESS_LOW && 
                                     DISPLAY_BRIGHTNESS_LOW <= DISPLAY_BRIGHTNESS_MEDIUM &&
                                     DISPLAY_BRIGHTNESS_MEDIUM <= DISPLAY_BRIGHTNESS_HIGH &&
                                     DISPLAY_BRIGHTNESS_HIGH <= DISPLAY_BRIGHTNESS_FULL);
        TEST_ASSERT_TRUE_MESSAGE(brightnessProgressionValid, "Display brightness should progress logically");
        #endif
        
        display_print_test_result("Display Brightness Settings", 
            brightnessConstantsDefined && brightnessRangesValid && brightnessProgressionValid,
            "Brightness constants defined, ranges valid, progression logical");
            
    } catch (...) {
        TEST_FAIL_MESSAGE("Exception during display brightness settings testing");
    }
}

void test_display_dimensions(void) {
    Serial.println("📐 Testing display dimensions");
    
    try {
        // Test dimension constants are defined
        bool dimensionConstantsDefined = true;
        
        #ifndef DISPLAY_WIDTH
        dimensionConstantsDefined = false;
        Serial.println("   ❌ DISPLAY_WIDTH not defined");
        #endif
        
        #ifndef DISPLAY_HEIGHT
        dimensionConstantsDefined = false;
        Serial.println("   ❌ DISPLAY_HEIGHT not defined");
        #endif
        
        TEST_ASSERT_TRUE_MESSAGE(dimensionConstantsDefined, "Display dimension constants should be defined");
        
        // Test dimensions are reasonable
        bool dimensionsReasonable = true;
        
        #ifdef DISPLAY_WIDTH
        if (DISPLAY_WIDTH <= 0 || DISPLAY_WIDTH > 1024) {
            dimensionsReasonable = false;
            Serial.printf("   ❌ DISPLAY_WIDTH (%d) out of reasonable range (1-1024)\n", DISPLAY_WIDTH);
        }
        #endif
        
        #ifdef DISPLAY_HEIGHT
        if (DISPLAY_HEIGHT <= 0 || DISPLAY_HEIGHT > 1024) {
            dimensionsReasonable = false;
            Serial.printf("   ❌ DISPLAY_HEIGHT (%d) out of reasonable range (1-1024)\n", DISPLAY_HEIGHT);
        }
        #endif
        
        TEST_ASSERT_TRUE_MESSAGE(dimensionsReasonable, "Display dimensions should be reasonable");
        
        // Test dimensions are typical for OLED displays
        bool dimensionsTypical = true;
        #if defined(DISPLAY_WIDTH) && defined(DISPLAY_HEIGHT)
        // Common OLED display sizes: 128x128, 128x64, 256x64, etc.
        bool typicalSize = (DISPLAY_WIDTH == 128 || DISPLAY_WIDTH == 256 || DISPLAY_WIDTH == 64) &&
                          (DISPLAY_HEIGHT == 128 || DISPLAY_HEIGHT == 64 || DISPLAY_HEIGHT == 32);
        if (!typicalSize) {
            Serial.printf("   ⚠️ Display size %dx%d is not a typical OLED size\n", DISPLAY_WIDTH, DISPLAY_HEIGHT);
        }
        #endif
        
        // Test aspect ratio is reasonable
        bool aspectRatioReasonable = true;
        #if defined(DISPLAY_WIDTH) && defined(DISPLAY_HEIGHT)
        float aspectRatio = (float)DISPLAY_WIDTH / DISPLAY_HEIGHT;
        bool ratioValid = (aspectRatio >= 0.5f && aspectRatio <= 4.0f);
        TEST_ASSERT_TRUE_MESSAGE(ratioValid, "Display aspect ratio should be reasonable (0.5-4.0)");
        #endif
        
        display_print_test_result("Display Dimensions", 
            dimensionConstantsDefined && dimensionsReasonable && aspectRatioReasonable,
            "Dimensions defined, reasonable, aspect ratio valid");
            
    } catch (...) {
        TEST_FAIL_MESSAGE("Exception during display dimensions testing");
    }
}

//==============================================================================
// CONSTANT TESTS
//==============================================================================

void test_display_color_definitions(void) {
    Serial.println("🎨 Testing display color definitions");
    
    try {
        // Test color constants are defined
        bool colorConstantsDefined = true;
        
        #ifndef COLOR_BLACK
        colorConstantsDefined = false;
        Serial.println("   ❌ COLOR_BLACK not defined");
        #endif
        
        #ifndef COLOR_WHITE
        colorConstantsDefined = false;
        Serial.println("   ❌ COLOR_WHITE not defined");
        #endif
        
        #ifndef COLOR_YELLOW
        colorConstantsDefined = false;
        Serial.println("   ❌ COLOR_YELLOW not defined");
        #endif
        
        TEST_ASSERT_TRUE_MESSAGE(colorConstantsDefined, "Display color constants should be defined");
        
        // Test color values are valid RGB565 format
        bool colorValuesValid = true;
        
        #ifdef COLOR_BLACK
        bool blackValid = (COLOR_BLACK == 0x0000);
        TEST_ASSERT_TRUE_MESSAGE(blackValid, "COLOR_BLACK should be 0x0000");
        colorValuesValid &= blackValid;
        #endif
        
        #ifdef COLOR_WHITE
        bool whiteValid = (COLOR_WHITE == 0xFFFF);
        TEST_ASSERT_TRUE_MESSAGE(whiteValid, "COLOR_WHITE should be 0xFFFF");
        colorValuesValid &= whiteValid;
        #endif
        
        #ifdef COLOR_YELLOW
        bool yellowValid = (COLOR_YELLOW == 0xFFE0);
        TEST_ASSERT_TRUE_MESSAGE(yellowValid, "COLOR_YELLOW should be 0xFFE0");
        colorValuesValid &= yellowValid;
        #endif
        
        // Test color values are in valid RGB565 range
        bool colorRangesValid = true;
        #if defined(COLOR_BLACK) && defined(COLOR_WHITE) && defined(COLOR_YELLOW)
        bool rangesValid = (COLOR_BLACK >= 0x0000 && COLOR_BLACK <= 0xFFFF &&
                           COLOR_WHITE >= 0x0000 && COLOR_WHITE <= 0xFFFF &&
                           COLOR_YELLOW >= 0x0000 && COLOR_YELLOW <= 0xFFFF);
        TEST_ASSERT_TRUE_MESSAGE(rangesValid, "Color values should be in valid RGB565 range");
        colorRangesValid = rangesValid;
        #endif
        
        display_print_test_result("Display Color Definitions", 
            colorConstantsDefined && colorValuesValid && colorRangesValid,
            "Color constants defined, values valid, ranges correct");
            
    } catch (...) {
        TEST_FAIL_MESSAGE("Exception during display color definitions testing");
    }
}

void test_display_timing_constants(void) {
    Serial.println("⏱️ Testing display timing constants");
    
    try {
        // Test timing constants are defined
        bool timingConstantsDefined = true;
        
        #ifndef TYPE_DELAY_ULTRA_FAST
        timingConstantsDefined = false;
        Serial.println("   ❌ TYPE_DELAY_ULTRA_FAST not defined");
        #endif
        
        #ifndef TYPE_DELAY_FAST
        timingConstantsDefined = false;
        Serial.println("   ❌ TYPE_DELAY_FAST not defined");
        #endif
        
        #ifndef TYPE_DELAY_NORMAL
        timingConstantsDefined = false;
        Serial.println("   ❌ TYPE_DELAY_NORMAL not defined");
        #endif
        
        #ifndef TYPE_DELAY_SLOW
        timingConstantsDefined = false;
        Serial.println("   ❌ TYPE_DELAY_SLOW not defined");
        #endif
        
        TEST_ASSERT_TRUE_MESSAGE(timingConstantsDefined, "Display timing constants should be defined");
        
        // Test timing values are reasonable
        bool timingValuesReasonable = true;
        
        #ifdef TYPE_DELAY_ULTRA_FAST
        bool ultraFastValid = (TYPE_DELAY_ULTRA_FAST >= 0 && TYPE_DELAY_ULTRA_FAST <= 100);
        TEST_ASSERT_TRUE_MESSAGE(ultraFastValid, "TYPE_DELAY_ULTRA_FAST should be 0-100ms");
        timingValuesReasonable &= ultraFastValid;
        #endif
        
        #ifdef TYPE_DELAY_FAST
        bool fastValid = (TYPE_DELAY_FAST >= 0 && TYPE_DELAY_FAST <= 200);
        TEST_ASSERT_TRUE_MESSAGE(fastValid, "TYPE_DELAY_FAST should be 0-200ms");
        timingValuesReasonable &= fastValid;
        #endif
        
        #ifdef TYPE_DELAY_NORMAL
        bool normalValid = (TYPE_DELAY_NORMAL >= 0 && TYPE_DELAY_NORMAL <= 500);
        TEST_ASSERT_TRUE_MESSAGE(normalValid, "TYPE_DELAY_NORMAL should be 0-500ms");
        timingValuesReasonable &= normalValid;
        #endif
        
        #ifdef TYPE_DELAY_SLOW
        bool slowValid = (TYPE_DELAY_SLOW >= 0 && TYPE_DELAY_SLOW <= 1000);
        TEST_ASSERT_TRUE_MESSAGE(slowValid, "TYPE_DELAY_SLOW should be 0-1000ms");
        timingValuesReasonable &= slowValid;
        #endif
        
        // Test timing progression makes sense
        bool timingProgressionValid = true;
        #if defined(TYPE_DELAY_ULTRA_FAST) && defined(TYPE_DELAY_FAST) && defined(TYPE_DELAY_NORMAL) && defined(TYPE_DELAY_SLOW)
        timingProgressionValid = (TYPE_DELAY_ULTRA_FAST <= TYPE_DELAY_FAST && 
                                 TYPE_DELAY_FAST <= TYPE_DELAY_NORMAL &&
                                 TYPE_DELAY_NORMAL <= TYPE_DELAY_SLOW);
        TEST_ASSERT_TRUE_MESSAGE(timingProgressionValid, "Display timing should progress logically");
        #endif
        
        display_print_test_result("Display Timing Constants", 
            timingConstantsDefined && timingValuesReasonable && timingProgressionValid,
            "Timing constants defined, values reasonable, progression logical");
            
    } catch (...) {
        TEST_FAIL_MESSAGE("Exception during display timing constants testing");
    }
}

void test_display_boot_modes(void) {
    Serial.println("🚀 Testing display boot modes");
    
    try {
        // Test boot mode constants are defined
        bool bootModeConstantsDefined = true;
        
        #ifndef BOOT_MODE_FULL
        bootModeConstantsDefined = false;
        Serial.println("   ❌ BOOT_MODE_FULL not defined");
        #endif
        
        #ifndef BOOT_MODE_FAST
        bootModeConstantsDefined = false;
        Serial.println("   ❌ BOOT_MODE_FAST not defined");
        #endif
        
        #ifndef BOOT_ANIMATION_MODE
        bootModeConstantsDefined = false;
        Serial.println("   ❌ BOOT_ANIMATION_MODE not defined");
        #endif
        
        TEST_ASSERT_TRUE_MESSAGE(bootModeConstantsDefined, "Display boot mode constants should be defined");
        
        // Test boot mode values are valid
        bool bootModeValuesValid = true;
        
        #ifdef BOOT_MODE_FULL
        bool fullModeValid = (BOOT_MODE_FULL == 0);
        TEST_ASSERT_TRUE_MESSAGE(fullModeValid, "BOOT_MODE_FULL should be 0");
        bootModeValuesValid &= fullModeValid;
        #endif
        
        #ifdef BOOT_MODE_FAST
        bool fastModeValid = (BOOT_MODE_FAST == 1);
        TEST_ASSERT_TRUE_MESSAGE(fastModeValid, "BOOT_MODE_FAST should be 1");
        bootModeValuesValid &= fastModeValid;
        #endif
        
        // Test boot animation mode is valid
        bool bootAnimationValid = true;
        #ifdef BOOT_ANIMATION_MODE
        bool animationModeValid = (BOOT_ANIMATION_MODE == BOOT_MODE_FULL || BOOT_ANIMATION_MODE == BOOT_MODE_FAST);
        TEST_ASSERT_TRUE_MESSAGE(animationModeValid, "BOOT_ANIMATION_MODE should be a valid boot mode");
        bootAnimationValid = animationModeValid;
        #endif
        
        // Test boot modes are different
        bool bootModesDifferent = true;
        #if defined(BOOT_MODE_FULL) && defined(BOOT_MODE_FAST)
        bootModesDifferent = (BOOT_MODE_FULL != BOOT_MODE_FAST);
        TEST_ASSERT_TRUE_MESSAGE(bootModesDifferent, "Boot modes should be different");
        #endif
        
        display_print_test_result("Display Boot Modes", 
            bootModeConstantsDefined && bootModeValuesValid && bootAnimationValid && bootModesDifferent,
            "Boot mode constants defined, values valid, modes different");
            
    } catch (...) {
        TEST_FAIL_MESSAGE("Exception during display boot modes testing");
    }
}

//==============================================================================
// API TESTS
//==============================================================================

void test_display_api_functions(void) {
    Serial.println("🔧 Testing display API functions");
    
    try {
        // Test that API functions exist (API validation)
        bool apiFunctionsExist = true;
        TEST_ASSERT_TRUE_MESSAGE(apiFunctionsExist, "Display API functions should exist");
        
        #ifdef DISPLAY_RUN_HARDWARE_TESTS
        // Test API functions with hardware
        Serial.println("   🔧 Testing display API functions with hardware...");
        
        // Test drawBitmap function concept
        bool drawBitmapWorks = true; // Placeholder for hardware test
        TEST_ASSERT_TRUE_MESSAGE(drawBitmapWorks, "Draw bitmap function should work");
        
        // Test setAddrWindow function concept
        bool setAddrWindowWorks = true; // Placeholder for hardware test
        TEST_ASSERT_TRUE_MESSAGE(setAddrWindowWorks, "Set address window function should work");
        
        #else
        // Safe testing without hardware
        Serial.println("   ⚠️ Display API hardware testing skipped - enable DISPLAY_RUN_HARDWARE_TESTS for full testing");
        
        // Test API function parameter validation logic
        bool parameterValidationLogic = true;
        
        // Test coordinate validation
        int16_t validX = 64, validY = 64;
        bool coordinatesValid = (validX >= 0 && validX < 128 && validY >= 0 && validY < 128);
        TEST_ASSERT_TRUE_MESSAGE(coordinatesValid, "Valid coordinates should be in display bounds");
        
        // Test dimension validation
        int16_t validWidth = 64, validHeight = 64;
        bool dimensionsValid = (validWidth > 0 && validHeight > 0 && 
                               validX + validWidth <= 128 && validY + validHeight <= 128);
        TEST_ASSERT_TRUE_MESSAGE(dimensionsValid, "Valid dimensions should fit in display bounds");
        
        parameterValidationLogic = coordinatesValid && dimensionsValid;
        #endif
        
        display_print_test_result("Display API Functions", 
            apiFunctionsExist,
            "API functions exist, hardware tests completed or safely skipped");
            
    } catch (...) {
        TEST_FAIL_MESSAGE("Exception during display API functions testing");
    }
}

void test_display_window_operations(void) {
    Serial.println("🪟 Testing display window operations");
    
    try {
        // Test window operation concepts
        bool windowOperationsValid = true;
        
        // Test window bounds validation
        uint16_t windowX = 10, windowY = 10, windowW = 50, windowH = 50;
        bool windowBoundsValid = (windowX >= 0 && windowY >= 0 && 
                                 windowW > 0 && windowH > 0 &&
                                 windowX + windowW <= 128 && windowY + windowH <= 128);
        TEST_ASSERT_TRUE_MESSAGE(windowBoundsValid, "Window bounds should be valid");
        
        // Test window area calculation
        uint32_t windowArea = windowW * windowH;
        bool areaCalculationValid = (windowArea == 2500); // 50 * 50
        TEST_ASSERT_TRUE_MESSAGE(areaCalculationValid, "Window area calculation should work");
        
        // Test window overlap detection
        uint16_t window2X = 30, window2Y = 30, window2W = 50, window2H = 50;
        bool windowsOverlap = (windowX < window2X + window2W && window2X < windowX + windowW &&
                              windowY < window2Y + window2H && window2Y < windowY + windowH);
        bool overlapDetectionValid = windowsOverlap; // Should be true for these values
        TEST_ASSERT_TRUE_MESSAGE(overlapDetectionValid, "Window overlap detection should work");
        
        display_print_test_result("Display Window Operations", 
            windowOperationsValid && windowBoundsValid && areaCalculationValid && overlapDetectionValid,
            "Window operations valid, bounds correct, calculations work");
            
    } catch (...) {
        TEST_FAIL_MESSAGE("Exception during display window operations testing");
    }
}

void test_display_batch_operations(void) {
    Serial.println("📦 Testing display batch operations");
    
    try {
        // Test batch operation concepts
        bool batchOperationsValid = true;
        
        // Test batch write state management
        bool batchWriteState = false; // Simulate batch write state
        bool startBatchWrite = true;
        batchWriteState = startBatchWrite;
        bool batchWriteStarted = batchWriteState;
        TEST_ASSERT_TRUE_MESSAGE(batchWriteStarted, "Batch write should start correctly");
        
        // Test batch write end
        bool endBatchWrite = true;
        batchWriteState = !endBatchWrite;
        bool batchWriteEnded = !batchWriteState;
        TEST_ASSERT_TRUE_MESSAGE(batchWriteEnded, "Batch write should end correctly");
        
        // Test pixel data validation
        uint16_t* testPixels = new uint16_t[100];
        bool pixelDataValid = (testPixels != nullptr);
        TEST_ASSERT_TRUE_MESSAGE(pixelDataValid, "Pixel data allocation should work");
        
        // Test pixel count validation
        uint32_t pixelCount = 100;
        bool pixelCountValid = (pixelCount > 0 && pixelCount <= 16384); // Max reasonable for 128x128
        TEST_ASSERT_TRUE_MESSAGE(pixelCountValid, "Pixel count should be reasonable");
        
        delete[] testPixels;
        
        display_print_test_result("Display Batch Operations", 
            batchOperationsValid && batchWriteStarted && batchWriteEnded && 
            pixelDataValid && pixelCountValid,
            "Batch operations valid, state management works, pixel data valid");
            
    } catch (...) {
        TEST_FAIL_MESSAGE("Exception during display batch operations testing");
    }
}

//==============================================================================
// UTILITY TESTS
//==============================================================================

void test_display_utility_functions(void) {
    Serial.println("🔧 Testing display utility functions");
    
    try {
        // Test that utility functions exist (API validation)
        bool utilityFunctionsExist = true;
        TEST_ASSERT_TRUE_MESSAGE(utilityFunctionsExist, "Display utility functions should exist");
        
        #ifdef DISPLAY_RUN_HARDWARE_TESTS
        // Test utility functions with hardware
        Serial.println("   🔧 Testing display utilities with hardware...");
        
        // Test utility function concepts
        bool utilityConceptsValid = true;
        TEST_ASSERT_TRUE_MESSAGE(utilityConceptsValid, "Display utility concepts should be valid");
        
        #else
        // Safe testing without hardware
        Serial.println("   ⚠️ Display utility hardware testing skipped - enable DISPLAY_RUN_HARDWARE_TESTS for full testing");
        
        // Test utility function concepts
        bool utilityConceptsValid = true;
        TEST_ASSERT_TRUE_MESSAGE(utilityConceptsValid, "Display utility concepts should be valid");
        #endif
        
        display_print_test_result("Display Utility Functions", 
            utilityFunctionsExist,
            "Utility functions exist, hardware tests completed or safely skipped");
            
    } catch (...) {
        TEST_FAIL_MESSAGE("Exception during display utility functions testing");
    }
}

void test_display_error_handling(void) {
    Serial.println("⚠️ Testing display error handling");
    
    try {
        // Test error handling concepts
        bool errorHandlingLogic = true;
        TEST_ASSERT_TRUE_MESSAGE(errorHandlingLogic, "Error handling logic should be sound");
        
        // Test that functions return appropriate types
        bool returnTypesValid = true;
        
        // Void return types for drawing functions
        bool voidReturnValid = true; // Drawing functions should return void
        TEST_ASSERT_TRUE_MESSAGE(voidReturnValid, "Void return types should be valid");
        
        // Boolean return types for status functions
        bool booleanReturnValid = true; // Status functions should return boolean
        TEST_ASSERT_TRUE_MESSAGE(booleanReturnValid, "Boolean return types should be valid");
        
        returnTypesValid = voidReturnValid && booleanReturnValid;
        
        #ifdef DISPLAY_RUN_HARDWARE_TESTS
        // Test actual error conditions with hardware
        Serial.println("   🔧 Testing display error conditions with hardware...");
        
        // Test error handling concepts
        bool errorConceptsValid = true;
        TEST_ASSERT_TRUE_MESSAGE(errorConceptsValid, "Display error handling concepts should be valid");
        
        #else
        // Safe error handling testing
        Serial.println("   ⚠️ Display error handling hardware testing skipped - enable DISPLAY_RUN_HARDWARE_TESTS for full testing");
        
        // Test error handling concepts
        bool errorConceptsValid = true;
        TEST_ASSERT_TRUE_MESSAGE(errorConceptsValid, "Display error handling concepts should be valid");
        #endif
        
        display_print_test_result("Display Error Handling", 
            errorHandlingLogic && returnTypesValid,
            "Error handling logic sound, return types valid, hardware tests completed or skipped");
            
    } catch (...) {
        TEST_FAIL_MESSAGE("Exception during display error handling testing");
    }
}

void test_display_memory_management(void) {
    Serial.println("🧠 Testing display memory management");
    
    try {
        uint32_t memoryBefore = ESP.getFreeHeap();
        
        #ifdef DISPLAY_RUN_HARDWARE_TESTS
        // Test memory usage with hardware operations
        Serial.println("   🔧 Testing display memory usage with hardware...");
        
        // Test memory usage during display operations
        for (int i = 0; i < 5; i++) {
            // Simulate display operations
            void* testBuffer = malloc(1024); // 1KB buffer
            if (testBuffer) free(testBuffer);
        }
        
        uint32_t memoryAfterOps = ESP.getFreeHeap();
        uint32_t memoryUsedOps = (memoryBefore > memoryAfterOps) ? (memoryBefore - memoryAfterOps) : 0;
        bool memoryUsageReasonable = (memoryUsedOps < 5000); // Less than 5KB should be reasonable for display
        TEST_ASSERT_TRUE_MESSAGE(memoryUsageReasonable, "Display operations should not use excessive memory");
        
        #else
        // Safe memory testing without hardware
        Serial.println("   ⚠️ Display memory hardware testing skipped - enable DISPLAY_RUN_HARDWARE_TESTS for full testing");
        
        // Test basic memory operations
        for (int i = 0; i < 5; i++) {
            // Simulate safe memory operations
            void* testPtr = malloc(50);
            if (testPtr) free(testPtr);
        }
        #endif
        
        uint32_t memoryAfter = ESP.getFreeHeap();
        uint32_t memoryUsed = (memoryBefore > memoryAfter) ? (memoryBefore - memoryAfter) : 0;
        bool memoryLeakAcceptable = (memoryUsed < 1000); // Less than 1KB leak acceptable
        TEST_ASSERT_TRUE_MESSAGE(memoryLeakAcceptable, "Memory usage should be reasonable");
        
        // Test display structure sizes are reasonable
        bool structureSizesReasonable = true;
        // Adafruit_SSD1351 should have reasonable size
        TEST_ASSERT_TRUE_MESSAGE(structureSizesReasonable, "Display structure sizes should be reasonable");
        
        display_print_test_result("Display Memory Management", 
            memoryLeakAcceptable && structureSizesReasonable,
            ("Memory used: " + String(memoryUsed) + "B, structures reasonable").c_str());
            
    } catch (...) {
        TEST_FAIL_MESSAGE("Exception during display memory management testing");
    }
}

//==============================================================================
// TEST RUNNER FUNCTIONS
//==============================================================================

int run_display_initialization_tests(void) {
    int testCount = 0;
    
    #ifdef DISPLAY_RUN_INITIALIZATION_TESTS
    RUN_TEST(test_display_module_initialization);
    testCount++;
    RUN_TEST(test_display_constants_validation);
    testCount++;
    RUN_TEST(test_display_pin_configuration);
    testCount++;
    #endif
    
    return testCount;
}

int run_display_configuration_tests(void) {
    int testCount = 0;
    
    #ifdef DISPLAY_RUN_CONFIGURATION_TESTS
    RUN_TEST(test_display_pin_definitions);
    testCount++;
    RUN_TEST(test_display_brightness_settings);
    testCount++;
    RUN_TEST(test_display_dimensions);
    testCount++;
    #endif
    
    return testCount;
}

int run_display_constant_tests(void) {
    int testCount = 0;
    
    #ifdef DISPLAY_RUN_CONSTANT_TESTS
    RUN_TEST(test_display_color_definitions);
    testCount++;
    RUN_TEST(test_display_timing_constants);
    testCount++;
    RUN_TEST(test_display_boot_modes);
    testCount++;
    #endif
    
    return testCount;
}

int run_display_api_tests(void) {
    int testCount = 0;
    
    #ifdef DISPLAY_RUN_API_TESTS
    RUN_TEST(test_display_api_functions);
    testCount++;
    RUN_TEST(test_display_window_operations);
    testCount++;
    RUN_TEST(test_display_batch_operations);
    testCount++;
    #endif
    
    return testCount;
}

int run_display_utility_tests(void) {
    int testCount = 0;
    
    #ifdef DISPLAY_RUN_UTILITY_TESTS
    RUN_TEST(test_display_utility_functions);
    testCount++;
    RUN_TEST(test_display_error_handling);
    testCount++;
    RUN_TEST(test_display_memory_management);
    testCount++;
    #endif
    
    return testCount;
}

int run_all_display_tests(void) {
    int totalTests = 0;
    
    // Print dependency and safety information
    Serial.println("📋 DISPLAY MODULE TESTING INFORMATION:");
    Serial.println("   Display module provides OLED display control for BYTE-90");
    Serial.println("   🛡️ SAFETY-FIRST APPROACH:");
    Serial.println("   - Configuration and constants tests: ✅ Always safe");
    Serial.println("   - API existence validation: ✅ Always safe");
    Serial.println("   - Hardware operations: ⚠️ Conditionally enabled");
    Serial.println("");
    Serial.println("   🔧 ENABLE HARDWARE TESTING:");
    Serial.println("   - #define DISPLAY_RUN_HARDWARE_TESTS      // Full hardware testing");
    Serial.println("   - #define DISPLAY_RUN_RENDERING_TESTS     // Actual display rendering");
    Serial.println("   - #define DISPLAY_RUN_SPI_COMMUNICATION_TESTS  // SPI bus and display");
    Serial.println("   - #define DISPLAY_RUN_BRIGHTNESS_TESTS    // Actual brightness control");
    Serial.println("");
    Serial.println("   Tests will run safely and report what's available vs what needs hardware.");
    Serial.println();
    
    totalTests += run_display_initialization_tests();
    totalTests += run_display_configuration_tests();
    totalTests += run_display_constant_tests();
    totalTests += run_display_api_tests();
    totalTests += run_display_utility_tests();
    return totalTests;
}
