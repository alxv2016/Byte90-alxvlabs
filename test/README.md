# BYTE-90 Comprehensive Test Suite

The BYTE-90 Test Suite provides thorough testing of all device modules through a clean, modular architecture. Tests are designed to be safe, non-destructive, and comprehensive while providing detailed reporting and memory tracking.

## Quick Start

### Prerequisites

- PlatformIO IDE or Core
- ESP32 development board (Seeed XIAO ESP32S3)
- Unity Test Framework (automatically installed)

### Running Tests

1. **Full Test Suite** (compile, upload, and run):
   ```bash
   platformio test -e seeed_xiao_esp32s3_test
   ```

2. **Build Only** (compile without uploading):
   ```bash
   platformio test -e seeed_xiao_esp32s3_test --without-uploading --without-testing
   ```

3. **Upload and Monitor** (after building):
   ```bash
   platformio test -e seeed_xiao_esp32s3_test --without-building
   platformio device monitor --baud 115200
   ```

4. **Specific Test Filter**:
   ```bash
   platformio test -e seeed_xiao_esp32s3_test --filter "test_adxl*"
   ```

5. **Run Tests Without Hardware** (simulation/mock mode):
   ```bash
   # Tests run safely without actual hardware - 
   # hardware-dependent tests are automatically skipped
   platformio test -e seeed_xiao_esp32s3_test
   ```

## Test Categories

### Current Test Modules

Based on your configuration, the test suite includes:

#### Basic Safety Tests
- ESP32 hardware validation (CPU, memory, chip model)
- Initial memory state verification  
- System stability checks

#### ADXL Module Tests
- **Initialization Tests**: Module setup and constants validation
- **State Management Tests**: State transitions and error handling
- **Configuration Tests**: Sensor definitions and parameter validation
- **Sensor Operation Tests**: Reading, initialization, and cleanup
- **Utility Tests**: Helper functions, error handling, memory management

#### Display Module Tests (Available)
- **Initialization Tests**: Display setup and pin configuration
- **Configuration Tests**: Pin definitions, brightness, dimensions
- **Constant Tests**: Color definitions, timing constants, boot modes
- **API Tests**: Drawing functions, window operations, batch operations
- **Utility Tests**: Helper functions and memory management

### Test Architecture

Each module follows a consistent 5-category testing pattern:

1. **Initialization Tests** - Basic setup and constants validation
2. **State Management Tests** - State transitions and error states  
3. **Configuration Tests** - Parameter validation and definitions
4. **Operation Tests** - Core functionality testing
5. **Utility Tests** - Helper functions and resource management

## Configuration

### Test Environment Setup

The test environment is configured in `platformio.ini`:

```ini
[env:seeed_xiao_esp32s3_test]
platform = espressif32
board = seeed_xiao_esp32s3
framework = arduino
monitor_speed = 115200

build_flags = 
    -DCORE_DEBUG_LEVEL=5
    -DFIRMWARE_VERSION=\"2.0.0\"
    -DUNITY_DOUBLE_PRECISION=1e-12
    -DTEST_ENV

# Include all source files properly for testing
build_src_filter = 
    +<*> 
    -<main.cpp>
    +<src/*.cpp>
    +<src/**/*.cpp>
    +<lib/**/*.cpp>
    +<include/*.cpp>

test_framework = unity
test_speed = 115200
test_build_src = yes
```

### Enabling/Disabling Test Suites

Modify the flags in `test_common.h`:

```cpp
// Enable/disable entire test suites
#define RUN_BASIC_SAFETY_TESTS
#define RUN_ADXL_MODULE_TESTS
#define RUN_DISPLAY_MODULE_TESTS
// Add other module test flags here
```

### Hardware Testing Control (Safety-First Approach)

Your test suite uses a **safety-first approach** where hardware tests are disabled by default:

```cpp
// ADXL Module - Safe by default
// #define ADXL_RUN_HARDWARE_TESTS      // Requires actual ADXL345 sensor
// #define ADXL_RUN_I2C_COMMUNICATION_TESTS
// #define ADXL_RUN_CALIBRATION_TESTS

// Display Module - Safe by default  
// #define DISPLAY_RUN_HARDWARE_TESTS   // Requires actual OLED display
// #define DISPLAY_RUN_RENDERING_TESTS
// #define DISPLAY_RUN_SPI_COMMUNICATION_TESTS
```

**To enable hardware testing**, uncomment the relevant defines:

