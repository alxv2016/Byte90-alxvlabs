/**
 * @file wifi_endpoints.h
 * @brief Web server endpoints and API handlers for WiFi configuration portal
 */

#ifndef WIFI_ENDPOINTS_H
#define WIFI_ENDPOINTS_H

#include "common.h"
#include <WebServer.h>
#include <WiFi.h>
#include "wifi_common.h"

//==============================================================================
// WEB SERVER SETUP API
//==============================================================================

/**
 * @brief Initialize the web server and set up all endpoints
 * @return true if setup successful, false otherwise
 */
bool initWiFiEndpoints();

/**
 * @brief Handle web server client requests
 * Should be called regularly in main loop when web server is active
 */
void handleWebServer();

/**
 * @brief Stop the web server and clean up endpoints
 */
void stopWiFiEndpoints();

//==============================================================================
// ENDPOINT SETUP FUNCTIONS
//==============================================================================

/**
 * @brief Set up the root endpoint for the configuration portal
 */
void setupRootEndpoint();

/**
 * @brief Set up the restart endpoint (/restart)
 */
void setupRestartEndpoint();

/**
 * @brief Set up all web endpoints for the configuration portal
 */
void setupWebEndpoints();

/**
 * @brief Set up the network scan endpoint (/scan)
 * Returns JSON with available networks and their signal strengths
 */
void setupScanEndpoint();

/**
 * @brief Set up the connection status endpoint (/status)
 * Returns JSON with current connection information
 */
void setupConnectionStatusEndpoint();

/**
 * @brief Set up the connect endpoint (/connect)
 * Handles POST requests to connect to a specific network
 */
void setupConnectEndpoint();

/**
 * @brief Set up the disconnect endpoint (/disconnect)
 * Handles POST requests to disconnect from current network
 */
void setupDisconnectEndpoint();

//==============================================================================
// UTILITY FUNCTIONS
//==============================================================================

/**
 * @brief Create a JSON response with WiFi status information
 * @param success Whether the operation was successful
 * @param ssid Network SSID
 * @param rssi Signal strength
 * @param message Status message
 * @param connected Whether currently connected
 * @param networks JSON array of available networks
 * @return Formatted JSON response string
 */
String createJsonResponse(bool success, const String& ssid, int rssi, const String& message,
  bool connected, const String& networks);

//==============================================================================
// WIFI OPERATIONS API
//==============================================================================

/**
 * @brief Scan for available WiFi networks and return formatted JSON
 * @return JSON string with scan results including SSID, RSSI, and signal strength
 */
String scanWiFiNetworks();

/**
 * @brief Get current WiFi connection status as JSON
 * @return JSON string with current status (SSID, signal strength, connection state)
 */
String getWiFiStatusJson();

//==============================================================================
// WEB SERVER INTEGRATION
//==============================================================================

/**
 * @brief Get the web server instance for endpoint integration
 * @return Reference to the web server instance
 */
WebServer& getWiFiWebServer();

#endif /* WIFI_ENDPOINTS_H */
