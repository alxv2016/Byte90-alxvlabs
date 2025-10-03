/**
 * @file menu_button.cpp
 * @brief Clean button input handling for the menu system
 *
 * Provides comprehensive button input handling for the BYTE-90 device menu system,
 * including debouncing, event detection, and user feedback integration.
 *
 * This module handles:
 * - Button debouncing and interrupt handling
 * - Single click, double click, and long press detection
 * - Event state machine management
 * - Audio and haptic feedback integration
 * - Timeout processing and event queuing
 * - Button state reset and cleanup
 * - Interrupt-safe event processing
 * - Menu system integration and event delivery
 * - Power-efficient button monitoring
 */

#include "menu_button.h"
#include "soundsfx_module.h"
#include "haptics_module.h"
#include "haptics_effects.h"

//==============================================================================
// PRIVATE VARIABLES
//==============================================================================

static volatile ButtonState buttonState = BTN_IDLE;
static volatile unsigned long buttonPressStartTime = 0;
static volatile unsigned long lastReleaseTime = 0;
static volatile unsigned long lastDebounceTime = 0;
static volatile bool lastButtonReading = HIGH;
static volatile ButtonEvent pendingEvent = BUTTON_NONE;
static volatile bool eventReady = false;

//==============================================================================
// PRIVATE FUNCTIONS
//==============================================================================

/**
 * @brief Button interrupt handler
 * 
 * Interrupt service routine that handles button state changes. This function
 * is called whenever the button pin state changes and implements debouncing,
 * event detection, and state machine management.
 */
static void IRAM_ATTR buttonInterruptHandler() {
    unsigned long currentTime = millis();
    bool currentReading = digitalRead(MENU_BUTTON_PIN);
    
    // Debounce check
    if (currentTime - lastDebounceTime < MENU_DEBOUNCE_TIME) {
        return;
    }
    
    if (currentReading != lastButtonReading) {
        lastDebounceTime = currentTime;
        lastButtonReading = currentReading;
        
        if (currentReading == LOW) {
            // Button pressed
            buttonPressStartTime = currentTime;
            buttonState = BTN_PRESSED;
        } else {
            // Button released
            unsigned long pressDuration = currentTime - buttonPressStartTime;
            unsigned long timeSinceLastRelease = currentTime - lastReleaseTime;
            
            if (timeSinceLastRelease < MENU_DOUBLE_CLICK_TIME && lastReleaseTime > 0) {
                // Double click - check if this is second press in sequence
                pendingEvent = BUTTON_DOUBLE_CLICK;
                eventReady = true;
                buttonState = BTN_IDLE;
            } else if (buttonState != BTN_IDLE) {
                // Only process if not already handled as long press
                // Potential single click or first click of double click
                buttonState = BTN_POTENTIAL_DOUBLE;
                lastReleaseTime = currentTime;
            }
        }
    }
}

/**
 * @brief Process timeouts for button state machine
 * 
 * Processes timeout conditions for the button state machine. This function
 * handles the transition from potential double-click to single-click.
 * Long press detection is handled by menuButton_checkLongPress() in menu_module.
 */
static void processButtonTimeouts() {
    unsigned long currentTime = millis();
    
    // Check for single click timeout only
    // (Long press is handled by menuButton_checkLongPress() in menu_module)
    if (buttonState == BTN_POTENTIAL_DOUBLE && 
        (currentTime - lastReleaseTime) >= MENU_DOUBLE_CLICK_TIME) {
        pendingEvent = BUTTON_SINGLE_CLICK;
        eventReady = true;
        buttonState = BTN_IDLE;
    }
}

/**
 * @brief Play feedback for button event
 * @param event The button event to provide feedback for
 * 
 * Provides audio and haptic feedback for button events. This function
 * plays appropriate sound effects and haptic feedback based on the
 * type of button event that occurred.
 */
