/**
 * @file haptics_effects.cpp
 * @brief Implementation of haptic effect functions
 */

#include "haptics_effects.h"
#include "display_module.h"

//==============================================================================
// HIGH-LEVEL EFFECT FUNCTIONS
//==============================================================================

/**
 * @brief Quick click feedback
 * @param intensity 1=light, 2=medium, 3=strong
 * @return true if effect started successfully, false otherwise
 */
bool quickClick(uint8_t intensity) {
    if (!isHapticsReady()) {
        return false;
    }
    
    uint8_t effectId;
    switch (intensity) {
        case 1: // Light
            effectId = HAPTIC_SOFT_BUMP_100;
            break;
        case 2: // Medium
            effectId = HAPTIC_SHARP_CLICK_100;
            break;
        case 3: // Strong
            effectId = HAPTIC_STRONG_CLICK_100;
            break;
        default:
            effectId = HAPTIC_SHARP_CLICK_100;
            break;
    }
    
    return playHapticEffect(effectId);
}

/**
 * @brief Quick notification buzz
 * @param intensity 1=light, 2=medium, 3=strong
 * @return true if effect started successfully, false otherwise
 */
bool notificationBuzz(uint8_t intensity) {
    if (!isHapticsReady()) {
        return false;
    }
    
    uint8_t effectId;
    switch (intensity) {
        case 1: // Light
            effectId = HAPTIC_TRANSITION_CLICK_1_100;
            break;
        case 2: // Medium
            effectId = HAPTIC_STRONG_BUZZ_100;
            break;
        case 3: // Strong
            effectId = HAPTIC_ALERT_750MS;
            break;
        default:
            effectId = HAPTIC_STRONG_BUZZ_100;
            break;
    }
    
    return playHapticEffect(effectId);
}

/**
 * @brief Quick pulse vibration
 * @param intensity Vibration intensity (0-255)
 * @param durationMs Duration in milliseconds
 * @return true if pulse completed successfully, false otherwise
 */
bool pulseVibration(uint8_t intensity, uint16_t durationMs) {
    if (!isHapticsReady()) {
        return false;
    }
    
    // Start continuous vibration
    if (!startContinuousVibration(intensity)) {
        return false;
    }
    
    // Wait for duration
    delay(durationMs);
    
    // Stop vibration
    stopHapticEffect();
    
    return true;
}

/**
 * @brief Play 3 quick clicks followed by a strong buzz
 * @return true if effect sequence completed successfully, false otherwise
 */
bool tripleClickBuzz() {
    if (!isHapticsReady()) {
        return false;
    }
    
    // Play 3 quick clicks with short delays
    for (int i = 0; i < 3; i++) {
        if (!playHapticEffect(HAPTIC_SHARP_CLICK_100)) {
            ESP_LOGW(HAPTICS_LOG, "Failed to play click %d", i + 1);
            return false;
        }
        delay(20); // Short delay between clicks
    }
    
    // Brief pause before the buzz
    delay(10);
    
    // Play strong buzz
    if (!playHapticEffect(HAPTIC_STRONG_BUZZ_100)) {
        ESP_LOGW(HAPTICS_LOG, "Failed to play strong buzz");
        return false;
    }
    return true;
}

/**
 * @brief Test all haptic effects with display output
 */
void testHapticsEffects() {
    if (!isHapticsReady()) {
        ESP_LOGE(HAPTICS_LOG, "Haptics not ready for testing");
        return;
    }
    
    if (!areHapticsActive()) {
        ESP_LOGE(HAPTICS_LOG, "Haptics not active - cannot test");
        return;
    }
    
    ESP_LOGI(HAPTICS_LOG, "Starting haptic effects test...");
    
    // Test basic clicks
    ESP_LOGI(HAPTICS_LOG, "Testing basic clicks...");
    playHapticEffect(HAPTIC_STRONG_CLICK_100);
    delay(500);
    playHapticEffect(HAPTIC_SHARP_CLICK_100);
    delay(500);
    playHapticEffect(HAPTIC_SOFT_BUMP_100);
    delay(500);
    
    // Test double/triple clicks
    ESP_LOGI(HAPTICS_LOG, "Testing double/triple clicks...");
    playHapticEffect(HAPTIC_DOUBLE_CLICK_100);
    delay(500);
    playHapticEffect(HAPTIC_TRIPLE_CLICK_100);
    delay(500);
    
    // Test buzz effects
    ESP_LOGI(HAPTICS_LOG, "Testing buzz effects...");
    playHapticEffect(HAPTIC_STRONG_BUZZ_100);
    delay(500);
    playHapticEffect(HAPTIC_BUZZ_1_100);
    delay(500);
    
    // Test alerts
    ESP_LOGI(HAPTICS_LOG, "Testing alerts...");
    playHapticEffect(HAPTIC_ALERT_750MS);
    delay(1000);
    playHapticEffect(HAPTIC_ALERT_1000MS);
    delay(1000);
    
    // Test pulsing
    ESP_LOGI(HAPTICS_LOG, "Testing pulsing...");
    playHapticEffect(HAPTIC_PULSING_STRONG_1_100);
    delay(500);
    playHapticEffect(HAPTIC_PULSING_MEDIUM_1_100);
    delay(500);
    
    // Test transitions
    ESP_LOGI(HAPTICS_LOG, "Testing transitions...");
    playHapticEffect(HAPTIC_TRANSITION_CLICK_1_100);
    delay(500);
    playHapticEffect(HAPTIC_TRANSITION_HUM_1_100);
    delay(500);
    
    // Test ramp effects
    ESP_LOGI(HAPTICS_LOG, "Testing ramp effects...");
    playHapticEffect(HAPTIC_RAMP_DOWN_LONG_SMOOTH_1_100);
    delay(500);
    playHapticEffect(HAPTIC_RAMP_UP_LONG_SMOOTH_1_100);
    delay(500);
    
    ESP_LOGI(HAPTICS_LOG, "Haptic effects test completed!");
}