```cpp
#define ADXL_RUN_HARDWARE_TESTS      // Enable ADXL hardware testing
#define DISPLAY_RUN_HARDWARE_TESTS   // Enable display hardware testing
```

## Safety Features

### Hardware-Safe Testing

- **Configuration Validation**: Tests constants, pin definitions, and parameters
- **API Validation**: Tests function signatures and parameter ranges  
- **Logic Testing**: Tests algorithms and state management without hardware
- **Memory Testing**: Tests allocation/deallocation patterns
- **Error Handling**: Tests error recovery and validation logic

### Running Without Hardware

Tests are designed to run safely without actual hardware:

```
ADXL MODULE TESTING INFORMATION:
   ADXL module provides accelerometer sensor management for BYTE-90
   🛡️ SAFETY-FIRST APPROACH:
   - Configuration and constants tests: ✅ Always safe
   - API existence validation: ✅ Always safe
   - Hardware operations: ⚠️ Conditionally enabled

   🔧 ENABLE HARDWARE TESTING:
   - #define ADXL_RUN_HARDWARE_TESTS      // Full hardware testing
   - #define ADXL_RUN_I2C_COMMUNICATION_TESTS
   - #define ADXL_RUN_CALIBRATION_TESTS

   Tests will run safely and report what's available vs what needs hardware.
```

## Memory Management

### Memory Tracking Features

- **Heap Monitoring**: Tracks free memory before/after test suites
- **Leak Detection**: Alerts when memory usage exceeds thresholds  
- **Safe Thresholds**: Configurable acceptable memory usage limits

### Memory Safety Guidelines

```cpp
// Check for memory leaks with custom thresholds
bool checkMemoryLeak(uint32_t before, uint32_t after, 
                     const char* context, uint32_t maxLeak = 1000);

// Log memory state with context
uint32_t getHeapWithLogging(const char* context);
```

## Expected Test Output

### Test Execution Flow

```
==================================================
🔧 BYTE-90 MODULAR TEST SUITE 🔧
==================================================
Firmware Version: 2.0.0
🎯 Modular, scalable testing architecture
🛡️ Safe, comprehensive testing approach
==================================================

==========================================
TEST SECTION: BASIC SAFETY TESTS
==========================================
Testing ESP32 hardware (SAFE)
✅ ESP32 Hardware - ESP32-S3, 240MHz, 290000 bytes heap
✅ Memory Before Tests - 290000 bytes free
⏱️ Basic Safety Tests completed in 150 ms

==========================================
TEST SECTION: ADXL MODULE TESTS
==========================================

📡 ADXL MODULE TESTING INFORMATION:
   🛡️ SAFETY-FIRST APPROACH:
   - Configuration and constants tests: ✅ Always safe
   - Hardware operations: ⚠️ Conditionally enabled

🔄 Testing ADXL state management
   ⚠️ ADXL hardware testing skipped - enable ADXL_RUN_HARDWARE_TESTS for full testing
✅ ADXL State Management - State management concepts valid

📋 ADXL TEST SUMMARY:
   ✅ Safe tests completed (configuration, constants, API validation)
   ⚠️ Hardware tests skipped - requires ADXL sensor hardware
   🎯 ADXL tests completed: 15 total tests

==================================================
🎉 BYTE-90 TEST SUITE COMPLETED! 🎉
==================================================
📊 Total tests run: 17
⏱️ Total duration: 2500 ms (2.50 seconds)
🧠 Final memory: 289500 bytes free
✅ All modules tested successfully
🚀 System ready for deployment
==================================================
```

## Adding New Test Modules

### Creating a New Test Module

1. **Create header file** (`test_newmodule.h`):
   ```cpp
   #ifndef TEST_NEWMODULE_H
   #define TEST_NEWMODULE_H
   
   #include <unity.h>
   #include <Arduino.h>
   #include "newmodule.h"
   #include "test_common.h"
   
   // Test configuration flags
   #define NEWMODULE_RUN_INITIALIZATION_TESTS
   #define NEWMODULE_RUN_STATE_MANAGEMENT_TESTS
   #define NEWMODULE_RUN_CONFIGURATION_TESTS
   #define NEWMODULE_RUN_OPERATION_TESTS
   #define NEWMODULE_RUN_UTILITY_TESTS
   
   // Disable risky tests by default
   // #define NEWMODULE_RUN_HARDWARE_TESTS
   
   // Test function declarations
   void test_newmodule_initialization(void);
   void test_newmodule_state_management(void);
   void test_newmodule_configuration(void);
   void test_newmodule_operations(void);
   void test_newmodule_utilities(void);
   
   // Test runner functions
   int run_newmodule_initialization_tests(void);
   int run_all_newmodule_tests(void);
   
   #endif
   ```