static void playButtonFeedback(ButtonEvent event) {
    switch (event) {
        case BUTTON_SINGLE_CLICK:
            sfxPlay("menu");
            if (areHapticsActive()) {
                playHapticEffect(HAPTIC_STRONG_CLICK_100);
            }
            break;
        case BUTTON_DOUBLE_CLICK:
            sfxPlay("confirm");
            if (areHapticsActive()) {
                playHapticEffect(HAPTIC_DOUBLE_CLICK_100);
            }
            break;
        case BUTTON_LONG_PRESS:
            sfxPlay("confirm");
            if (areHapticsActive()) {
                playHapticEffect(HAPTIC_TRIPLE_CLICK_100);
            }
            break;
        default:
            break;
    }
}

//==============================================================================
// PUBLIC API IMPLEMENTATION
//==============================================================================

/**
 * @brief Initialize button input system
 * 
 * Initializes the button input system by configuring the button pin,
 * setting up interrupt handling, and initializing all internal state
 * variables. This function should be called during system initialization
 * to prepare the button system for operation.
 */
void menuButton_init() {
     pinMode(MENU_BUTTON_PIN, INPUT_PULLUP);
     attachInterrupt(digitalPinToInterrupt(MENU_BUTTON_PIN), buttonInterruptHandler, CHANGE);
     
     // Initialize state
     buttonState = BTN_IDLE;
     pendingEvent = BUTTON_NONE;
     eventReady = false;
     buttonPressStartTime = 0;
     lastReleaseTime = 0;
     lastDebounceTime = 0;
     lastButtonReading = HIGH;
 }

/**
 * @brief Process button input and return events
 * @return ButtonEvent that occurred, or BUTTON_NONE if no event
 * 
 * Processes button input and returns the next available button event.
 * This function should be called regularly from the main loop to
 * process button events and provide user feedback.
 */
ButtonEvent menuButton_getEvent() {
     // Process any timeouts first
     processButtonTimeouts();
     
     if (eventReady) {
         ButtonEvent event = pendingEvent;
         
         // Returning button event
         
         // Clear the event
         pendingEvent = BUTTON_NONE;
         eventReady = false;
         
         // Play feedback
         playButtonFeedback(event);
         
         return event;
     }
     
     return BUTTON_NONE;
 }



/**
 * @brief Check if a long press has occurred and handle it based on menu state
 * @param longPressTime Custom long press time threshold in milliseconds
 * @return true if a long press was handled, false otherwise
 * 
 * Checks if a long press event has occurred and handles it appropriately.
 * This function allows for custom long press timing and is useful when
 * different menu states require different long press thresholds
 */
bool menuButton_checkLongPress(unsigned long longPressTime) {
     unsigned long currentTime = millis();
     
     // Check if button is currently pressed and has been held long enough
     if (buttonState == BTN_PRESSED && 
         (currentTime - buttonPressStartTime) >= longPressTime) {
         
         // Set the long press event
         pendingEvent = BUTTON_LONG_PRESS;
         eventReady = true;
         buttonState = BTN_IDLE;
         
         return true;
     }
     
     return false;
 }

/**
 * @brief Reset button state
 * 
 * Resets the button system to its initial state by clearing all internal
 * state variables and reinitializing the interrupt handler. This function
 * is useful for cleanup operations or when the button system needs to be
 * reset to a known state.
 */
void menuButton_reset() {
     // Disable interrupts temporarily
     detachInterrupt(digitalPinToInterrupt(MENU_BUTTON_PIN));
     
     // Reset all state
     buttonState = BTN_IDLE;
     pendingEvent = BUTTON_NONE;
     eventReady = false;
     buttonPressStartTime = 0;
     lastReleaseTime = 0;
     lastDebounceTime = 0;
     lastButtonReading = HIGH;
     
     // Re-enable interrupts
     attachInterrupt(digitalPinToInterrupt(MENU_BUTTON_PIN), buttonInterruptHandler, CHANGE);
 }
