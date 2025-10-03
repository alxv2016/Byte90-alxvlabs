/**
 * @file wifi_endpoints.cpp
 * @brief Implementation of web server endpoints and API handlers for WiFi
 * configuration portal
 */

#include "wifi_endpoints.h"
#include "common.h"
#include "flash_module.h"
#include "ota_module.h"
#include "preferences_module.h"
#include "states_module.h"
#include "wifi_common.h"
#include "wifi_module.h"

//==============================================================================
// CONSTANTS & DEFINITIONS
//==============================================================================

static const char *WIFI_ENDPOINTS_LOG = "::WIFI_ENDPOINTS::";

//==============================================================================
// GLOBAL VARIABLES
//==============================================================================

WebServer webServer(WEB_SERVER_PORT);

// Access Point configuration using common constants
IPAddress AP_LOCAL_IP = getDefaultAPIP();
IPAddress AP_GATEWAY = getDefaultAPGateway();
IPAddress AP_NETWORK_MASK = getDefaultAPNetworkMask();

//==============================================================================
// UTILITY FUNCTIONS
//==============================================================================

/**
 * @brief Create a JSON response with WiFi status information
 */
String createJsonResponse(bool success, const String &ssid, int rssi,
                          const String &message, bool connected,
                          const String &networks) {

  const char *stateStr = getStateString(getCurrentState());
  String signalStr = getNetworkSignalStrength(rssi);

  size_t estimatedSize = 150 + ssid.length() + message.length() +
                         networks.length() + strlen(stateStr) +
                         signalStr.length();

  String response;
  response.reserve(estimatedSize); // Allocate memory upfront

  response += "{\"success\":";
  response += success ? "true" : "false";
  response += ",\"status\":\"";
  response += stateStr;
  response += "\",\"ssid\":\"";
  response += ssid;
  response += "\",\"rssi\":";
  response += rssi;
  response += ",\"signal_strength\":\"";
  response += signalStr;
  response += "\",\"message\":\"";
  response += message;
  response += "\",\"connected\":";
  response += connected ? "true" : "false";
  response += ",\"networks\":";
  response += networks;
  response += "}";

  return response;
}

//==============================================================================
// WEB SERVER SETUP FUNCTIONS
//==============================================================================

/**
 * @brief Set up the root endpoint for the configuration portal
 */
void setupRootEndpoint() {
  getWiFiWebServer().on("/", HTTP_GET, []() {
    File file = LittleFS.open("/index.html", "r");
    if (!file) {
      String message = "Failed to load configuration page";
      getWiFiWebServer().send(500, "text/plain", message);
      return;
    }

    getWiFiWebServer().streamFile(file, "text/html");
    file.close();
  });
}

/**
 * @brief Set up the restart endpoint
 */
void setupRestartEndpoint() {
  getWiFiWebServer().on("/restart", HTTP_POST, []() {
    getWiFiWebServer().send(200, "text/plain", "Restarting...");
    delay(1000);
    ESP.restart();
  });
}

/**
 * @brief Set up all web endpoints for the configuration portal
 */
void setupWebEndpoints() {
  setupRootEndpoint();
  setupRestartEndpoint();
  setupScanEndpoint();
  setupConnectionStatusEndpoint();
  setupConnectEndpoint();
  setupDisconnectEndpoint();

  // Set up catch-all handler for 404
  getWiFiWebServer().onNotFound(
      []() { getWiFiWebServer().send(404, "text/plain", "Not found"); });
}

/**
 * @brief Set up the network scan endpoint
 * Returns JSON with available networks and their signal strengths
 */
void setupScanEndpoint() {
  getWiFiWebServer().on("/scan", HTTP_GET, []() {
    ESP_LOGI(WIFI_ENDPOINTS_LOG, "Network scan requested");

    String networks = scanWiFiNetworks();
    String response =
        createJsonResponse(true, "", 0, "Scan completed", false, networks);

    getWiFiWebServer().send(200, "application/json", response);
  });
}

/**
 * @brief Set up the connection status endpoint
 * Returns JSON with current connection information
 */
