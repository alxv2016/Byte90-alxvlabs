/**
 * @file wifi_common.h
 * @brief Shared constants, types, and utilities for WiFi system modules
 */

#ifndef WIFI_COMMON_H
#define WIFI_COMMON_H

#include "common.h"
#include <Arduino.h>
#include <IPAddress.h>
#include <WiFi.h>

//==============================================================================
// CONSTANTS & DEFINITIONS
//==============================================================================

// WiFi Connection Settings
#define WIFI_CONNECTION_ATTEMPTS 30
#define WEB_SERVER_PORT 80

// Access Point Configuration
#define WIFI_AP_SSID "BYTE90_Setup"
#define WIFI_AP_PASSWORD "00000000"
#define WIFI_AP_IP "192.168.4.1"

// Access Point Network Configuration
#define AP_LOCAL_IP_1 192
#define AP_LOCAL_IP_2 168
#define AP_LOCAL_IP_3 4
#define AP_LOCAL_IP_4 1

#define AP_GATEWAY_1 192
#define AP_GATEWAY_2 168
#define AP_GATEWAY_3 4
#define AP_GATEWAY_4 1

#define AP_NETWORK_MASK_1 255
#define AP_NETWORK_MASK_2 255
#define AP_NETWORK_MASK_3 255
#define AP_NETWORK_MASK_4 0

// WiFi Timeout Settings
#define WIFI_CONNECT_TIMEOUT_MS 15000
#define WIFI_SCAN_TIMEOUT_MS 10000
#define WIFI_EVENT_TIMEOUT_MS 5000

//==============================================================================
// UTILITY FUNCTIONS
//==============================================================================

/**
 * @brief Convert encryption type to human-readable string
 * @param encType WiFi encryption type
 * @return String description of security type
 */
String getNetworkSecurityType(wifi_auth_mode_t encType);

/**
 * @brief Convert RSSI value to a human-readable signal strength description
 * @param rssi RSSI value from WiFi
 * @return String description of signal strength
 */
String getNetworkSignalStrength(int rssi);

/**
 * @brief Get default Access Point IP address
 * @return IPAddress for the default AP configuration
 */
IPAddress getDefaultAPIP();

/**
 * @brief Get default Access Point gateway
 * @return IPAddress for the default AP gateway
 */
IPAddress getDefaultAPGateway();

/**
 * @brief Get default Access Point network mask
 * @return IPAddress for the default AP network mask
 */
IPAddress getDefaultAPNetworkMask();


//==============================================================================
// WIFI STATUS FUNCTIONS
//==============================================================================

/**
 * @brief Get current WiFi status information
 * @return String representation of current WiFi status
 */
String getCurrentWiFiStatus();

/**
 * @brief Get current WiFi network name
 * @return String with current network SSID or "--" if not connected
 */
String getCurrentWiFiNetwork();

/**
 * @brief Get current WiFi signal strength
 * @return String with signal strength description or "--" if not connected
 */
String getCurrentWiFiSignalStrength();

/**
 * @brief Get current WiFi IP address
 * @return String with current IP address or "--" if not connected
 */
String getCurrentWiFiIP();

#endif /* WIFI_COMMON_H */