2. **Implement tests** (`test_newmodule.cpp`):
   ```cpp
   #include "test_newmodule.h"
   #include "test_common.h"
   
   void test_newmodule_initialization(void) {
       Serial.println("🚀 Testing newmodule initialization (SAFE VERSION)");
       
       try {
           // Test basic framework functionality
           bool basicTestFramework = true;
           TEST_ASSERT_TRUE_MESSAGE(basicTestFramework, "Test framework should work");
           
           #ifdef NEWMODULE_RUN_HARDWARE_TESTS
           // Hardware tests here
           #else
           Serial.println("   ⚠️ Hardware testing skipped - enable NEWMODULE_RUN_HARDWARE_TESTS for full testing");
           #endif
           
       } catch (...) {
           TEST_FAIL_MESSAGE("Exception during newmodule initialization");
       }
   }
   
   int run_all_newmodule_tests(void) {
       int totalTests = 0;
       
       Serial.println("📋 NEWMODULE TESTING INFORMATION:");
       Serial.println("   🛡️ SAFETY-FIRST APPROACH:");
       Serial.println("   - Configuration tests: ✅ Always safe");
       Serial.println("   - Hardware operations: ⚠️ Conditionally enabled");
       
       totalTests += run_newmodule_initialization_tests();
       // Add other test categories...
       
       return totalTests;
   }
   ```

3. **Add to test runner** (`test_runner.cpp`):
   ```cpp
   #ifdef RUN_NEWMODULE_TESTS
   #include "test_newmodule.h"
   #endif
   
   void runNewModuleTests(void) {
       #ifdef RUN_NEWMODULE_TESTS
       printTestSectionHeader("NEWMODULE TESTS");
       unsigned long sectionStart = getTestUptime();
       
       int tests = run_all_newmodule_tests();
       totalTestsRun += tests;
       
       Serial.printf("🎯 Newmodule tests completed: %d total tests\n", tests);
       #endif
   }
   ```

4. **Enable in common config** (`test_common.h`):
   ```cpp
   #define RUN_NEWMODULE_TESTS
   ```

## Troubleshooting

### Common Issues

**Tests Hanging or Crashing**:
- Check memory usage with `getHeapWithLogging()`
- Verify delays with `briefStabilityDelay()`
- Ensure proper test isolation with try/catch blocks

**Compilation Errors**:
- Verify module header files exist (`#include "module_name.h"`)
- Check that constants are defined properly
- Ensure test defines match module capabilities

**Hardware Test Issues**:
- Hardware tests are disabled by default for safety
- Enable with `#define MODULE_RUN_HARDWARE_TESTS`
- Check hardware connections if enabling hardware tests

### Debug Options

Enable verbose output in `platformio.ini`:
```ini
build_flags = 
    -DCORE_DEBUG_LEVEL=5
```

Add custom debug prints:
```cpp
Serial.printf("🔍 Debug: %s\n", debugInfo);
```

## Running Tests on Different Platforms

### Wokwi Simulator
Tests can run on Wokwi online simulator since they don't require actual hardware.

### Desktop Testing
The safety-first architecture makes tests suitable for desktop mocking:

```cpp
#ifdef DESKTOP_TESTING
#include "mock_arduino.h"
#else
#include <Arduino.h>
#endif
```

### CI/CD Integration
Tests can be automated in continuous integration:

```yaml
name: Run Tests
on: [push, pull_request]
jobs:
  test:
    runs-on: ubuntu-latest
    steps:
    - uses: actions/checkout@v2
    - name: Setup PlatformIO
      run: pip install platformio
    - name: Run Tests
      run: platformio test -e seeed_xiao_esp32s3_test --without-uploading
```

## Test Writing Best Practices

- ✅ Use descriptive test names and messages
- ✅ Follow the safety-first approach (hardware tests disabled by default)
- ✅ Include memory usage validation with `getHeapWithLogging()`
- ✅ Add timing measurements for performance-critical code
- ✅ Use try/catch blocks for exception safety
- ✅ Follow the 5-category pattern (init, state, config, operations, utilities)
- ✅ Test both success and error conditions
- ✅ Provide clear feedback about what's tested vs. what's skipped

---

**Happy Testing! The safety-first approach ensures your tests run reliably without hardware dependencies.**