void setupConnectionStatusEndpoint() {
  getWiFiWebServer().on("/status", HTTP_GET, []() {
    String statusJson = getWiFiStatusJson();
    getWiFiWebServer().send(200, "application/json", statusJson);
  });
}

/**
 * @brief Set up the connect endpoint
 * Handles POST requests to connect to a specific network
 */
void setupConnectEndpoint() {
  getWiFiWebServer().on("/connect", HTTP_POST, []() {
    if (!getWiFiWebServer().hasArg("ssid") ||
        !getWiFiWebServer().hasArg("password")) {
      String response = createJsonResponse(
          false, "", 0, "Missing SSID or password", false, "[]");
      getWiFiWebServer().send(400, "application/json", response);
      return;
    }

    String ssid = getWiFiWebServer().arg("ssid");
    String password = getWiFiWebServer().arg("password");

    ESP_LOGI(WIFI_ENDPOINTS_LOG, "Connection attempt to: %s", ssid.c_str());

    // Use the new connectToWiFi function with parameters (includes validation)
    bool connectionInitiated = connectToWiFi(ssid.c_str(), password.c_str());

    if (!connectionInitiated) {
      // Check if we're already connected to the requested network
      if (isWifiNetworkConnected() && WiFi.SSID().equals(ssid)) {
        ESP_LOGI(WIFI_ENDPOINTS_LOG, "Already connected to requested network");

        // Save credentials since we're connected to this network
        if (saveWiFiCredentials(ssid.c_str(), password.c_str())) {
          ESP_LOGI(WIFI_ENDPOINTS_LOG, "Credentials saved successfully");
        }

        String response = createJsonResponse(true, ssid, WiFi.RSSI(),
                                             "Already connected", true, "[]");
        getWiFiWebServer().send(200, "application/json", response);
        return;
      } else {
        // Connection failed due to validation or other error
        ESP_LOGE(WIFI_ENDPOINTS_LOG,
                 "Connection failed - validation error or invalid parameters");
        String response = createJsonResponse(
            false, ssid, 0, "Invalid credentials format or connection failed",
            false, "[]");
        getWiFiWebServer().send(400, "application/json", response);
        return;
      }
    }

    // Connection attempt initiated - wait for result with timeout
    ESP_LOGI(WIFI_ENDPOINTS_LOG,
             "Connection attempt initiated, waiting for result...");

    unsigned long startTime = millis();
    const unsigned long TIMEOUT_MS = 15000; // 15 seconds timeout

    while (millis() - startTime < TIMEOUT_MS) {
      delay(500);

      if (isWifiNetworkConnected()) {
        ESP_LOGI(WIFI_ENDPOINTS_LOG, "Connection successful");

        // Save credentials on successful connection
        if (saveWiFiCredentials(ssid.c_str(), password.c_str())) {
          ESP_LOGI(WIFI_ENDPOINTS_LOG, "Credentials saved successfully");
        } else {
          ESP_LOGW(WIFI_ENDPOINTS_LOG,
                   "Failed to save credentials, but connection successful");
        }

        String response = createJsonResponse(
            true, ssid, WiFi.RSSI(), "Connected successfully", true, "[]");
        getWiFiWebServer().send(200, "application/json", response);
        return;
      }

      // Continue connection attempt
      connectToWiFi(ssid.c_str(), password.c_str());
    }

    // Timeout reached without successful connection
    ESP_LOGE(WIFI_ENDPOINTS_LOG, "Connection timeout reached");
    String response = createJsonResponse(
        false, ssid, 0, "Connection timeout - please verify password", false,
        "[]");
    getWiFiWebServer().send(500, "application/json", response);
  });
}

/**
 * @brief Set up the disconnect endpoint
 * Handles POST requests to disconnect from current network
 */
