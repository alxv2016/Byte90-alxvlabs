/**
 * @file wifi_module.h
 * @brief Header for WiFi management module
 *
 * Provides functionality for WiFi connection management, Access Point operations,
 * ESP-NOW configuration, and web server handling for the BYTE-90 device.
 */

#ifndef WIFI_MODULE_H
#define WIFI_MODULE_H

#include <Arduino.h>
#include <WiFi.h>

//==============================================================================
// CONSTANTS & DEFINITIONS
//==============================================================================

static const char *WIFI_LOG = "::WIFI_MODULE::";

//==============================================================================
// GLOBAL VARIABLES
//==============================================================================

/**
 * @brief Global flag indicating if WiFi network is currently connected
 */
extern bool wifiNetworkConnected;

/**
 * @brief Global buffer storing the current WiFi SSID
 */
extern char wifi_ssid[32];

/**
 * @brief Global buffer storing the current WiFi password
 */
extern char wifi_password[64];

//==============================================================================
// PUBLIC API FUNCTIONS
//==============================================================================

/**
 * @brief Initializes the WiFi manager and sets up event handlers
 */
void initWiFiManager();

/**
 * @brief Enables WiFi in station mode
 */
void enableWiFi();

/**
 * @brief Disables WiFi completely
 */
void disableWiFi();

/**
 * @brief Enables WiFi in ESP-NOW compatible mode
 */
void enableESPNowWiFi();

/**
 * @brief Connects to a WiFi network with specified credentials
 * @param ssid WiFi network SSID to connect to
 * @param password WiFi network password
 * @return true if connection successful, false otherwise
 */
bool connectToWiFi(const char* ssid, const char* password);

/**
 * @brief Connects to WiFi using credentials stored in preferences
 * @return true if connection successful, false otherwise
 */
bool connectToWiFiFromPreferences();

/**
 * @brief Disconnects from the current WiFi network
 */
void disconnectFromWiFi();

/**
 * @brief Starts WiFi Access Point mode
 */
void startWiFiAP();

/**
 * @brief Stops WiFi Access Point mode
 * @param restart If true, restarts WiFi after stopping AP mode
 */
void stopWiFiAP(bool restart = false);

/**
 * @brief Checks if WiFi network is currently connected
 * @return true if connected to WiFi network, false otherwise
 */
bool isWifiNetworkConnected();

/**
 * @brief Handles web server requests and responses
 */
void handleWebServer();

//==============================================================================
// WIFI EVENT HANDLERS
//==============================================================================

/**
 * @brief Event handler for WiFi station connected event
 * @param event WiFi event type
 * @param info WiFi event information structure
 */
void onWiFiStationConnected(WiFiEvent_t event, WiFiEventInfo_t info);

/**
 * @brief Event handler for WiFi station disconnected event
 * @param event WiFi event type
 * @param info WiFi event information structure
 */
void onWiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info);

/**
 * @brief Event handler for WiFi station got IP address event
 * @param event WiFi event type
 * @param info WiFi event information structure
 */
void onWiFiStationGotIP(WiFiEvent_t event, WiFiEventInfo_t info);

/**
 * @brief Event handler for WiFi Access Point started event
 * @param event WiFi event type
 * @param info WiFi event information structure
 */
void onWiFiAPStarted(WiFiEvent_t event, WiFiEventInfo_t info);

/**
 * @brief Event handler for WiFi Access Point stopped event
 * @param event WiFi event type
 * @param info WiFi event information structure
 */
void onWiFiAPStopped(WiFiEvent_t event, WiFiEventInfo_t info);

//==============================================================================
// UTILITY FUNCTIONS
//==============================================================================

/**
 * @brief Converts WiFi disconnect reason code to human-readable string
 * @param reason WiFi disconnect reason code
 * @return String representation of the disconnect reason
 */
const char* getDisconnectReasonString(uint8_t reason);

/**
 * @brief Prints current WiFi status information to serial output
 */
void printWiFiStatus();

/**
 * @brief Enable or disable debug logging for WiFi operations
 * @param enabled true to enable debug logging, false to disable
 */
void setWifiDebug(bool enabled);

#endif /* WIFI_MODULE_H */