void setupDisconnectEndpoint() {
  getWiFiWebServer().on("/disconnect", HTTP_POST, []() {
    ESP_LOGI(WIFI_ENDPOINTS_LOG, "Disconnect requested");

    String currentSSID = WiFi.SSID();
    bool wasConnected = isWifiNetworkConnected();

    // Disconnect from WiFi
    disconnectFromWiFi();

    // Check if disconnection was successful
    bool isNowDisconnected = !isWifiNetworkConnected();

    if (isNowDisconnected) {
      String message = wasConnected ? "Disconnected from " + currentSSID
                                    : "WiFi was already disconnected";
      String response = createJsonResponse(true, "", 0, message, false, "[]");
      getWiFiWebServer().send(200, "application/json", response);
    } else {
      String response =
          createJsonResponse(false, "", 0, "Disconnect failed", false, "[]");
      getWiFiWebServer().send(500, "application/json", response);
    }
  });
}

//==============================================================================
// WEB SERVER MANAGEMENT API
//==============================================================================

/**
 * @brief Initialize the web server and set up all endpoints
 */
bool initWiFiEndpoints() {
  ESP_LOGI(WIFI_ENDPOINTS_LOG, "Initializing WiFi endpoints");

  // Set up all endpoints
  setupWebEndpoints();

  // Start the web server
  getWiFiWebServer().begin();

  ESP_LOGI(WIFI_ENDPOINTS_LOG, "WiFi endpoints initialized successfully");
  return true;
}

/**
 * @brief Stop the web server and clean up endpoints
 */
void stopWiFiEndpoints() {
  ESP_LOGI(WIFI_ENDPOINTS_LOG, "Stopping WiFi endpoints");
  getWiFiWebServer().close();
}

//==============================================================================
// WIFI OPERATIONS FUNCTIONS
//==============================================================================

/**
 * @brief Scan for available WiFi networks and return JSON response
 */
String scanWiFiNetworks() {

  int networkCount = WiFi.scanNetworks(false);
  bool scanSuccess = networkCount >= 0;

  String message = scanSuccess ? "Network scan complete, found " +
                                     String(networkCount) + " networks."
                               : "Failed to scan networks, please try again.";

  String networks = "[";
  if (scanSuccess) {
    for (int i = 0; i < networkCount; i++) {
      if (i > 0)
        networks += ",";
      int rssi = WiFi.RSSI(i);
      wifi_auth_mode_t encryptionType = WiFi.encryptionType(i);

      networks += "{\"ssid\":\"";
      networks += WiFi.SSID(i);
      networks += "\",\"rssi\":";
      networks += rssi;
      networks += ",\"signal_strength\":\"";
      networks += getNetworkSignalStrength(rssi);
      networks += "\",\"encryption_type\":";
      networks += encryptionType;
      networks += ",\"is_open\":";
      networks += (encryptionType == WIFI_AUTH_OPEN ? "true" : "false");
      networks += ",\"security\":\"";
      networks += getNetworkSecurityType(encryptionType);
      networks += "\"}";
    }
  }
  networks += "]";

  String jsonResponse = createJsonResponse(scanSuccess, "", 0, message,
                                           isWifiNetworkConnected(), networks);

  WiFi.scanDelete();
  return jsonResponse;
}

/**
 * @brief Get current WiFi status as JSON
 */
String getWiFiStatusJson() {
  wifi_mode_t mode = WiFi.getMode();
  wl_status_t status = WiFi.status();
  
  bool isConnected = isWifiNetworkConnected();
  bool wifiModeEnabled = (getCurrentState() == WIFI_MODE);

  String currentSSID = isConnected ? WiFi.SSID() : "";
  int rssi = isConnected ? WiFi.RSSI() : 0;

  String message;
  if (isConnected) {
    message = "Connected to " + currentSSID;
  } else if (wifiModeEnabled) {
    message = "WiFi mode active - ready for configuration";
  } else {
    message = "WiFi mode disabled";
  }

  message += " [DEBUG: mode=" + String(mode) + " status=" + String(status) + 
             " flag=" + String(wifiNetworkConnected ? "true" : "false") + "]";
  
  bool success = isConnected;

  return createJsonResponse(success, currentSSID, rssi, message, isConnected,
                            "[]");
}

//==============================================================================
// WEB SERVER INTEGRATION
//==============================================================================

/**
 * @brief Get the web server instance for endpoint integration
 */
WebServer &getWiFiWebServer() { return webServer; }
